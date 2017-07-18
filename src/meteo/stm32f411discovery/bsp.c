
#include <libopencm3/cm3/nvic.h>

#include <stdint.h>

#include "../../bsp.h"

#define I2C1_PERIPH_ID 0
#define I2C2_PERIPH_ID 1
#define I2C3_PERIPH_ID 2

void rust_i2c_interrupt_handler(uint32_t periph_id);

void i2c1_ev_isr(void) {
	rust_i2c_interrupt_handler(I2C1_PERIPH_ID);
}

void i2c2_ev_isr(void) {
	rust_i2c_interrupt_handler(I2C2_PERIPH_ID);
}

void i2c3_ev_isr(void) {
	rust_i2c_interrupt_handler(I2C3_PERIPH_ID);
}
