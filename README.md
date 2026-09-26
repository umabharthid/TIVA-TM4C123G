# E3-257 Embedded System Design — Assignments & Project

Coursework for **E3-257 Embedded System Design** (M.Tech, DESE, IISc Bangalore), Semester 2.
All embedded work targets the **TI Tiva C Series TM4C123GH6PM LaunchPad** and is written at the
register level using `inc/tm4c123gh6pm.h` (no TivaWare driverlib).

**Author:** Uma Bharathi D

## Contents

| Folder | Topic | Peripherals |
|---|---|---|
| [Assignment_1_Student_Records_Linked_List](Assignment_1_Student_Records_Linked_List) | Menu-driven student database using a linked list (host PC, C) | — |
| [Assignment_2_RGB_LED_Switches](Assignment_2_RGB_LED_Switches) | RGB LED colour / blink-rate control with onboard switches | GPIO |
| [Assignment_3_UART_LCD](Assignment_3_UART_LCD) | UART command interface driving an LED and a 16x2 LCD | UART0, GPIO, LCD |
| [Assignment_5_7Segment_Menu_RGB](Assignment_5_7Segment_Menu_RGB) | Interrupt-driven menu on a 4-digit 7-segment display | GPIO interrupts, SysTick |
| [Assignment_6_UART_Switch_Control](Assignment_6_UART_Switch_Control) | Same menu, controllable from either switches or UART | UART0 interrupt, GPIO, SysTick |
| [Assignment_7_ADC_PWM](Assignment_7_ADC_PWM) | Potentiometer (ADC) to PWM duty cycle | ADC0, PWM0 |
| [Washing_Machine_Controller](Washing_Machine_Controller) | Washing-machine state machine with LCD, 7-segment and UART | UART, LCD, GPIO, SysTick |
| [Quadcopter_Final_Project](Quadcopter_Final_Project) | Quadcopter flight controller (final project) | PWM, SPI/I2C (MPU-9250), GPIO capture, UART |

## Building the TM4C123 code

The embedded projects were built with **Code Composer Studio (CCS)** using the GNU ARM toolchain.
Assignments 2 and 3 include complete CCS projects that can be imported directly
(*File → Import → CCS Projects*). For folders that contain only a `main.c`, create a new
TM4C123GH6PM project in CCS (or copy the Assignment 2 project), replace `main.c`, and make sure the
interrupt handlers used by that file are registered in `tm4c123gh6pm_startup_ccs_gcc.c`.

The TM4C123GH6PM datasheet is kept locally in `docs/datasheets/` but is not committed.
