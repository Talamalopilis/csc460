#include <avr/io.h>


void setupTimer() {
	TCCR4A = 0;
	TCCR4B = 0;
	
	// set to CTC (mode 4)
	TCCR4B |= (1 << WGM32);
	
	// set prescaler to 256
	TCCR4B |= (1 << CS42);
	
	// set TOP value (0.01 seconds)
	OCR4A = 625;
	
	// Enable interrupt A for timer 3
	TIMSK4 |= (1 << OCIE4A);
	
	// Set timer to 0
	TCNT4 = 0;
	
	Enable_Interrupt();
	
}

