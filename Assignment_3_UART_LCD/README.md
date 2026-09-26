# Assignment 3 — UART Command Interface with LED and 16x2 LCD

Text commands typed on a serial terminal control the onboard red LED and a character LCD on the
TM4C123 LaunchPad. The main loop runs three cooperative tasks (UART, LED, LCD) off a ~1 ms
software tick, each implemented as a small state machine.

## Commands
Send over UART0 at **9600 baud, 8N1**, terminated with Enter. Characters are echoed back.

| Command | Effect |
|---|---|
| `ON` | Red LED on |
| `OFF` | Red LED off |
| `BLINK` | Red LED blinks (500 ms period) |
| `PRINT <text>` | Clears the LCD and prints `<text>` on line 1 |
| `FWD` | Scrolls the LCD display left continuously (300 ms step) |
| `BWD` | Scrolls the LCD display right continuously |
| `CLEAR` | Clears the LCD and turns the LED off |

## Hardware Connections
| Signal | Pin |
|---|---|
| UART0 RX / TX | PA0 / PA1 (on-board USB virtual COM port) |
| LCD D0–D7 (8-bit mode) | PB0–PB7 |
| LCD RS | PA6 |
| LCD E | PA7 |
| Red LED | PF1 |

## Files
| Path | Purpose |
|---|---|
| `UART/` | Complete Code Composer Studio project |
| `UART/main.c` | Application code |
| `UART_LCD_Uma_Bharathi_D.docx` | Assignment report |

## Build & Run
Import `UART/` into Code Composer Studio (*File → Import → CCS Projects*), build and flash.
Open a serial terminal (PuTTY, Tera Term, etc.) on the LaunchPad's COM port at 9600 baud.
