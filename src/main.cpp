#include <Arduino.h>
#include "crsf.h"

#define RXD2 2
#define TXD2 1
#define SBUS_BUFFER_SIZE 25

uint8_t _rcs_buf[25] {};
uint16_t _raw_rc_values[RC_INPUT_MAX_CHANNELS] {};
uint16_t _raw_rc_count{};

int aileronsPin = 12;
int elevatorPin = 13;
int throttlePin = 14;
int rudderPin = 15;

void SetServoPos(float percent, int pin)
{
    uint32_t duty = map(percent, 0, 100, 3277, 6554);
    ledcWrite(pin, duty);
}

void setup() {
  Serial.begin(460800);
  Serial2.begin(420000, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial Txd is on pin: " + String(TX));
  Serial.println("Serial Rxd is on pin: " + String(RX));

  ledcAttach(aileronsPin, 50, 16);
  ledcAttach(elevatorPin, 50, 16);
  ledcAttach(throttlePin, 50, 16);
  ledcAttach(rudderPin, 50, 16);
}

void loop() { 
  while (Serial2.available()) {
    size_t numBytesRead = Serial2.readBytes(_rcs_buf, SBUS_BUFFER_SIZE);
    if(numBytesRead > 0)
    {
      crsf_parse(&_rcs_buf[0], SBUS_BUFFER_SIZE, &_raw_rc_values[0], &_raw_rc_count, RC_INPUT_MAX_CHANNELS );
      
      Serial.print("Channel 1: ");
      Serial.print(_raw_rc_values[0]);
      Serial.print("\tChannel 2: ");
      Serial.print(_raw_rc_values[1]);
      Serial.print("\tChannel 3: ");
      Serial.print(_raw_rc_values[2]);
      Serial.print("\tChannel 4: ");
      Serial.print(_raw_rc_values[3]);
      Serial.print("\tChannel 11: ");
      Serial.println(_raw_rc_values[10]);

      int aileronsMapped = map(_raw_rc_values[0], 1000, 2000, 0, 100);
      int elevatorMapped = map(_raw_rc_values[1], 1000, 2000, 0, 100);
      int throttleMapped = map(_raw_rc_values[2], 1000, 2000, 0, 100);
      int rudderMapped = map(_raw_rc_values[3], 1000, 2000, 0, 100);
      
      SetServoPos(aileronsMapped, aileronsPin);
      SetServoPos(elevatorMapped, elevatorPin);
      SetServoPos(throttleMapped, throttlePin);
      SetServoPos(rudderMapped, rudderPin);
    }
  }
}
