#include <Arduino.h>
#include "ST3215HalfDuplex.h"

// Waveshare ST3215 bus servo example for the Seeed Studio XIAO ESP32C6.
//
// Wiring (servo connector 5264-3A: 1=DATA, 2=VCC, 3=GND):
//   XIAO D8 (GPIO19)  -> servo DATA
//   XIAO GND          -> servo GND
//   External 6-12.6V  -> servo VCC  (do NOT power the servo from the 3V3 pin)
//
// The ST3215 ships with ID = 1 and a 1 Mbps half-duplex TTL bus.

SMS_STS_HalfDuplex st;

const uint8_t SERVO_ID = 1;
const int16_t POS_MIN = 200;
const int16_t POS_MAX = 3800;
const int16_t POS_MID = 2048;
const uint16_t SPEED = 1500;
const uint8_t ACC = 50;

static void printFeedback() {
  int pos = st.ReadPos(SERVO_ID);
  int speed = st.ReadSpeed(SERVO_ID);
  int voltage = st.ReadVoltage(SERVO_ID);
  int temper = st.ReadTemper(SERVO_ID);
  int load = st.ReadLoad(SERVO_ID);
  int current = st.ReadCurrent(SERVO_ID);

  Serial.print(F("pos="));
  Serial.print(pos);
  Serial.print(F(" speed="));
  Serial.print(speed);
  Serial.print(F(" load="));
  Serial.print(load);
  Serial.print(F(" V="));
  Serial.print((float)voltage / 10.0f, 1);
  Serial.print(F("V temp="));
  Serial.print(temper);
  Serial.print(F("C I="));
  Serial.print(current);
  Serial.println(F("mA"));
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("\nST3215 single-wire demo - Seeed XIAO ESP32C6 (D8=GPIO19)"));
  st.begin(D8);

  Serial.print(F("Ping servo ID "));
  Serial.print(SERVO_ID);
  Serial.print(F(" ... "));
  int id = st.Ping(SERVO_ID);
  if (id >= 0) {
    Serial.print(F("OK ("));
    Serial.print(id);
    Serial.println(F(")"));
  } else {
    Serial.println(F("no response - check DATA/GND wiring and servo power"));
  }

  Serial.print(F("Enable torque ... "));
  int ok = st.EnableTorque(SERVO_ID, 1);
  Serial.println(ok ? F("OK") : F("no ack"));

  Serial.println(F("Moving to center position"));
  st.WritePosEx(SERVO_ID, POS_MID, SPEED, ACC);
  delay(1500);
}

void loop() {
  Serial.println(F("Move -> POS_MIN"));
  st.WritePosEx(SERVO_ID, POS_MIN, SPEED, ACC);
  delay(1500);
  printFeedback();

  Serial.println(F("Move -> POS_MAX"));
  st.WritePosEx(SERVO_ID, POS_MAX, SPEED, ACC);
  delay(1500);
  printFeedback();

  Serial.println(F("Move -> center"));
  st.WritePosEx(SERVO_ID, POS_MID, SPEED, ACC);
  delay(1500);
  printFeedback();

  delay(2000);
}
