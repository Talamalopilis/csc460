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
unsigned char * cur; // pointer to current character index in buffer

void basic_RR_test();
void write_out();

void test_main() {
	cur = results;
	memset(results, 0, BUFFER_SIZE);
	Task_Create_RR(basic_RR_test, 0);
}

void basic_RR_test() {
	int i;
	for(i = 'a'; i < 'a' + 10; ++i) {
		*cur++ = i % 0xff;
	}
	Task_Create_System(write_out, 0);
}

void write_out() {
	unsigned char * p;
	uart_init(BAUD_CALC(115200));
	for(p = results; p < cur; ++p) {
		uart0_putc(*p);
	}
	uart0_putc('\r');
	uart0_putc('\n');
}

#endif