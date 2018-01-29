int JoyStick_X = A0;
int JoyStick_Y = A1;
int JoyStick_Z = 32;
int LED_PIN = 13;
float alpha = 0.5;

void setup() {
  // put your setup code here, to run once:
  pinMode(JoyStick_X, INPUT);
  pinMode(JoyStick_Y, INPUT);
  pinMode(JoyStick_Z, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  float x, y;
  int z;
  int brightness = 0;
  x = analogRead(JoyStick_X);
  y = analogRead(JoyStick_Y);
  while(1) {
    x = alpha * analogRead(JoyStick_X) + alpha * x;
    y = alpha * analogRead(JoyStick_Y) + alpha * y;
    z = digitalRead(JoyStick_Z);

    // just use x axis to adjust brightness
    brightness += ((int)x - 509) / 50;
    if(brightness > 255) {
      brightness = 255;
    } else if (brightness < 0) {
      brightness = 0;
    }
    analogWrite(LED_PIN, brightness);
    
//    Serial.print ((int)x, DEC);
//    Serial.print (",");
//    Serial.print ((int)y, DEC);
//    Serial.print (",");
//    Serial.println ((int)brightness, DEC);
    delay (100);
  }
//  
}
