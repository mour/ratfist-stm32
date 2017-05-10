/**
 * @file
 *
 * This file contains the implementation of the Spinner specific BSP functions for STM32F072 Discovery.
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
	(0b1000 << 16 | 0b0001) << 6,
	(0b0000 << 16 | 0b0011) << 6,
	(0b0001 << 16 | 0b0010) << 6,
	(0b0000 << 16 | 0b0110) << 6,
	(0b0010 << 16 | 0b0100) << 6,
	(0b0000 << 16 | 0b1100) << 6,
	(0b0100 << 16 | 0b1000) << 6,
	(0b0000 << 16 | 0b1001) << 6,
};

static uint32_t stepper_pole_states_rev[] = {
	(0b0010 << 16 | 0b0001) << 6,
	(0b0000 << 16 | 0b1001) << 6,
	(0b0001 << 16 | 0b1000) << 6,
	(0b0000 << 16 | 0b1100) << 6,
	(0b1000 << 16 | 0b0100) << 6,
	(0b0000 << 16 | 0b0110) << 6,
	(0b0100 << 16 | 0b0010) << 6,
	(0b0000 << 16 | 0b0011) << 6,
};

static bool stepper_dir_is_fwd = true;

static bool spinner_initialized = false;

/**
 * - Set the DMA controller to circular copy the pole settings to GPIO.
 * - TIM15 will generate the timebase.
 * - TIM1 will keep count of the position (it is set up to use TIM1 overflows as
 *   a clock)
 */
void bsp_spinner_init(void)
{
	// Initialize the output pins
	rcc_periph_clock_enable(RCC_GPIOB);

	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLDOWN,
	                GPIO6 | GPIO7 | GPIO8 | GPIO9);



	// Initialize DMA controller (Channel 5 - TIM15_UP)
	rcc_periph_clock_enable(RCC_DMA);
	dma_channel_reset(DMA1, DMA_CHANNEL5);

	dma_set_priority(DMA1, DMA_CHANNEL5, DMA_CCR_PL_MEDIUM);

	dma_set_memory_address(DMA1, DMA_CHANNEL5, (uint32_t) stepper_pole_states_fwd);
	dma_set_number_of_data(DMA1, DMA_CHANNEL5, ARRAY_SIZE(stepper_pole_states_fwd));
	dma_set_memory_size(DMA1, DMA_CHANNEL5, DMA_CCR_MSIZE_32BIT);
	dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL5);

	dma_set_peripheral_address(DMA1, DMA_CHANNEL5, (uint32_t) &GPIOB_BSRR);
	dma_set_peripheral_size(DMA1, DMA_CHANNEL5, DMA_CCR_PSIZE_32BIT);
	dma_disable_peripheral_increment_mode(DMA1, DMA_CHANNEL5);

	dma_enable_circular_mode(DMA1, DMA_CHANNEL5);

	dma_set_read_from_memory(DMA1, DMA_CHANNEL5);

	dma_clear_interrupt_flags(DMA1, DMA_CHANNEL5, DMA_IFCR_CTEIF5);
	dma_clear_interrupt_flags(DMA1, DMA_CHANNEL5, DMA_IFCR_CGIF5);

	dma_enable_channel(DMA1, DMA_CHANNEL5);


	// Initialize timer to drive DMA controller
	rcc_periph_clock_enable(RCC_TIM15);

	timer_set_mode(TIM15, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

	timer_enable_preload(TIM15);

	timer_set_prescaler(TIM15, 0xffff);
	timer_set_period(TIM15, 0xffff);

	timer_set_master_mode(TIM15, TIM_CR2_MMS_UPDATE);

	timer_generate_event(TIM15, TIM_EGR_UG);

	timer_enable_irq(TIM15, TIM_DIER_UDE);


	// This timer will keep contain the position (half pole count)
	rcc_periph_clock_enable(RCC_TIM1);
	timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

	timer_slave_set_mode(TIM1, TIM_SMCR_SMS_ECM1);
	timer_slave_set_trigger(TIM1, TIM_SMCR_TS_ITR0);

	timer_set_prescaler(TIM1, 0);
	timer_set_period(TIM1, 4096);

	timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_FROZEN);
	timer_set_oc_value(TIM1, TIM_OC1, 0xffff);

	timer_enable_counter(TIM1);

	nvic_set_priority(NVIC_TIM1_CC_IRQ, 0);
	nvic_enable_irq(NVIC_TIM1_CC_IRQ);


	spinner_initialized = true;
}

bool bsp_stepper_is_moving(uint8_t stepper_id)
{
	cm3_assert(spinner_initialized);
	if (stepper_id != 0) {
		return false;
	}

	return (bool) (TIM_CR1(TIM15) & TIM_CR1_CEN);
}

void bsp_stepper_start(uint8_t stepper_id)
{
	cm3_assert(spinner_initialized);
	if (stepper_id != 0) {
		return;
	}

	timer_enable_counter(TIM15);
}

void bsp_stepper_stop(uint8_t stepper_id)
{
	cm3_assert(spinner_initialized);
	if (stepper_id != 0) {
		return;
	}

	timer_disable_counter(TIM15);
}

float bsp_stepper_get_pos(uint8_t stepper_id)
{
	cm3_assert(spinner_initialized);
	if (stepper_id != 0) {
		return 0;
	}

	float ret = ((float) timer_get_counter(TIM1) * 360.0f) / 4096.f;

	return ret;
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
	timer_set_oc_value(TIM1, TIM_OC1, stop_pos);
	timer_clear_flag(TIM1, TIM_SR_CC1IF);

	timer_enable_irq(TIM1, TIM_DIER_CC1IE);
}

void bsp_stepper_remove_stop_pos(uint8_t stepper_id)
{
	cm3_assert(spinner_initialized);
	if (stepper_id != 0) {
		return;
	}

	timer_disable_irq(TIM1, TIM_DIER_CC1IE);
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
		float tim_ticks_total = 4800000.0f / abs_rate_pct;
		float presc_flt = tim_ticks_total / 65536.0f;

		presc = (uint16_t) presc_flt;

		ctr_val = (uint16_t) (tim_ticks_total / (presc + 1.0f));
	}

	timer_disable_counter(TIM15);

	if (new_dir_is_fwd != stepper_dir_is_fwd) {
		dma_disable_channel(DMA1, DMA_CHANNEL5);

		if (new_dir_is_fwd) {
			timer_direction_up(TIM1);
			dma_set_memory_address(DMA1, DMA_CHANNEL5,
			         (uint32_t) stepper_pole_states_fwd);
		} else {
			timer_direction_down(TIM1);
			dma_set_memory_address(DMA1, DMA_CHANNEL5,
			         (uint32_t) stepper_pole_states_rev);
		}

		stepper_dir_is_fwd = new_dir_is_fwd;
		dma_enable_channel(DMA1, DMA_CHANNEL5);
	}

	timer_set_prescaler(TIM15, presc);
	timer_set_period(TIM15, ctr_val);

	timer_disable_counter(TIM1);
	timer_disable_irq(TIM15, TIM_DIER_UDE);

	timer_generate_event(TIM15, TIM_EGR_UG);

	timer_enable_irq(TIM15, TIM_DIER_UDE);
	timer_enable_counter(TIM1);

	if (!(presc == 0xffff && ctr_val == 0xffff) && was_running) {
		timer_enable_counter(TIM15);
	}
}

/**
 * Interrupt handler for TIM1 capture/compare interrupts.
 *
 * Gets called when stepper 0 hits its stop position.
 */
void tim1_cc_isr(void)
{
	timer_clear_flag(TIM1, TIM_SR_CC1IF);

	bsp_stepper_stop(0);
}

