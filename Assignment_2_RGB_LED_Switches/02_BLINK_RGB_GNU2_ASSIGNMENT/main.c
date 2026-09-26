//02_BLINK_RGB_GNU2_ASSIGNMENT
/* Read a switch and write it to the LED */
/* This program reads SW2 of Tiva LaunchPad and write the changed Delay to the LED.
This program reads SW1 of Tiva LaunchPad and write the changed color to the LED.
* SW2 & SW1 is low when pressed. LED is on when high. */
/* SW2 is connected to PORTF0, which is an NMI pin. */
/* In order to use this pin for any function other than NMI, the pin needs be unlocked first. */

#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

void delayMs(int);

#define RED   0x02
#define GREEN 0x08
#define BLUE  0x04

uint8_t Led_Colour = RED;
uint32_t Delay_Count = 64;
uint32_t LED_flag = 0, n = 0; // LED_flag stores the status of the LED, and n is count variable

void GPIO_init();
void LED_color_change();
void LED_delay_change();

int main(void)
{

    GPIO_init(); //Initialise all ports and GPIO pins

    while(1)
    {
              /* read data from PORTF */

        if (!(GPIO_PORTF_DATA_R & 0x10))//Switch1
        {   delayMs(40);
            if (!(GPIO_PORTF_DATA_R & 0x10))
            LED_color_change();
        }

        if (!(GPIO_PORTF_DATA_R & 0x01))
        {
            delayMs(40);
            if (!(GPIO_PORTF_DATA_R & 0x01))
            {
                LED_delay_change();
                while (!(GPIO_PORTF_DATA_R & 0x01)); // wait for release
            }
        }


        if (n<Delay_Count && LED_flag==1)
            n++;
        if (n==Delay_Count && LED_flag==1)
        {
            LED_flag = 0;
            GPIO_PORTF_DATA_R = 0;
            n = 0;

        }
        if (n<Delay_Count && LED_flag==0)
            n++;
        if (n==Delay_Count && LED_flag==0)
        {
            LED_flag = 1;
            GPIO_PORTF_DATA_R = Led_Colour;
            n=0;

        }
        delayMs(1);
     }

}

void delayMs(int n) {
    int i, j;
    for(i = 0 ; i < n; i++)
    for(j = 0; j < 3180; j++) {}      /* do nothing for 1 ms*/

}
void GPIO_init()
{
        SYSCTL_RCGC2_R |= 0x20;       /* enable clock to GPIOF */
        delayMs(100);
        GPIO_PORTF_LOCK_R = 0x4C4F434B;     /* unlock commit register */
        GPIO_PORTF_CR_R = 0x11;             /* make PORTF0 & PORTF4 configurable */
        GPIO_PORTF_DIR_R = 0x0E;            /* set PORTF LED pins as output pin */
        /* and PORTF4 & PORTF0 as input, SW1 & SW2 is on  PORTF4 & PORTF0 */
        GPIO_PORTF_DEN_R = 0x1F;            /* set PORTF pins 4-0 as digital pins */
        GPIO_PORTF_PUR_R = 0x11;   /* enable pull up for pin 0 & pin 4*/
        //LED_delay_change();
        LED_color_change();
}
void LED_color_change()
{
    switch(Led_Colour)
    {
                    case RED:       Led_Colour=GREEN;
                                    break;
                    case GREEN:     Led_Colour=BLUE;
                                    break;
                    case BLUE:      Led_Colour=RED;
                                    break;

    }
    GPIO_PORTF_DATA_R = Led_Colour;
}
void LED_delay_change()
{
    switch(Delay_Count)
    {
                    case 64:        Delay_Count=128;
                                    break;
                    case 128:       Delay_Count=256;
                                    break;
                    case 256:       Delay_Count=64;
                                    break;
                    //default :       Delay_Count=128;
                                    //break;



    }
    n=0;//Very important to ensure theres no locking
}
