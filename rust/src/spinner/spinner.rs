use mouros::mailbox::Mailbox;
use mouros::CVoid;
use mouros::tasks;

use bindings::constants;
use bindings::message_dispatcher;

use core::convert::TryFrom;

use bindings::message_dispatcher::{MemManagement, MessageWrapper};

mod MsgTypes {
    pub const SET_PLAN: u32 = 0;
    pub const GET_PLAN: u32 = 1;
    pub const PLAN_REPLY: u32 = 2;
    pub const SET_STATE: u32 = 3;
    pub const GET_STATE: u32 = 4;
    pub const STATE_REPLY: u32 = 5;
    pub const RET_VAL: u32 = 6;
}

#[repr(C)]
pub struct spin_plan_data {
    pub channel_num: u8,
    pub spin_plan_leg_count: u32,
    pub plan_legs: [pwm_leg; 10usize],
}

#[repr(C)]
pub struct pwm_leg {
    pub duration_msecs: u32,
    pub target_pct: f32,
}

#[repr(C)]
pub struct spin_channel {
    pub channel_num: u8,
}

#[repr(C)]
pub struct ret_val {
    pub ret_val: i32,
}

mod SpinStates {
    pub const STOPPED: u32 = 0;
    pub const RUNNING: u32 = 1;
    pub const SPINNING_DOWN: u32 = 2;
}

#[repr(C)]
pub struct spin_state_set_data {
    pub channel_num: u8,
    pub state: u32,
}

#[repr(C)]
pub struct spin_state_data {
    pub channel_num: u8,
    pub state: u32,
    pub plan_time_elapsed_msecs: u32,
    pub output_val_pct: f32,
}

pub enum Message<'a> {
    SetSpinPlan(&'a mut spin_plan_data),
    GetSpinPlan(&'a mut spin_channel),
    SpinPlanReply(&'a mut spin_plan_data),
    SetSpinState(&'a mut spin_state_set_data),
    GetSpinState(&'a mut spin_channel),
    SpinStateReply(&'a mut spin_state_data),
    ReturnValue(&'a mut ret_val),
}

impl<'a> message_dispatcher::MemManagement for Message<'a> {
    fn alloc(msg_type: u32) -> *mut message_dispatcher::message {
        unsafe { get_alloc_fn()(msg_type) }
    }

    fn free(msg_ptr: *mut message_dispatcher::message) {
        unsafe { get_free_fn()(msg_ptr) }
    }
}

impl<'a> TryFrom<*mut message_dispatcher::message> for Message<'a> {
    type Error = ();

    fn try_from(raw_msg_ptr: *mut message_dispatcher::message) -> Result<Self, Self::Error> {
        unsafe {
            if raw_msg_ptr.is_null() || (*raw_msg_ptr).data.is_null() {
                return Err(());
            }

            match (*raw_msg_ptr).msg_type {
                MsgTypes::SET_PLAN => Ok(Message::SetSpinPlan(
                    &mut *((*raw_msg_ptr).data as *mut spin_plan_data),
                )),
                MsgTypes::GET_PLAN => Ok(Message::GetSpinPlan(
                    &mut *((*raw_msg_ptr).data as *mut spin_channel),
                )),
                MsgTypes::PLAN_REPLY => Ok(Message::SpinPlanReply(
                    &mut *((*raw_msg_ptr).data as *mut spin_plan_data),
                )),
                MsgTypes::SET_STATE => Ok(Message::SetSpinState(
                    &mut *((*raw_msg_ptr).data as *mut spin_state_set_data),
                )),
                MsgTypes::GET_STATE => Ok(Message::GetSpinState(
                    &mut *((*raw_msg_ptr).data as *mut spin_channel),
                )),
                MsgTypes::STATE_REPLY => Ok(Message::SpinStateReply(
                    &mut *((*raw_msg_ptr).data as *mut spin_state_data),
                )),
                MsgTypes::RET_VAL => Ok(Message::ReturnValue(
                    &mut *((*raw_msg_ptr).data as *mut ret_val),
                )),
                _ => {
                    Self::free(raw_msg_ptr);
                    Err(())
                }
            }
        }
    }
}

const NUM_CHANNELS: usize = 1;

enum SpinnerEvent<'a> {
    SetPlan(&'a spin_plan_data),
    GetPlan,
    SetState(&'a spin_state_set_data),
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
    elapsed_time_msecs: u32,
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
    fn send_event(&mut self, trans_id: u32, event: SpinnerEvent) {
        match self.state {
            ChannelState::Stopped => match event {
                SpinnerEvent::SetPlan(data) => {
                    self.set_channel_plan(data);
                    self.send_return_val(trans_id, 0);
                }
                SpinnerEvent::GetPlan => {
                    self.send_spin_plan_reply(trans_id);
                }
                SpinnerEvent::SetState(data) => {
                    self.set_channel_state(data);
                    self.send_return_val(trans_id, 0);
                }
                SpinnerEvent::GetState => {
                    self.send_spin_state_reply(trans_id);
                }
            },
            ChannelState::Running => match event {
                SpinnerEvent::GetPlan => {
                    self.send_spin_plan_reply(trans_id);
                }
                SpinnerEvent::SetState(data) => {
                    self.set_channel_state(data);
                    self.send_return_val(trans_id, 0);
                }
                SpinnerEvent::GetState => {
                    self.send_spin_state_reply(trans_id);
                }
                _ => {
                    self.send_return_val(trans_id, 1);
                }
            },
            ChannelState::Spindown => match event {
                SpinnerEvent::GetPlan => {
                    self.send_spin_plan_reply(trans_id);
                }
                SpinnerEvent::GetState => {
                    self.send_spin_state_reply(trans_id);
                }
                _ => {
                    self.send_return_val(trans_id, 1);
                }
            },
        }
    }

    fn send_spin_plan_reply(&self, trans_id: u32) {
        let mut msg = if let Ok(msg) = MessageWrapper::new(trans_id, MsgTypes::PLAN_REPLY) {
            msg
        } else {
            return;
        };

        if let Message::SpinPlanReply(ref mut data) = *msg {
            data.channel_num = self.id;
            data.spin_plan_leg_count = self.plan_len as u32;
            for leg_idx in 0..self.plan_len {
                data.plan_legs[leg_idx].duration_msecs = self.plan[leg_idx].duration_msecs;
                data.plan_legs[leg_idx].target_pct = self.plan[leg_idx].target_pct;
            }
        } else {
            return;
        }

        get_outgoing_queue().write(msg.into());
    }

    fn send_spin_state_reply(&self, trans_id: u32) {
        let mut msg = if let Ok(msg) = MessageWrapper::new(trans_id, MsgTypes::STATE_REPLY) {
            msg
        } else {
            return;
        };

        if let Message::SpinStateReply(ref mut data) = *msg {
            data.channel_num = self.id;
            data.state = match self.state {
                ChannelState::Stopped => SpinStates::STOPPED,
                ChannelState::Running => SpinStates::RUNNING,
                ChannelState::Spindown => SpinStates::SPINNING_DOWN,
            };
            data.plan_time_elapsed_msecs = self.elapsed_time_msecs;
            data.output_val_pct = self.output_val_pct;
        } else {
            return;
        }

        get_outgoing_queue().write(msg.into());
    }

    fn send_return_val(&self, trans_id: u32, ret_val: i32) {
        let mut msg = if let Ok(msg) = MessageWrapper::new(trans_id, MsgTypes::RET_VAL) {
            msg
        } else {
            return;
        };

        if let Message::ReturnValue(ref mut data) = *msg {
            data.ret_val = ret_val;
        } else {
            return;
        }

        get_outgoing_queue().write(msg.into());
    }

    fn set_channel_state(&mut self, data: &spin_state_set_data) {
        critical!({
            if self.state == ChannelState::Running && data.state == SpinStates::STOPPED {
                self.state = ChannelState::Spindown;
            } else if self.state == ChannelState::Stopped && data.state == SpinStates::RUNNING {
                self.state = ChannelState::Running;
            }
        });
    }

    fn set_channel_plan(&mut self, data: &spin_plan_data) {
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

struct SpinnerContext {
    channels: [Channel; NUM_CHANNELS],
    subsystem_conf: &'static message_dispatcher::subsystem_message_conf,
}

static mut SPINNER_CTX: Option<SpinnerContext> = None;

fn get_channels() -> &'static mut [Channel] {
    unsafe {
        match SPINNER_CTX {
            Some(ref mut ctx) => &mut ctx.channels,
            None => panic!(),
        }
    }
}

fn get_incoming_queue() -> &'static mut Mailbox<'static, *mut message_dispatcher::message> {
    unsafe {
        match SPINNER_CTX {
            Some(ref ctx) => {
                &mut *(ctx.subsystem_conf.incoming_msg_queue
                    as *mut Mailbox<'static, *mut message_dispatcher::message>)
            }
            None => panic!(),
        }
    }
}

fn get_outgoing_queue() -> &'static mut Mailbox<'static, *mut message_dispatcher::message> {
    unsafe {
        match SPINNER_CTX {
            Some(ref ctx) => {
                &mut *(ctx.subsystem_conf.outgoing_msg_queue
                    as *mut Mailbox<'static, *mut message_dispatcher::message>)
            }
            None => panic!(),
        }
    }
}

fn get_outgoing_error_queue() -> &'static mut Mailbox<'static, i32> {
    unsafe {
        match SPINNER_CTX {
            Some(ref ctx) => {
                &mut *(ctx.subsystem_conf.outgoing_err_queue as *mut Mailbox<'static, i32>)
            }
            None => panic!(),
        }
    }
}

fn get_alloc_fn() -> unsafe extern "C" fn(msg_type_id: u32) -> *mut message_dispatcher::message {
    unsafe {
        match SPINNER_CTX {
            Some(ref ctx) => ctx.subsystem_conf.alloc_message.unwrap(),
            None => panic!(),
        }
    }
}

fn get_free_fn() -> unsafe extern "C" fn(msg: *mut message_dispatcher::message) {
    unsafe {
        match SPINNER_CTX {
            Some(ref ctx) => ctx.subsystem_conf.free_message.unwrap(),
            None => panic!(),
        }
    }
}

#[no_mangle]
pub extern "C" fn spinner_rust_init(conf: *mut message_dispatcher::subsystem_message_conf) {
    unsafe {
        if conf.is_null() || (*conf).incoming_msg_queue.is_null()
            || (*conf).outgoing_msg_queue.is_null()
            || (*conf).outgoing_err_queue.is_null() || (*conf).alloc_message.is_none()
            || (*conf).free_message.is_none()
        {
            panic!();
        }

        SPINNER_CTX = Some(SpinnerContext {
            channels: [Channel::default(); NUM_CHANNELS],
            subsystem_conf: &*conf,
        });
    }
}

#[no_mangle]
pub extern "C" fn spinner_comm_loop(_params: *mut CVoid) {
    let channels = get_channels();
    let in_queue = get_incoming_queue();

    if let Some(msg_ptr) = in_queue.read() {
        if let Ok(msg) = MessageWrapper::try_from(msg_ptr) {
            match *msg {
                Message::SetSpinPlan(ref data) => {
                    let ch_num = data.channel_num as usize;
                    if ch_num < NUM_CHANNELS {
                        channels[ch_num]
                            .send_event(msg.get_transaction_id(), SpinnerEvent::SetPlan(data));
                    }
                }
                Message::GetSpinPlan(ref data) => {
                    let ch_num = data.channel_num as usize;
                    if ch_num < NUM_CHANNELS {
                        channels[ch_num]
                            .send_event(msg.get_transaction_id(), SpinnerEvent::GetPlan);
                    }
                }
                Message::SetSpinState(ref data) => {
                    let ch_num = data.channel_num as usize;
                    if ch_num < NUM_CHANNELS {
                        channels[ch_num]
                            .send_event(msg.get_transaction_id(), SpinnerEvent::SetState(data));
                    }
                }
                Message::GetSpinState(ref data) => {
                    let ch_num = data.channel_num as usize;
                    if ch_num < NUM_CHANNELS {
                        channels[ch_num]
                            .send_event(msg.get_transaction_id(), SpinnerEvent::GetState);
                    }
                }
                _ => {}
            }
        }

        return;
    }

    tasks::sleep(50);
}
