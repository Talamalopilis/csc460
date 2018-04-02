// #define TESTING 1 // testing flag; disable if not testing
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
void priority_test_setup();
void system_task_test();
void periodic_task_test();
void rr_task_test();
void rr_test_1();
void rr_test_2();
void rr_sched_test();
void srr_basic_test();
void srr_sender();
void srr_reciever();
void srr_async_test();
void asrr_sender();
void asrr_reciever();


void test_main() {
	cur = 0;
	memset(results, 0, BUFFER_SIZE);
	Task_Create_RR(basic_RR_test, 0);
}

/* 
	just making sure UART capabilities work and testing if RR
	tasks run at all
	expected trace for this is
	a, b, c, d, e, f, g, h, i, j
*/
void basic_RR_test() {
	int i;
	for(i = 'a'; i < 'a' + 10; ++i) {
		results[cur++] = i;
	}
	results[cur++] = 0;
	//Task_Create_System(srr_basic_test, 0);
	Task_Create_Period(interleaved_periodic_task_1,0,10,100,0);
	Task_Create_Period(interleaved_periodic_task_2,0,5,100,0);
}


/*
	periodic task interleaving test
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
	Task_Create_System(priority_test_setup, 0);
}

/*
	task priority test
	expected trace is
	a, b, c
*/

void priority_test_setup() {
	Task_Create_System(system_task_test, 0);
	Task_Create_Period(periodic_task_test, 0, 10, 0, 0);
	Task_Create_RR(rr_task_test, 0);
}

void system_task_test() {
	results[cur++] = 'a';
}

void periodic_task_test() {
	results[cur++] = 'b';
}

void rr_task_test() {
	results[cur++] = 'c';
	results[cur++] = 0;
	Task_Create_System(rr_sched_test, 0);
}

/*
	checks if two tasks of the same priority
	level interleave properly
	expected trace is
	a, b, a, b, a, b, a, b, a, b
*/

void rr_sched_test() {
	Task_Create_RR(rr_test_1, 0);
	Task_Create_RR(rr_test_2, 0);
}

void rr_test_1() {
	int i;
	for(i = 0; i < 5; ++i) {
		results[cur++] = 'a';
		Task_Next();
	}
}

void rr_test_2() {
	int i;
	for(i = 0; i < 5; ++i) {
		results[cur++] = 'b';
		Task_Next();
	}
	results[cur++] = 0;
	Task_Create_System(srr_basic_test, 0);
}

// trace expected to be aba
void srr_basic_test(){
	PID to = Task_Create_RR(srr_reciever, 0);
	Task_Create_RR(srr_sender, to);
}

void srr_sender(){
	PID to = Task_GetArg();
	results[cur++] = 'a';
	unsigned int msg = 'b';
	Msg_Send(to, 1, &msg);
	results[cur++] = 'a';
	results[cur++] = 0;
	Task_Create_System(srr_async_test, 0);
}

void srr_reciever(){
	unsigned int msg;
	PID from = Msg_Recv(0xff, &msg);
	results[cur++] = msg;
	Msg_Rply(from, 0);
}

// expected trace is aaca
void srr_async_test(){
	PID to = Task_Create_RR(asrr_reciever, 0);
	Task_Create_RR(asrr_sender, to);

}

void asrr_sender(){
	PID to = Task_GetArg();
	results[cur++] = 'a';
	unsigned int msg = 'b';
	Msg_ASend(0,1,msg); //sending to task obviously not receiving
	results[cur++] = 'a';
	Task_Next();
	msg = 'c';
	Msg_ASend(to,1,msg);
	Task_Next();
	results[cur++] = 'a';
	results[cur++] = 0;
	Task_Create_RR(write_out, 0);
}

void asrr_reciever(){
	unsigned int msg;
	Msg_Recv(0xff, &msg);
	results[cur++] = msg;
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