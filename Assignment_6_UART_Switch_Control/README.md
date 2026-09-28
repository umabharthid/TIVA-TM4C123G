# Assignment 6: Menu from switches or UART

This extends Assignment 5. It's the same 7-segment menu for the RGB LED, but now you can also drive it from a serial terminal. Only one of the two is in control at a time.

## Switch mode (default)

Same as Assignment 5. SW2 goes to the next mode (rS > SC > SP > En > St) and SW1 steps the value 1-7.

## UART mode

UART0 at 115200 baud, 8N1. Send each command followed by Enter.

```
Enter UART    take control from the switches
Rst           reset colour and speed
Sel_color     colour select mode
Sel_speed     speed select mode
i             then a number 1-7 to pick the colour or speed
En            start blinking
Stop          stop
Exit UART     go back to switch control
```

The switches are ignored while UART mode is on.

## Pins

- Segments: PB0-PB7, digit select: PA4-PA7
- UART0: PA0 / PA1 (USB COM port)
- RGB LED: PF1-PF3
- SW1 / SW2: PF4 / PF0

## Files

- `main.c` - menu logic, switch interrupt, UART command handling
- `drivers/sevenseg.c/.h` - display driver (uses SysTick)
- `drivers/uart.c/.h` - polled UART0 driver with a line buffer and `istypeduart()` for matching commands
- `tm4c123gh6pm_startup_ccs_gcc.c` - startup file with the handlers registered
- `Assignment_6.docx` - report
