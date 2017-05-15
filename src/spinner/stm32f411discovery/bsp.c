/**
 * @file
 *
 * This file contains the implementation of the Spinner specific BSP functions for STM32F411 Discovery.
 */

#include <math.h> // For fabsf()

#include <libopencm3/cm3/assert.h> // For the cm3_assert macros.
#include <libopencm3/stm32/gpio.h> // For the GPIO manipulation functions.
#include <libopencm3/stm32/timer.h> // For the timer functions.
#include <libopencm3/stm32/dma.h> // For the DMA controller.
#include <libopencm3/cm3/nvic.h> // For nvic* functions.
#include <libopencm3/stm32/rcc.h> // For the RCC manipulation functions.
#include <libopencm3/cm3/cortex.h> // For the critical section macros.

#include <mouros/common.h> // For ARRAY_SIZE()

#include "../bsp.h"

static uint32_t stepper_pole_states_fwd[] = {
	0b1000 << 16 | 0b0001,
	0b0000 << 16 | 0b0011,
	0b0001 << 16 | 0b0010,
	0b0000 << 16 | 0b0110,
	0b0010 << 16 | 0b0100,
	0b0000 << 16 | 0b1100,
	0b0100 << 16 | 0b1000,
	0b0000 << 16 | 0b1001,
};

static uint32_t stepper_pole_states_rev[] = {
	0b0010 << 16 | 0b0001,
	0b0000 << 16 | 0b1001,
	0b0001 << 16 | 0b1000,
	0b0000 << 16 | 0b1100,
	0b1000 << 16 | 0b0100,
	0b0000 << 16 | 0b0110,
	0b0100 << 16 | 0b0010,
	0b0000 << 16 | 0b0011,
};

static bool stepper_dir_is_fwd = true;

static bool spinner_initialized = false;

/**
 * - Set the DMA controller to circular copy the pole settings to GPIO.
 * - TIM1 will generate the timebase.
 * - TIM3 will keep count of the position (it is set up to use TIM1 overflows as
 *   a clock)
 */
void bsp_spinner_init(void)
{
	// Initialize the output pins
	rcc_periph_clock_enable(RCC_GPIOD);

	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
	                GPIO0 | GPIO1 | GPIO2 | GPIO3);


	// Initialize DMA controller (DMA2 - Channel 6 - Stream 5 - TIM1_UP)
	rcc_periph_clock_enable(RCC_DMA2);

	dma_stream_reset(DMA2, DMA_STREAM5);

	dma_set_priority(DMA2, DMA_STREAM5, DMA_SxCR_PL_LOW);

	dma_set_memory_size(DMA2, DMA_STREAM5, DMA_SxCR_MSIZE_32BIT);
	dma_set_peripheral_size(DMA2, DMA_STREAM5, DMA_SxCR_PSIZE_32BIT);

	dma_enable_memory_increment_mode(DMA2, DMA_STREAM5);
	dma_disable_peripheral_increment_mode(DMA2, DMA_STREAM5);
	dma_enable_circular_mode(DMA2, DMA_STREAM5);

	dma_set_transfer_mode(DMA2, DMA_STREAM5, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);

	dma_set_memory_burst(DMA2, DMA_STREAM5, DMA_SxCR_MBURST_SINGLE);
	dma_set_peripheral_burst(DMA2, DMA_STREAM5, DMA_SxCR_PBURST_SINGLE);

	dma_set_peripheral_address(DMA2, DMA_STREAM5, (uint32_t) &GPIOD_BSRR);
	dma_set_memory_address(DMA2, DMA_STREAM5, (uint32_t) stepper_pole_states_fwd);

	dma_set_number_of_data(DMA2, DMA_STREAM5, ARRAY_SIZE(stepper_pole_states_fwd));

	dma_enable_direct_mode(DMA2, DMA_STREAM5);

	dma_channel_select(DMA2, DMA_STREAM5, DMA_SxCR_CHSEL_6);

	dma_clear_interrupt_flags(DMA2, DMA_STREAM5,
			DMA_TCIF | DMA_HTIF | DMA_TEIF | DMA_DMEIF | DMA_FEIF);


	dma_enable_stream(DMA2, DMA_STREAM5);



	// Initialize timer to drive DMA controller
	rcc_periph_clock_enable(RCC_TIM1);

	timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

	timer_enable_preload(TIM1);

	timer_set_prescaler(TIM1, 0xffff);
	timer_set_period(TIM1, 0xffff);

	timer_set_master_mode(TIM1, TIM_CR2_MMS_UPDATE);
	timer_update_on_overflow(TIM1);

	timer_generate_event(TIM1, TIM_EGR_UG);

	timer_enable_irq(TIM1, TIM_DIER_UDE);

	// This timer will keep contain the position (half pole count)
	rcc_periph_clock_enable(RCC_TIM3);
	timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

	timer_slave_set_mode(TIM3, TIM_SMCR_SMS_ECM1);
	timer_slave_set_trigger(TIM3, TIM_SMCR_TS_ITR0);

	timer_set_prescaler(TIM3, 0);
	timer_set_period(TIM3, 4096);

	timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_FROZEN);
	timer_set_oc_value(TIM3, TIM_OC1, 0xffff);

	timer_enable_counter(TIM3);

	nvic_set_priority(NVIC_TIM3_IRQ, 0);
	nvic_enable_irq(NVIC_TIM3_IRQ);


	spinner_initialized = true;
}


bool bsp_stepper_is_moving(uint8_t stepper_id)
{
	cm3_assert(spinner_initialized);
	if (stepper_id != 0) {
		return false;
	}

	return (bool) (TIM_CR1(TIM1) & TIM_CR1_CEN);
}

void bsp_stepper_start(uint8_t stepper_id)
{
	cm3_assert(spinner_initialized);
	if (stepper_id != 0) {
		return;
	}

	timer_enable_counter(TIM1);
}

void bsp_stepper_stop(uint8_t stepper_id)
{
	cm3_assert(spinner_initialized);
	if (stepper_id != 0) {
		return;
	}

	timer_disable_counter(TIM1);
}

float bsp_stepper_get_pos(uint8_t stepper_id)
{
	cm3_assert(spinner_initialized);
	if (stepper_id != 0) {
		return 0;
	}

	return ((float) timer_get_counter(TIM3) * 360.0f) / 4096.f;
}

void bsp_stepper_set_stop_pos(uint8_t stepper_id, float stop_pos_deg)
{
	cm3_assert(spinner_initialized);
	if (stepper_id != 0) {
		return;
	}

	float stop_pos_deg_adj = fmodf(stop_pos_deg, 360.0f);

	if (stop_pos_deg_adj < 0) {
		stop_pos_deg_adj += 360.0f;
	}

	uint32_t stop_pos = (uint32_t) ((4096.0f * stop_pos_deg_adj) / 360.0f + 0.5f);

	CM_ATOMIC_CONTEXT();
	timer_set_oc_value(TIM3, TIM_OC1, stop_pos);
	timer_clear_flag(TIM3, TIM_SR_CC1IF);

	timer_enable_irq(TIM3, TIM_DIER_CC1IE);
}

void bsp_stepper_remove_stop_pos(uint8_t stepper_id)
{
	cm3_assert(spinner_initialized);
	if (stepper_id != 0) {
		return;
	}

	timer_disable_irq(TIM3, TIM_DIER_CC1IE);
}

void bsp_stepper_set_rate(uint8_t stepper_id, float rate_pct)
{
	cm3_assert(spinner_initialized);
	if (stepper_id != 0) {
		return;
	}

	bool was_running = bsp_stepper_is_moving(stepper_id);
	float adj_rate_pct = (rate_pct > 100.0f) ?
	                         100.0f : ((rate_pct < -100.0f) ?
	                                      -100.0f : rate_pct);

	uint16_t presc = 0xffff;
	uint16_t ctr_val = 0xffff;
	bool new_dir_is_fwd = adj_rate_pct >= 0;

	float abs_rate_pct = fabsf(adj_rate_pct);
	if (abs_rate_pct > 0.1) {
		float tim_ticks_total = 9600000.0f / abs_rate_pct;
		float presc_flt = tim_ticks_total / 65536.0f;

		presc = (uint16_t) presc_flt;

		ctr_val = (uint16_t) (tim_ticks_total / (presc + 1.0f));
	}

	timer_disable_counter(TIM1);

	if (new_dir_is_fwd != stepper_dir_is_fwd) {
		dma_disable_stream(DMA2, DMA_STREAM5);

		if (new_dir_is_fwd) {
			timer_direction_up(TIM3);
			dma_set_memory_address(DMA2, DMA_STREAM5,
			         (uint32_t) stepper_pole_states_fwd);
		} else {
			timer_direction_down(TIM3);
			dma_set_memory_address(DMA2, DMA_STREAM5,
			         (uint32_t) stepper_pole_states_rev);
		}

		stepper_dir_is_fwd = new_dir_is_fwd;
		dma_enable_stream(DMA2, DMA_STREAM5);
	}

	timer_set_prescaler(TIM1, presc);
	timer_set_period(TIM1, ctr_val);

	timer_disable_counter(TIM3);
	timer_disable_irq(TIM1, TIM_DIER_UDE);

	timer_generate_event(TIM1, TIM_EGR_UG);

	timer_enable_irq(TIM1, TIM_DIER_UDE);
	timer_enable_counter(TIM3);

	if (!(presc == 0xffff && ctr_val == 0xffff) && was_running) {
		timer_enable_counter(TIM1);
	}
}

/**
 * Interrupt handler for TIM3 capture/compare interrupts.
 *
 * Gets called when stepper 0 hits its stop position.
 */
void tim3_isr(void)
{
	timer_clear_flag(TIM3, TIM_SR_CC1IF);

	bsp_stepper_stop(0);
}
