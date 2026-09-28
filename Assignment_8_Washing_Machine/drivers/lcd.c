#include <stdint.h>
#include "lcd.h"
#include "inc/tm4c123gh6pm.h"

static void delayMs(int n)
{
    int i,j;
    for(i=0;i<n;i++)
        for(j=0;j<3180;j++);
}

void lcd_command(uint8_t cmd)
{
    GPIO_PORTA_DATA_R &= ~LCD_RS;   // RS = 0
    GPIO_PORTB_DATA_R = cmd;

    GPIO_PORTA_DATA_R |= LCD_E;     // E = 1
    delayMs(1);
    GPIO_PORTA_DATA_R &= ~LCD_E;    // E = 0
}

void lcd_data(uint8_t data)
{
    GPIO_PORTA_DATA_R |= LCD_RS;    // RS = 1
    GPIO_PORTB_DATA_R = data;

    GPIO_PORTA_DATA_R |= LCD_E;
    delayMs(1);
    GPIO_PORTA_DATA_R &= ~LCD_E;
}

void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t address;
    if(row == 0) address = 0x00 + col;
    else         address = 0x40 + col;
    lcd_command(0x80 | address);
}

void lcd_print_string(char *str)
{
    while(*str)
        lcd_data(*str++);
}

void lcd_clear(void)
{
    lcd_command(0x01);
    delayMs(2);
}

void lcd_init(void)
{
    SYSCTL_RCGC2_R |= 0x03;
    delayMs(1);

    GPIO_PORTA_AFSEL_R &= ~0xC0;
    GPIO_PORTA_AMSEL_R &= ~0xC0;
    GPIO_PORTA_DIR_R   |=  0xC0;
    GPIO_PORTA_DEN_R   |=  0xC0;

    GPIO_PORTB_AFSEL_R &= ~0xFF;
    GPIO_PORTB_AMSEL_R &= ~0xFF;
    GPIO_PORTB_DIR_R   |=  0xFF;
    GPIO_PORTB_DEN_R   |=  0xFF;

    GPIO_PORTA_DATA_R &= ~(LCD_RS | LCD_E);

    delayMs(20);

    lcd_command(0x38);
    lcd_command(0x0C);
    lcd_command(0x06);
    lcd_clear();
}
