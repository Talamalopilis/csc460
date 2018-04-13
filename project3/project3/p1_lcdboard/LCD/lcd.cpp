#include <LiquidCrystal.h>

extern "C" {
	#include <util/delay.h>	
	#include <stdio.h>
	#include "../os.h"
	#include "../struct.h"
	extern union system_data sdata;
}

extern "C" void lcd_task() {
	LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
	unsigned long int i;
	char line1 [17];
	char line2 [17];
	lcd.begin(16,2);
	lcd.setCursor(0,0);
	for(;;) {
		snprintf(line1, 17, "%4d %4d %1d", sdata.state.sjs_x, sdata.state.sjs_y, sdata.state.sjs_z);
		snprintf(line2, 17, "%4d %4d %5d", sdata.state.rjs_x, sdata.state.rjs_y, 0);
		lcd.home();
		lcd.print(line1);
		lcd.setCursor(0, 1);
		lcd.print(line2);
		Task_Next();
	}
}