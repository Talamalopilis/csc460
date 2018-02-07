#include <Servo.h>
#include <LiquidCrystal.h>
#include "scheduler.h"
int LED_PIN = 13;
int laserpin = 38;
const int panpin = 2;
int const tiltpin = 3;
float alpha = 0.5;
int idle_pin = 40;

byte csbuff[6];

struct controlstate {
  int ppos:11;
  int tpos:11;
  bool laser:1;
};

union btin {
	controlstate cs;
	byte bt[sizeof(controlstate)];
};


union btin btin;
Servo pan;
Servo tilt;
void initcs();
void readbt();
void changestate();

void initcs() {
	btin.cs.ppos = 1500;
	btin.cs.tpos = 1500;
	btin.cs.laser = false;
}

void readbt() {
	if (Serial.available() >= sizeof(controlstate)) {
		Serial.readBytes(btin.bt,sizeof(controlstate));
	}
}

void changestate()
{
  pan.writeMicroseconds(3000 - btin.cs.ppos);
  tilt.writeMicroseconds(btin.cs.tpos);
  digitalWrite(laserpin, btin.cs.laser);
}

void setup()
{
	initcs();
  pinMode(38, OUTPUT);
  pinMode(idle_pin, OUTPUT);
  Serial.begin(9600);
  pan.attach(panpin);
  tilt.attach(tiltpin);
  pan.writeMicroseconds(btin.cs.ppos);
  tilt.writeMicroseconds(btin.cs.tpos);
  delay(200);

  // init scheduler related tasks

  Scheduler_Init();
  Scheduler_StartTask(0, 20, readbt);
  Scheduler_StartTask(10, 20, changestate);
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
