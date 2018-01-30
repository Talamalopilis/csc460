#include <Servo.h>

int JoyStick_X = A0;
int JoyStick_Y = A1;
int LIGHT_SENSOR = A2;

int JoyStick_Z = 32;
int LED_PIN = 13;
const int panpin = 9;
int const tiltpin = 10;
float alpha = 0.5;
int ppos = 1500;
int tpos = 1500;
Servo pan;
Servo tilt;

void setup()
{
  // put your setup code here, to run once:
  pinMode(JoyStick_X, INPUT);
  pinMode(JoyStick_Y, INPUT);
  pinMode(JoyStick_Z, INPUT_PULLUP);
  pinMode(13, OUTPUT);
  pinMode(38, OUTPUT);
  Serial.begin(9600);
  pan.attach(panpin);
  tilt.attach(tiltpin);
  pan.writeMicroseconds(ppos);
  tilt.writeMicroseconds(tpos);
  delay(200);
}

void loop()
{
  float x, y;
  int z, ls0;
  x = analogRead(JoyStick_X);
  y = analogRead(JoyStick_Y);
  while (1)
  {
    x = alpha * analogRead(JoyStick_X) + alpha * x;
    y = alpha * analogRead(JoyStick_Y) + alpha * y;
    z = digitalRead(JoyStick_Z);
    ls0 = analogRead(LIGHT_SENSOR);

    // just use x axis to adjust brightness
    ppos += ((int)x - 509) / 50;
    tpos += ((int)y - 510) / 50;

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
    if (!z)
    {
      digitalWrite(38, HIGH);
    }
    else
    {
      digitalWrite(38, LOW);
    }
    pan.writeMicroseconds(ppos);
    tilt.writeMicroseconds(tpos);
    analogWrite(LED_PIN, 0);
    Serial.print((int)x, DEC);
    Serial.print(",");
    Serial.print((int)y, DEC);
    Serial.print(",");
    Serial.print(z);
    Serial.print(",");
    Serial.print((int)ppos, DEC);

    Serial.print(",");
    Serial.println(ls0, DEC);
    delay(10);
  }
  //
}
