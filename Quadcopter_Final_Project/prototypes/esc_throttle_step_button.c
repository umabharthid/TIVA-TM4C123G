#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

/* ================== GLOBALS ================== */
volatile uint32_t tick = 0;
volatile uint32_t last_time = 0;

// 100 µs resolution
volatile uint32_t ontime = 10;   // 1.0 ms
volatile uint32_t offtime = 190; // 20 ms total

// Button handling
uint8_t last_button_state = 1;
uint32_t last_debounce_time = 0;
#define DEBOUNCE_TIME 50   // 50 ms

/* ================== INIT ================== */
void Init_Hardware(void)
{
    SYSCTL_RCGCGPIO_R |= (1 << 1) | (1 << 5);
    while(!(SYSCTL_PRGPIO_R & ((1 << 1) | (1 << 5))));

    // PB6 → output
    GPIO_PORTB_DIR_R |= (1 << 6);
    GPIO_PORTB_DEN_R |= (1 << 6);

    // PF4 → SW1 input
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R |= (1 << 4);
    GPIO_PORTF_DIR_R &= ~(1 << 4);
    GPIO_PORTF_DEN_R |= (1 << 4);
    GPIO_PORTF_PUR_R |= (1 << 4);

    // SysTick = 100 µs
    NVIC_ST_CTRL_R = 0;
    NVIC_ST_RELOAD_R = 1600 - 1;
    NVIC_ST_CURRENT_R = 0;
    NVIC_ST_CTRL_R = 0x07;
}

/* ================== ISR ================== */
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
        /* -------- NON-BLOCKING BUTTON -------- */
        uint8_t current_state = (GPIO_PORTF_DATA_R & (1 << 4)) ? 1 : 0;

        // Detect press (falling edge)
        if(last_button_state == 1 && current_state == 0)
        {
            // Check debounce using tick (100 µs units)
            if((tick - last_debounce_time) > (DEBOUNCE_TIME * 10))
            {
                last_debounce_time = tick;

                // 🔥 Increase throttle
                if(ontime < 20)
                {
                    ontime++;
                    offtime--;
                }
            }
        }

        last_button_state = current_state;

        /* -------- PWM GENERATION -------- */
        if(tick < (last_time + ontime))
        {
            GPIO_PORTB_DATA_R |= (1 << 6);
        }
        else if(tick < (last_time + ontime + offtime))
        {
            GPIO_PORTB_DATA_R &= ~(1 << 6);
        }
        else
        {
            last_time += (ontime + offtime);
        }
    }
}
