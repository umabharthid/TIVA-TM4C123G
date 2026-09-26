#include <stdint.h>
#include "inc/tm4c123gh6pm.h"
#include <string.h>

/* ================== CONFIG ================== */

#define RED_LED 0x02

#define LCD_RS  0x40
#define LCD_E   0x80

#define SCROLL_PERIOD 300      // ms
#define BLINK_PERIOD  500      // ms

/* ================== STATES ================== */

typedef enum {
    LED_OFF,
    LED_ON,
    LED_BLINK
} LED_State;

typedef enum {
    LCD_IDLE,
    LCD_STATIC,
    LCD_SCROLL_FWD,
    LCD_SCROLL_BWD
} LCD_State;

/* ================== GLOBALS ================== */

LED_State led_state = LED_OFF;
LCD_State lcd_state = LCD_IDLE;

uint32_t tick = 0;
uint32_t led_timer = 0;
uint32_t lcd_timer = 0;

char lcd_buffer[32];
uint8_t uart_index = 0;

/* ================== SIMPLE DELAY ================== */

void delay_us(int n)
{
    for(int i=0;i<n;i++)
        for(int j=0;j<3;j++);
}

/* ================== LCD LOW LEVEL ================== */

void lcd_command(uint8_t cmd)
{
    GPIO_PORTA_DATA_R &= ~LCD_RS;     // RS = 0
    GPIO_PORTB_DATA_R = cmd;
    GPIO_PORTA_DATA_R |= LCD_E;       // E = 1
    delay_us(5);
    GPIO_PORTA_DATA_R &= ~LCD_E;      // E = 0
}

void lcd_data(uint8_t data)
{
    GPIO_PORTA_DATA_R |= LCD_RS;      // RS = 1
    GPIO_PORTB_DATA_R = data;
    GPIO_PORTA_DATA_R |= LCD_E;
    delay_us(5);
    GPIO_PORTA_DATA_R &= ~LCD_E;
}

void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0) ? col : (0x40 + col);
    lcd_command(0x80 | addr);
}

void lcd_print(char *s)
{
    while(*s)
        lcd_data(*s++);
}

void lcd_clear(void)
{
    lcd_command(0x01);
    delay_us(2000);    // IMPORTANT delay after clear
}

/* ================== HARDWARE INIT ================== */

void Init_Hardware(void)
{
    SYSCTL_RCGC2_R |= 0x23;   // A, B, F
    SYSCTL_RCGCUART_R |= 0x01;
    SYSCTL_RCGCGPIO_R |= 0x01;

    /* LCD */
    GPIO_PORTA_DIR_R |= (LCD_RS | LCD_E);
    GPIO_PORTA_DEN_R |= (LCD_RS | LCD_E);

    GPIO_PORTB_DIR_R |= 0xFF;
    GPIO_PORTB_DEN_R |= 0xFF;

    /* LED */
    GPIO_PORTF_DIR_R |= RED_LED;
    GPIO_PORTF_DEN_R |= RED_LED;

    /* UART0 */
    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_PCTL_R &= ~0x000000FF;
    GPIO_PORTA_PCTL_R |=  0x00000011;
    GPIO_PORTA_DEN_R |= 0x03;

    UART0_CTL_R &= ~0x01;
    UART0_IBRD_R = 104;    // 9600 baud @16MHz
    UART0_FBRD_R = 11;
    UART0_LCRH_R = 0x70;
    UART0_CTL_R = 0x301;

    /* LCD INIT */
    delay_us(20000);
    lcd_command(0x38);
    lcd_command(0x0C);
    lcd_command(0x06);
    lcd_clear();
}

/* ================== LED TASK ================== */

void LED_Task(void)
{
    switch(led_state)
    {
        case LED_OFF:
            GPIO_PORTF_DATA_R &= ~RED_LED;
            break;

        case LED_ON:
            GPIO_PORTF_DATA_R |= RED_LED;
            break;

        case LED_BLINK:
            if(tick - led_timer >= BLINK_PERIOD)
            {
                GPIO_PORTF_DATA_R ^= RED_LED;
                led_timer = tick;
            }
            break;
    }
}

/* ================== LCD TASK ================== */

void LCD_Task(void)
{
    switch(lcd_state)
    {
        case LCD_IDLE:
        case LCD_STATIC:
            break;

        case LCD_SCROLL_FWD:
            if(tick - lcd_timer >= SCROLL_PERIOD)
            {
                lcd_command(0x18);
                lcd_timer = tick;
            }
            break;

        case LCD_SCROLL_BWD:
            if(tick - lcd_timer >= SCROLL_PERIOD)
            {
                lcd_command(0x1C);
                lcd_timer = tick;
            }
            break;
    }
}

/* ================== COMMAND EXEC ================== */

void process_command(void)
{
    if(strcmp(lcd_buffer,"ON")==0)
    {
        led_state = LED_ON;
    }

    else if(strcmp(lcd_buffer,"OFF")==0)
    {
        led_state = LED_OFF;
    }

    else if(strcmp(lcd_buffer,"BLINK")==0)
    {
        led_state = LED_BLINK;
        led_timer = tick;    // reset blink timing
    }

    else if(strncmp(lcd_buffer,"PRINT ",6)==0)
    {
        lcd_clear();
        lcd_set_cursor(0,0);
        lcd_print(&lcd_buffer[6]);
        lcd_state = LCD_STATIC;
    }

    else if(strcmp(lcd_buffer,"FWD")==0)
    {
        lcd_state = LCD_SCROLL_FWD;
        lcd_timer = tick;
    }

    else if(strcmp(lcd_buffer,"BWD")==0)
    {
        lcd_state = LCD_SCROLL_BWD;
        lcd_timer = tick;
    }

    else if(strcmp(lcd_buffer,"CLEAR")==0)
    {
        lcd_clear();
        lcd_state = LCD_IDLE;
        led_state = LED_OFF;
    }
}

/* ================== UART TASK ================== */

void UART_Task(void)
{
    if(UART0_FR_R & 0x10) return;   // RXFE

    char c = UART0_DR_R;
    UART0_DR_R = c;   // echo back

    if(c == '\r' || c == '\n')
    {
        lcd_buffer[uart_index] = '\0';
        process_command();
        uart_index = 0;
    }
    else
    {
        if(uart_index < 31)
            lcd_buffer[uart_index++] = c;
    }
}

/* ================== MAIN ================== */

int main(void)
{
    Init_Hardware();

    while(1)
    {
        delay_us(1000);   // ~1ms system tick
        tick++;

        UART_Task();
        LED_Task();
        LCD_Task();
    }
}
