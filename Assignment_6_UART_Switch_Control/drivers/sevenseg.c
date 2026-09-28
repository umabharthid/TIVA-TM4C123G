#include "sevenseg.h"
#include "inc/tm4c123gh6pm.h"

/* display buffer */
static volatile uint8_t display_buffer[4];
volatile uint32_t system_time = 0;

/* current digit */
static uint8_t current_digit = 0;

/* initialize hardware + SysTick */
void sevenseg_init(void)
{
    /* Enable clock for Port A and B */
    SYSCTL_RCGCGPIO_R |= (1<<0) | (1<<1);
    while(!(SYSCTL_PRGPIO_R & ((1<<0)|(1<<1))));

    /* Configure digit select pins (PA4–PA7) */
    GPIO_PORTA_DIR_R |= 0xF0;
    GPIO_PORTA_DEN_R |= 0xF0;

    /* Configure segment pins (PB0–PB7) */
    GPIO_PORTB_DIR_R |= 0xFF;
    GPIO_PORTB_DEN_R |= 0xFF;

    /* SysTick setup (1ms interrupt) */
    NVIC_ST_CTRL_R = 0;
    NVIC_ST_RELOAD_R = 16000 - 1;   // 1ms @16MHz
    NVIC_ST_CURRENT_R = 0;
    NVIC_ST_CTRL_R = 0x07;          // enable + interrupt
}

/* set display digits */
void sevenseg_set_digits(uint8_t d,uint8_t c,uint8_t b,uint8_t a)
{
    display_buffer[0] = a;
    display_buffer[1] = b;
    display_buffer[2] = c;
    display_buffer[3] = d;
}

/* SysTick interrupt refreshes display */
void SysTick_Handler(void)
{
    /* turn off all digits */
    GPIO_PORTA_DATA_R = 0;

    system_time++;

    /* output segment pattern */
    GPIO_PORTB_DATA_R = display_buffer[current_digit];

    /* enable correct digit */
    GPIO_PORTA_DATA_R = (1 << (4 + current_digit));

    current_digit++;

    if(current_digit >= 4)
        current_digit = 0;
}
