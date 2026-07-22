// TODO: Add and test failsafe timeout handling.

#include <Arduino.h>
#include "crsf.h"

#define RXD2 2
#define TXD2 1
#define CRSF_READ_BUFFER_SIZE 64
#define BATTERY_TELEMETRY_INTERVAL_MS 1000

uint8_t _rcs_buf[CRSF_READ_BUFFER_SIZE] {};
uint16_t _raw_rc_values[RC_INPUT_MAX_CHANNELS] {};
uint16_t _raw_rc_count {};

uint32_t lastBatteryTelemetryMs {};

int aileronsPin = 12;
int elevatorPin = 13;
int throttlePin = 14;
int rudderPin = 15;

void SetServoPos(float percent, int pin)
{
    percent = constrain(percent, 0, 100);

    uint32_t duty = map(percent, 0, 100, 3277, 6554);
    ledcWrite(pin, duty);
}

void SendRandomBatteryTelemetry()
{
    // CRSF encodes voltage and current in tenths of their respective units.
    const uint16_t voltage = random(120, 169);  // 12.0–16.8 V
    const uint16_t current = random(0, 301);    // 0.0–30.0 A
    const int usedMah = random(0, 3001);        // 0–3000 mAh
    const uint8_t remaining = random(0, 101);   // 0–100%

    uint8_t frame[12];

    const size_t frameLength = crsf_build_telemetry_battery(
        frame,
        sizeof(frame),
        voltage,
        current,
        usedMah,
        remaining
    );

    if (frameLength > 0) {
        Serial2.write(frame, frameLength);

        Serial.print("Battery telemetry: ");
        Serial.print(voltage / 10.0f, 1);
        Serial.print(" V, ");
        Serial.print(current / 10.0f, 1);
        Serial.print(" A, ");
        Serial.print(usedMah);
        Serial.print(" mAh, ");
        Serial.print(remaining);
        Serial.println("%");
    }
}

void setup()
{
    Serial.begin(460800);
    Serial2.begin(420000, SERIAL_8N1, RXD2, TXD2);

    Serial.print("CRSF TX pin: ");
    Serial.println(TXD2);

    Serial.print("CRSF RX pin: ");
    Serial.println(RXD2);

    ledcAttach(aileronsPin, 50, 16);
    ledcAttach(elevatorPin, 50, 16);
    ledcAttach(throttlePin, 50, 16);
    ledcAttach(rudderPin, 50, 16);
}

void loop()
{
    while (Serial2.available() > 0) {
        const size_t availableBytes =
            static_cast<size_t>(Serial2.available());

        const size_t bytesToRead =
            min(availableBytes, sizeof(_rcs_buf));

        const size_t numBytesRead =
            Serial2.readBytes(_rcs_buf, bytesToRead);

        if (numBytesRead == 0) {
            break;
        }

        const bool channelsUpdated = crsf_parse(
            _rcs_buf,
            numBytesRead,
            _raw_rc_values,
            &_raw_rc_count,
            RC_INPUT_MAX_CHANNELS
        );

        // Only update outputs after receiving a valid RC frame.
        if (channelsUpdated && _raw_rc_count >= 4) {
            Serial.print("Channel 1: ");
            Serial.print(_raw_rc_values[0]);

            Serial.print("\tChannel 2: ");
            Serial.print(_raw_rc_values[1]);

            Serial.print("\tChannel 3: ");
            Serial.print(_raw_rc_values[2]);

            Serial.print("\tChannel 4: ");
            Serial.print(_raw_rc_values[3]);

            if (_raw_rc_count > 10) {
                Serial.print("\tChannel 11: ");
                Serial.print(_raw_rc_values[10]);
            }

            Serial.println();

            const int aileronsValue =
                constrain(_raw_rc_values[0], 1000, 2000);

            const int elevatorValue =
                constrain(_raw_rc_values[1], 1000, 2000);

            const int throttleValue =
                constrain(_raw_rc_values[2], 1000, 2000);

            const int rudderValue =
                constrain(_raw_rc_values[3], 1000, 2000);

            const int aileronsMapped =
                map(aileronsValue, 1000, 2000, 0, 100);

            const int elevatorMapped =
                map(elevatorValue, 1000, 2000, 0, 100);

            const int throttleMapped =
                map(throttleValue, 1000, 2000, 0, 100);

            const int rudderMapped =
                map(rudderValue, 1000, 2000, 0, 100);

            SetServoPos(aileronsMapped, aileronsPin);
            SetServoPos(elevatorMapped, elevatorPin);
            SetServoPos(throttleMapped, throttlePin);
            SetServoPos(rudderMapped, rudderPin);
        }
    }

    const uint32_t now = millis();

    if (
        now - lastBatteryTelemetryMs >=
        BATTERY_TELEMETRY_INTERVAL_MS
    ) {
        lastBatteryTelemetryMs = now;
        SendRandomBatteryTelemetry();
    }
}
