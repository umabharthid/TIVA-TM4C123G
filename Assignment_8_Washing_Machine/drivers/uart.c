#include <stdint.h>
#include <string.h>
#include "inc/tm4c123gh6pm.h"
#include "uart.h"

#define UART_BUFFER_SIZE 128

/* internal command buffer */
static char buffer[UART_BUFFER_SIZE];
static uint8_t uart_index = 0;
uint8_t command_ready = 0;


/* ================= UART INIT ================= */

void uart_init(void)
{
    SYSCTL_RCGCUART_R |= 1;
    SYSCTL_RCGCGPIO_R |= 1;

    while(!(SYSCTL_PRGPIO_R & 1));

    GPIO_PORTA_AMSEL_R &= ~0x03;
    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_PCTL_R &= ~0xFF;
    GPIO_PORTA_PCTL_R |= 0x11;

    GPIO_PORTA_DIR_R &= ~0x01;
    GPIO_PORTA_DIR_R |=  0x02;

    GPIO_PORTA_DEN_R |= 0x03;

    UART0_CTL_R &= ~1;

    UART0_IBRD_R = 8;
    UART0_FBRD_R = 44;

    UART0_LCRH_R = 0x70;

    UART0_CTL_R = 0x301;
}


/* ================= SEND CHAR ================= */

static void uart_sendchar(char c)
{
    while(UART0_FR_R & 0x20);
    UART0_DR_R = c;
}


/* ================= PRINT STRING ================= */

void printuart(char *str)
{
    while(*str)
        uart_sendchar(*str++);
}
char* uart_get_buffer(void)
{return buffer;}

/* ================= UPDATE UART ================= */
/* non-blocking polling */

void uart_update(void)
{
    static uint8_t last_was_cr = 0;

    while(!(UART0_FR_R & 0x10))   /* RX FIFO not empty */
    {
        char c = UART0_DR_R;

        /* show typed characters */
        uart_sendchar(c);

        if(c == '\r')
        {
            buffer[uart_index] = '\0';
            command_ready = 1;
            uart_index = 0;
            last_was_cr = 1;
        }
        else if(c == '\n')
        {
            if(!last_was_cr)   /* standalone \n — treat same as \r */
            {
                buffer[uart_index] = '\0';
                command_ready = 1;
                uart_index = 0;
            }
            /* if last_was_cr: this is the \n of a \r\n pair — skip it */
            last_was_cr = 0;
        }
        else
        {
            last_was_cr = 0;
            if(uart_index < UART_BUFFER_SIZE-1)
                buffer[uart_index++] = c;
        }
    }
}


/* ================= COMMAND PARSER ================= */

uint8_t istypeduart(char *cmd)
{
    if(command_ready)
    {
        if(strcmp(buffer, cmd) == 0)
        {
            command_ready = 0;
            return 1;
        }
    }
    return 0;
}
uint8_t uart_command_ready(){return command_ready;}

void uart_clear_command(){command_ready=0;}

