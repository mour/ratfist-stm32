
use mouros_rust_bindings::mailbox::{Mailbox, MailboxRaw};
use mouros_rust_bindings::CVoid;
use mouros_rust_bindings::tasks;

use bindings::messages;
use bindings::constants;
use bindings::message_dispatcher;

use core::ptr;

const NUM_CHANNELS: usize = 1;


enum SpinnerEvent<'a> {
    SetPlan(&'a messages::spin_plan_data),
    GetPlan,
    SetState(&'a messages::spin_state_set_data),
    GetState,
}

#[derive(PartialEq, Eq)]
enum ChannelState {
    Stopped,
    Running,
    Spindown,
}


#[derive(Default, Clone, Copy)]
struct PlanLeg {
    duration_msecs: u32,
    target_pct: f32,
}

struct Channel {
    id: u8,
    state: ChannelState,
    plan: [PlanLeg; constants::MAX_SPIN_PLAN_LEGS as usize],
    plan_len: usize,
    elapsed_time_msecs: u64,
    output_val_pct: f32,
}

impl Default for Channel {
    fn default() -> Channel {
        Channel {
            id: 0,
            state: ChannelState::Stopped,
            plan: [PlanLeg::default(); constants::MAX_SPIN_PLAN_LEGS as usize],
            plan_len: 0,
            elapsed_time_msecs: 0,
            output_val_pct: 0f32,
        }
    }
}

impl Channel {
    fn send_event(&mut self, event: SpinnerEvent) {
        match self.state {
            ChannelState::Stopped => {
                match event {
                    SpinnerEvent::SetPlan(data) => {
                        self.set_channel_plan(data);
                    }
                    SpinnerEvent::GetPlan => {
                        self.send_reply(messages::MsgTypes::SPIN_PLAN_REPLY);
                    }
                    SpinnerEvent::SetState(data) => {
                        self.set_channel_state(data);
                    }
                    SpinnerEvent::GetState => {
                        self.send_reply(messages::MsgTypes::SPIN_STATE_REPLY);
                    }
                }
            }
            ChannelState::Running => {
                match event {
                    SpinnerEvent::GetPlan => {
                        self.send_reply(messages::MsgTypes::SPIN_PLAN_REPLY);
                    }
                    SpinnerEvent::SetState(data) => {
                        self.set_channel_state(data);
                    }
                    SpinnerEvent::GetState => {
                        self.send_reply(messages::MsgTypes::SPIN_PLAN_REPLY);
                    }
                    _ => {}
                }
            }
            ChannelState::Spindown => {
                match event {
                    SpinnerEvent::GetPlan => {
                        self.send_reply(messages::MsgTypes::SPIN_PLAN_REPLY);
                    }
                    SpinnerEvent::GetState => {
                        self.send_reply(messages::MsgTypes::SPIN_PLAN_REPLY);
                    }
                    _ => {}
                }
            }
        }
    }

    fn send_reply(&self, msg_type: i32) {
        let mut msg = if let Ok(msg) = messages::MessageWrapper::new(msg_type) {
            msg
        } else {
            return;
        };

        match *msg {
            messages::Message::SpinPlanReply(ref mut data) => {
                data.channel_num = self.id;
                data.spin_plan_leg_count = self.plan_len as u32;
                for leg_idx in 0..self.plan_len {
                    data.plan_legs[leg_idx].duration_msecs = self.plan[leg_idx].duration_msecs;
                    data.plan_legs[leg_idx].target_pct = self.plan[leg_idx].target_pct;
                }
            }
            messages::Message::SpinStateReply(ref mut data) => {
                data.channel_num = self.id;
                data.state = match self.state {
                    ChannelState::Stopped => messages::SpinStates::STOPPED,
                    ChannelState::Running => messages::SpinStates::RUNNING,
                    ChannelState::Spindown => messages::SpinStates::SPINNING_DOWN,
                };
                data.plan_time_elapsed_msecs = self.elapsed_time_msecs;
                data.output_val_pct = self.output_val_pct;
            }
            _ => {
                return;
            }
        }

        message_dispatcher::send_message(msg);
    }

    fn set_channel_state(&mut self, data: &messages::spin_state_set_data) {
        critical!({
            if self.state == ChannelState::Running && data.state == messages::SpinStates::STOPPED {
                self.state = ChannelState::Spindown;
            } else if self.state == ChannelState::Stopped &&
                      data.state == messages::SpinStates::RUNNING {
                self.state = ChannelState::Running;
            }
        });
    }

    fn set_channel_plan(&mut self, data: &messages::spin_plan_data) {
        critical!({
            if self.state != ChannelState::Stopped {
                return;
            }
        });

        self.plan_len = data.spin_plan_leg_count as usize;
        for leg_idx in 0..self.plan_len {
            self.plan[leg_idx].duration_msecs = data.plan_legs[leg_idx].duration_msecs;
            self.plan[leg_idx].target_pct = data.plan_legs[leg_idx].target_pct;
        }
    }
}




pub struct SpinnerContext {
    channels: [Channel; NUM_CHANNELS],
    msg_buf: Mailbox<*mut messages::message>,
}

static mut MSG_BUF_MEM: [*mut messages::message; 30] = [0 as *mut messages::message; 30];
static mut SPINNER_CONTEXT: Option<SpinnerContext> = None;


#[no_mangle]
pub static mut SPINNER_MSG_QUEUE_PTR: *const MailboxRaw = 0 as *const MailboxRaw;

#[no_mangle]
pub extern "C" fn spinner_get_context() -> *mut CVoid {
    unsafe {
        if SPINNER_CONTEXT.is_none() {
            let mut ctx = SpinnerContext {
                channels: [Channel::default(); NUM_CHANNELS],
                msg_buf: Mailbox::new(&mut MSG_BUF_MEM),
            };

            for (id, ch) in ctx.channels.iter_mut().enumerate() {
                ch.id = id as u8;
            }

            SPINNER_CONTEXT = Some(ctx);
            SPINNER_MSG_QUEUE_PTR = SPINNER_CONTEXT.as_mut().unwrap().msg_buf.get_raw_mb_struct();
        }

        SPINNER_CONTEXT.as_mut().unwrap() as *mut SpinnerContext as *mut CVoid
    }
}

#[no_mangle]
pub extern "C" fn spinner_comm_loop(context_ptr: *mut CVoid) {

    let context: &mut SpinnerContext = unsafe {
        if context_ptr != ptr::null_mut() {
            &mut *(context_ptr as *mut SpinnerContext)
        } else {
            panic!();
        }
    };

    if let Some(msg_ptr) = context.msg_buf.read() {
        if let Ok(msg) = messages::MessageWrapper::from_raw(msg_ptr) {
            match *msg {
                messages::Message::SetSpinPlan(ref data) => {
                    let ch_num = data.channel_num as usize;
                    if ch_num < NUM_CHANNELS {
                        context.channels[ch_num].send_event(SpinnerEvent::SetPlan(data));
                    }
                }
                messages::Message::GetSpinPlan(ref data) => {
                    let ch_num = data.channel_num as usize;
                    if ch_num < NUM_CHANNELS {
                        context.channels[ch_num].send_event(SpinnerEvent::GetPlan);
                    }
                }
                messages::Message::SetSpinState(ref data) => {
                    let ch_num = data.channel_num as usize;
                    if ch_num < NUM_CHANNELS {
                        context.channels[ch_num].send_event(SpinnerEvent::SetState(data));
                    }
                }
                messages::Message::GetSpinState(ref data) => {
                    let ch_num = data.channel_num as usize;
                    if ch_num < NUM_CHANNELS {
                        context.channels[ch_num].send_event(SpinnerEvent::GetState);
                    }
                }
                _ => {}
            }
        }

        return;
    }

    tasks::sleep(50);
}
