#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

volatile uint32_t rise_time   = 0;
volatile uint32_t pulse_width = 1000;  // safe default
volatile uint32_t last_valid  = 1000;  // holds last good reading

void SysTick_Init(void)
{
    NVIC_ST_CTRL_R    = 0;
    NVIC_ST_RELOAD_R  = 0xFFFFFF;
    NVIC_ST_CURRENT_R = 0;
    NVIC_ST_CTRL_R    = 0x05;
}

uint32_t micros(void)
{
    return (0xFFFFFF - NVIC_ST_CURRENT_R) / 16;
}

void GPIO_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x02;
    while((SYSCTL_PRGPIO_R & 0x02)==0);
    GPIO_PORTB_DIR_R  &= ~0x40;
    GPIO_PORTB_DEN_R  |=  0x40;
    GPIO_PORTB_IS_R   &= ~0x40;
    GPIO_PORTB_IBE_R  |=  0x40;
    GPIO_PORTB_ICR_R  |=  0x40;
    GPIO_PORTB_IM_R   |=  0x40;
    NVIC_EN0_R        |= (1 << 1);
}

void GPIOPortB_Handler(void)
{
    uint32_t now = micros();
    uint8_t  pin = GPIO_PORTB_DATA_R & 0x40;  // read pin before clearing
    GPIO_PORTB_ICR_R = 0x40;

    if(pin)
    {
        rise_time = now;
    }
    else
    {
        uint32_t width = now - rise_time;

        if(width >= 900 && width <= 2100)
        {
            pulse_width = width;
            last_valid  = width;
        }
        else
            pulse_width = last_valid;
    }
}

void UART0_Init(void)
{
    SYSCTL_RCGCUART_R |= 1;
    SYSCTL_RCGCGPIO_R |= 1;
    while((SYSCTL_PRGPIO_R & 1)==0);
    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_PCTL_R  |= 0x00000011;
    GPIO_PORTA_DEN_R   |= 0x03;
    UART0_CTL_R  = 0;
    UART0_IBRD_R = 8;
    UART0_FBRD_R = 44;
    UART0_LCRH_R = 0x60;
    UART0_CC_R   = 0;
    UART0_CTL_R  = 0x301;
}

void UART0_WriteChar(char c)
{ while(UART0_FR_R & 0x20); UART0_DR_R = c; }

void UART0_WriteString(char *s)
{ while(*s) UART0_WriteChar(*s++); }

void UART0_WriteInt(uint32_t num)
{
    char buf[10]; int i=0;
    do { buf[i++] = num%10+'0'; num/=10; } while(num);
    while(i--) UART0_WriteChar(buf[i]);
}

int main(void)
{
    SysTick_Init();
    UART0_Init();
    GPIO_Init();
    UART0_WriteString("PWM Decode Start\r\n");
    while(1)
    {
        UART0_WriteString("Throttle(us)=");
        UART0_WriteInt(pulse_width);
        UART0_WriteString("\r\n");
        for(int i=0;i<200000;i++);
    }
}
