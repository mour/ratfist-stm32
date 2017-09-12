
use mouros_rust_bindings::CVoid;
use mouros_rust_bindings::tasks;
use mouros_rust_bindings::tasks::CriticalLock;
use mouros_rust_bindings::pool_alloc::Pool;

use mouros_rust_bindings::mailbox;
use mouros_rust_bindings::mailbox::Mailbox;
use mouros_rust_bindings::mailbox::{TxChannelSpsc, RxChannelSpsc};


use bindings::message_dispatcher as md;

use bsp::i2c;


use core::ptr;

use core::mem;

mod bmp085;



const GET_TEMPERATURE_MSG_ID: u32 = 0;
const GET_HUMIDITY_MSG_ID: u32 = 1;



static mut MESSAGE_ARR: Option<[md::message; 20]> = None;
static mut MESSAGE_POOL: Option<CriticalLock<Pool<md::message>>> = None;


unsafe extern "C" fn meteo_alloc(msg_type_id: u32) -> *mut md::message {
    let pool = MESSAGE_POOL.as_ref().expect("message pool not initialized");

    match msg_type_id {
        GET_TEMPERATURE_MSG_ID |
        GET_HUMIDITY_MSG_ID => pool.lock().take().unwrap_or(&mut *ptr::null_mut()),
        _ => ptr::null_mut(),
    }
}

unsafe extern "C" fn meteo_free(msg: *mut md::message) {
    if msg.is_null() {
        return;
    }

    let pool = MESSAGE_POOL.as_ref().expect("message pool not initialized");

    match (*msg).msg_type {
        GET_TEMPERATURE_MSG_ID |
        GET_HUMIDITY_MSG_ID => pool.lock().give(&mut *msg),
        _ => {}
    }
}


unsafe extern "C" fn get_temperature_parse(_msg_ptr: *mut md::message, _save_ptr: *mut i8) -> bool {true}

unsafe extern "C" fn get_humidity_parse(_msg_ptr: *mut md::message, _save_ptr: *mut i8) -> bool {true}


enum IncomingMsg {
    GetTemperature,
    GetHumidity,
}

impl md::MemManagement for IncomingMsg {
    fn alloc(msg_type: u32) -> *mut md::message {
        unsafe { meteo_alloc(msg_type) }
    }

    fn free(msg_ptr: *mut md::message) {
        unsafe {
            meteo_free(msg_ptr);
        }
    }
}



static mut RX_QUEUE_ARR: Option<[*mut md::message; 20]> = None;
static mut RX_QUEUE: Option<Mailbox<*mut md::message>> = None;

static mut TX_QUEUE_ARR: Option<[*mut md::message; 20]> = None;
static mut TX_QUEUE: Option<Mailbox<*mut md::message>> = None;

static mut ERR_QUEUE_ARR: Option<[i32; 20]> = None;
static mut ERR_QUEUE: Option<Mailbox<i32>> = None;

static mut MSG_HANDLERS: Option<[md::message_handler; 2]> = None;
static mut MSG_CONF: Option<md::subsystem_message_conf> = None;



struct MeteoTaskCtx<'mb, 'mem: 'mb> {
    rx_msg_queue: RxChannelSpsc<'mb, 'mem, *mut md::message>,
    tx_msg_queue: TxChannelSpsc<'mb, 'mem, *mut md::message>,
    err_msg_queue: TxChannelSpsc<'mb, 'mem, i32>,
    hum_sensor: bmp085::Bmp085
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
    MESSAGE_POOL = Some(CriticalLock::new(
        Pool::new(MESSAGE_ARR.as_mut().unwrap().as_mut()),
    ));

    let (rx_msg_queue, _) = mailbox::channel_spsc(RX_QUEUE.as_mut().unwrap());
    let (_, tx_msg_queue) = mailbox::channel_spsc(TX_QUEUE.as_mut().unwrap());
    let (_, err_msg_queue) = mailbox::channel_spsc(ERR_QUEUE.as_mut().unwrap());


    MSG_HANDLERS = Some(
        [
            md::message_handler {
                message_name: b"GET_TEMPERATURE\0" as *const u8,
                parsing_func: Some(get_temperature_parse),
                serialization_func: None,
            },
            md::message_handler {
                message_name: b"GET_HUMIDITY\0" as *const u8,
                parsing_func: Some(get_humidity_parse),
                serialization_func: None,
            },
        ],
    );

    MSG_CONF = Some(md::subsystem_message_conf {
        subsystem_name: b"METEO\0" as *const u8,
        message_handlers: MSG_HANDLERS.as_ref().unwrap() as *const md::message_handler,
        num_message_types: 2,
        incoming_msg_queue: rx_msg_queue.get_raw_mailbox(),
        outgoing_msg_queue: tx_msg_queue.get_raw_mailbox(),
        outgoing_err_queue: err_msg_queue.get_raw_mailbox(),
        alloc_message: Some(meteo_alloc),
        free_message: Some(meteo_free),
    });



    md::dispatcher_register_subsystem(MSG_CONF.as_mut().unwrap());


    let mut hum_sensor = bmp085::Bmp085::new(i2c::Peripheral::I2C3);
    hum_sensor.set_precision(bmp085::Precision::UltraHigh);


    METEO_CTX = Some(MeteoTaskCtx {
        rx_msg_queue,
        tx_msg_queue,
        err_msg_queue,
        hum_sensor
    });

    METEO_CTX.as_ref().unwrap() as *const MeteoTaskCtx as *const CVoid
}

#[no_mangle]
pub extern "C" fn rust_meteo_comm_loop(ctx_raw: *mut CVoid) {

    let ctx = unsafe { &mut *(ctx_raw as *mut MeteoTaskCtx) };

    if let Ok((_temp, _press)) = ctx.hum_sensor.measure() {
        tasks::sleep(5000);
    };

    tasks::sleep(1000);
}
