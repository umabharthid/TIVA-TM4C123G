# E3-257 Embedded System Design

My assignments and final project for E3-257 (Embedded System Design), Semester 2, M.Tech DESE at IISc.

Everything runs on the TI Tiva C TM4C123GH6PM LaunchPad. I wrote the code at register level using `inc/tm4c123gh6pm.h` and didn't use TivaWare driverlib. I built it in Code Composer Studio with the GNU ARM compiler.

## What's here

- `Assignment_2_RGB_LED_Switches` - blink the RGB LED, SW1 changes colour and SW2 changes speed
- `Assignment_3_Digital_Safe` - 4x4 keypad + 7-segment display acting as a PIN-locked safe
- `Assignment_4_UART_LCD` - typed UART commands control an LED and a 16x2 LCD
- `Assignment_5_7Segment_Menu_RGB` - menu on a 4-digit 7-segment display, switches handled with interrupts
- `Assignment_6_UART_Switch_Control` - same menu as 5, but it can also be driven from UART
- `Assignment_7_ADC_PWM` - potentiometer on the ADC sets a PWM duty cycle
- `Assignment_8_Washing_Machine` - washing machine controller (keypad, LCD, 7-seg, motor PWM, UART debug commands)
- `Quadcopter_Final_Project` - quadcopter rate controller with the MPU-9250 gyro

Where an assignment has separate driver files (7-segment, UART, LCD, keypad), they're in a `drivers/` folder inside that assignment. The versions change a bit between assignments, so each one keeps its own copy.

## Building

Assignments 2, 4 and 8 are full CCS projects, so you can import them with File > Import > CCS Projects.

For the others, make a new TM4C123GH6PM project in CCS and copy in the `.c` files (and the `drivers/` folder if there is one). Check that the interrupt handlers the code uses are registered in the startup file's vector table. Where a startup file is included in the folder, it already has them.

You'll need TivaWare installed and on the include path for `inc/tm4c123gh6pm.h`.
