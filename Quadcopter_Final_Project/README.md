# Quadcopter Flight Controller — E3-257 Final Project

A bare-metal quadcopter flight controller on the TM4C123GH6PM LaunchPad, built up step by step:
reading the RC receiver, driving the ESCs, reading the MPU-9250 gyroscope, and finally closing a
rate-control loop around roll, pitch and yaw.

The full write-up is in [`docs/Final_Project_Report.pdf`](docs/Final_Project_Report.pdf).

## Folder Layout
```
firmware/
  01_open_loop_throttle/            RC throttle → 4 ESCs, no stabilisation
  02_rate_controller_fixed_throttle/ Gyro rate loop with a fixed throttle, UART telemetry (bench test)
  03_closed_loop_rate_controller/   Gyro rate loop with live RC throttle (final version)
prototypes/                         Earlier single-feature test programs
docs/
  Final_Project_Report.pdf
  bench_testing_guide.md            Bench / tuning / first-hover checklist
```

## Firmware Versions
| Version | What it does |
|---|---|
| **01 Open loop** | Measures the receiver's throttle pulse on PC4 (edge interrupts, 900–2100 µs validity check), clamps it to 1000–2000 µs and sends the same value to all four ESCs. Prints the throttle over UART. Includes its own startup file with `GPIOPortC_Handler` registered. |
| **02 Rate controller, fixed throttle** | Adds the MPU-9250 over SPI, gyro offset calibration, and a P-only rate controller with X-configuration motor mixing. Throttle is hard-coded to 1200 µs for safe bench testing; rates and motor commands are printed over UART. |
| **03 Closed-loop rate controller** | As 02, but throttle comes from the RC receiver (capped at 1800 µs). The RGB LED shows calibration status (red with green flicker while sampling, solid green when done). |

### Control Loop (versions 02 and 03)
- Gyro configured for ±500 °/s (65.5 LSB per °/s) with the digital low-pass filter enabled.
- 2000-sample gyro offset calibration at start-up: **keep the drone still**.
- Control loop runs every **4 ms (250 Hz)**; ESC outputs update every **20 ms (50 Hz)**.
- P-only rate control: `Kp_roll = Kp_pitch = 1.5`, `Kp_yaw = 1.0` (desired rate = 0).
- Motor mixing (X frame):
  ```
  M1 = thr − roll − pitch − yaw
  M2 = thr − roll + pitch + yaw
  M3 = thr + roll + pitch − yaw
  M4 = thr + roll − pitch + yaw
  ```
  Each output is clamped to 1000–2000 µs.

## Hardware Connections
| Signal | Pin |
|---|---|
| ESC 1–4 (50 Hz PWM) | PB6, PB7, PB4, PB5 (M0PWM0–3) |
| RC receiver throttle channel | PC4 |
| MPU-9250 (SPI0): SCK / CS / MISO / MOSI | PA2 / PA3 / PA4 / PA5 |
| UART0 telemetry (115200 baud) | PA0 / PA1 |
| Status LED | PF1–PF3 |

PWM clock = 16 MHz / 64 = 250 kHz, `LOAD = 5000`, giving a 20 ms period (4 µs per count).

## Prototypes
| File | Purpose |
|---|---|
| `esc_calibration.c` | Software 50 Hz pulse on PB6: 2 ms for 4 s, then 1 ms (ESC range calibration) |
| `esc_throttle_step_button.c` | Steps the ESC pulse width up on each SW1 press (100 µs resolution) |
| `rc_pwm_decode.c` | Measures the receiver pulse width with edge interrupts |
| `rc_pwm_decode_uart.c` | Same, printing the measured throttle over UART |
| `imu_gyro_rates.c` | MPU-9250 over I2C0 (PB2/PB3), calibrated gyro rates over UART |
| `uart_gyro_v1.c`, `uart_gyro_v2.c` | Earlier I2C gyro → UART experiments |
| `imu_plus_throttle.c` | Gyro rates and RC throttle read together |

## Building
Create a TM4C123GH6PM project in Code Composer Studio, add the chosen version's `main.c`, and use
a startup file that registers `GPIOPortC_Handler` (the one in `01_open_loop_throttle/` does).

## Safety
Always bench test **with propellers removed**. See `docs/bench_testing_guide.md` for the
axis-direction and motor-response checks to do before flying.

Datasheets (TM4C123, MPU-9250, reference quadcopter manual) are kept locally in `docs/datasheets/`
and are not committed.
