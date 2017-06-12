#![allow(dead_code)]

const BIT0: u32 = 1;
const BIT1: u32 = 2;
const BIT2: u32 = 4;
const BIT3: u32 = 8;
const BIT4: u32 = 16;
const BIT5: u32 = 32;
const BIT6: u32 = 64;
const BIT7: u32 = 128;
const BIT8: u32 = 256;
const BIT9: u32 = 512;
const BIT10: u32 = 1024;
const BIT11: u32 = 2048;
const BIT12: u32 = 4096;
const BIT13: u32 = 8192;
const BIT14: u32 = 16384;
const BIT15: u32 = 32768;
const BIT16: u32 = 65536;
const BIT17: u32 = 131072;
const BIT18: u32 = 262144;
const BIT19: u32 = 524288;
const BIT20: u32 = 1048576;
const BIT21: u32 = 2097152;
const BIT22: u32 = 4194304;
const BIT23: u32 = 8388608;
const BIT24: u32 = 16777216;
const BIT25: u32 = 33554432;
const BIT26: u32 = 67108864;
const BIT27: u32 = 134217728;
const BIT28: u32 = 268435456;
const BIT29: u32 = 536870912;
const BIT30: u32 = 1073741824;
const BIT31: u32 = 2147483648;
const PPBI_BASE: u32 = 3758096384;
const SCS_BASE: u32 = 3758153728;
const SYS_TICK_BASE: u32 = 3758153744;
const NVIC_BASE: u32 = 3758153984;
const SCB_BASE: u32 = 3758157056;
const MPU_BASE: u32 = 3758157200;
const PERIPH_BASE: u32 = 1073741824;
const PERIPH_BASE_APB1: u32 = 1073741824;
const PERIPH_BASE_APB2: u32 = 1073807360;
const PERIPH_BASE_AHB1: u32 = 1073872896;
const PERIPH_BASE_AHB2: u32 = 1342177280;
const PERIPH_BASE_AHB3: u32 = 1610612736;
const TIM2_BASE: u32 = 1073741824;
const TIM3_BASE: u32 = 1073742848;
const TIM4_BASE: u32 = 1073743872;
const TIM5_BASE: u32 = 1073744896;
const TIM6_BASE: u32 = 1073745920;
const TIM7_BASE: u32 = 1073746944;
const TIM12_BASE: u32 = 1073747968;
const TIM13_BASE: u32 = 1073748992;
const TIM14_BASE: u32 = 1073750016;
const RTC_BASE: u32 = 1073752064;
const WWDG_BASE: u32 = 1073753088;
const IWDG_BASE: u32 = 1073754112;
const I2S2_EXT_BASE: u32 = 1073755136;
const SPI2_BASE: u32 = 1073756160;
const SPI3_BASE: u32 = 1073757184;
const I2S3_EXT_BASE: u32 = 1073758208;
const USART2_BASE: u32 = 1073759232;
const USART3_BASE: u32 = 1073760256;
const UART4_BASE: u32 = 1073761280;
const UART5_BASE: u32 = 1073762304;
const I2C1_BASE: u32 = 1073763328;
const I2C2_BASE: u32 = 1073764352;
const I2C3_BASE: u32 = 1073765376;
const BX_CAN1_BASE: u32 = 1073767424;
const BX_CAN2_BASE: u32 = 1073768448;
const POWER_CONTROL_BASE: u32 = 1073770496;
const DAC_BASE: u32 = 1073771520;
const UART7_BASE: u32 = 1073772544;
const UART8_BASE: u32 = 1073773568;
const TIM1_BASE: u32 = 1073807360;
const TIM8_BASE: u32 = 1073808384;
const USART1_BASE: u32 = 1073811456;
const USART6_BASE: u32 = 1073812480;
const ADC1_BASE: u32 = 1073815552;
const ADC2_BASE: u32 = 1073815808;
const ADC3_BASE: u32 = 1073816064;
const ADC_COMMON_BASE: u32 = 1073816320;
const SDIO_BASE: u32 = 1073818624;
const SPI1_BASE: u32 = 1073819648;
const SPI4_BASE: u32 = 1073820672;
const SYSCFG_BASE: u32 = 1073821696;
const EXTI_BASE: u32 = 1073822720;
const TIM9_BASE: u32 = 1073823744;
const TIM10_BASE: u32 = 1073824768;
const TIM11_BASE: u32 = 1073825792;
const SPI5_BASE: u32 = 1073827840;
const SPI6_BASE: u32 = 1073828864;
const SAI1_BASE: u32 = 1073829888;
const LTDC_BASE: u32 = 1073833984;
const DSI_BASE: u32 = 1073835008;
const GPIO_PORT_A_BASE: u32 = 1073872896;
const GPIO_PORT_B_BASE: u32 = 1073873920;
const GPIO_PORT_C_BASE: u32 = 1073874944;
const GPIO_PORT_D_BASE: u32 = 1073875968;
const GPIO_PORT_E_BASE: u32 = 1073876992;
const GPIO_PORT_F_BASE: u32 = 1073878016;
const GPIO_PORT_G_BASE: u32 = 1073879040;
const GPIO_PORT_H_BASE: u32 = 1073880064;
const GPIO_PORT_I_BASE: u32 = 1073881088;
const GPIO_PORT_J_BASE: u32 = 1073882112;
const GPIO_PORT_K_BASE: u32 = 1073883136;
const CRC_BASE: u32 = 1073885184;
const RCC_BASE: u32 = 1073887232;
const FLASH_MEM_INTERFACE_BASE: u32 = 1073888256;
const BKPSRAM_BASE: u32 = 1073889280;
const DMA1_BASE: u32 = 1073897472;
const DMA2_BASE: u32 = 1073898496;
const ETHERNET_BASE: u32 = 1073905664;
const DMA2D_BASE: u32 = 1073917952;
const USB_OTG_HS_BASE: u32 = 1074003968;
const USB_OTG_FS_BASE: u32 = 1342177280;
const DCMI_BASE: u32 = 1342504960;
const CRYP_BASE: u32 = 1342570496;
const HASH_BASE: u32 = 1342571520;
const RNG_BASE: u32 = 1342572544;
const FMC_BANK1: u32 = 1610612736;
const FMC_BANK2: u32 = 1879048192;
const FMC_BANK3: u32 = 2147483648;
const QUADSPI_BANK: u32 = 2415919104;
const FSMC_BASE: u32 = 2684354560;
const FMC_BASE: u32 = 2684354560;
const QUADSPI_BASE: u32 = 2684358656;
const FMC_BANK5: u32 = 3221225472;
const FMC_BANK6: u32 = 3489660928;
const DBGMCU_BASE: u32 = 3758366720;
const DESIG_FLASH_SIZE_BASE: u32 = 536836642;
const DESIG_UNIQUE_ID_BASE: u32 = 536836624;

const I2C1: u32 = 1073763328;
const I2C2: u32 = 1073764352;

const I2C_CR1: u32 = 0x00;
const I2C_CR2: u32 = 0x04;
const I2C_OAR1: u32 = 0x08;
const I2C_OAR2: u32 = 0x0c;
const I2C_DR: u32 = 0x10;
const I2C_SR1: u32 = 0x14;
const I2C_SR2: u32 = 0x18;
const I2C_CCR: u32 = 0x1c;
const I2C_TRISE: u32 = 0x20;

const I2C_CR1_SWRST: u16 = 32768;
const I2C_CR1_ALERT: u16 = 8192;
const I2C_CR1_PEC: u16 = 4096;
const I2C_CR1_POS: u16 = 2048;
const I2C_CR1_ACK: u16 = 1024;
const I2C_CR1_STOP: u16 = 512;
const I2C_CR1_START: u16 = 256;
const I2C_CR1_NOSTRETCH: u16 = 128;
const I2C_CR1_ENGC: u16 = 64;
const I2C_CR1_ENPEC: u16 = 32;
const I2C_CR1_ENARP: u16 = 16;
const I2C_CR1_SMBTYPE: u16 = 8;
const I2C_CR1_SMBUS: u16 = 2;
const I2C_CR1_PE: u16 = 1;
const I2C_CR2_LAST: u16 = 4096;
const I2C_CR2_DMAEN: u16 = 2048;
const I2C_CR2_ITBUFEN: u16 = 1024;
const I2C_CR2_ITEVTEN: u16 = 512;
const I2C_CR2_ITERREN: u16 = 256;
const I2C_CR2_FREQ_2MHZ: u16 = 2;
const I2C_CR2_FREQ_3MHZ: u16 = 3;
const I2C_CR2_FREQ_4MHZ: u16 = 4;
const I2C_CR2_FREQ_5MHZ: u16 = 5;
const I2C_CR2_FREQ_6MHZ: u16 = 6;
const I2C_CR2_FREQ_7MHZ: u16 = 7;
const I2C_CR2_FREQ_8MHZ: u16 = 8;
const I2C_CR2_FREQ_9MHZ: u16 = 9;
const I2C_CR2_FREQ_10MHZ: u16 = 10;
const I2C_CR2_FREQ_11MHZ: u16 = 11;
const I2C_CR2_FREQ_12MHZ: u16 = 12;
const I2C_CR2_FREQ_13MHZ: u16 = 13;
const I2C_CR2_FREQ_14MHZ: u16 = 14;
const I2C_CR2_FREQ_15MHZ: u16 = 15;
const I2C_CR2_FREQ_16MHZ: u16 = 16;
const I2C_CR2_FREQ_17MHZ: u16 = 17;
const I2C_CR2_FREQ_18MHZ: u16 = 18;
const I2C_CR2_FREQ_19MHZ: u16 = 19;
const I2C_CR2_FREQ_20MHZ: u16 = 20;
const I2C_CR2_FREQ_21MHZ: u16 = 21;
const I2C_CR2_FREQ_22MHZ: u16 = 22;
const I2C_CR2_FREQ_23MHZ: u16 = 23;
const I2C_CR2_FREQ_24MHZ: u16 = 24;
const I2C_CR2_FREQ_25MHZ: u16 = 25;
const I2C_CR2_FREQ_26MHZ: u16 = 26;
const I2C_CR2_FREQ_27MHZ: u16 = 27;
const I2C_CR2_FREQ_28MHZ: u16 = 28;
const I2C_CR2_FREQ_29MHZ: u16 = 29;
const I2C_CR2_FREQ_30MHZ: u16 = 30;
const I2C_CR2_FREQ_31MHZ: u16 = 31;
const I2C_CR2_FREQ_32MHZ: u16 = 32;
const I2C_CR2_FREQ_33MHZ: u16 = 33;
const I2C_CR2_FREQ_34MHZ: u16 = 34;
const I2C_CR2_FREQ_35MHZ: u16 = 35;
const I2C_CR2_FREQ_36MHZ: u16 = 36;
const I2C_CR2_FREQ_37MHZ: u16 = 37;
const I2C_CR2_FREQ_38MHZ: u16 = 38;
const I2C_CR2_FREQ_39MHZ: u16 = 39;
const I2C_CR2_FREQ_40MHZ: u16 = 40;
const I2C_CR2_FREQ_41MHZ: u16 = 41;
const I2C_CR2_FREQ_42MHZ: u16 = 42;
const I2C_OAR1_ADDMODE: u16 = 32768;
const I2C_OAR1_ADDMODE_7BIT: u16 = 0;
const I2C_OAR1_ADDMODE_10BIT: u16 = 1;
const I2C_OAR2_ENDUAL: u16 = 1;
const I2C_SR1_SMBALERT: u16 = 32768;
const I2C_SR1_TIMEOUT: u16 = 16384;
const I2C_SR1_PECERR: u16 = 4096;
const I2C_SR1_OVR: u16 = 2048;
const I2C_SR1_AF: u16 = 1024;
const I2C_SR1_ARLO: u16 = 512;
const I2C_SR1_BERR: u16 = 256;
const I2C_SR1_TXE: u16 = 128;
const I2C_SR1_RXNE: u16 = 64;
const I2C_SR1_STOPF: u16 = 16;
const I2C_SR1_ADD10: u16 = 8;
const I2C_SR1_BTF: u16 = 4;
const I2C_SR1_ADDR: u16 = 2;
const I2C_SR1_SB: u16 = 1;
const I2C_SR2_DUALF: u16 = 128;
const I2C_SR2_SMBHOST: u16 = 64;
const I2C_SR2_SMBDEFAULT: u16 = 32;
const I2C_SR2_GENCALL: u16 = 16;
const I2C_SR2_TRA: u16 = 4;
const I2C_SR2_BUSY: u16 = 2;
const I2C_SR2_MSL: u16 = 1;
const I2C_CCR_FS: u16 = 32768;
const I2C_CCR_DUTY: u16 = 16384;
const I2C_CCR_DUTY_DIV2: u16 = 0;
const I2C_CCR_DUTY_16_DIV_9: u16 = 1;
const I2C_WRITE: u8 = 0;
const I2C_READ: u8 = 1;
const I2C3: u32 = 1073765376;



use super::InterruptContext;
use super::PeripheralState;
use super::Step;

use volatile_register::RW;

enum CommDir {
    Read,
    Write,
}

#[repr(C)]
struct I2C {
    cr1: RW<u16>,
    cr2: RW<u16>,
    oar1: RW<u16>,
    oar2: RW<u16>,
    dr: RW<u16>,
    sr1: RW<u16>,
    sr2: RW<u16>,
    ccr: RW<u16>,
    trise: RW<u16>,
    fltr: RW<u16>,
}

impl I2C {
    fn get_periph(periph_addr: usize) -> &'static I2C {
        unsafe { &*(periph_addr as *const I2C) }
    }
    
    fn enable_periph(&self) {
        unsafe {
            self.cr1.modify(|curr| curr | I2C_CR1_PE);
        }
    }

    fn disable_periph(&self) {
        unsafe {
            self.cr1.modify(|curr| curr & !I2C_CR1_PE);
        }
    }
    
    fn enable_ack(&self) {
        unsafe {
            self.cr1.modify(|curr| curr | I2C_CR1_ACK);
        }
    }

    fn disable_ack(&self) {
        unsafe {
            self.cr1.modify(|curr| curr & !I2C_CR1_ACK);
        }
    }
    
    fn disable_ack_after_next_byte(&self) {
        unsafe {
            self.cr1.modify(|curr| (curr & !I2C_CR1_ACK) | I2C_CR1_POS);
        }
    }
    
    fn send_start(&self) {
        unsafe {
            self.cr1.modify(|curr| curr | I2C_CR1_START);
        }
    }
        
    fn send_stop(&self) {
        unsafe {
            self.cr1.modify(|curr| curr | I2C_CR1_STOP)
        }
    }
    
    fn clear_addr_flag(&self) {
        self.sr1.read();
        self.sr2.read();
    }
    
    fn is_addr_set(&self) -> bool {
        self.sr1.read() & I2C_SR1_ADDR != 0
    }
    
    fn is_txe_set(&self) -> bool {
        self.sr1.read() & I2C_SR1_TXE != 0
    }
    
    fn is_rxne_set(&self) -> bool {
        self.sr1.read() & I2C_SR1_RXNE != 0
    }
    
    fn is_btf_set(&self) -> bool {
        self.sr1.read() & I2C_SR1_BTF != 0
    }

    fn send_addr(&self, addr: u8, dir: CommDir) {
        let dir_bit = match dir {
            CommDir::Read => 1,
            CommDir::Write => 0,
        };

        self.send_data(addr | dir_bit);
    }

    fn read_data(&self) -> u8 {
        self.dr.read() as u8
    }

    fn send_data(&self, data_byte: u8) {
        unsafe {
            self.dr.write(data_byte as u16);
        }
    }
}


#[no_mangle]
pub extern "C" fn interrupt_handler(it_context: *mut InterruptContext) {
    let ctx: &mut InterruptContext = unsafe { &mut *it_context };

    // Handle interrupts
    // SB - start bit sent - cleared automatically by reading SR1 & writing to DR
    // ADDR - (in master mode) address acked - cleared by reading SR1 & SR2
    // BTF - byte transfer finished - cleared by reading/writing from/to DR
    // RxNE - Rx buffer not empty - cleared by reading from DR
    // TxNE - Tx buffer not empty - cleared by writing to DR
    // Errors:
    // BERR - bus error - misplaced start or stop bit - Cleared by writing 0.
    // ARLO - arbitration lost - cleared by writing 0.
    // AF - acknowledge failure - set when no ACK received. Cleared by writing 0.
    let periph: &I2C = I2C::get_periph(ctx.periph_addr);

    // TODO Abort if an error flag is set

    if let Some(ref mut trans) = ctx.current_transaction {
        
        let curr_step = &mut trans.steps[trans.curr_step];

        match ctx.state {
            PeripheralState::StartBitSet => {
                // Send address
                match *curr_step {
                    Step::Read(_) => periph.send_addr(trans.device_addr, CommDir::Read),
                    Step::Write(_) => periph.send_addr(trans.device_addr, CommDir::Write),
                }
                ctx.state = PeripheralState::AddressSent;
            }

            PeripheralState::AddressSent => {
                if !periph.is_addr_set() {
                    return;
                }
                
                match *curr_step {
                    Step::Read(_) => {
                        if curr_step.len() == 1 {
                            periph.disable_ack();
                        } else if curr_step.len() == 2 {
                            periph.disable_ack_after_next_byte();
                        }

                        periph.clear_addr_flag();
                        
                        ctx.state = PeripheralState::Receiving(0);
                    }
                    Step::Write(ref data) => {
                        periph.clear_addr_flag();
                        periph.send_data(data[0]);
                        ctx.state = PeripheralState::Transmitting(1);
                    }
                }
            }

            PeripheralState::Transmitting(next_byte_pos) => {
                if (next_byte_pos >= curr_step.len() && !periph.is_btf_set()) || 
                    !periph.is_txe_set() {
                    return;
                }
                
                if next_byte_pos >= curr_step.len() {
                    periph.send_stop();
                    ctx.state = PeripheralState::Done(0);
                } else {
                    periph.send_data(curr_step.as_ref()[next_byte_pos]);
                    ctx.state = PeripheralState::Transmitting(next_byte_pos + 1);
                }
            }
            
            PeripheralState::Receiving(next_byte_pos) => {
                if next_byte_pos == curr_step.len() - 1 {
                    if !periph.is_btf_set() {
                        return;
                    }
                    
                    periph.send_stop();
                    
                    curr_step.as_mut()[next_byte_pos] = periph.read_data();
                    
                    ctx.state = PeripheralState::Done(0);
                    
                } else if next_byte_pos == curr_step.len() - 2 {
                    if !periph.is_btf_set() {
                        return;
                    }
                    
                    periph.send_stop();
                    
                    curr_step.as_mut()[next_byte_pos] = periph.read_data();
                    curr_step.as_mut()[next_byte_pos + 1] = periph.read_data();

                    ctx.state = PeripheralState::Done(0);                    
                    
                } else {
                    if !periph.is_rxne_set() {
                        return;
                    }
                    
                    if next_byte_pos == curr_step.len() - 3 {
                        periph.disable_ack();
                    }
                    
                    curr_step.as_mut()[next_byte_pos] = periph.read_data();
                    
                    ctx.state = PeripheralState::Receiving(next_byte_pos + 1);
                }
            }

            PeripheralState::Done(_) => {
                // Should not really happen?
            }
        }
    } else {
        // Shouldn't be getting interrupt if no transaction is set. Disable periph.
        periph.disable_periph();
    }
}
