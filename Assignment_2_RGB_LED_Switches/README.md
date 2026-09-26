# Assignment 2 — RGB LED Control with Switches on TM4C123

RGB LED control on the **Tiva C Series TM4C123 LaunchPad** using the two onboard switches.

## Features
- **SW1 (PF4):** cycles LED colour (Red → Green → Blue)
- **SW2 (PF0):** changes the LED blink rate
- Software debounce on both switches
- Software-timed blinking using a loop counter
- PF0 (NMI pin) is unlocked before use

## Hardware
- Board: TM4C123GH6PM Tiva C LaunchPad
- LEDs: PF1 (Red), PF2 (Blue), PF3 (Green)
- Switches: SW1 → PF4 (active low), SW2 → PF0 (active low, unlocked)

## How It Works
- The LED toggles ON/OFF after `Delay_Count` iterations of the main loop.
- SW1 changes the current LED colour; SW2 changes the blink delay.
- The delay counter is reset whenever the delay changes, to avoid lock-up or inconsistent blinking.

## Files
| Path | Purpose |
|---|---|
| `02_BLINK_RGB_GNU2_ASSIGNMENT/` | Complete Code Composer Studio project |
| `02_BLINK_RGB_GNU2_ASSIGNMENT/main.c` | Application code |
| `02_BLINK_RGB_GNU2_ASSIGNMENT/tm4c123gh6pm_startup_ccs_gcc.c` | Startup / vector table |
| `02_BLINK_RGB_GNU2_ASSIGNMENT/tm4c123gh6pm.lds` | Linker script |
| `RGB_LED_Control_Report_Uma.pdf` | Assignment report |

## Build & Run
1. In Code Composer Studio: *File → Import → CCS Projects*, select `02_BLINK_RGB_GNU2_ASSIGNMENT`.
2. Build, connect the LaunchPad over USB, and click *Debug* to flash.
