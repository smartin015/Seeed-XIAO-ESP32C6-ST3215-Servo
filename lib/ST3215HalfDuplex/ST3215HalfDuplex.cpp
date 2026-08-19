#include "ST3215HalfDuplex.h"

#include <soc/gpio_struct.h>

SMS_STS_HalfDuplex::SMS_STS_HalfDuplex() : SMS_STS() {}

void SMS_STS_HalfDuplex::begin(int8_t pin, HardwareSerial &serial) {
  _pin = pin;
  _serial = &serial;
  _mask = (uint32_t)(1ULL << pin);
  _txMode = false;
  _txBytesWritten = 0;

  // Keep the base-class serial pointer valid as well.
  pSerial = &serial;

  // Map both UART RX and TX onto the same GPIO. The ESP32 UART driver
  // explicitly supports this single-pin (rx == tx) configuration.
  serial.setPins(pin, pin);
  serial.begin(1000000, SERIAL_8N1);

  // While the TX pad driver is tri-stated (RX mode) the internal pull-up
  // keeps the shared line in the UART idle (high) state.
  gpio_set_pull_mode((gpio_num_t)pin, GPIO_PULLUP_ONLY);

  setRx();
}

int SMS_STS_HalfDuplex::changeId(uint8_t currentId, uint8_t newId) {
  // 0xFE is the broadcast ID and cannot be used as a normal servo ID.
  if (newId > 0xFD) {
    return 0;
  }
  if (currentId == newId) {
    return 1;
  }

  // EEPROM writes are protected: unlock, write the new ID, then lock again
  // using the new ID (the servo answers with its new ID after the write).
  if (!unLockEprom(currentId)) {
    return 0;
  }

  // The acknowledgement for this write can already carry the new ID on some
  // firmware revisions, so its return value is not meaningful. Waveshare's
  // reference code ignores it for the same reason.
  writeByte(currentId, SMS_STS_ID, newId);

  return LockEprom(newId) == 1;
}

void SMS_STS_HalfDuplex::setTx() {
  if (_txMode) {
    return;
  }

  // Re-enable the pad output driver. The UART TX signal is still routed to
  // this pad by Serial.setPins(), so it drives the servo DATA line.
  GPIO.enable_w1ts.val = _mask;
  _txMode = true;
}

void SMS_STS_HalfDuplex::setRx() {
  if (!_txMode) {
    return;
  }

  // Tri-state the pad output driver so the servo can drive the shared line
  // while it sends its response. RX remains routed to the same pad.
  GPIO.enable_w1tc.val = _mask;
  _txMode = false;
}

int SMS_STS_HalfDuplex::writeSCS(unsigned char *nDat, int nLen) {
  if (nDat == nullptr || nLen <= 0) {
    return 0;
  }

  setTx();
  size_t written = _serial->write(nDat, (size_t)nLen);
  _txBytesWritten += (int)written;
  return (int)written;
}

int SMS_STS_HalfDuplex::writeSCS(unsigned char bDat) {
  setTx();
  size_t written = _serial->write(&bDat, 1);
  _txBytesWritten += (int)written;
  return (int)written;
}

int SMS_STS_HalfDuplex::readSCS(unsigned char *nDat, int nLen) {
  setRx();

  int size = 0;
  unsigned long t_begin = millis();
  while (size < nLen) {
    int c = _serial->read();
    if (c != -1) {
      if (nDat != nullptr) {
        nDat[size] = (unsigned char)c;
      }
      size++;
      t_begin = millis();
    }

    if (millis() - t_begin > IOTimeOut) {
      break;
    }
  }

  return size;
}

void SMS_STS_HalfDuplex::rFlushSCS() {
  setRx();
  _txBytesWritten = 0;

  // Drop any stale bytes that may be sitting in the RX FIFO before a new
  // command/response transaction starts.
  while (_serial->read() != -1) {
  }
}

void SMS_STS_HalfDuplex::wFlushSCS() {
  // Wait until the last command byte has left the UART shift register.
  _serial->flush();
  delayMicroseconds(20);

  // Because RX and TX share one GPIO, the UART sees its own transmission as
  // loopback echo. Discard exactly the bytes we transmitted so the next
  // read() starts at the beginning of the servo response.
  int remaining = _txBytesWritten;
  unsigned long t = millis();
  while (remaining > 0) {
    if (_serial->read() != -1) {
      remaining--;
      t = millis();
    }
    if (millis() - t > 2) {
      break;  // safety timeout, should never happen
    }
  }
  _txBytesWritten = 0;

  // Hand the shared line back to the servo before reading its reply.
  setRx();
}
