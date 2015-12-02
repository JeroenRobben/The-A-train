#include <Wire.h>

//******Pin allocation************

//Sensors
byte PIN_SPEED = A3;
byte PIN_COLLISION = A4;

//Motor
byte PIN_MOTOR_2 = 9;
byte PIN_MOTOR_1 = 5;

//I2C Communication
byte PIN_SCA = 2;
byte PIN_SCL = 3;

//Emergency interrupts
byte PIN_EM_OUT = 0;
byte PIN_EM_IN = 7;

byte PIN_LCD = 1;

byte I2C_ADDRESS = 2;
//******Variables************


void setup() {
  pinMode(PIN_SPEED, INPUT);
  pinMode(PIN_COLLISION, INPUT);
  pinMode(PIN_EM_OUT, OUTPUT);
  pinMode(PIN_EM_IN, INPUT);
  pinMode(PIN_MOTOR_1, OUTPUT);
  pinMode(PIN_MOTOR_2, OUTPUT);

  Serial.begin(9600); //Communication with LCD

  Wire.begin(I2C_ADDRESS); //I2C Communication 
}

void loop() {
  // fade in from min to max in increments of 5 points:
  for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += 5) {
    // sets the value (range from 0 to 255):
    analogWrite(PIN_MOTOR_1, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(30);
  }
delay(2000);
  // fade out from max to min in increments of 5 points:
  for (int fadeValue = 255 ; fadeValue >= 0; fadeValue -= 5) {
    // sets the value (range from 0 to 255):
    analogWrite(PIN_MOTOR_1, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(30);
  }
    delay(2000);
    // fade in from min to max in increments of 5 points:
  for (int fadeValue = 0 ; fadeValue <= 255; fadeValue += 5) {
    // sets the value (range from 0 to 255):
    analogWrite(PIN_MOTOR_2, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(30);
  }
delay(2000);
  // fade out from max to min in increments of 5 points:
  for (int fadeValue = 255 ; fadeValue >= 0; fadeValue -= 5) {
    // sets the value (range from 0 to 255):
    analogWrite(PIN_MOTOR_2, fadeValue);
    // wait for 30 milliseconds to see the dimming effect
    delay(30);
  }

  delay(2000);
  
}
