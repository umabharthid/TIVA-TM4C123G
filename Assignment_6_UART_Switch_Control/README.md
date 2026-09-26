# Assignment 6 — Menu Control from Switches or UART

Extends Assignment 5: the same five-mode RGB LED menu can be driven either from the onboard
switches or from a serial terminal, with only one source in control at a time. Both the switches
and UART receive are interrupt driven, with 30 ms switch debouncing done in the SysTick handler.

## Modes
| Display | Mode | Effect |
|---|---|---|
| `SC` | Select Colour | "Increment" selects the next of 8 colours |
| `SP` | Select sPeed | "Increment" selects the next of 8 blink speeds |
| `En` | ENable | LED starts blinking |
| `St` | SToP | LED stops |
| `rS` | ReSet | Colour and speed reset to 0 (start-up mode) |

Display layout: `[mode][mode][colour][speed]`.

## Controls
| Switch mode (default) | UART mode | Action |
|---|---|---|
| SW2 (PF0) | `m` | Next mode |
| SW1 (PF4) | `i` | Increment colour / speed |
| — | `c` | Hand control to UART (switches ignored) |
| — | `x` | Hand control back to switches |

UART0 runs at **115200 baud, 8N1**. Commands are single characters, case-insensitive, and echoed
back; backspace is handled.

## Hardware Connections
| Signal | Pin |
|---|---|
| 7-segment segments | PB0–PB7 |
| Digit enables | PA4–PA7 |
| UART0 RX / TX | PA0 / PA1 (USB virtual COM port) |
| RGB LED | PF1–PF3 |
| SW1 / SW2 | PF4 / PF0 |

## Files
| File | Purpose |
|---|---|
| `main.c` | Application code |
| `Assignment_6.docx` | Assignment report |

## Build & Run
Create a TM4C123GH6PM project in Code Composer Studio, replace `main.c`, and register
`GPIOF_Handler`, `UART0_Handler` and `SysTick_Handler` in the startup file's vector table.
