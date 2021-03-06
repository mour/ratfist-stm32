use mouros::CVoid;
use mouros::tasks;
use mouros::tasks::CriticalLock;
use mouros::pool_alloc::Pool;

use mouros::mailbox;
use mouros::mailbox::Mailbox;
use mouros::mailbox::{RxChannelSpsc, TxChannelSpsc};

use crate::bindings::message_dispatcher as md;

use crate::bsp::i2c;

use core::ptr;
use core::mem;
use core::slice;

use core::convert::TryFrom;
use core::fmt::Write;

mod bmp085;
mod tsl2651;

#[derive(Copy, Clone)]
struct StdResponsePayload {
    channel: u32,
    value: f32,
}

#[allow(dead_code)]
union MessagePayload {
    channel_num_payload: u32,
    std_response: StdResponsePayload,
}

const GET_TEMPERATURE_MSG_ID: u32 = 0;
const GET_PRESSURE_MSG_ID: u32 = 1;
const GET_HUMIDITY_MSG_ID: u32 = 2;
const GET_LIGHT_LEVEL_MSG_ID: u32 = 3;

const TEMPERATURE_REPLY_MSG_ID: u32 = 4;
const PRESSURE_REPLY_MSG_ID: u32 = 5;
const HUMIDITY_REPLY_MSG_ID: u32 = 6;
const LIGHT_LEVEL_REPLY_MSG_ID: u32 = 7;

#[repr(i32)]
#[derive(Copy, Clone, PartialEq, Eq)]
pub enum MeteoError {
    I2CCommError = -1,
    TxQueueFullError = -2,
    InvalidMsgError = -3,
    MsgCreationError = -4,
    SensorNotReadyError = -5,
    InvalidSensorData = -6,
}

static mut MESSAGE_ARR: Option<[md::message; 20]> = None;
static mut MESSAGE_POOL: Option<CriticalLock<Pool<md::message>>> = None;

static mut MESSAGE_PAYLOAD_ARR: Option<[MessagePayload; 20]> = None;
static mut MESSAGE_PAYLOAD_POOL: Option<CriticalLock<Pool<MessagePayload>>> = None;

unsafe extern "C" fn meteo_alloc(msg_type_id: u32) -> *mut md::message {
    let msg_pool = MESSAGE_POOL.as_ref().expect("message pool not initialized");
    let payload_pool = MESSAGE_PAYLOAD_POOL
        .as_ref()
        .expect("message payload pool not initialized");

    match msg_type_id {
        GET_TEMPERATURE_MSG_ID
        | GET_PRESSURE_MSG_ID
        | GET_HUMIDITY_MSG_ID
        | GET_LIGHT_LEVEL_MSG_ID
        | TEMPERATURE_REPLY_MSG_ID
        | PRESSURE_REPLY_MSG_ID
        | HUMIDITY_REPLY_MSG_ID
        | LIGHT_LEVEL_REPLY_MSG_ID => {
            let m = msg_pool.lock().take().and_then(|m| {
                if let Some(payload) = payload_pool.lock().take() {
                    *m = mem::zeroed();
                    *payload = mem::zeroed();

                    m.msg_type = msg_type_id;
                    m.data = payload as *mut MessagePayload as *mut CVoid;

                    Some(m)
                } else {
                    meteo_free(m);
                    None
                }
            });

            m.unwrap_or(&mut *ptr::null_mut())
        }
        _ => ptr::null_mut(),
    }
}

unsafe extern "C" fn meteo_free(msg: *mut md::message) {
    if msg.is_null() {
        return;
    }

    let msg_pool = MESSAGE_POOL.as_ref().expect("message pool not initialized");
    let payload_pool = MESSAGE_PAYLOAD_POOL
        .as_ref()
        .expect("message payload pool not initialized");

    match (*msg).msg_type {
        GET_TEMPERATURE_MSG_ID
        | GET_PRESSURE_MSG_ID
        | GET_HUMIDITY_MSG_ID
        | GET_LIGHT_LEVEL_MSG_ID
        | TEMPERATURE_REPLY_MSG_ID
        | PRESSURE_REPLY_MSG_ID
        | HUMIDITY_REPLY_MSG_ID
        | LIGHT_LEVEL_REPLY_MSG_ID => {
            if !(*msg).data.is_null() {
                payload_pool
                    .lock()
                    .give(&mut *((*msg).data as *mut MessagePayload));
            }
            msg_pool.lock().give(&mut *msg);
        }
        _ => {}
    }
}

unsafe extern "C" fn single_channel_num_parse(
    msg_ptr: *mut md::message,
    save_ptr: *mut u8,
) -> bool {
    if msg_ptr.is_null() || (*msg_ptr).data.is_null() || save_ptr.is_null() {
        return false;
    }

    let payload = unwrap_or_ret_false!(md::cstr_ptr_to_str_slice(save_ptr));
    let ch = unwrap_or_ret_false!(payload.parse::<u32>());

    let msg_data = (*msg_ptr).data as *mut MessagePayload;
    (*msg_data).channel_num_payload = ch;

    true
}

unsafe extern "C" fn std_response_serialize(
    msg_ptr: *const md::message,
    output_str: *mut u8,
    output_str_max_len: u32,
) -> isize {
    if msg_ptr.is_null() || (*msg_ptr).data.is_null() {
        return -1;
    }

    let buf = slice::from_raw_parts_mut(output_str, output_str_max_len as usize);
    let mut w = md::CStrWriter::new(buf);

    let payload = &*((*msg_ptr).data as *const StdResponsePayload);

    if write!(w, ",{},{}", payload.channel, payload.value).is_ok() {
        w.len() as isize
    } else {
        -1
    }
}

enum IncomingMsg<'payload> {
    GetTemperature(&'payload u32),
    GetPressure(&'payload u32),
    GetHumidity(&'payload u32),
    GetLightLevel(&'payload u32),
}

derive_message_wrapper!(IncomingMsg<'payload>, [
    GET_TEMPERATURE_MSG_ID => (|payload_ptr| {
        Ok(IncomingMsg::GetTemperature(&*(payload_ptr as *const u32)))
    }),
    GET_PRESSURE_MSG_ID => (|payload_ptr| {
        Ok(IncomingMsg::GetPressure(&*(payload_ptr as *const u32)))
    }),
    GET_HUMIDITY_MSG_ID => (|payload_ptr| {
        Ok(IncomingMsg::GetHumidity(&*(payload_ptr as *const u32)))
    }),
    GET_LIGHT_LEVEL_MSG_ID => (|payload_ptr| {
        Ok(IncomingMsg::GetLightLevel(&*(payload_ptr as *const u32)))
    })],
    meteo_alloc,
    meteo_free);

struct TemperatureReply<'payload>(&'payload mut u32, &'payload mut f32);

derive_message_wrapper!(TemperatureReply<'payload>,
    TEMPERATURE_REPLY_MSG_ID => (|payload_ptr| {
            let payload = &mut *(payload_ptr as *mut StdResponsePayload);
            Ok(TemperatureReply(&mut payload.channel, &mut payload.value))
    }),
    meteo_alloc,
    meteo_free);

struct PressureReply<'payload>(&'payload mut u32, &'payload mut f32);

derive_message_wrapper!(PressureReply<'payload>,
    PRESSURE_REPLY_MSG_ID => (|payload_ptr| {
            let payload = &mut *(payload_ptr as *mut StdResponsePayload);
            Ok(PressureReply(&mut payload.channel, &mut payload.value))
    }),
    meteo_alloc,
    meteo_free);

struct HumidityReply<'payload>(&'payload mut u32, &'payload mut f32);

derive_message_wrapper!(HumidityReply<'payload>,
    HUMIDITY_REPLY_MSG_ID => (|payload_ptr| {
            let payload = &mut *(payload_ptr as *mut StdResponsePayload);
            Ok(HumidityReply(&mut payload.channel, &mut payload.value))
    }),
    meteo_alloc,
    meteo_free);

struct LightLevelReply<'payload>(&'payload mut u32, &'payload mut f32);

derive_message_wrapper!(LightLevelReply<'payload>,
    LIGHT_LEVEL_REPLY_MSG_ID => (|payload_ptr| {
            let payload = &mut *(payload_ptr as *mut StdResponsePayload);
            Ok(LightLevelReply(&mut payload.channel, &mut payload.value))
    }),
    meteo_alloc,
    meteo_free);

static mut RX_QUEUE_ARR: Option<[*mut md::message; 20]> = None;
static mut RX_QUEUE: Option<Mailbox<*mut md::message>> = None;

static mut TX_QUEUE_ARR: Option<[*mut md::message; 20]> = None;
static mut TX_QUEUE: Option<Mailbox<*mut md::message>> = None;

static mut ERR_QUEUE_ARR: Option<[i32; 20]> = None;
static mut ERR_QUEUE: Option<Mailbox<i32>> = None;

static mut MSG_HANDLERS: Option<[md::message_handler; 8]> = None;
static mut MSG_CONF: Option<md::subsystem_message_conf> = None;

struct MeteoTaskCtx<'mb, 'mem: 'mb> {
    rx_msg_queue: RxChannelSpsc<'mb, 'mem, *mut md::message>,
    tx_msg_queue: TxChannelSpsc<'mb, 'mem, *mut md::message>,
    err_msg_queue: TxChannelSpsc<'mb, 'mem, i32>,
    press_sensor: bmp085::Bmp085,
    light_sensor: tsl2651::Tsl2561,
}

impl<'mb, 'mem> MeteoTaskCtx<'mb, 'mem> {
    fn send_msg<T>(&self, msg: md::MessageWrapper<T>) -> Result<(), MeteoError>
    where
        T: md::Wrappable,
    {
        self.tx_msg_queue.try_send(msg.into()).or_else(|err| {
            T::free(err.0);
            Err(MeteoError::TxQueueFullError)
        })
    }

    fn send_err(&self, err: MeteoError) {
        let _ = self.err_msg_queue.try_send(err as i32);
    }
}

static mut METEO_CTX: Option<MeteoTaskCtx> = None;

#[no_mangle]
pub unsafe extern "C" fn rust_meteo_init() -> *const CVoid {
    if METEO_CTX.is_some() {
        panic!("Meteo module already initialized.");
    }

    RX_QUEUE_ARR = Some(mem::zeroed());
    RX_QUEUE = Some(Mailbox::new(RX_QUEUE_ARR.as_mut().unwrap().as_mut()));

    TX_QUEUE_ARR = Some(mem::zeroed());
    TX_QUEUE = Some(Mailbox::new(TX_QUEUE_ARR.as_mut().unwrap().as_mut()));

    ERR_QUEUE_ARR = Some(mem::zeroed());
    ERR_QUEUE = Some(Mailbox::new(ERR_QUEUE_ARR.as_mut().unwrap().as_mut()));

    MESSAGE_ARR = Some(mem::zeroed());
    MESSAGE_POOL = Some(CriticalLock::new(Pool::new(
        MESSAGE_ARR.as_mut().unwrap().as_mut(),
    )));

    MESSAGE_PAYLOAD_ARR = Some(mem::zeroed());
    MESSAGE_PAYLOAD_POOL = Some(CriticalLock::new(Pool::new(
        MESSAGE_PAYLOAD_ARR.as_mut().unwrap().as_mut(),
    )));

    let (rx_msg_queue, _) = mailbox::channel_spsc(RX_QUEUE.as_mut().unwrap());
    let (_, tx_msg_queue) = mailbox::channel_spsc(TX_QUEUE.as_mut().unwrap());
    let (_, err_msg_queue) = mailbox::channel_spsc(ERR_QUEUE.as_mut().unwrap());

    MSG_HANDLERS = Some([
        md::message_handler {
            message_name: b"GET_TEMPERATURE\0" as *const u8,
            parsing_func: Some(single_channel_num_parse),
            serialization_func: None,
        },
        md::message_handler {
            message_name: b"GET_PRESSURE\0" as *const u8,
            parsing_func: Some(single_channel_num_parse),
            serialization_func: None,
        },
        md::message_handler {
            message_name: b"GET_HUMIDITY\0" as *const u8,
            parsing_func: Some(single_channel_num_parse),
            serialization_func: None,
        },
        md::message_handler {
            message_name: b"GET_LIGHT_LEVEL\0" as *const u8,
            parsing_func: Some(single_channel_num_parse),
            serialization_func: None,
        },
        md::message_handler {
            message_name: b"TEMPERATURE_REPLY\0" as *const u8,
            parsing_func: None,
            serialization_func: Some(std_response_serialize),
        },
        md::message_handler {
            message_name: b"PRESSURE_REPLY\0" as *const u8,
            parsing_func: None,
            serialization_func: Some(std_response_serialize),
        },
        md::message_handler {
            message_name: b"HUMIDITY_REPLY\0" as *const u8,
            parsing_func: None,
            serialization_func: Some(std_response_serialize),
        },
        md::message_handler {
            message_name: b"LIGHT_LEVEL_REPLY\0" as *const u8,
            parsing_func: None,
            serialization_func: Some(std_response_serialize),
        },
    ]);

    MSG_CONF = Some(md::subsystem_message_conf {
        subsystem_name: b"METEO\0" as *const u8,
        message_handlers: MSG_HANDLERS.as_ref().unwrap() as *const md::message_handler,
        num_message_types: MSG_HANDLERS.as_ref().unwrap().len() as u32,
        incoming_msg_queue: rx_msg_queue.get_raw_mailbox(),
        outgoing_msg_queue: tx_msg_queue.get_raw_mailbox(),
        outgoing_err_queue: err_msg_queue.get_raw_mailbox(),
        alloc_message: Some(meteo_alloc),
        free_message: Some(meteo_free),
    });

    md::dispatcher_register_subsystem(MSG_CONF.as_mut().unwrap());

    let mut press_sensor = bmp085::Bmp085::new(i2c::Peripheral::I2C3);
    press_sensor.set_precision(bmp085::Precision::UltraHigh);

    let light_sensor = tsl2651::Tsl2561::new(i2c::Peripheral::I2C3);

    METEO_CTX = Some(MeteoTaskCtx {
        rx_msg_queue,
        tx_msg_queue,
        err_msg_queue,
        press_sensor,
        light_sensor,
    });

    METEO_CTX.as_ref().unwrap() as *const MeteoTaskCtx as *const CVoid
}

fn process_msg(ctx: &mut MeteoTaskCtx, msg: md::MessageWrapper<IncomingMsg>) {
    let res = match *msg {
        IncomingMsg::GetTemperature(ch) => ctx.press_sensor.measure().and_then(|(t, _)| {
            md::MessageWrapper::new(msg.get_transaction_id(), TEMPERATURE_REPLY_MSG_ID)
                .map_err(|_| MeteoError::MsgCreationError)
                .and_then(|mut reply: md::MessageWrapper<TemperatureReply>| {
                    *reply.0 = *ch;
                    *reply.1 = t;

                    ctx.send_msg(reply)
                })
        }),
        IncomingMsg::GetPressure(ch) => ctx.press_sensor.measure().and_then(|(_, p)| {
            md::MessageWrapper::new(msg.get_transaction_id(), PRESSURE_REPLY_MSG_ID)
                .map_err(|_| MeteoError::MsgCreationError)
                .and_then(|mut reply: md::MessageWrapper<PressureReply>| {
                    *reply.0 = *ch;
                    *reply.1 = p;

                    ctx.send_msg(reply)
                })
        }),
        IncomingMsg::GetHumidity(ch) => {
            md::MessageWrapper::new(msg.get_transaction_id(), HUMIDITY_REPLY_MSG_ID)
                .map_err(|_| MeteoError::MsgCreationError)
                .and_then(|mut reply: md::MessageWrapper<HumidityReply>| {
                    *reply.0 = *ch;
                    *reply.1 = 0.0;

                    ctx.send_msg(reply)
                })
        }
        IncomingMsg::GetLightLevel(ch) => ctx.light_sensor
            .measure()
            .and_then(|level| {
                md::MessageWrapper::new(msg.get_transaction_id(), LIGHT_LEVEL_REPLY_MSG_ID)
                    .map_err(|_| MeteoError::MsgCreationError)
                    .and_then(|mut reply: md::MessageWrapper<LightLevelReply>| {
                        *reply.0 = *ch;
                        *reply.1 = level;

                        ctx.send_msg(reply)
                    })
            })
            .or_else(|err| {
                if err == MeteoError::SensorNotReadyError {
                    let _ = ctx.light_sensor.set_gain_and_integ_time(
                        tsl2651::Gain::Gain1x,
                        tsl2651::IntegrationTime::Time101ms,
                    );
                    let _ = ctx.light_sensor.set_state(tsl2651::State::On);
                }

                Err(err)
            }),
    };

    if let Err(e) = res {
        ctx.send_err(e);
    }
}

#[no_mangle]
pub extern "C" fn rust_meteo_comm_loop(ctx_raw: *mut CVoid) {
    let ctx = unsafe { &mut *(ctx_raw as *mut MeteoTaskCtx) };

    if let Ok(msg_ptr) = ctx.rx_msg_queue.try_recv() {
        if let Ok(msg) = md::MessageWrapper::try_from(msg_ptr) {
            process_msg(ctx, msg);
        } else {
            ctx.send_err(MeteoError::InvalidMsgError);
        }
    }

    tasks::sleep(50);
}
