#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

/* ================== GLOBALS ================== */
volatile uint32_t tick = 0;
volatile uint32_t last_time = 0;

volatile uint32_t ontime = 2;   // ms
volatile uint32_t offtime = 18; // ms

/* ================== INIT ================== */
void Init_Hardware(void)
{
    // Enable Port B clock
    SYSCTL_RCGCGPIO_R |= (1 << 1);
    while(!(SYSCTL_PRGPIO_R & (1 << 1)));

    // PB6 as output
    GPIO_PORTB_DIR_R |= (1 << 6);
    GPIO_PORTB_DEN_R |= (1 << 6);

    // -------- SysTick (1 ms interrupt) --------
    NVIC_ST_CTRL_R = 0;
    NVIC_ST_RELOAD_R = 16000 - 1; // 1 ms @ 16 MHz
    NVIC_ST_CURRENT_R = 0;
    NVIC_ST_CTRL_R = 0x07; // enable + interrupt + system clock
}

/* ================== SYSTICK ISR ================== */
void SysTick_Handler(void)
{
    tick++;
}

/* ================== MAIN ================== */
int main(void)
{
    Init_Hardware();

    while(1)
    {
        // After ~4 seconds → switch to 1 ms pulse
        if(tick > 4000)
        {
            ontime = 1;
            offtime = 19;
        }

        // HIGH phase
        if(tick < (last_time + ontime))
        {
            GPIO_PORTB_DATA_R |= (1 << 6);
        }
        // LOW phase
        else if(tick < (last_time + ontime + offtime))
        {
            GPIO_PORTB_DATA_R &= ~(1 << 6);
        }
        // Reset 20 ms frame
        else
        {
            last_time += (ontime + offtime);
        }
    }
}
