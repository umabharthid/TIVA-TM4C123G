# Assignment 2: RGB LED with SW1 / SW2

The onboard RGB LED blinks. SW1 changes the colour and SW2 changes how fast it blinks.

The task asked for two globals, `Led_Colour` and `Delay_Count`, and a `while(1)` loop that polls both switches.

## Pins

- LEDs: PF1 red, PF2 blue, PF3 green
- SW1: PF4, SW2: PF0 (both active low, internal pull-ups on)
- PF0 is the NMI pin, so it has to be unlocked (`GPIO_PORTF_LOCK_R = 0x4C4F434B`, then `CR = 0x11`) before you can use it as an input

## How it works

Each pass of the main loop takes about 1 ms (`delayMs(1)`). In each pass:

1. If SW1 is low, wait 40 ms for debounce, check again, and move to the next colour (red > green > blue > red).
2. If SW2 is low, same debounce, then step `Delay_Count` through 64 > 128 > 256 > 64. It waits for SW2 to be released, so one press is one step.
3. Count `n` up to `Delay_Count`, then toggle the LED and reset `n`.

So the LED stays on/off for roughly 64, 128 or 256 ms. `delayMs()` is just a busy loop tuned for 16 MHz, so the times are approximate.

`LED_delay_change()` sets `n = 0` on purpose. Without that, going from 256 back to 64 while `n` was already above 64 meant `n == Delay_Count` never matched again, and the LED got stuck.

Two things I noticed while testing:
- It starts on green, not red, because `GPIO_init()` calls `LED_color_change()` once.
- SW1 doesn't wait for release, so holding it keeps cycling colours.

## Files

- `02_BLINK_RGB_GNU2_ASSIGNMENT/` - the CCS project (`main.c`, startup file, linker script, target config)
- `RGB_LED_Control_Report_Uma.pdf` - report

To run it, import the project folder into CCS, build, and debug/flash over USB.
