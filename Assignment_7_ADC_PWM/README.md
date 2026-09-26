# Assignment 7 — Part I: Potentiometer (ADC) to PWM

Reads an analog voltage on ADC0 and maps it linearly onto the duty cycle of a PWM output, e.g.
to control LED brightness or motor speed with a potentiometer.

## How It Works
- **ADC0, sample sequencer 3**, software-triggered, reads AIN0 on **PE3** (12-bit, 0–4095).
- **PWM0 generator 0** drives **M0PWM0 on PB6**. PWM clock = system clock / 64, `LOAD = 2500`
  (about 100 Hz at the default 16 MHz system clock).
- The main loop continuously sets `CMPA = LOAD − (adc × LOAD / 4095)`, so duty cycle rises from
  0 % to 100 % as the input goes from 0 V to 3.3 V.
- SysTick is configured for a 1 ms tick (`tick` counter) for later parts of the assignment.

## Hardware Connections
| Signal | Pin |
|---|---|
| Potentiometer wiper (0–3.3 V) | PE3 / AIN0 |
| PWM output | PB6 / M0PWM0 |

## Files
| File | Purpose |
|---|---|
| `part1_adc_pwm.c` | Part I source code |

## Notes
- The header comment mentions keypad control with status on an LCD; that part is not implemented
  in this file yet.
- `uart.h` is included but not used, and is not part of this folder. Remove the include or add
  the header before building.
- Register `SysTick_Handler` in the startup file's vector table.
