# Quadcopter Flight Controller (Final Project)

The final project for E3-257: a bare-metal flight controller for a quadcopter on the TM4C123 LaunchPad. I built it up one piece at a time: first reading the RC receiver, then driving the ESCs, then reading the MPU-9250 gyro, and finally closing a rate loop on roll, pitch and yaw.

The full write-up is in `docs/Final_Project_Report.pdf`.

## Layout

```
firmware/
  01_open_loop_throttle/              RC throttle straight to all 4 ESCs, no stabilisation
  02_rate_controller_fixed_throttle/  gyro rate loop, throttle fixed at 1200 us (bench testing)
  03_closed_loop_rate_controller/     gyro rate loop with RC throttle (final version)
prototypes/                           small test programs from the way there
docs/                                 report + bench testing checklist
```

## The three firmware versions

**01** measures the throttle pulse from the receiver on PC4 using edge interrupts. It throws away anything outside 900-2100 us, clamps the rest to 1000-2000 us, and sends it to all four ESCs. The throttle is also printed over UART. This folder has the startup file with `GPIOPortC_Handler` registered.

**02** adds the MPU-9250 over SPI, calibrates the gyro offset, and runs a P-only rate controller with X-frame mixing. The throttle is hard-coded to 1200 us so it's safe on the bench. Gyro rates and motor outputs are printed over UART.

**03** is the same as 02, but the throttle comes from the receiver (capped at 1800 us). The RGB LED shows calibration: red with a green flicker while it's sampling, and solid green when it's done.

## Control loop (02 and 03)

- Gyro at +/-500 deg/s (65.5 LSB per deg/s), DLPF on
- 2000-sample offset calibration at start-up, so keep the drone still
- Control loop every 4 ms (250 Hz), ESC update every 20 ms (50 Hz)
- P only: Kp roll = pitch = 1.5, Kp yaw = 1.0, target rate 0
- Mixing, with each output clamped to 1000-2000 us:

```
M1 = thr - roll - pitch - yaw
M2 = thr - roll + pitch + yaw
M3 = thr + roll + pitch - yaw
M4 = thr + roll - pitch + yaw
```

## Pins

- ESC 1-4: PB6, PB7, PB4, PB5 (M0PWM0-3, 50 Hz)
- Receiver throttle: PC4
- MPU-9250 on SPI0: SCK PA2, CS PA3, MISO PA4, MOSI PA5
- UART0 telemetry, 115200: PA0 / PA1
- Status LED: PF1-PF3

The PWM clock is 16 MHz / 64 = 250 kHz and `LOAD = 5000`, so the period is 20 ms and each count is 4 us.

## Prototypes

- `esc_calibration.c` - 2 ms pulse for 4 s, then 1 ms, to calibrate the ESC range
- `esc_throttle_step_button.c` - SW1 steps the ESC pulse up by 100 us
- `rc_pwm_decode.c`, `rc_pwm_decode_uart.c` - measure the receiver pulse (second one prints it)
- `imu_gyro_rates.c` - MPU-9250 over I2C0 (PB2/PB3), calibrated rates over UART
- `uart_gyro_v1.c`, `uart_gyro_v2.c` - earlier I2C gyro tests
- `imu_plus_throttle.c` - gyro and receiver together

## Building

Make a TM4C123GH6PM project in CCS, add the `main.c` you want, and use the startup file from `01_open_loop_throttle/` (it registers `GPIOPortC_Handler`).

**Take the props off for all bench testing.** `docs/bench_testing_guide.md` has the checks I did before flying.

The datasheets are in `docs/datasheets/` on my machine but aren't committed.
