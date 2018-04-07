#include "os.h"
#include "struct.h"
#include <pins_arduino.h>
#include <wiring_private.h>

extern void lcd_task();

uint8_t analog_reference = DEFAULT;

union system_data sdata;

uint16_t joystick_x = 0;
uint16_t joystick_y = 0;
uint8_t joystick_z = 0;

void analogReference(uint8_t mode) {
	// can't actually set the register here because the default setting
	// will connect AVCC and the AREF pin, which would cause a short if
	// there's something connected to AREF.
	analog_reference = mode;
}


int analogRead(uint8_t pin) {
	uint8_t low, high;
	if (pin >= 54) pin -= 54; // allow for channel or pin numbers

	// the MUX5 bit of ADCSRB selects whether we're reading from channels
	// 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
	ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	
	// set the analog reference (high two bits of ADMUX) and select the
	// channel (low 4 bits).  this also sets ADLAR (left-adjust result)
	// to 0 (the default).
	ADMUX = (analog_reference << 6) | (pin & 0x07);

	// start the conversion
	ADCSRA |= (1 << ADSC);
	ADCSRA |= (1 << ADEN);

	// ADSC is cleared when the conversion finishes
	while (bit_is_set(ADCSRA, ADSC));

	// we have to read ADCL first; doing so locks both ADCL
	// and ADCH until ADCH is read.  reading ADCL second would
	// cause the results of each conversion to be discarded,
	// as ADCL and ADCH would be locked when it completed.
	low  = ADCL;
	high = ADCH;

	// combine the two bytes
	return (high << 8) | low;
}

void joystick_task() {
	DDRC &= ~1;
	PORTC |= 1;
	for(;;) {
		sdata.state.sjs_x = analogRead(PIN_A8);
		sdata.state.sjs_y = analogRead(PIN_A9);
		sdata.state.rjs_x = analogRead(PIN_A10);
		sdata.state.rjs_y = analogRead(PIN_A11);
		sdata.state.sjs_z = (PINC & 1) ^ 1;
		Task_Next();
	}
}

void a_main() {
	analogReference(1);
	Task_Create_Period(joystick_task, 0, 5, 10, 0);
	Task_Create_Period(lcd_task, 0, 50, 100, 10);
}