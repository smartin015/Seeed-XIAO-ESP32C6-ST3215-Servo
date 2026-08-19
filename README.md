# ST3215 Bus Servo on Seeed Studio XIAO ESP32C6 (PlatformIO)

This project controls a [Waveshare ST3215 serial bus servo](https://www.waveshare.com/wiki/ST3215_Servo)
from a Seeed Studio XIAO ESP32C6 using pin **D8** as the single-wire
half-duplex data line.

## Hardware

ST3215 connector (5264-3A): `1=DATA`, `2=VCC`, `3=GND`.

| XIAO ESP32C6 | Servo / supply |
| --- | --- |
| `D8` (GPIO19) | servo `DATA` |
| `GND` | servo `GND` |
| external 6-12.6 V | servo `VCC` (do **not** use the 3V3 logic pin) |

On the XIAO ESP32C6 the silk-screen pin `D8` is **GPIO19** (the board's
`SCK` pin; on the ESP32 GPIO matrix any pin can be routed to a UART).

The ST3215 ships with ID `1` and a 1 Mbps, 8N1, half-duplex TTL bus.
Only one servo may use a given ID at a time.

## Build and flash

```bash
pio run
pio run -t upload
pio device monitor
```

PlatformIO target: `seeed_xiao_esp32c6` (Arduino framework).

## How it works

- `lib/SCServo/` is the official Waveshare SCServo library (unmodified).
- `lib/ST3215HalfDuplex/` adds `SMS_STS_HalfDuplex`, a transport that
  reuses the library's `SMS_STS` protocol layer but maps both UART RX and
  TX to the same GPIO.
- The TX pad driver is tri-stated while receiving, so the servo can drive
  the shared DATA line without bus contention.
- Because RX and TX share one pad, the UART sees its own transmission as
  loopback echo; `wFlushSCS()` discards exactly those bytes before reading
  the servo response.

If you build a custom carrier board, the recommended production interface
is the Waveshare
[bus servo control circuit](https://files.waveshare.com/upload/d/d3/Bus_servo_control_circuit.pdf)
(push-pull tri-state buffer with auto-direction). The single-pin approach
here is intended for breadboarding/prototyping.

## Demo behaviour

1. Pings servo ID 1.
2. Enables torque.
3. Sweeps between positions 200, 3800 and 2048 (0-4095 full turn) and
   prints position, speed, load, voltage, temperature and current.
