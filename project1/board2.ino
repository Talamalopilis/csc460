#include <Servo.h>
#include <LiquidCrystal.h>
#include "scheduler.h"

const uint16_t JS_SERVO_X = A8;
const uint16_t JS_SERVO_Y = A9;
const uint16_t JS_SERVO_Z = 32;

const uint16_t JS_ROOMBA_X = A11;
const uint16_t JS_ROOMBA_Y = A12;
const uint16_t JS_ROOMBA_Z = 33;

const uint16_t LIGHT_SENSOR = A10;
const uint16_t LED_PIN = 13;
const uint16_t IDLE_PIN = 40;

const uint16_t JS_SENSITIVITY = 50; // note, higher number means JS moves slower
const uint16_t JS_THRESHOLD_MAX = 800;
const uint16_t JS_THRESHOLD_MIN = 200;
const uint16_t JS_SERVO_X_NEUTRAL = 509;
const uint16_t JS_SERVO_Y_NEUTRAL = 510;

const uint16_t SERVO_MAX = 2000;
const uint16_t SERVO_MIN = 1000;

uint16_t ls = 0;

struct controlstate {
  uint16_t ppos;
  uint16_t tpos;
  char rcom;
  bool laser;
};

union Data {
  controlstate cs;
  byte bt[sizeof(controlstate)];
};

struct joyvals {
  uint16_t x, y;
  uint16_t z;
};

struct joyvals js_servo;
struct joyvals js_roomba;
union Data data;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
void getjoystick();
void getlightsensor();
void printlcd();

void getjoystick() {
  js_servo.x = analogRead(JS_SERVO_X);
  js_servo.y = analogRead(JS_SERVO_Y);
  js_servo.z = digitalRead(JS_SERVO_Z);

  js_roomba.x = analogRead(JS_ROOMBA_X);
  js_roomba.y = analogRead(JS_ROOMBA_Y);
  js_roomba.z = digitalRead(JS_ROOMBA_Z);

  int ppos = data.cs.ppos;
  int tpos = data.cs.tpos;

  ppos += (js_servo.x - JS_SERVO_X_NEUTRAL) / 50;
  tpos += (js_servo.y - JS_SERVO_Y_NEUTRAL) / 50;

  if (ppos > SERVO_MAX)
  {
    ppos = SERVO_MAX;
  }
  else if (ppos < SERVO_MIN)
  {
    ppos = SERVO_MIN;
  }
  if (tpos > SERVO_MAX)
  {
    tpos = SERVO_MAX;
  }
  else if (tpos < SERVO_MIN)
  {
    tpos = SERVO_MIN;
  }
  if (!js_servo.z) {
    data.cs.laser = true;
  }
  else {
    data.cs.laser = false;
  }
  data.cs.ppos = ppos;
  data.cs.tpos = tpos;

  if (js_roomba.y < JS_THRESHOLD_MIN) {
    data.cs.rcom = 'f';
  } else if (js_roomba.y > JS_THRESHOLD_MAX) {
    data.cs.rcom = 'b';
  } else if (js_roomba.x > JS_THRESHOLD_MAX) {
    data.cs.rcom = 'r';
  } else if (js_roomba.x < JS_THRESHOLD_MIN) {
    data.cs.rcom = 'l';
  } else {
    data.cs.rcom = 's';
  }
}

void getlightsensor()
{
  ls = analogRead(LIGHT_SENSOR);
}

void printlcd()
{
  lcd.home();
  lcd.print(3000 - data.cs.ppos);
  lcd.print(' ');
  lcd.print(data.cs.tpos);
  lcd.setCursor(0, 1);
  lcd.print(data.cs.laser ? "1 " : "0 ");
  lcd.print(ls >= 1000 ? 999 : ls);
  lcd.print(' ');
  lcd.print(data.cs.rcom);
}

void writebt() {
  Serial1.print('$');
  Serial1.write(data.bt, sizeof(controlstate));
}

void setup()
{
  // put your setup code here, to run once:
  pinMode(JS_SERVO_X, INPUT);
  pinMode(JS_SERVO_Y, INPUT);
  pinMode(JS_SERVO_Z, INPUT_PULLUP);

  pinMode(JS_ROOMBA_X, INPUT);
  pinMode(JS_ROOMBA_Y, INPUT);
//  pinMode(JS_SERVO_Z, INPUT_PULLUP);

  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(IDLE_PIN, OUTPUT);

  Serial1.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);

  data.cs.ppos = 1500;
  data.cs.tpos = 1500;

  js_servo.x = analogRead(JS_SERVO_X);
  js_servo.y = analogRead(JS_SERVO_Y);
  js_servo.z = 0;

  // init scheduler related tasks

  Scheduler_Init();
  Scheduler_StartTask(0, 10, getjoystick);
  Scheduler_StartTask(2, 50, getlightsensor);
  Scheduler_StartTask(4, 15, writebt);
  Scheduler_StartTask(6, 500, printlcd);
}

void idle(uint32_t idle_period)
{
  digitalWrite(IDLE_PIN, HIGH);
  delay(idle_period);
  digitalWrite(IDLE_PIN, LOW);
}
void loop()
{
  uint32_t idle_period = Scheduler_Dispatch();
  if (idle_period)
  {
    idle(idle_period);
  }
}
