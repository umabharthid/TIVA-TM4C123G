# Assignment 8: Washing Machine Controller

A washing machine controller on the LaunchPad. You set a wash time, close the door and load detergent, then start. The motor runs through pre-wash, wash and spin. The LCD shows the state, the 7-segment shows the time left, and everything can also be controlled and debugged over UART.

## Controls

Keypad:
```
1  set time (cycles 5 / 15 / 30 s)     2  door open/close
3  load detergent                      D  start
B  pause / resume                      *  abort
```

Switches: SW1 aborts, SW2 pauses/resumes (both on interrupts).

UART0 (115200 baud):
```
set timer 5|15|30   door open   door close   load dt
start   pause   resume   abort   status   reset
```

It won't start if the door is open (E0), there's no detergent (E1), or no timer is set (E2). The red LED comes on and the LCD shows the error. While running, the green LED blinks.

The cycle is split into pre-wash for the first 20% of the time, wash for the middle 60%, and spin for the last 20%. Each phase runs the motor at a different PWM speed.

## Lab 8 additions: debug commands

These are for looking at and changing memory while it runs:

```
peek <addr> <n>          dump n bytes (also: peek debug / state / lcd)
poke hex <addr> <bytes>  write hex bytes
poke str <addr> <text>   write a string
sim sw1 | sw2 | run <xx> | phase <0-2>   fake inputs
debug on | off           turn on phase messages
lcd msg <text>           write to the LCD message buffer
addrs                    print useful addresses
```

`lcd_msg_buf` is placed at a fixed address by the linker script (`tm4c123gh6pm.lds`), so you can poke a string there and it shows up on the LCD.

## Pins

- Motor PWM: PE5 (M0PWM5), direction: PA2 / PA3
- 7-segment: segments PB0-PB7, digits PA4 / PA5
- LCD: data PB0-PB7, RS PA6, E PA7
- Keypad: rows PE0-PE3, columns PC4-PC7
- UART0: PA0 / PA1
- LEDs: PF1-PF3, SW1 / SW2: PF4 / PF0

## Files

- `main.c` - the whole controller
- `drivers/` - separate LCD, keypad and UART drivers I wrote along the way. The current `main.c` has its own versions of these functions and doesn't use them, but they're kept for reference.
- `tm4c123gh6pm.lds` - linker script with the `lcd_msg_buf` section
- The rest is the CCS project, which you can import directly.
