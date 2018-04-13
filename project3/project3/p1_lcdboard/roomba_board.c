#ifndef CONTROL
#include "os.h"
#include "struct.h"
#include <pins_arduino.h>
#include <wiring_private.h>
#include "UART/usart.h"

#define HIGH_BYTE(x) (x>>8)
#define LOW_BYTE(x)  (x&0xFF)

#define START 128   // start the Roomba's serial command interface
#define BAUD  129   // set the SCI's baudrate (default on full power cycle is 57600
#define CONTROL 130   // enable control via SCI
#define SAFE  131   // enter safe mode
#define FULL  132   // enter full mode
#define POWER 133   // put the Roomba to sleep
#define SPOT  134   // start spot cleaning cycle
#define CLEAN 135   // start normal cleaning cycle
#define MAX   136   // start maximum time cleaning cycle
#define DRIVE 137   // control wheels
#define MOTORS  138   // turn cleaning motors on or off
#define LEDS  139   // activate LEDs
#define SONG  140   // load a song into memory
#define PLAY  141   // play a song that was loaded using SONG
#define SENSORS 142   // retrieve one of the sensor packets
#define DOCK  143   // force the Roomba to seek its dock.
#define STOP 173

extern void lcd_task();

static uint8_t analog_reference = DEFAULT;

static char cruise_out;
static char escape_out;
static char user_out;

static struct roomba_state rs;

int tpos = 1500;
int ppos = 1500;

union system_data sdata;

char current_action;
char action_source;

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

void servo_task(){
	TCCR1A|=(1<<COM1A1)|(1<<COM1B1)|(1<<WGM11);
	TCCR1B|=(1<<WGM13)|(1<<WGM12)|(1<<CS11)|(1<<CS10);

	ICR1=4999;

	DDRB|=(1<<PB5)|(1<<PB6); // pin 11, 12
	while(1)
	{
		ppos += ((int)sdata.state.sjs_x - 509) / 50;
		tpos += ((int)sdata.state.sjs_y - 510) / 50;

		if (ppos > 2000)
		{
			ppos = 2000;
		}
		else if (ppos < 1000)
		{
			ppos = 1000;
		}
		if (tpos > 2000)
		{
			tpos = 2000;
		}
		else if (tpos < 1000)
		{
			tpos = 1000;
		}

		//sdata.state.tpos = tpos;
		//sdata.state.ppos = ppos;
		OCR1A=(3000 - ppos)/4;
		OCR1B=(tpos)/4;
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
		if(rs.bumper_pressed || rs.vwall_detected) {
			int i;
			for(i = 0; i < 10; ++i) {
				escape_out = 'b'; // reverse
				Task_Next();
			}
			for(i = 0; i < 10; ++i) {
				escape_out = 'r';
				Task_Next(); // turn right
			}
			escape_out = NULL;
		} else {
			escape_out = NULL;
		}
		
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
			action_source = ESCAPE;
			current_action = escape_out;
		} else if (user_out != NULL) {
			action_source = USER;
			current_action = user_out;
		} else {
			action_source = CRUISE;
			current_action = cruise_out;
		}
		Task_Next();
	}
}

void roomba_task() {
	uart0_init(BAUD_CALC(19200));
	uart2_init(BAUD_CALC(19200));
	uart2_putc(START);
	uart2_putc(SAFE);
	uart2_putc(LEDS);
	uart2_putc(4);
	uart2_putc(0);
	uart2_putc(0);
	for(;;){
		//if (uart0_AvailableBytes()){
		////uart2_putc(uart0_getc());
		//}
		
		uart2_putc(SENSORS);
		uart2_putc(7);
		char packet = 0;
		Task_Next();
		if (uart2_AvailableBytes()) {
			// populate roomba state (rs) here
			packet = uart2_getc();
			uart0_putint(packet);
			uart0_putc_('\n');
			
			if (packet == 1){
				uart2_putc(LEDS);
				uart2_putc(4);
				uart2_putc(0);
				uart2_putc(0);
			}
			if (packet == 2){
				uart2_putc(LEDS);
				uart2_putc(4);
				uart2_putc(0);
				uart2_putc(128);
			}
		}
		Task_Next();
	}
	
}

void a_main() {
	sdata.state.laser_time = 30000 / (MSECPERTICK * LASER_PERIOD);
	analogReference(DEFAULT);
	Task_Create_Period(laser_task, 0, LASER_PERIOD, 10, 1);
	Task_Create_Period(escape_task, 0, 5, 1, 2);
	Task_Create_Period(user_ai_task, 0, 5, 1, 3);
	Task_Create_Period(cruise_task, 0, 5, 1, 4);
	Task_Create_Period(choose_ai_routine, 0, 5, 1, 5);
	Task_Create_Period(roomba_task, 0, 20, 1000, 0);
}


#endif