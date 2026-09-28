#ifndef LCD_H
#define LCD_H

#include <stdint.h>

/* LCD pin definitions */
#define LCD_RS 0x40   // PA6
#define LCD_E  0x80   // PA7

/* Public LCD functions */
void lcd_init(void);
void lcd_command(uint8_t cmd);
void lcd_data(uint8_t data);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_print_string(char *str);
void lcd_clear(void);

#endif
