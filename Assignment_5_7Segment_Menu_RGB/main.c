#include <stdint.h>
#include "inc/tm4c123gh6pm.h"
 
/* ------------------- GLOBALS ------------------- */
 
volatile uint8_t current_digit = 0;
volatile uint32_t ticks = 0;
 
volatile uint32_t blink_counter = 0;
volatile uint8_t blink_toggle = 0;
 
typedef enum {SC,SP,EN,ST,RS} menu_state;
volatile menu_state current_state = SC;
 
volatile uint8_t color = 0;
volatile uint8_t speed = 0;
 
/* ------------------- 7 SEGMENT CODES ------------------- */
 
#define seg_S 0x6D
#define seg_C 0x39
#define seg_P 0x73
#define seg_T 0x78
#define seg_E 0x79
#define seg_N 0x54
#define seg_R 0x70
 
#define seg_0 0x3F
#define seg_1 0x06
#define seg_2 0x5B
#define seg_3 0x4F
#define seg_4 0x66
#define seg_5 0x6D
#define seg_6 0x7D
#define seg_7 0x07
 
const uint8_t num[8]={seg_0,seg_1,seg_2,seg_3,seg_4,seg_5,seg_6,seg_7};
uint8_t display_buf[4];
 
const uint32_t blink_table[8]={2000,1500,1000,750,500,300,200,100};
 
/* ------------------- PORT F INIT (SWITCH + RGB) ------------------- */
 
void PortF_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x20;
    while((SYSCTL_PRGPIO_R & 0x20)==0);
 
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R |= 0x1F;
 
    GPIO_PORTF_AMSEL_R = 0;
    GPIO_PORTF_PCTL_R = 0;
    GPIO_PORTF_DIR_R = 0x0E;      // PF1-3 output
    GPIO_PORTF_DEN_R = 0x1F;
    GPIO_PORTF_PUR_R = 0x11;      // PF0 & PF4 pullup
 
    /* Interrupts */
    GPIO_PORTF_IS_R &= ~0x11;
    GPIO_PORTF_IBE_R &= ~0x11;
    GPIO_PORTF_IEV_R &= ~0x11;    // falling edge
    GPIO_PORTF_ICR_R = 0x11;
    GPIO_PORTF_IM_R |= 0x11;
 
    NVIC_PRI7_R = (NVIC_PRI7_R&0xFF1FFFFF)|(5<<21);
    NVIC_EN0_R |= (1<<30);
}
 
/* ------------------- 7 SEGMENT INIT ------------------- */
 
void Seg7_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x03;
    while((SYSCTL_PRGPIO_R & 0x03)==0);
 
    /* segments -> PORTB */
    GPIO_PORTB_DIR_R = 0xFF;
    GPIO_PORTB_DEN_R = 0xFF;
 
    /* digits -> PA2 PA3 PA4 PA5 */
    GPIO_PORTA_DIR_R |= 0x3C;
    GPIO_PORTA_DEN_R |= 0x3C;
}
 
/* ------------------- SYSTICK ------------------- */
 
void SysTick_Init(void)
{
    NVIC_ST_CTRL_R = 0;
    NVIC_ST_RELOAD_R = 16000-1;   // 1ms @16MHz
    NVIC_ST_CURRENT_R = 0;
    NVIC_ST_CTRL_R = 7;
}
 
/* ------------------- INTERRUPT: SWITCHES ------------------- */
 
void GPIOPortF_Handler(void)
{
    if(GPIO_PORTF_RIS_R & 0x01)   // SW2 (mode)
    {
        current_state = (current_state+1)%5;
        GPIO_PORTF_ICR_R = 0x01;
    }
 
    if(GPIO_PORTF_RIS_R & 0x10)   // SW1 (change)
    {
        if(current_state==SC) color=(color+1)%8;
        else if(current_state==SP) speed=(speed+1)%8;
        GPIO_PORTF_ICR_R = 0x10;
    }
}
 
/* ------------------- DISPLAY UPDATE ------------------- */
 
void update_menu(void)
{
    switch(current_state)
    {
        case SC: display_buf[3]=seg_S; display_buf[2]=seg_C; break;
        case SP: display_buf[3]=seg_S; display_buf[2]=seg_P; break;
        case EN: display_buf[3]=seg_E; display_buf[2]=seg_N; break;
        case ST: display_buf[3]=seg_S; display_buf[2]=seg_T; break;
        case RS:
            display_buf[3]=seg_R;
            display_buf[2]=seg_S;
            color=0; speed=0;
            break;
    }
 
    display_buf[1]=num[color];
    display_buf[0]=num[speed];
}
 
/* ------------------- LED CONTROL ------------------- */
 
void LED_Task(void)
{
    uint8_t led_bits = (color<<1)&0x0E;
 
    if(current_state==EN)
    {
        if(blink_toggle)
            GPIO_PORTF_DATA_R=(GPIO_PORTF_DATA_R&~0x0E)|led_bits;
        else
            GPIO_PORTF_DATA_R&=~0x0E;
    }
    else
        GPIO_PORTF_DATA_R&=~0x0E;
}
 
/* ------------------- SYSTICK HANDLER ------------------- */
 
void SysTick_Handler(void)
{
    ticks++;
 
    /* multiplex display */
    GPIO_PORTA_DATA_R &= ~0x3C;
    GPIO_PORTB_DATA_R = ~display_buf[current_digit];
    GPIO_PORTA_DATA_R |= (0x04<<current_digit);
 
    current_digit++;
    if(current_digit>3) current_digit=0;
 
    /* blink timing */
    if(current_state==EN)
    {
        blink_counter++;
        if(blink_counter>=blink_table[speed])
        {
            blink_counter=0;
            blink_toggle^=1;
        }
    }
}
 
/* ------------------- MAIN ------------------- */
 
int main(void)
{
    PortF_Init();
    Seg7_Init();
    SysTick_Init();
 
    __asm(" CPSIE I");   // enable interrupts
 
    while(1)
    {
        update_menu();
        LED_Task();
    }
}