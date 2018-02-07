#include <Servo.h>
#include <LiquidCrystal.h>
#include "scheduler.h"

int JoyStick_X = A1;
int JoyStick_Y = A2;
int LIGHT_SENSOR = A3;
int JoyStick_Z = 32;
int LED_PIN = 13;
int laserpin = 38;
const int panpin = 2;
int const tiltpin = 3;
float alpha = 0.5;
int idle_pin = 40;

struct joyvals {
  float x, y;
  int z;
};

struct controlstate {
  int ppos = 1500;
  int tpos = 1500;
  bool laser = false;
  int ls = 0;
};

struct controlstate cs;
struct joyvals j;
Servo pan;
Servo tilt;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
void getjoystick();
void getlightsensor();
void changestate();
void printlcd();

void getjoystick() {
  j.x = analogRead(JoyStick_X);
  j.y = analogRead(JoyStick_Y);
  j.z = digitalRead(JoyStick_Z);
  int ppos = cs.ppos;
  int tpos = cs.tpos;

  ppos += ((int)j.x - 509) / 50;
  tpos += ((int)j.y - 510) / 50;

  if (ppos > 2000)
  {
    ppos = 2000;
  }
  else if (ppos < 1000)
  {
    ppos = 1000;
  }
  if (tpos > 2000)
  {
    tpos = 2000;
  }
  else if (tpos < 1000)
  {
    tpos = 1000;
  }
  if (!j.z) {
    cs.laser = true;
  }
  else {
    cs.laser = false;
  }
  cs.ppos = ppos;
  cs.tpos = tpos;
}

void getlightsensor()
{
  cs.ls = analogRead(LIGHT_SENSOR);
}

void changestate()
{
  pan.writeMicroseconds(3000 - cs.ppos);
  tilt.writeMicroseconds(cs.tpos);
  digitalWrite(laserpin, cs.laser);
}

void printlcd()
{
  lcd.home();
  lcd.print(3000 - cs.ppos);
  lcd.print(" ");
  lcd.print(cs.tpos);
  lcd.setCursor(0, 1);
 // lcd.print("LSR ");
  lcd.print(cs.laser ? "1 " : "0 ");
  lcd.print(cs.ls >= 1000 ? 999 : cs.ls);
}


void setup()
{
  // put your setup code here, to run once:
  pinMode(JoyStick_X, INPUT);
  pinMode(JoyStick_Y, INPUT);
  pinMode(JoyStick_Z, INPUT_PULLUP);
  pinMode(13, OUTPUT);
  pinMode(38, OUTPUT);
  pinMode(idle_pin, OUTPUT);
  //Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  pan.attach(panpin);
  tilt.attach(tiltpin);
  pan.writeMicroseconds(cs.ppos);
  tilt.writeMicroseconds(cs.tpos);
  delay(200);

  j.x = analogRead(JoyStick_X);
  j.y = analogRead(JoyStick_Y);
  j.z = 0;

  // init scheduler related tasks

  Scheduler_Init();
  Scheduler_StartTask(0, 10, getjoystick);
  Scheduler_StartTask(2, 40, getlightsensor);
  Scheduler_StartTask(4, 20, changestate);
  Scheduler_StartTask(6, 500, printlcd);
}
void idle(uint32_t idle_period)
{
	// this function can perform some low-priority task while the scheduler has nothing to run.
	// It should return before the idle period (measured in ms) has expired.  For example, it
	// could sleep or respond to I/O.

	// example idle function that just pulses a pin.
	digitalWrite(idle_pin, HIGH);
	delay(idle_period);
	digitalWrite(idle_pin, LOW);
}
void loop()
{
	uint32_t idle_period = Scheduler_Dispatch();
	if (idle_period)
	{
		idle(idle_period);
	}
}
