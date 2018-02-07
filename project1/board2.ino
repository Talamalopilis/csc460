#include <Servo.h>
#include <LiquidCrystal.h>
#include "scheduler.h"

const int JoyStick_X = A8;
const int JoyStick_Y = A9;
const int LIGHT_SENSOR = A10;
const int JoyStick_Z = 32;
const int LED_PIN = 13;
const float alpha = 0.5;
const int idle_pin = 40;

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
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
void getjoystick();
void getlightsensor();
void printlcd();

void getjoystick() {
  j.x = analogRead(JoyStick_X);
  j.y = analogRead(JoyStick_Y);
  j.z = digitalRead(JoyStick_Z);
  int ppos = cs.ppos;
  int tpos = cs.tpos;

  ppos += ((int)j.x - 509) / 100;
  tpos += ((int)j.y - 510) / 100;

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
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(13, OUTPUT);
  pinMode(38, OUTPUT);
  pinMode(idle_pin, OUTPUT);
  //Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);

  j.x = analogRead(JoyStick_X);
  j.y = analogRead(JoyStick_Y);
  j.z = 0;

  // init scheduler related tasks

  Scheduler_Init();
  Scheduler_StartTask(0, 10, getjoystick);
  Scheduler_StartTask(2, 40, getlightsensor);
  Scheduler_StartTask(6, 500, printlcd);
}

void idle(uint32_t idle_period)
{
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
