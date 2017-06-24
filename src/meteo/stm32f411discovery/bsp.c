
#include <libopencm3/cm3/nvic.h>

#include <stdint.h>

#define I2C1_PERIPH_ID 0

void rust_i2c_interrupt_handler(uint32_t periph_id);

void i2c1_ev_isr(void) {
	rust_i2c_interrupt_handler(I2C1_PERIPH_ID);
}
