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

uint16_t ls = 0;

struct controlstate {
  uint16_t ppos;
  uint16_t tpos;
  bool laser;
};

union Data {
  controlstate cs;
  byte bt[sizeof(controlstate)];
};

struct joyvals {
  float x, y;
  int z;
};

struct joyvals j;
union Data data;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
void getjoystick();
void getlightsensor();
void printlcd();

void getjoystick() {
  j.x = analogRead(JoyStick_X);
  j.y = analogRead(JoyStick_Y);
  j.z = digitalRead(JoyStick_Z);
  int ppos = data.cs.ppos;
  int tpos = data.cs.tpos;

  ppos += ((int)j.x - 509) / 75;
  tpos += ((int)j.y - 510) / 75;

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
    data.cs.laser = true;
  }
  else {
    data.cs.laser = false;
  }
  data.cs.ppos = ppos;
  data.cs.tpos = tpos;
}

void getlightsensor()
{
  ls = analogRead(LIGHT_SENSOR);
}

void printlcd()
{
  lcd.home();
  lcd.print(3000 - data.cs.ppos);
  lcd.print(" ");
  lcd.print(data.cs.tpos);
  lcd.setCursor(0, 1);
 // lcd.print("LSR ");
  lcd.print(data.cs.laser ? "1 " : "0 ");
  lcd.print(ls >= 1000 ? 999 : ls);
}

void writebt() {
  Serial1.print('$');
  Serial1.write(data.bt, sizeof(controlstate));
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
  Serial1.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);

  data.cs.ppos = 1500;
  data.cs.tpos = 1500;

  j.x = analogRead(JoyStick_X);
  j.y = analogRead(JoyStick_Y);
  j.z = 0;

  // init scheduler related tasks

  Scheduler_Init();
  Scheduler_StartTask(0, 10, getjoystick);
  Scheduler_StartTask(2, 50, getlightsensor);
  Scheduler_StartTask(4, 25, writebt);
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
