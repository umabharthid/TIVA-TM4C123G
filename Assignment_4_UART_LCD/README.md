# Assignment 4: UART commands to LED and LCD

I type commands in a serial terminal and they control the red LED and a 16x2 LCD. The main loop runs three small state machines (UART, LED, LCD) off a roughly 1 ms software tick, so nothing blocks.

## Commands

UART0 at 9600 baud, 8N1. End each command with Enter. Characters are echoed back.

```
ON            red LED on
OFF           red LED off
BLINK         red LED blinks (500 ms)
PRINT <text>  clear LCD and print text on line 1
FWD           scroll the LCD left continuously (300 ms per step)
BWD           scroll the LCD right
CLEAR         clear LCD, LED off
```

## Pins

- UART0: PA0 / PA1 (the LaunchPad's USB COM port)
- LCD data D0-D7: PB0-PB7 (8-bit mode)
- LCD RS: PA6, LCD E: PA7
- Red LED: PF1

## Files

- `UART/` - the CCS project, code is in `UART/main.c`
- `UART_LCD_Uma_Bharathi_D.docx` - report

Import `UART/` into CCS, build and flash, then open PuTTY or Tera Term on the LaunchPad's COM port at 9600.
