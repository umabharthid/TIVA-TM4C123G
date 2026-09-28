#include <stdint.h>
#include "inc/tm4c123gh6pm.h"
#include "keypad.h"

/* Key mapping (can be modified if needed) */
static const char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

/* Simple delay (approx ms) */
static void delayMs(int n)
{
    int i, j;
    for(i = 0; i < n; i++)
        for(j = 0; j < 3180; j++);
}

/* Initialize GPIO for keypad */
void Keypad_Init(void)
{
    /* Enable clocks for Port C and E */
    SYSCTL_RCGC2_R |= 0x14;
    delayMs(1);

    /* Columns PC4–PC7 - input with pull-up */
    GPIO_PORTC_AFSEL_R &= ~0xF0;
    GPIO_PORTC_AMSEL_R &= ~0xF0;
    GPIO_PORTC_DIR_R   &= ~0xF0;
    GPIO_PORTC_PUR_R   |=  0xF0;
    GPIO_PORTC_DEN_R   |=  0xF0;

    /* Rows PE0–PE3 - output open-drain */
    GPIO_PORTE_AFSEL_R &= ~0x0F;
    GPIO_PORTE_AMSEL_R &= ~0x0F;
    GPIO_PORTE_DIR_R   |=  0x0F;
    GPIO_PORTE_ODR_R   |=  0x0F;
    GPIO_PORTE_DEN_R   |=  0x0F;

    /* Set all rows HIGH (inactive) */
    GPIO_PORTE_DATA_R  |=  0x0F;
}

/* Non-blocking scan */
char Keypad_Scan(void)
{
    int r, c;

    for (r = 0; r < 4; r++)
    {
        /* Drive all rows HIGH */
        GPIO_PORTE_DATA_R |= 0x0F;

        /* Pull one row LOW */
        GPIO_PORTE_DATA_R &= ~(1 << r);

        delayMs(1);

        /* Check if any column is LOW (key press) */
        if ((GPIO_PORTC_DATA_R & 0xF0) != 0xF0)
        {
            /* Identify column */
            switch (GPIO_PORTC_DATA_R & 0xF0)
            {
                case 0xE0: c = 0; break;
                case 0xD0: c = 1; break;
                case 0xB0: c = 2; break;
                case 0x70: c = 3; break;
                default: return 0;
            }

            /* Debounce delay */
            delayMs(20);

            /* Wait until key is released */
            while ((GPIO_PORTC_DATA_R & 0xF0) != 0xF0);

            return keymap[r][c];
        }
    }

    return 0; // no key pressed
}

/* Blocking function */
char Keypad_GetKey(void)
{
    char key = 0;

    while(key == 0)
    {
        key = Keypad_Scan();
    }

    return key;
}
