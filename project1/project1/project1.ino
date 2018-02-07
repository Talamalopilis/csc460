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
void getjoystick(struct joyvals*, struct controlstate*);
void getlightsensor(struct controlstate*);
void changestate(struct controlstate*);
void printlcd(struct joyvals*, struct controlstate*);

void getjoystick(struct joyvals *j, struct controlstate *cs) {
  j->x = analogRead(JoyStick_X);
  j->y = analogRead(JoyStick_Y);
  j->z = digitalRead(JoyStick_Z);
  int ppos = cs->ppos;
  int tpos = cs->tpos;

  ppos += ((int)j->x - 509) / 50;
  tpos += ((int)j->y - 510) / 50;

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
  if (!j->z) {
    cs->laser = true;
  }
  else {
    cs->laser = false;
  }
  cs->ppos = ppos;
  cs->tpos = tpos;
}

void getlightsensor(controlstate *cs)
{
  cs->ls = analogRead(LIGHT_SENSOR);
}

void changestate(controlstate *cs)
{
  pan.writeMicroseconds(3000 - cs->ppos);
  tilt.writeMicroseconds(cs->tpos);
  digitalWrite(laserpin, cs->laser);
}

void printlcd(joyvals *j, controlstate *cs)
{
  lcd.home();
  lcd.print(3000 - cs->ppos);
  lcd.print(" ");
  lcd.print(cs->tpos);
  lcd.setCursor(0, 1);
  lcd.print("LSR ");
  lcd.print(cs->laser ? " ON " : "OFF ");
  lcd.print(cs->ls >= 1000 ? 999 : cs->ls);

  Serial.print((int)j->x, DEC);
  Serial.print(",");
  Serial.print((int)j->y, DEC);
  Serial.print(",");
  Serial.print(j->z);
  Serial.print(",");
  Serial.print((int)cs->ppos, DEC);

  Serial.print(",");
  Serial.println(cs->ls, DEC);
  delay(10);
}


void setup()
{
  // put your setup code here, to run once:
  pinMode(JoyStick_X, INPUT);
  pinMode(JoyStick_Y, INPUT);
  pinMode(JoyStick_Z, INPUT_PULLUP);
  pinMode(13, OUTPUT);
  pinMode(38, OUTPUT);
  Serial.begin(9600);
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

  Scheduler_StartTask(0, 500
}

void loop()
{
  getjoystick(&j, &cs);
  getlightsensor(&cs);
  changestate(&cs);
  //  analogWrite(LED_PIN, 0);
  printlcd(&j, &cs);
}
