#include<stdint.h>
//#include "inc/tm4c123gh6pm.h"
void delayMs(int);

/**
 * main.c
 */
#define SYSCTL_RCGC2_R   (*((volatile uint32_t *)0x400FE108))
#define GPIO_PORTF_DIR_R   (*((volatile uint32_t *)0x40025400))
#define GPIO_PORTF_DATA_R   (*((volatile uint32_t *)0x400253FC))
#define GPIO_PORTF_ENABLE_R   (*((volatile uint32_t *)0x4002551C))



int main(void)
{
    SYSCTL_RCGC2_R |= 0x20;
    GPIO_PORTF_DIR_R |= 0x06;
    GPIO_PORTF_ENABLE_R |= 0x06;

    while(1)
    {
        GPIO_PORTF_DATA_R = 0x06;
        delayMs(100);
        GPIO_PORTF_DATA_R = 0x00;
        delayMs(100);

    }
}
//clock is 16MHz, delay 1ms function below

void delayMs(int n) {
    int i, j;
    for(i = 0 ; i < n; i++)
    for(j = 0; j < 3180; j++) {}      /* do nothing for 1 ms*/

}

