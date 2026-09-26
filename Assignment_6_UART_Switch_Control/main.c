#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

//STATES

typedef enum {
    MODE_SC=0,
    MODE_SP,
    MODE_EN,
    MODE_ST,
    MODE_RS
} SystemMode;

volatile SystemMode currentMode = MODE_RS;


// CONFIG SOURCE

typedef enum {
    CONFIG_SWITCH=0,
    CONFIG_UART
} ConfigSource;

volatile ConfigSource configOwner = CONFIG_SWITCH;


// GLOBAL VARIABLES

volatile uint8_t colorIndex = 0;
volatile uint8_t speedIndex = 0;
volatile uint8_t runFlag = 0;

volatile uint8_t debounce_sw1 = 0;
volatile uint8_t debounce_sw2 = 0;

uint32_t blinkCounter = 0;
uint8_t ledState = 0;

char disp[4] = {'r','S','0','0'};


// TABLES

const uint8_t colorTable[8] =
{0x00,0x02,0x04,0x08,0x06,0x0A,0x0C,0x0E};

const uint32_t speedDelay[8] =
{2000,1600,1200,900,600,400,200,100};


// SYSTICK

void SysTick_Init(void)
{
    NVIC_ST_CTRL_R = 0;
    NVIC_ST_RELOAD_R = 16000 - 1; //1ms @16MHz
    NVIC_ST_CURRENT_R = 0;
    NVIC_ST_CTRL_R = 7; //enable + interrupt
}

void SysTick_Handler(void)
{
    // SW1 debounce
    if(debounce_sw1 > 0)
    {
        debounce_sw1--;

        if(debounce_sw1 == 0)
        {
            if((GPIO_PORTF_DATA_R & 0x10) == 0)
                debounce_sw1 = 1; //wait until release
        }
    }

    // SW2 debounce
    if(debounce_sw2 > 0)
    {
        debounce_sw2--;

        if(debounce_sw2 == 0)
        {
            if((GPIO_PORTF_DATA_R & 0x01) == 0)
                debounce_sw2 = 1; //wait until release
        }
    }

    // LED blink
    if(runFlag)
    {
        blinkCounter++;
        if(blinkCounter >= speedDelay[speedIndex])
        {
            blinkCounter = 0;
            ledState ^= 1;
        }
    }
}


// ENCODE

uint8_t encode(char c)
{
    switch(c)
    {
        case '0': return 0x3F;
        case '1': return 0x06;
        case '2': return 0x5B;
        case '3': return 0x4F;
        case '4': return 0x66;
        case '5': return 0x6D;
        case '6': return 0x7D;
        case '7': return 0x07;
        case '8': return 0x7F;
        case '9': return 0x6F;
        case 'S': return 0x6D;
        case 'C': return 0x39;
        case 'P': return 0x73;
        case 'E': return 0x79;
        case 'n': return 0x37;
        case 't': return 0x78;
        case 'r': return 0x50;
        default: return 0x00;
    }
}


// DISPLAY

void SevenSeg_refresh(void)
{
    static uint8_t pos = 0;

    GPIO_PORTA_DATA_R &= ~0xF0;
    GPIO_PORTB_DATA_R = encode(disp[pos]);
    GPIO_PORTA_DATA_R |= (0x80 >> pos);

    pos = (pos + 1) & 0x03;

    for(volatile int i=0;i<1000;i++);
}


// GPIO INIT

void GPIO_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x23; //enable clk to gpio port A,B,F
    while((SYSCTL_PRGPIO_R & 0x23)==0); //wait till ready

    GPIO_PORTA_DIR_R |= 0xF0; //PA4-7 set as output
    GPIO_PORTA_DEN_R |= 0xF0; //digital enable

    GPIO_PORTB_DIR_R |= 0xFF; //PB0-7
    GPIO_PORTB_DEN_R |= 0xFF;

    GPIO_PORTF_LOCK_R = 0x4C4F434B; //unlock (ASCII for LOCK)
    GPIO_PORTF_CR_R |= 0x1F; //to allow changes in PF0-4

    GPIO_PORTF_DIR_R |= 0x0E; //0000 1110
    GPIO_PORTF_DIR_R &= ~0x11; //0001 0001 PF0 -sw2, PF4 -sw1

    GPIO_PORTF_PUR_R |= 0x11; //Pullup(GND)
    GPIO_PORTF_DEN_R |= 0x1F;

    GPIO_PORTF_IS_R &= ~0x11; //interrupt sense 1-level sensitive
    GPIO_PORTF_IBE_R &= ~0x11; //interrupt both edges
    GPIO_PORTF_IEV_R &= ~0x11; //falling edge trigger
    GPIO_PORTF_ICR_R = 0x11; //clear registers
    GPIO_PORTF_IM_R |= 0x11; //mask

    NVIC_EN0_R |= (1<<30); //30 intrrupt - GPIO port F
}


// ISR

void GPIOF_Handler(void)
{
    uint32_t status = GPIO_PORTF_RIS_R; //Raw Interrupt Status Register - which switch caused interrupt
    GPIO_PORTF_ICR_R = status;

    if(configOwner != CONFIG_SWITCH)
        return;

    // SW2
    if((status & 0x01) && debounce_sw2 == 0)
    {
        if((GPIO_PORTF_DATA_R & 0x01) == 0) //confirm LOW
        {
            debounce_sw2 = 30; //30ms debounce

            currentMode = (currentMode + 1) % 5;

            if(currentMode == MODE_EN)
            {
                runFlag = 1; //allows blinking
                blinkCounter = 0;
                ledState = 1;
            }
            else if(currentMode == MODE_ST)
            {
                runFlag = 0;
                ledState = 0;
            }
            else if(currentMode == MODE_RS)
            {
                colorIndex = 0;
                speedIndex = 0;
                runFlag = 0;
                ledState = 0;
            }
        }
    }

    // SW1
    if((status & 0x10) && debounce_sw1 == 0)
    {
        if((GPIO_PORTF_DATA_R & 0x10) == 0) //confirm LOW
        {
            debounce_sw1 = 30; //30ms debounce

            if(currentMode == MODE_SC)
                colorIndex = (colorIndex + 1) % 8;
            else if(currentMode == MODE_SP)
                speedIndex = (speedIndex + 1) % 8;
        }
    }
}


// UART INIT

void UART0_Init(void)
{
    SYSCTL_RCGCUART_R |= 0x01; //enable UART0 clock
    SYSCTL_RCGCGPIO_R |= 0x01; //enable GPIOA clock

    while((SYSCTL_PRUART_R & 0x01)==0); //wait till UART ready
    while((SYSCTL_PRGPIO_R & 0x01)==0); //wait till GPIO ready

    UART0_CTL_R &= ~0x01; //disable UART

    UART0_IBRD_R = 8; //115200 baud @16MHz
    UART0_FBRD_R = 44;
    UART0_LCRH_R = 0x60; //8bit, no parity, 1 stop
    UART0_CC_R = 0x00; //system clock

    UART0_CTL_R = 0x301; //enable UART, TXE, RXE

    GPIO_PORTA_AFSEL_R |= 0x03; //PA0,PA1 alternate
    GPIO_PORTA_PCTL_R &= ~0xFF;
    GPIO_PORTA_PCTL_R |= 0x11; //UART function
    GPIO_PORTA_DEN_R |= 0x03;
    GPIO_PORTA_AMSEL_R &= ~0x03;

    UART0_IM_R |= 0x10; //RX interrupt enable
    NVIC_EN0_R |= (1<<5); //enable UART0 interrupt
}


// UART ISR

void UART0_Handler(void)
{
    if(UART0_MIS_R & 0x10) //RX interrupt
    {
        char ch = UART0_DR_R; //read data
        UART0_ICR_R = 0x10; //clear interrupt

        if(ch==0x08 || ch==0x7F) //backspace handling
        {
            while(UART0_FR_R & 0x20);
            UART0_DR_R = '\b';
            while(UART0_FR_R & 0x20);
            UART0_DR_R = ' ';
            while(UART0_FR_R & 0x20);
            UART0_DR_R = '\b';
            return;
        }

        while(UART0_FR_R & 0x20);
        UART0_DR_R = ch; //echo

        if(ch>='A' && ch<='Z')
            ch = ch + 32; //case insensitive

        if(ch=='c')
        {
            configOwner = CONFIG_UART;
            return;
        }

        if(ch=='x')
        {
            configOwner = CONFIG_SWITCH;
            return;
        }

        if(configOwner != CONFIG_UART)
            return;

        if(ch=='m') //same as SW2
        {
            currentMode = (currentMode + 1) % 5;

            if(currentMode==MODE_EN)
            {
                runFlag = 1;
                blinkCounter = 0;
                ledState = 1;
            }
            else if(currentMode==MODE_ST)
            {
                runFlag = 0;
                ledState = 0;
            }
            else if(currentMode==MODE_RS)
            {
                colorIndex = 0;
                speedIndex = 0;
                runFlag = 0;
                ledState = 0;
            }
        }
        else if(ch=='i') //same as SW1
        {
            if(currentMode==MODE_SC)
                colorIndex = (colorIndex + 1) % 8;
            else if(currentMode==MODE_SP)
                speedIndex = (speedIndex + 1) % 8;
        }
    }
}


// MAIN

int main(void)
{
    GPIO_Init();
    UART0_Init();
    SysTick_Init();
    __asm(" CPSIE i "); //enable global interrupts

    while(1)
    {
        if(currentMode==MODE_SC)
        {
            disp[0]='S';
            disp[1]='C';
        }
        else if(currentMode==MODE_SP)
        {
            disp[0]='S';
            disp[1]='P';
        }
        else if(currentMode==MODE_EN)
        {
            disp[0]='E';
            disp[1]='n';
        }
        else if(currentMode==MODE_ST)
        {
            disp[0]='S';
            disp[1]='t';
        }
        else
        {
            disp[0]='r';
            disp[1]='S';
        }

        disp[2]='0'+colorIndex;
        disp[3]='0'+speedIndex;

        SevenSeg_refresh();

        if(ledState)
            GPIO_PORTF_DATA_R =
                (GPIO_PORTF_DATA_R & ~0x0E) | colorTable[colorIndex];
        else
            GPIO_PORTF_DATA_R &= ~0x0E;
    }
}
