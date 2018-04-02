#include "os.h"
#include "LCD/lcd.h"

extern void lcd_task();

void a_main() {
	Task_Create_RR(lcd_task, 0);
}
