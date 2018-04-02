#include <LiquidCrystal.h>
#include "lcd.h"
extern "C" {
	#include <util/delay.h>	
};

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

extern "C" void lcd_task() {
	unsigned long int i;
	lcd.begin(16,2);
	lcd.setCursor(0,0);
	for(i = 0;; i++) {
		lcd.print(i);
		lcd.home();
		//_delay_ms(1000);
		//lcd.clear();
	}
}