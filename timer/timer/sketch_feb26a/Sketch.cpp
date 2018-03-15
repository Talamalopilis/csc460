/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>

/*End of auto generated code by Atmel studio */

#define F_CPU 16000000UL

#include <LiquidCrystal.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
//Beginning of Auto generated function prototypes by Atmel Studio
void setupTimer();
ISR(TIMER4_COMPA_vect );
int main();
//End of Auto generated function prototypes by Atmel Studio

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int x = 0;

void setupTimer() {
  TCCR4A = 0;
  TCCR4B = 0;
  
  //set to CTC (mode 4)
  TCCR4B |= (1 << WGM32);
  
  //set prescaler to 256
  TCCR4B |= (1 << CS42);
  
  //set TOP value (0.01 seconds)
  OCR4A = 625;
  
  //Enable interrupt A for timer 3
  TIMSK4 |= (1 << OCIE4A);
  
  //Set timer to 0
  TCNT4 = 0;
  
  DDRL = 0xFF;                //PB as output
  PORTL = 0x00;                //keep all LEDs off
  
  sei();
  
}

ISR(TIMER4_COMPA_vect) {
  PORTL = ~PORTL;
  x++;
  lcd.home();
  lcd.print(x/6000, DEC);
  lcd.print(":");
  lcd.print(x/100 % 60, DEC);
  lcd.print(":");
  lcd.print(x % 100, DEC);
  
  PORTL = ~PORTL;
}

int main() {
  setupTimer();
  for (;;);
  
}

