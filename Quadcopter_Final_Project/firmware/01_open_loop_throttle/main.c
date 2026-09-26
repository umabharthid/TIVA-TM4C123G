#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

#define PWM_LOAD 5000   // 20ms
#define MIN_PULSE 1000
#define MAX_PULSE 2000

volatile uint32_t rise_time = 0;
volatile uint32_t pulse_width = 1000;
volatile uint32_t last_valid = 1000;

/* ---------- SYSTICK ---------- */
void SysTick_Init(void)
{
    NVIC_ST_CTRL_R = 0;
    NVIC_ST_RELOAD_R = 0xFFFFFF;
    NVIC_ST_CURRENT_R = 0;
    NVIC_ST_CTRL_R = 0x05;
}

uint32_t micros(void)
{
    return (0xFFFFFF - NVIC_ST_CURRENT_R) / 16;
}

/* ---------- PWM ---------- */
void PWM0_Init(void)
{
    SYSCTL_RCGCPWM_R |= 0x01;
    SYSCTL_RCGCGPIO_R |= 0x02;
    while((SYSCTL_PRGPIO_R & 0x02)==0);

    SYSCTL_RCC_R |= (1<<20);
    SYSCTL_RCC_R &= ~(0x7<<17);
    SYSCTL_RCC_R |= (0x6<<17);

    GPIO_PORTB_AFSEL_R |= 0xF0;
    GPIO_PORTB_PCTL_R &= ~0xFFFF0000;
    GPIO_PORTB_PCTL_R |=  0x44440000;
    GPIO_PORTB_DEN_R  |= 0xF0;

    PWM0_0_CTL_R = 0;
    PWM0_1_CTL_R = 0;

    PWM0_0_LOAD_R = PWM_LOAD - 1;
    PWM0_1_LOAD_R = PWM_LOAD - 1;

    PWM0_0_GENA_R = 0x8C;
    PWM0_0_GENB_R = 0x80C;
    PWM0_1_GENA_R = 0x8C;
    PWM0_1_GENB_R = 0x80C;

    PWM0_0_CTL_R |= 1;
    PWM0_1_CTL_R |= 1;

    PWM0_ENABLE_R |= 0x0F;
}

/* ---------- INPUT (PC4) ---------- */
void GPIO_Input_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x04;
    while((SYSCTL_PRGPIO_R & 0x04)==0);

    GPIO_PORTC_DIR_R &= ~0x10;
    GPIO_PORTC_DEN_R |=  0x10;

    GPIO_PORTC_IS_R  &= ~0x10;
    GPIO_PORTC_IBE_R |=  0x10;
    GPIO_PORTC_ICR_R |=  0x10;
    GPIO_PORTC_IM_R  |=  0x10;

    NVIC_EN0_R |= (1 << 2); // Port C interrupt
}

void GPIOPortC_Handler(void)
{
    uint32_t now = micros();
    uint8_t pin = GPIO_PORTC_DATA_R & 0x10;
    GPIO_PORTC_ICR_R = 0x10;

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
            last_valid = width;
        }
        else
            pulse_width = last_valid;
    }
}

/* ---------- UART ---------- */
void UART0_Init(void)
{
    SYSCTL_RCGCUART_R |= 1;
    SYSCTL_RCGCGPIO_R |= 1;
    while((SYSCTL_PRGPIO_R & 1)==0);

    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_PCTL_R  |= 0x11;
    GPIO_PORTA_DEN_R   |= 0x03;

    UART0_CTL_R  = 0;
    UART0_IBRD_R = 8;
    UART0_FBRD_R = 44;
    UART0_LCRH_R = 0x60;
    UART0_CTL_R  = 0x301;
}

void UART0_WriteChar(char c)
{
    while(UART0_FR_R & 0x20);
    UART0_DR_R = c;
}

void UART0_WriteString(char *s)
{
    while(*s) UART0_WriteChar(*s++);
}

void UART0_WriteInt(uint32_t num)
{
    char buf[10]; int i=0;
    do { buf[i++] = num%10+'0'; num/=10; } while(num);
    while(i--) UART0_WriteChar(buf[i]);
}

/* ---------- MAIN ---------- */
int main(void)
{
    SysTick_Init();
    PWM0_Init();
    GPIO_Input_Init();
    UART0_Init();

    UART0_WriteString("Throttle → PWM\r\n");

    while(1)
    {
        uint32_t pw = pulse_width;

        if(pw < MIN_PULSE) pw = MIN_PULSE;
        if(pw > MAX_PULSE) pw = MAX_PULSE;

        uint32_t ticks = pw / 4;

        uint32_t cmp = PWM_LOAD - ticks;

        PWM0_0_CMPA_R = cmp;
        PWM0_0_CMPB_R = cmp;
        PWM0_1_CMPA_R = cmp;
        PWM0_1_CMPB_R = cmp;

        UART0_WriteString("Throttle=");
        UART0_WriteInt(pw);
        UART0_WriteString("\r\n");
    }
}
