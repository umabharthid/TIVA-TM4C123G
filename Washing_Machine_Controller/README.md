# Washing Machine Controller

A washing-machine state machine on the TM4C123 LaunchPad. The machine is set up and started over
UART, shows its status on a 16x2 LCD, counts down the remaining time on a 2-digit 7-segment
display, and indicates the current phase with the RGB LED.

## States
`IDLE → PREWASH → WASH → SPIN → COMPLETE → IDLE`, plus `PAUSED` and `ERROR`.

- **PREWASH:** first 20 % of the cycle time — green LED blinks slowly (500 ms)
- **WASH:** 20–80 % — green LED solid
- **SPIN:** last 20 % — green LED blinks fast (200 ms)
- **COMPLETE:** white LED for 5 s, then back to IDLE (door and detergent flags are cleared)
- **ERROR:** red LED blinks; the LCD shows the reason

## UART Commands
| Command | Effect |
|---|---|
| `door_close` / `door_open` | Set door state |
| `load_dt` | Mark detergent as loaded |
| `time 5`, `time 15`, `time 30` | Set cycle time (seconds) |
| `start` | Start the cycle. If the door is open, no detergent is loaded or no time is set, the controller goes to ERROR (`DOOR OPEN`, `NO DT`, `SET TIME`) and clears it once the missing condition is fixed. |

In IDLE, the LCD prompts for the next missing step: *Close Door → Load Dt → Set Time → Press Start*.

## Switches
| Switch | Action |
|---|---|
| SW1 (PF4) | Abort: return to IDLE and clear all settings |
| SW2 (PF0) | Pause / resume (resumes into the phase matching the progress so far) |

## Hardware Connections
| Signal | Pin |
|---|---|
| 7-segment segments | PB0–PB7 |
| 7-segment digit enables (ones / tens) | PA4 / PA5 |
| RGB LED | PF1 (R), PF2 (B), PF3 (G) |
| SW1 / SW2 | PF4 / PF0 |
| LCD, UART | As configured in `lcd.h` / `uart.h` |

## Files
| File | Purpose |
|---|---|
| `main.c` | Application code |

## Notes
This file depends on `lcd.h` and `uart.h` (`lcd_init`, `lcd_clear`, `lcd_set_cursor`,
`lcd_print_string`, `uart_init`, `uart_update`, `uart_command_ready`, `uart_get_buffer`,
`uart_clear_command`), which are **not in this folder**. Add them (and their `.c` files) before
building, and register `SysTick_Handler` in the startup file.
