# Assignment 3: Digital Safe

A 4x4 keypad and a 4-digit 7-segment display act as a PIN-locked safe. The red LED means locked and the green LED means open.

## How to use it

- The default PIN is `0000`. Digits show up on the display as you type.
- When locked, entering the right 4 digits opens the safe after a short hold (about 600 ms). A wrong PIN blinks the red LED 3 times and clears the display.
- When open, entering the PIN again locks it.
- When open, press `#` to set a new PIN. Type 4 digits, and the safe locks itself with the new PIN.

Keys are debounced in software, and each key is only taken once until it's released.

## Pins

- 7-segment segments: PB0-PB7
- Digit select: PA4-PA7
- Keypad rows: PE0-PE3 (outputs, open drain)
- Keypad columns: PC4-PC7 (inputs, pull-ups)
- LEDs: PF1 red, PF3 green

## Files

- `main.c` - all the code (keypad scan, display multiplexing, safe logic)
- `tm4c123gh6pm_startup_ccs_gcc.c` - startup file
- `Digital_Safe_ReadMe_Uma_Bharathi_D.docx` - report

The display is refreshed from the main loop, so there are no interrupt handlers to register.
