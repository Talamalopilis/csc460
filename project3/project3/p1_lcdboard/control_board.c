#include "os.h"
#include "struct.h"
#include <pins_arduino.h>
#include <wiring_private.h>
#include "UART/usart.h"

extern void lcd_task();

static uint8_t analog_reference = DEFAULT;

static char cruise_out;
static char escape_out;
static char user_out;

union system_data sdata;

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
	DDRC &= ~0x01;
	PORTC |= 0x01;
	for(;;) {
		sdata.state.sjs_x = analogRead(PIN_A8);
		sdata.state.sjs_y = analogRead(PIN_A9);
		sdata.state.rjs_x = analogRead(PIN_A10);
		sdata.state.rjs_y = analogRead(PIN_A11);
		sdata.state.sjs_z = (PINC & 0x01) ^ 0x01;
		Task_Next();
	}
}

void laser_task() {
	DDRG |= 0x02;
	for(;;) {
		if(sdata.state.sjs_z && sdata.state.laser_time > 0) {
			PORTG |= 0x02;
			--sdata.state.laser_time;
		} else {
			PORTG &= ~0x02;
		}
		Task_Next();
	}
}

void escape_task() {
	for(;;) {
		escape_out = NULL;
		Task_Next();
	}
}

void cruise_task() {
	for(;;) {
		cruise_out = 'f';
		Task_Next();
	}
}



void user_ai_task() {
	for(;;) {
		if(sdata.state.rjs_x > 600 || sdata.state.rjs_x < 400 || sdata.state.rjs_y > 600 || sdata.state.rjs_y < 400) {
			user_out = 'f';
		} else {
			user_out = NULL;
		}
		Task_Next();
	}	
}

void choose_ai_routine() {
	for(;;) {
		if(escape_out != NULL) {
			sdata.state.action_source = ESCAPE;
			sdata.state.current_action = escape_out;
		} else if (user_out != NULL) {
			sdata.state.action_source = USER;
			sdata.state.current_action = user_out;
		} else {
			sdata.state.action_source = CRUISE;
			sdata.state.current_action = cruise_out;
		}
		Task_Next();
	}
}

void send_bt() {
	int i;
	uart1_init(BAUD_CALC(115200));
	for (;;) {
		uart1_putc('$');
		for(i = 0; i < sizeof(struct system_state); i++) {
			uart1_putc(sdata.data[i]);
		}
		Task_Next();
	}
}

void a_main() {
	sdata.state.laser_time = 30000 / (MSECPERTICK * LASER_PERIOD);
	analogReference(DEFAULT);
	Task_Create_Period(joystick_task, 0, 5, 10, 0);
	Task_Create_Period(lcd_task, 0, 50, 100, 10);
	Task_Create_Period(laser_task, 0, LASER_PERIOD, 10, 1);
	Task_Create_Period(escape_task, 0, 10, 1, 2);
	Task_Create_Period(user_ai_task, 0, 10, 1, 3);
	Task_Create_Period(cruise_task, 0, 10, 1, 4);
	Task_Create_Period(choose_ai_routine, 0, 10, 1, 5);
	Task_Create_Period(send_bt, 0, 30, 1, 6);
}
