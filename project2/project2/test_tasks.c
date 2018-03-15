#define TESTING 1 // testing flag; disable if not testing

#include <avr/io.h>
#include <string.h>
#include "uart.h"
#include "os.h"

#ifdef TESTING

#define BUFFER_SIZE 1024

unsigned char results[BUFFER_SIZE]; // use 1 kb of space for test results
unsigned char* cur = results; // pointer to current character index in buffer

void basic_RR_test();
void write_out();

void test_main() {
	memset(results, 0, BUFFER_SIZE);
	Task_Create_RR(basic_RR_test, 0);
}

void basic_RR_test() {
	int i;
	for(i = 0; i < 10; ++i) {
		*cur++ = i % 0xff;
	}
	Task_Create_System(write_out, 0);
}

void write_out() {
	unsigned char * p;
	uart0_init(9600);
	uart0_puts("test");
	for(p = results; p < cur; ++p) {
		char c = *p;
		if(!c) {
			uart0_putc(c);
		} else {
			uart0_putc('\n');
		}
	}
}

#endif