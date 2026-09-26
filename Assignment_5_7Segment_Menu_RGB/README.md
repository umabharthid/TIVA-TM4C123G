# Assignment 5 — Interrupt-Driven Menu on a 4-Digit 7-Segment Display

A five-state menu controls the LaunchPad's RGB LED. The current mode, colour and blink speed are
shown on a multiplexed 4-digit 7-segment display. Switches are handled with GPIO edge interrupts,
and SysTick (1 ms) refreshes the display and times the blinking.

## Menu
**SW2 (PF0)** steps through the modes; **SW1 (PF4)** changes the value in the current mode.

| Display | Mode | SW1 action |
|---|---|---|
| `SC` | Select Colour | Next colour (0–7, RGB bit combinations) |
| `SP` | Select sPeed | Next blink speed (0–7) |
| `En` | ENable | — LED blinks with the chosen colour and speed |
| `St` | SToP | — LED off |
| `rS` | ReSet | — colour and speed reset to 0 |

Display layout: `[mode][mode][colour][speed]`. Blink half-periods range from 2000 ms (speed 0)
down to 100 ms (speed 7).

## Hardware Connections
| Signal | Pin |
|---|---|
| 7-segment segments a–g, dp | PB0–PB7 (driven active low) |
| Digit enables (4 digits) | PA2–PA5 |
| RGB LED | PF1 (R), PF2 (B), PF3 (G) |
| SW1 / SW2 | PF4 / PF0 (internal pull-ups, falling-edge interrupts) |

## Files
| File | Purpose |
|---|---|
| `main.c` | Application code |
| `Assignment_5.docx` | Assignment report |

## Build & Run
Create a TM4C123GH6PM project in Code Composer Studio (or reuse the Assignment 2 project), replace
`main.c`, and register `GPIOPortF_Handler` and `SysTick_Handler` in the startup file's vector table.
