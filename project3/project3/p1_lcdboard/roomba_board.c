#include "struct.h"
#ifndef CONTROL
#include "os.h"
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
static uint16_t light_sensor;

static uint8_t roomba_alive = 1;
static uint8_t move_switch = 1;
static TICK laser_time; 

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
void dead_task(){
	PORTG &= ~0x02;
		uart2_putc(DRIVE);
		uart2_putc(HIGH_BYTE(0));
		uart2_putc(LOW_BYTE(0));
		uart2_putc(HIGH_BYTE(0));
		uart2_putc(LOW_BYTE(0));
	for(;;);
}
void light_sensor_read() {
	
	//uart0_init(BAUD_CALC(9600));
	for (;;){
	light_sensor = analogRead(PIN_A1);
	//uart0_putint(light_sensor);
	//uart0_putc('\n');
	if(light_sensor > 950) {
		Task_Create_System(dead_task, 0);
	}
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
		if(sdata.state.sjs_z && laser_time > 0) {
			PORTG |= 0x02;
			--laser_time;
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
			for(i = 0; i < 75; ++i) {
				escape_out = 'b'; // reverse
				Task_Next();
			}
			for(i = 0; i < 75; ++i) {
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
		if(sdata.state.rjs_y < 200) {
			user_out = 'f';
		} else if (sdata.state.rjs_y > 800) {
			user_out = 'b';
		} else if (sdata.state.rjs_x > 800) {
			user_out = 'r';
		} else if (sdata.state.rjs_x < 200) {
			user_out = 'l';
		} else {
			if (sdata.state.rjs_z){
				user_out = 's';
			}
			else{
			user_out = NULL;
			}
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

void receive_bt() {
	uart0_init(BAUD_CALC(9600));
	uart1_init(BAUD_CALC(9600));
	for(;;){
		if (uart1_AvailableBytes() > sizeof(struct system_state)){
			while (uart1_peek() != '$') {
				if (uart1_AvailableBytes()) {
					uart1_getc();
				}
				else {
					Task_Next();
				}
			}
			if (uart1_AvailableBytes() > sizeof(struct system_state)) {
				uart1_getc();
				for (int i = 0; i < sizeof(struct system_state); i++){
					sdata.data[i] = uart1_getc();
				}
				//uart0_putc('\n');
				//uart0_putint(sdata.state.rjs_x);
				//uart0_putc('\n');
				//uart0_putint(sdata.state.rjs_y);
				//uart0_putc('\n');
				//uart0_putint(sdata.state.rjs_z);
				//uart0_putc('\n');
				//
				//uart0_putint(sdata.state.sjs_x);
				//uart0_putc('\n');
				//uart0_putint(sdata.state.sjs_y);
				//uart0_putc('\n');
				//uart0_putint(sdata.state.sjs_z);
				//uart0_putc('\n');
			}
		}
		Task_Next();
	}
}

void move_switch_task(){
	for (;;){
	move_switch ^= 1;
	Task_Next();
	}
}

void roomba_task() {
	//uart0_init(BAUD_CALC(19200));
	uart2_init(BAUD_CALC(19200));
	uart2_putc(START);
	uart2_putc(SAFE);
	uart2_putc(LEDS);
	uart2_putc(4);
	uart2_putc(0);
	uart2_putc(0);
	for(;;){
		
		switch(current_action) {
			case 'l':
				uart2_putc(DRIVE);
				uart2_putc(HIGH_BYTE(50));
				uart2_putc(LOW_BYTE(50));
				uart2_putc(HIGH_BYTE(1));
				uart2_putc(LOW_BYTE(1));
				break;
			case 'r':
				uart2_putc(DRIVE);
				uart2_putc(HIGH_BYTE(50));
				uart2_putc(LOW_BYTE(50));
				uart2_putc(HIGH_BYTE(-1));
				uart2_putc(LOW_BYTE(-1));
				break;
			case 'b':
				if (move_switch){
				uart2_putc(DRIVE);
				uart2_putc(HIGH_BYTE(-50));
				uart2_putc(LOW_BYTE(-50));
				uart2_putc(HIGH_BYTE(32768));
				uart2_putc(LOW_BYTE(32768));
				}
				else{
					uart2_putc(DRIVE);
					uart2_putc(HIGH_BYTE(0));
					uart2_putc(LOW_BYTE(0));
					uart2_putc(HIGH_BYTE(0));
					uart2_putc(LOW_BYTE(0));
				}
				break;
			case 'f':
				if (move_switch){
				uart2_putc(DRIVE);
				uart2_putc(HIGH_BYTE(50));
				uart2_putc(LOW_BYTE(50));
				uart2_putc(HIGH_BYTE(32768));
				uart2_putc(LOW_BYTE(32768));
				}
				else {
					uart2_putc(DRIVE);
					uart2_putc(HIGH_BYTE(0));
					uart2_putc(LOW_BYTE(0));
					uart2_putc(HIGH_BYTE(0));
					uart2_putc(LOW_BYTE(0));
				}
				break;
			default:
				uart2_putc(DRIVE);
				uart2_putc(HIGH_BYTE(0));
				uart2_putc(LOW_BYTE(0));
				uart2_putc(HIGH_BYTE(0));
				uart2_putc(LOW_BYTE(0));
				break;
		}
		
		uart2_putc(149);
		uart2_putc(2);
		uart2_putc(7);
		uart2_putc(13);
		char packet1 = 0;
		char packet2 = 0;
		Task_Next();
		if (uart2_AvailableBytes()) {
			// populate roomba state (rs) here
			rs.bumper_pressed = uart2_getc();
			rs.vwall_detected = uart2_getc();	
		}
		Task_Next();
	}
	
}

void a_main() {
	laser_time = 30000 / (MSECPERTICK * LASER_PERIOD);
	analogReference(DEFAULT);
	Task_Create_Period(laser_task, 0, LASER_PERIOD, 10, 1);
	Task_Create_Period(escape_task, 0, 2, 1, 2);
	Task_Create_Period(user_ai_task, 0, 2, 1, 3);
	Task_Create_Period(cruise_task, 0, 2, 1, 4);
	Task_Create_Period(choose_ai_routine, 0, 2, 1, 5);
	Task_Create_Period(receive_bt, 0, 3, 1, 0);
	Task_Create_Period(roomba_task, 0, 5, 1000, 0);
	Task_Create_Period(move_switch_task, 0, 6000, 10000, 6000);
	Task_Create_Period(servo_task, 0, 3, 10, 1);
	Task_Create_Period(light_sensor_read, 0, 10, 10, 0);
}


#endif