# Assignment 7 (Part 1): Potentiometer to PWM

Reads a potentiometer on the ADC and uses it to set the duty cycle of a PWM output. I used it for LED brightness and motor speed.

## How it works

- ADC0, sample sequencer 3, software triggered, reading AIN0 on PE3 (12-bit, 0-4095)
- PWM0 generator 0 drives M0PWM0 on PB6, with `LOAD = 2500` and the PWM clock at sysclk/64
- The main loop keeps setting `CMPA = LOAD - adc * LOAD / 4095`, so 0 V gives 0% duty and 3.3 V gives 100%
- SysTick is set up for a 1 ms tick, which I planned to use in the later parts

## Pins

- Pot wiper: PE3 (AIN0)
- PWM out: PB6

## Notes

- The comment at the top mentions keypad control and an LCD. That's the next part and isn't in this file yet.
- It has `#include "uart.h"` but doesn't use it. Remove the line, or copy `drivers/uart.*` from Assignment 6, before building.
- `SysTick_Handler` needs to be in the vector table.
