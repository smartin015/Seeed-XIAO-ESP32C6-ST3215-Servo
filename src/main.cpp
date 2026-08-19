#include <Arduino.h>
#include "ST3215HalfDuplex.h"

// Waveshare ST3215 two-servo WASD demo for the Seeed Studio XIAO ESP32C6.
//
// Wiring (servo connector 5264-3A: 1=DATA, 2=VCC, 3=GND):
//   XIAO D8 (GPIO19)  -> both servo DATA lines (daisy chained)
//   XIAO GND          -> both servo GND lines
//   External 6-12.6V  -> both servo VCC lines (do NOT use the 3V3 pin)
//
// The ST3215 ships with ID = 1. To control two servos they must have unique
// IDs (this demo uses 1 and 2). Connect only ONE servo at a time when
// changing IDs, then send 'i' in the serial monitor.

SMS_STS_HalfDuplex st;

const uint8_t SERVO_A_ID = 1;
const uint8_t SERVO_B_ID = 2;

// The ST3215 reports 0-4095 for one full turn (360/4096 deg per step), so
// +/-1 is the smallest rotation the servo can represent.
const int16_t POS_MIN = 0;
const int16_t POS_MAX = 4095;
const int16_t POS_MID = 2048;
const int16_t POS_STEP = 1;

const uint16_t SPEED = 2000;  // steps/sec (max for ST series is ~3073)
const uint8_t ACC = 50;       // start/stop acceleration (max 150)

int16_t posA = POS_MID;
int16_t posB = POS_MID;

static int16_t clampPos(int32_t v) {
  if (v < POS_MIN) {
    return POS_MIN;
  }
  if (v > POS_MAX) {
    return POS_MAX;
  }
  return (int16_t)v;
}

static int16_t readPosOr(uint8_t id, int16_t fallback) {
  int16_t p = (int16_t)st.ReadPos(id);
  if (p < POS_MIN || p > POS_MAX) {
    return fallback;
  }
  return p;
}

static void printHelp() {
  Serial.println(F("\nWASD controls (one keypress = one 0.088 deg step):"));
  Serial.println(F("  W/S : servo ID 1  +/- one step"));
  Serial.println(F("  A/D : servo ID 2  +/- one step"));
  Serial.println(F("  i   : change a servo ID (connect only ONE servo!)"));
  Serial.println(F("  h   : show this help"));
}

// Command both servos together with a single sync-write frame, so they move
// in the same transaction.
static void moveBoth() {
  uint8_t ids[2] = {SERVO_A_ID, SERVO_B_ID};
  int16_t positions[2] = {posA, posB};
  uint16_t speeds[2] = {SPEED, SPEED};
  uint8_t accs[2] = {ACC, ACC};

  st.SyncWritePosEx(ids, 2, positions, speeds, accs);

  Serial.print(F("target A(1)="));
  Serial.print(posA);
  Serial.print(F("  B(2)="));
  Serial.println(posB);
}

static void changeIdInteractive() {
  Serial.setTimeout(15000);
  Serial.println(F("\n--- Change servo ID ---"));
  Serial.println(F("Connect ONLY the servo whose ID you want to change."));
  Serial.print(F("Current ID: "));
  long oldId = Serial.parseInt();
  Serial.print(F("New ID: "));
  long newId = Serial.parseInt();

  // Discard the rest of the line so it is not read as a WASD key.
  while (Serial.available()) {
    Serial.read();
  }

  if (oldId < 0 || oldId > 0xFD || newId < 0 || newId > 0xFD) {
    Serial.println(F("Invalid ID - valid range is 0-253"));
    return;
  }

  Serial.print(F("Changing ID "));
  Serial.print((int)oldId);
  Serial.print(F(" -> "));
  Serial.println((int)newId);

  int ok = st.changeId((uint8_t)oldId, (uint8_t)newId);
  switch (ok) {
    case 1:
      Serial.println(F("OK - ID stored in servo EEPROM"));
      break;
    case -1:
      Serial.println(F("FAILED - invalid new ID (0-253 only)"));
      break;
    case -2:
      Serial.println(F("FAILED - could not unlock EEPROM (ack timeout)"));
      break;
    case -3:
      Serial.println(F("FAILED - ID write did not take effect (new ID not found)"));
      break;
    case -4:
      Serial.println(F("FAILED - EEPROM re-lock failed"));
      break;
    default:
      Serial.println(F("FAILED - check wiring and servo power"));
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println(F("\nST3215 two-servo WASD demo - Seeed XIAO ESP32C6 (D8=GPIO19)"));
  st.begin(D8);

  // Make sure both servos are present and their torque is enabled.
  uint8_t ids[2] = {SERVO_A_ID, SERVO_B_ID};
  for (uint8_t i = 0; i < 2; i++) {
    Serial.print(F("Ping servo ID "));
    Serial.print(ids[i]);
    Serial.print(F(" ... "));
    int id = st.Ping(ids[i]);
    if (id >= 0) {
      Serial.print(F("OK, enable torque ... "));
      int ok = st.EnableTorque(ids[i], 1);
      Serial.println(ok ? F("OK") : F("no ack"));
    } else {
      Serial.println(F("no response - check DATA/GND wiring and servo power"));
    }
  }

  // Start from the servos' real positions when they are readable.
  posA = readPosOr(SERVO_A_ID, POS_MID);
  posB = readPosOr(SERVO_B_ID, POS_MID);

  printHelp();
  Serial.print(F("Initial targets: A(1)="));
  Serial.print(posA);
  Serial.print(F("  B(2)="));
  Serial.println(posB);
}

void loop() {
  if (!Serial.available()) {
    return;
  }

  char c = (char)Serial.read();
  switch (c) {
    case 'w':
    case 'W':
      posA = clampPos((int32_t)posA + POS_STEP);
      moveBoth();
      break;

    case 's':
    case 'S':
      posA = clampPos((int32_t)posA - POS_STEP);
      moveBoth();
      break;

    case 'a':
    case 'A':
      posB = clampPos((int32_t)posB + POS_STEP);
      moveBoth();
      break;

    case 'd':
    case 'D':
      posB = clampPos((int32_t)posB - POS_STEP);
      moveBoth();
      break;

    case 'i':
    case 'I':
      changeIdInteractive();
      break;

    case 'h':
    case 'H':
      printHelp();
      break;

    case '\r':
    case '\n':
      break;

    default:
      Serial.print(F("Unknown key '"));
      Serial.print(c);
      Serial.println(F("' - press h for help"));
      break;
  }
}
