#include <LiquidCrystal.h>

extern "C" {
	#include <util/delay.h>	
	#include <stdio.h>
	#include "../os.h"
	extern uint16_t joystick_x;
};

extern "C" void lcd_task() {
	LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
	unsigned long int i;
	char s [17];
	lcd.begin(16,2);
	lcd.setCursor(0,0);
	for(;;) {
		snprintf(s, 17, "%4d", joystick_x);
		lcd.home();
		lcd.print(s);
		Task_Next();
	}
}