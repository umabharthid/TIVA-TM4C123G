#ifndef SEVENSEG_H
#define SEVENSEG_H
//Remember to enable systick in the startup file
#include <stdint.h>

/* Segment definitions */
#define SEG_0 0x3F
#define SEG_1 0x06
#define SEG_2 0x5B
#define SEG_3 0x4F
#define SEG_4 0x66
#define SEG_5 0x6D
#define SEG_6 0x7D
#define SEG_7 0x07
#define SEG_8 0x7F
#define SEG_9 0x6F

void sevenseg_init(void);
void sevenseg_set_digits(uint8_t d,uint8_t c,uint8_t b,uint8_t a);
extern volatile uint32_t system_time;
#endif
