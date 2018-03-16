#define TESTING 1 // testing flag; disable if not testing
#define F_CPU 16000000UL

#ifdef TESTING

#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include "UART/usart.h"
#include "os.h"

#define BUFFER_SIZE 1024

unsigned char results[BUFFER_SIZE]; // use 1 kb of space for test results
volatile uint16_t cur; // index of current character in buffer

void basic_RR_test();
void write_out();
void interleaved_periodic_task_1();
void interleaved_periodic_task_2();

void test_main() {
	cur = 0;
	memset(results, 0, BUFFER_SIZE);
	Task_Create_RR(basic_RR_test, 0);
}

/* 
	expected trace for this is
	a, b, c, d, e, f, g, h, i, j
*/
void basic_RR_test() {
	int i;
	for(i = 'a'; i < 'a' + 10; ++i) {
		results[cur++] = i;
	}
	results[cur++] = 0;
	Task_Create_Period(interleaved_periodic_task_1,0,10,100,0);
	Task_Create_Period(interleaved_periodic_task_2,0,5,100,0);
}


/*
	expected trace for this test is
	a, b, b, a, b, b, a, b, b
*/

void interleaved_periodic_task_1() {
	uint8_t i;
	for(i = 0; i < 3; ++i) {
		results[cur++] = 'a';
		Task_Next();
	}
}

void interleaved_periodic_task_2() {
	uint8_t i;
	for(i = 0; i < 6; ++i) {
		results[cur++] = 'b';
		Task_Next();
	}
	results[cur++] = 0;
	Task_Create_RR(write_out, 0);
}

void write_out() {
	uint16_t p;
	uart_init(BAUD_CALC(115200));
	for(p = 0; p < cur; ++p) {
		if(!results[p]) {
			uart0_putc('\n');
		} else {
			uart0_putc(results[p]);
			uart0_putc(' ');
		}
	}
	uart0_putc('\r');
	uart0_putc('\n');
}

#endif