#[allow(dead_code)]
pub mod periph_base_addr {
    pub const I2C1: usize = 1073763328;
    pub const I2C2: usize = 1073764352;
    pub const I2C3: usize = 1073765376;
    pub const GPIOA: usize = 0x40020000;
    pub const GPIOB: usize = 0x40020400;
    pub const GPIOC: usize = 0x40020800;
    pub const GPIOD: usize = 0x40020C00;
    pub const GPIOE: usize = 0x40021000;
    pub const GPIOH: usize = 0x40021C00;
    pub const RCC: usize = 0x40023800;
    pub const NVIC: usize = 0xE000E100;
    pub const SCB: usize = 0xE000E008;
}

pub mod nvic {
    use volatile_register::{RO, RW, WO};
    use super::scb;

    const NVIC_IPR_PRIO_OFFSET: u8 = 4;

    struct Nvic {
        iser: [RW<u32>; 3],
        _padding1: [u8; 0x74],
        icer: [RW<u32>; 3],
        _padding2: [u8; 0x74],
        _ispr: [RW<u32>; 3],
        _padding3: [u8; 0x74],
        _icpr: [RW<u32>; 3],
        _padding4: [u8; 0x74],
        _iabr: [RO<u32>; 3],
        _padding5: [u8; 0xf4],
        ipr: [RW<u8>; 81],
        _padding6: [u8; 0xaaf],
        _stir: WO<u32>,
    }

    #[allow(non_camel_case_types)]
    #[repr(u32)]
    #[derive(Clone, Copy)]
    pub enum Interrupt {
        Wwdg = 0,
        Exti16Pvd = 1,
        Exti21TampStamp = 2,
        Exti22RtcWkup = 3,
        Flash = 4,
        Rcc = 5,
        Exti0 = 6,
        Exti1 = 7,
        Exti2 = 8,
        Exti3 = 9,
        Exti4 = 10,
        Dma1Stream0 = 11,
        Dma1Stream1 = 12,
        Dma1Stream2 = 13,
        Dma1Stream3 = 14,
        Dma1Stream4 = 15,
        Dma1Stream5 = 16,
        Dma1Stream6 = 17,
        Adc = 18,
        Exti9_5 = 23,
        Tim1BrkTim9 = 24,
        Tim1UpTim10 = 25,
        Tim1TrgComTim11 = 26,
        Tim1Cc = 27,
        Tim2 = 28,
        Tim3 = 29,
        Tim4 = 30,
        I2C1Ev = 31,
        I2C1Er = 32,
        I2C2Ev = 33,
        I2C2Er = 34,
        Spi1 = 35,
        Spi2 = 36,
        Usart1 = 37,
        Usart2 = 38,
        Exti15_10 = 40,
        Exti17RtcAlarm = 41,
        Exti18OtgFsWkup = 42,
        Dma1Stream7 = 47,
        Sdio = 49,
        Tim5 = 50,
        Spi3 = 51,
        Dma2Stream0 = 56,
        Dma2Stream1 = 57,
        Dma2Stream2 = 58,
        Dma2Stream3 = 59,
        Dma2Stream4 = 60,
        OtgFs = 67,
        Dma2Stream5 = 68,
        Dma2Stream6 = 69,
        Dma2Stream7 = 70,
        Usart6 = 71,
        I2C3Ev = 72,
        I2C3Er = 73,
        Fpu = 81,
        Spi4 = 84,
        Spi5 = 85,
    }

    pub fn enable_interrupt(int: Interrupt) {
        let nvic = unsafe { &*(super::periph_base_addr::NVIC as *const Nvic) };

        let reg_no = (int as usize) / 32;
        let offset = (int as usize) % 32;

        unsafe {
            nvic.iser[reg_no].write(1 << offset);
        }
    }

    pub fn disable_interrupt(int: Interrupt) {
        let nvic = unsafe { &*(super::periph_base_addr::NVIC as *const Nvic) };

        let reg_no = (int as usize) / 32;
        let offset = (int as usize) % 32;

        unsafe {
            nvic.icer[reg_no].write(1 << offset);
        }
    }

    pub fn set_priority(int: Interrupt, prio: u8) {
        let nvic = unsafe { &*(super::periph_base_addr::NVIC as *const Nvic) };

        let bp_offset = get_prio_grouping_binpoint_offset(scb::get_priority_grouping());
        let prio_mask = 0xff << bp_offset << NVIC_IPR_PRIO_OFFSET;

        unsafe {
            nvic.ipr[int as usize]
                .modify(|curr| (curr & !prio_mask) | ((prio << bp_offset) << NVIC_IPR_PRIO_OFFSET));
        }
    }

    pub fn set_subpriority(int: Interrupt, subprio: u8) {
        let nvic = unsafe { &*(super::periph_base_addr::NVIC as *const Nvic) };

        let bp_offset = get_prio_grouping_binpoint_offset(scb::get_priority_grouping());
        let prio_mask = 0xff << bp_offset << NVIC_IPR_PRIO_OFFSET;

        let subprio_shifted_masked = (subprio << NVIC_IPR_PRIO_OFFSET) & !prio_mask;

        unsafe {
            nvic.ipr[int as usize].modify(|curr| (curr & prio_mask) | subprio_shifted_masked);
        }
    }

    fn get_prio_grouping_binpoint_offset(grouping: scb::ItPriorityGrouping) -> u8 {
        match grouping {
            scb::ItPriorityGrouping::SixteenNone => 0,
            scb::ItPriorityGrouping::EightTwo => 1,
            scb::ItPriorityGrouping::FourFour => 2,
            scb::ItPriorityGrouping::TwoFour => 3,
            scb::ItPriorityGrouping::NoneSixteen => 4,
        }
    }
}

pub mod scb {
    use volatile_register::{RO, RW};
    use core::mem;

    const SCB_AIRCR_VECTKEY: u32 = 0x05fa0000;
    const SCB_AIRCR_VECTKEY_MASK: u32 = 0xffff0000;
    const SCB_AIRCR_PRIGROUP_OFFSET: u32 = 8;
    const SCB_AIRCR_PRIGROUP_MASK: u32 = 0x00000700;

    #[repr(C)]
    struct Scb {
        _actlr: RW<u32>,
        _padding1: [u8; 0xcf4],
        _cpuid: RO<u32>,
        _icsr: RW<u32>,
        _vtor: RW<u32>,
        aircr: RW<u32>,
        _scr: RW<u32>,
        _ccr: RW<u32>,
        _shpr1: RW<u32>,
        _shpr2: RW<u32>,
        _shpr3: RW<u32>,
        _shcrs: RW<u32>,
        _cfsr: RW<u32>,
        _hfsr: RW<u32>,
        _padding2: [u8; 4],
        _mmar: RW<u32>,
        _bfar: RW<u32>,
        _afsr: RW<u32>,
    }

    #[repr(u32)]
    #[derive(Copy, Clone)]
    pub enum ItPriorityGrouping {
        SixteenNone = 0b000,
        EightTwo = 0b100,
        FourFour = 0b101,
        TwoFour = 0b110,
        NoneSixteen = 0b111,
    }

    pub fn set_priority_grouping(grouping: ItPriorityGrouping) {
        let scb = unsafe { &*(super::periph_base_addr::SCB as *const Scb) };

        unsafe {
            scb.aircr.modify(|curr| {
                (curr & !SCB_AIRCR_VECTKEY_MASK) | SCB_AIRCR_VECTKEY
                    | ((grouping as u32) << SCB_AIRCR_PRIGROUP_OFFSET)
            });
        }
    }

    pub fn get_priority_grouping() -> ItPriorityGrouping {
        let scb = unsafe { &*(super::periph_base_addr::SCB as *const Scb) };

        let val = (scb.aircr.read() & SCB_AIRCR_PRIGROUP_MASK) >> SCB_AIRCR_PRIGROUP_OFFSET;

        // Any combination of bits where they're 0b0xx means sixteen priorities, and zero subpriorities.
        if val & 0b100 == 0 {
            ItPriorityGrouping::SixteenNone
        } else {
            unsafe { mem::transmute(val) }
        }
    }
}

pub mod rcc {
    use volatile_register::RW;

    const RCC_AHB1_DMA2: u32 = 1 << 22;
    const RCC_AHB1_DMA1: u32 = 1 << 21;
    const RCC_AHB1_CRC: u32 = 1 << 12;
    const RCC_AHB1_GPIOH: u32 = 1 << 7;
    const RCC_AHB1_GPIOE: u32 = 1 << 4;
    const RCC_AHB1_GPIOD: u32 = 1 << 3;
    const RCC_AHB1_GPIOC: u32 = 1 << 2;
    const RCC_AHB1_GPIOB: u32 = 1 << 1;
    const RCC_AHB1_GPIOA: u32 = 1;

    const RCC_AHB2_OTGFS: u32 = 1 << 7;

    const RCC_APB1_PWR: u32 = 1 << 28;
    const RCC_APB1_I2C3: u32 = 1 << 23;
    const RCC_APB1_I2C2: u32 = 1 << 22;
    const RCC_APB1_I2C1: u32 = 1 << 21;
    const RCC_APB1_USART2: u32 = 1 << 17;
    const RCC_APB1_SPI3: u32 = 1 << 15;
    const RCC_APB1_SPI2: u32 = 1 << 14;
    const RCC_APB1_WWDG: u32 = 1 << 11;
    const RCC_APB1_TIM5: u32 = 1 << 3;
    const RCC_APB1_TIM4: u32 = 1 << 2;
    const RCC_APB1_TIM3: u32 = 1 << 1;
    const RCC_APB1_TIM2: u32 = 1;

    const RCC_APB2_SPI5: u32 = 1 << 20;
    const RCC_APB2_TIM11: u32 = 1 << 18;
    const RCC_APB2_TIM10: u32 = 1 << 17;
    const RCC_APB2_TIM9: u32 = 1 << 16;
    const RCC_APB2_SYSCFG: u32 = 1 << 14;
    const RCC_APB2_SPI4: u32 = 1 << 13;
    const RCC_APB2_SPI1: u32 = 1 << 12;
    const RCC_APB2_SDIO: u32 = 1 << 11;
    const RCC_APB2_ADC1: u32 = 1 << 8;
    const RCC_APB2_USART6: u32 = 1 << 5;
    const RCC_APB2_USART1: u32 = 1 << 4;
    const RCC_APB2_TIM1: u32 = 1;

    struct Rcc {
        _cr: RW<u32>,
        _pllcfgr: RW<u32>,
        _cfgr: RW<u32>,
        _cir: RW<u32>,
        ahb1rstr: RW<u32>,
        ahb2rstr: RW<u32>,
        _padding1: RW<u32>,
        _padding2: RW<u32>,
        apb1rstr: RW<u32>,
        apb2rstr: RW<u32>,
        _padding3: RW<u32>,
        _padding4: RW<u32>,
        ahb1enr: RW<u32>,
        ahb2enr: RW<u32>,
        _padding5: RW<u32>,
        _padding6: RW<u32>,
        apb1enr: RW<u32>,
        apb2enr: RW<u32>,
        _padding7: RW<u32>,
        _padding8: RW<u32>,
        _ahb1lpenr: RW<u32>,
        _ahb2lpenr: RW<u32>,
        _padding9: RW<u32>,
        _padding10: RW<u32>,
        _apb1lpenr: RW<u32>,
        _apb2lpenr: RW<u32>,
        _padding11: RW<u32>,
        _padding12: RW<u32>,
        _bdcr: RW<u32>,
        _csr: RW<u32>,
        _padding13: RW<u32>,
        _padding14: RW<u32>,
        _sscgr: RW<u32>,
        _plli2scfgr: RW<u32>,
        _dckcfgr: RW<u32>,
    }

    #[derive(Copy, Clone)]
    enum Bus {
        Ahb1,
        Ahb2,
        Apb1,
        Apb2,
    }

    #[derive(Copy, Clone)]
    pub enum Peripheral {
        GpioA,
        GpioB,
        GpioC,
        GpioD,
        GpioE,
        GpioH,
        Crc,
        Dma1,
        Dma2,

        OtgFs,

        Tim2,
        Tim3,
        Tim4,
        Tim5,
        Wwdg,
        Spi2,
        Spi3,
        Usart2,
        I2C1,
        I2C2,
        I2C3,
        Pwr,

        Tim1,
        Usart1,
        Usart6,
        Adc1,
        Sdio,
        Spi1,
        Spi4,
        SysCfg,
        Tim9,
        Tim10,
        Tim11,
        Spi5,
    }

    impl Peripheral {
        fn get_bus(&self) -> Bus {
            match *self {
                Peripheral::GpioA
                | Peripheral::GpioB
                | Peripheral::GpioC
                | Peripheral::GpioD
                | Peripheral::GpioE
                | Peripheral::GpioH
                | Peripheral::Crc
                | Peripheral::Dma1
                | Peripheral::Dma2 => Bus::Ahb1,
                Peripheral::OtgFs => Bus::Ahb2,
                Peripheral::Tim2
                | Peripheral::Tim3
                | Peripheral::Tim4
                | Peripheral::Tim5
                | Peripheral::Wwdg
                | Peripheral::Spi2
                | Peripheral::Spi3
                | Peripheral::Usart2
                | Peripheral::I2C1
                | Peripheral::I2C2
                | Peripheral::I2C3
                | Peripheral::Pwr => Bus::Apb1,
                Peripheral::Tim1
                | Peripheral::Usart1
                | Peripheral::Usart6
                | Peripheral::Adc1
                | Peripheral::Sdio
                | Peripheral::Spi1
                | Peripheral::Spi4
                | Peripheral::SysCfg
                | Peripheral::Tim9
                | Peripheral::Tim10
                | Peripheral::Tim11
                | Peripheral::Spi5 => Bus::Apb2,
            }
        }

        fn get_mask(&self) -> u32 {
            match *self {
                Peripheral::GpioA => RCC_AHB1_GPIOA,
                Peripheral::GpioB => RCC_AHB1_GPIOB,
                Peripheral::GpioC => RCC_AHB1_GPIOC,
                Peripheral::GpioD => RCC_AHB1_GPIOD,
                Peripheral::GpioE => RCC_AHB1_GPIOE,
                Peripheral::GpioH => RCC_AHB1_GPIOH,
                Peripheral::Crc => RCC_AHB1_CRC,
                Peripheral::Dma1 => RCC_AHB1_DMA1,
                Peripheral::Dma2 => RCC_AHB1_DMA2,
                Peripheral::OtgFs => RCC_AHB2_OTGFS,
                Peripheral::Tim2 => RCC_APB1_TIM2,
                Peripheral::Tim3 => RCC_APB1_TIM3,
                Peripheral::Tim4 => RCC_APB1_TIM4,
                Peripheral::Tim5 => RCC_APB1_TIM5,
                Peripheral::Wwdg => RCC_APB1_WWDG,
                Peripheral::Spi2 => RCC_APB1_SPI2,
                Peripheral::Spi3 => RCC_APB1_SPI3,
                Peripheral::Usart2 => RCC_APB1_USART2,
                Peripheral::I2C1 => RCC_APB1_I2C1,
                Peripheral::I2C2 => RCC_APB1_I2C2,
                Peripheral::I2C3 => RCC_APB1_I2C3,
                Peripheral::Pwr => RCC_APB1_PWR,
                Peripheral::Tim1 => RCC_APB2_TIM1,
                Peripheral::Usart1 => RCC_APB2_USART1,
                Peripheral::Usart6 => RCC_APB2_USART6,
                Peripheral::Adc1 => RCC_APB2_ADC1,
                Peripheral::Sdio => RCC_APB2_SDIO,
                Peripheral::Spi1 => RCC_APB2_SPI1,
                Peripheral::Spi4 => RCC_APB2_SPI4,
                Peripheral::SysCfg => RCC_APB2_SYSCFG,
                Peripheral::Tim9 => RCC_APB2_TIM9,
                Peripheral::Tim10 => RCC_APB2_TIM10,
                Peripheral::Tim11 => RCC_APB2_TIM11,
                Peripheral::Spi5 => RCC_APB2_SPI5,
            }
        }
    }

    pub fn enable_periph(periph: Peripheral) {
        let rcc = unsafe { &*(super::periph_base_addr::RCC as *const Rcc) };

        let reg = match periph.get_bus() {
            Bus::Ahb1 => &rcc.ahb1enr,
            Bus::Ahb2 => &rcc.ahb2enr,
            Bus::Apb1 => &rcc.apb1enr,
            Bus::Apb2 => &rcc.apb2enr,
        };

        unsafe {
            reg.modify(|curr| curr | periph.get_mask());
        }
    }

    pub fn disable_periph(periph: Peripheral) {
        let rcc = unsafe { &*(super::periph_base_addr::RCC as *const Rcc) };

        let reg = match periph.get_bus() {
            Bus::Ahb1 => &rcc.ahb1enr,
            Bus::Ahb2 => &rcc.ahb2enr,
            Bus::Apb1 => &rcc.apb1enr,
            Bus::Apb2 => &rcc.apb2enr,
        };

        unsafe {
            reg.modify(|curr| curr & !periph.get_mask());
        }
    }

    pub fn reset_periph(periph: Peripheral) {
        let rcc = unsafe { &*(super::periph_base_addr::RCC as *const Rcc) };

        let reg = match periph.get_bus() {
            Bus::Ahb1 => &rcc.ahb1rstr,
            Bus::Ahb2 => &rcc.ahb2rstr,
            Bus::Apb1 => &rcc.apb1rstr,
            Bus::Apb2 => &rcc.apb2rstr,
        };

        unsafe {
            reg.modify(|curr| curr | periph.get_mask());
            reg.modify(|curr| curr & !periph.get_mask());
        }
    }
}

pub fn board_specific_init() {
    scb::set_priority_grouping(scb::ItPriorityGrouping::EightTwo);
}
