
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
                Peripheral::GpioA | Peripheral::GpioB | Peripheral::GpioC | Peripheral::GpioD |
                Peripheral::GpioE | Peripheral::GpioH | Peripheral::Crc | Peripheral::Dma1 |
                Peripheral::Dma2 => Bus::Ahb1,
                Peripheral::OtgFs => Bus::Ahb2,
                Peripheral::Tim2 |
                Peripheral::Tim3 |
                Peripheral::Tim4 |
                Peripheral::Tim5 |
                Peripheral::Wwdg |
                Peripheral::Spi2 |
                Peripheral::Spi3 |
                Peripheral::Usart2 |
                Peripheral::I2C1 |
                Peripheral::I2C2 |
                Peripheral::I2C3 |
                Peripheral::Pwr => Bus::Apb1,
                Peripheral::Tim1 |
                Peripheral::Usart1 |
                Peripheral::Usart6 |
                Peripheral::Adc1 |
                Peripheral::Sdio |
                Peripheral::Spi1 |
                Peripheral::Spi4 |
                Peripheral::SysCfg |
                Peripheral::Tim9 |
                Peripheral::Tim10 |
                Peripheral::Tim11 |
                Peripheral::Spi5 => Bus::Apb2,
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



pub fn board_specific_init() {}
