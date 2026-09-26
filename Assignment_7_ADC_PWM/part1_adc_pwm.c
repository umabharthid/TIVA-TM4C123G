//Use keypad for control and show all status on lcd

#include <stdint.h>
#include "inc/tm4c123gh6pm.h"
#include "uart.h"


volatile uint32_t tick,last_toggle=0;

void SysTick_Handler()
{

    tick++;
    }


// ================= PWM INIT =================
void PWM0_Init(void)
{
    SYSCTL_RCGCPWM_R |= 1;        // Enable PWM0
    SYSCTL_RCGCGPIO_R |= (1<<1);  // Enable Port B
    while((SYSCTL_PRGPIO_R & (1<<1)) == 0);

    // Configure PB6 as M0PWM0
    GPIO_PORTB_AFSEL_R |= (1<<6);
    GPIO_PORTB_PCTL_R &= ~(0xF << 24);
    GPIO_PORTB_PCTL_R |= (4 << 24);
    GPIO_PORTB_DEN_R |= (1<<6);

    // PWM clock = system clock / 64
    SYSCTL_RCC_R |= (1<<20); // USEPWMDIV
    SYSCTL_RCC_R = (SYSCTL_RCC_R & ~0x000E0000) | (0x5 << 17);

    PWM0_0_CTL_R = 0;        // Disable generator 0

    PWM0_0_LOAD_R = 2500;    // Frequency setting (~few kHz)

    PWM0_0_CMPA_R = 1250;    // Start with 50% duty

    PWM0_0_GENA_R = 0x8C;    // Set on LOAD, clear on CMPA

    PWM0_0_CTL_R |= 1;       // Enable generator

    PWM0_ENABLE_R |= 1;      // Enable PWM output (M0PWM0)

    NVIC_ST_CTRL_R = 0;
                NVIC_ST_RELOAD_R = 15999;
                NVIC_ST_CURRENT_R = 0;
                NVIC_ST_CTRL_R = 0x07;
                //__enable_irq();
}


// ================= ADC INIT =================
void ADC0_Init(void)
{
    SYSCTL_RCGCADC_R |= 1;        // Enable ADC0
    SYSCTL_RCGCGPIO_R |= (1<<4);  // Enable Port E
    while((SYSCTL_PRGPIO_R & (1<<4)) == 0);

    // Configure PE3 (AIN0)
    GPIO_PORTE_AFSEL_R |= (1<<3);
    GPIO_PORTE_DEN_R &= ~(1<<3);
    GPIO_PORTE_AMSEL_R |= (1<<3);

    ADC0_ACTSS_R &= ~8;           // Disable SS3
    ADC0_EMUX_R &= ~0xF000;       // Software trigger
    ADC0_SSMUX3_R = 0;            // AIN0
    ADC0_SSCTL3_R = 6;            // IE0 + END0
    ADC0_ACTSS_R |= 8;            // Enable SS3
}


// ================= ADC READ =================
uint16_t ADC0_Read(void)
{
    ADC0_PSSI_R = 8;                    // Start conversion
    while((ADC0_RIS_R & 8) == 0);       // Wait
    uint16_t value = ADC0_SSFIFO3_R;    // Read
    ADC0_ISC_R = 8;                     // Clear flag
    return value;
}


// ================= MAIN =================
int main(void)
{
    PWM0_Init();
    ADC0_Init();

    while(1)
    {
        uint16_t adc_val = ADC0_Read();   // 0–4095

        uint32_t load = PWM0_0_LOAD_R;

        // Map ADC  PWM duty
        uint32_t cmp = load - ((adc_val * load) / 4095);

        PWM0_0_CMPA_R = cmp;
    }
}
