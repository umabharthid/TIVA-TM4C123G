#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

/* ---------- GLOBALS ---------- */
volatile uint32_t rise_time   = 0;
volatile uint32_t pulse_width = 1000;   // safe default = min throttle
volatile uint32_t last_valid  = 1000;

/* ---------- FUNCTION DECLARATIONS ---------- */
void SysTick_Init(void);
uint32_t micros(void);
void delay_ms(int n);

void GPIO_PWM_Init(void);

void UART0_Init(void);
void UART0_WriteChar(char c);
void UART0_WriteString(char *s);
void UART0_WriteInt(uint32_t num);

/* ---------- MAIN ---------- */
int main(void)
{
    SysTick_Init();     // always first
    UART0_Init();
    GPIO_PWM_Init();

    UART0_WriteString("PWM Decode Start\r\n");

    while(1)
    {
        UART0_WriteString("Throttle(us)=");
        UART0_WriteInt(pulse_width);
        UART0_WriteString("\r\n");
        delay_ms(100);
    }
}

/* ---------- SYSTICK — free running 24-bit counter ---------- */
// At 16MHz: 1 tick = 0.0625us, wraps every ~1 second
// Used by both micros() and delay_ms()
void SysTick_Init(void)
{
    NVIC_ST_CTRL_R    = 0;
    NVIC_ST_RELOAD_R  = 0xFFFFFF;   // 24-bit max, free running
    NVIC_ST_CURRENT_R = 0;
    NVIC_ST_CTRL_R    = 0x05;       // enable + system clock, no interrupt
}

uint32_t micros(void)
{
    return (0xFFFFFF - NVIC_ST_CURRENT_R) / 16;
}

void delay_ms(int n)
{
    uint32_t start = micros();
    uint32_t wait  = (uint32_t)n * 1000;   // ms to us
    while((micros() - start) < wait);
}

/* ---------- PWM INPUT — PB6, GPIO interrupt ---------- */
void GPIO_PWM_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x02;             // Port B clock
    while((SYSCTL_PRGPIO_R & 0x02) == 0);

    GPIO_PORTB_DIR_R  &= ~0x40;            // PB6 input
    GPIO_PORTB_DEN_R  |=  0x40;            // digital enable
    GPIO_PORTB_IS_R   &= ~0x40;            // edge sensitive
    GPIO_PORTB_IBE_R  |=  0x40;            // both edges
    GPIO_PORTB_ICR_R  |=  0x40;            // clear any pending flag
    GPIO_PORTB_IM_R   |=  0x40;            // unmask interrupt
    NVIC_EN0_R        |=  (1 << 1);        // Port B = IRQ1
}

/* ---------- PWM ISR ---------- */
void GPIOPortB_Handler(void)
{
    uint32_t now = micros();                       // timestamp first
    uint8_t  pin = GPIO_PORTB_DATA_R & 0x40;       // pin state second
    GPIO_PORTB_ICR_R = 0x40;                       // clear flag last

    if(pin)                                         // HIGH = rising edge
    {
        rise_time = now;
    }
    else                                            // LOW = falling edge
    {
        uint32_t width = now - rise_time;

        if(width >= 900 && width <= 2100)           // valid range with ±100 tolerance
        {
            pulse_width = width;
            last_valid  = width;
        }
        else
            pulse_width = last_valid;               // out of range — hold last good value
    }
}

/* ---------- UART INIT ---------- */
void UART0_Init(void)
{
    SYSCTL_RCGCUART_R  |= 1;
    SYSCTL_RCGCGPIO_R  |= 1;
    while((SYSCTL_PRGPIO_R & 1) == 0);

    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_PCTL_R  |= 0x00000011;
    GPIO_PORTA_DEN_R   |= 0x03;

    UART0_CTL_R  = 0;
    UART0_IBRD_R = 8;       // 115200 baud at 16MHz
    UART0_FBRD_R = 44;
    UART0_LCRH_R = 0x60;
    UART0_CC_R   = 0;
    UART0_CTL_R  = 0x301;
}

/* ---------- UART HELPERS ---------- */
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
    char buf[10];
    int i = 0;
    do {
        buf[i++] = num % 10 + '0';
        num /= 10;
    } while(num);
    while(i--) UART0_WriteChar(buf[i]);
}
