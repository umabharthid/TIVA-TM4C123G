# Assignment 5: 7-segment menu for the RGB LED

A small menu on a 4-digit 7-segment display that sets the colour and blink speed of the RGB LED. The switches use GPIO falling-edge interrupts. SysTick runs at 1 ms to multiplex the display and keep time for blinking.

## Menu

SW2 moves to the next mode. SW1 steps the number (1-7) in the colour and speed modes.

```
rS . .   reset (start here)
SC x .   select colour  - 1 red, 2 blue, 3 green, 4 yellow, 5 magenta, 6 cyan, 7 white
SP x y   select speed   - 1 is slowest (2000 ms), 7 is fastest (100 ms)
En x y   LED blinks with the chosen colour and speed
St x y   stop, LED off
```

Then it goes back to `rS`.

## Pins

- Segments a-g, dp: PB0-PB7
- Digit select: PA4-PA7
- RGB LED: PF1-PF3
- SW1 / SW2: PF4 / PF0

## Files

- `main.c` - menu logic and switch interrupt (`GPIOF_Handler`)
- `drivers/sevenseg.c/.h` - display driver; its `SysTick_Handler` does the multiplexing and keeps `system_time`
- `tm4c123gh6pm_startup_ccs_gcc.c` - startup file with `GPIOF_Handler` and `SysTick_Handler` registered
- `Assignment_5.docx` - report

To build, add these files to a TM4C123GH6PM project in CCS. `main.c` includes `drivers/sevenseg.h`, so keep the folder as it is.
