#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

/* ---------------- Timing ---------------- */
#define DEBOUNCE_MS     10
#define HOLD_TIME_MS   600
#define BLINK_TIME_MS  200
#define BLINK_COUNT      6   // 3 blinks

/* ---------------- 7-Segment ---------------- */
#define SEG_0    0x3F
#define SEG_1    0x06
#define SEG_2    0x5B
#define SEG_3    0x4F
#define SEG_4    0x66
#define SEG_5    0x6D
#define SEG_6    0x7D
#define SEG_7    0x07
#define SEG_8    0x7F
#define SEG_9    0x6F
#define SEG_DASH 0x40

/* ---------------- LEDs ---------------- */
#define RED_LED    0x02   // PF1
#define GREEN_LED  0x08   // PF3

/* ---------------- Globals ---------------- */
volatile int key_active = 0;
volatile char key = '-';

int safe_open = 0;
int pending_toggle = 0;
int pending_lock = 0;
int invalid_blink = 0;

int hold_counter = 0;
int blink_counter = 0;
int blink_state = 0;

int pin_change_mode = 0;
int pin_index = 0;

uint8_t disp_buf[4]   = {SEG_DASH, SEG_DASH, SEG_DASH, SEG_DASH};
uint8_t stored_pin[4] = {SEG_0, SEG_0, SEG_0, SEG_0};   // default 0000

/* ---------------- Keypad Map ---------------- */
const char keymap[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

/* ---------------- Prototypes ---------------- */
void delayMs(int n);
void delay7seg(void);
void Init_Hardware(void);
int  scankey(void);
int  scankey_debounced(void);
int  key_to_seg(char k);
void update_display_buffer(char k);
void Display_sevensegment(uint8_t d, uint8_t c, uint8_t b, uint8_t a);
int  check_password(void);
void toggle_safe(void);

/* ---------------- Delays ---------------- */
void delayMs(int n)
{
    for (int i = 0; i < n; i++)
        for (int j = 0; j < 2000; j++) {}
}

void delay7seg(void)
{
    for (volatile int i = 0; i < 800; i++) {}
}

/* ---------------- Init ---------------- */
void Init_Hardware(void)
{
    SYSCTL_RCGC2_R |= 0x37;   // A B C E F
    delayMs(1);

    GPIO_PORTA_DIR_R = 0xF0; GPIO_PORTA_DEN_R = 0xF0;
    GPIO_PORTB_DIR_R = 0xFF; GPIO_PORTB_DEN_R = 0xFF;

    GPIO_PORTC_DIR_R &= ~0xF0;
    GPIO_PORTC_PUR_R |=  0xF0;
    GPIO_PORTC_DEN_R |=  0xF0;

    GPIO_PORTE_DIR_R |=  0x0F;
    GPIO_PORTE_ODR_R |=  0x0F;
    GPIO_PORTE_DEN_R |=  0x0F;
    GPIO_PORTE_DATA_R |= 0x0F;

    GPIO_PORTF_DIR_R |= (RED_LED | GREEN_LED);
    GPIO_PORTF_DEN_R |= (RED_LED | GREEN_LED);
    GPIO_PORTF_DATA_R = RED_LED;   // start locked
}

/* ---------------- Key Scan ---------------- */
int scankey(void)
{
    for (int r = 0; r < 4; r++)
    {
        GPIO_PORTE_DATA_R |= 0x0F;
        GPIO_PORTE_DATA_R &= ~(1 << r);
        delayMs(1);

        if ((GPIO_PORTC_DATA_R & 0xF0) != 0xF0)
        {
            int c;
            switch (GPIO_PORTC_DATA_R & 0xF0)
            {
                case 0xE0: c = 0; break;
                case 0xD0: c = 1; break;
                case 0xB0: c = 2; break;
                case 0x70: c = 3; break;
                default: return 0;
            }
            key = keymap[r][c];
            GPIO_PORTE_DATA_R |= 0x0F;
            return 1;
        }
    }
    return 0;
}

int scankey_debounced(void)
{
    if (scankey())
    {
        delayMs(DEBOUNCE_MS);
        if (scankey())
            return 1;
    }
    return 0;
}

/* ---------------- Helpers ---------------- */
int key_to_seg(char k)
{
    switch (k)
    {
        case '0': return SEG_0;
        case '1': return SEG_1;
        case '2': return SEG_2;
        case '3': return SEG_3;
        case '4': return SEG_4;
        case '5': return SEG_5;
        case '6': return SEG_6;
        case '7': return SEG_7;
        case '8': return SEG_8;
        case '9': return SEG_9;
        default:  return SEG_DASH;
    }
}

void update_display_buffer(char k)
{
    disp_buf[0] = disp_buf[1];
    disp_buf[1] = disp_buf[2];
    disp_buf[2] = disp_buf[3];
    disp_buf[3] = key_to_seg(k);
}

int check_password(void)
{
    for (int i = 0; i < 4; i++)
        if (disp_buf[i] != stored_pin[i])
            return 0;
    return 1;
}

void toggle_safe(void)
{
    safe_open ^= 1;
    GPIO_PORTF_DATA_R = safe_open ? GREEN_LED : RED_LED;
}

/* ---------------- Display ---------------- */
void Display_sevensegment(uint8_t d, uint8_t c, uint8_t b, uint8_t a)
{
    GPIO_PORTB_DATA_R = a; GPIO_PORTA_DATA_R = 0x10; delay7seg();
    GPIO_PORTB_DATA_R = b; GPIO_PORTA_DATA_R = 0x20; delay7seg();
    GPIO_PORTB_DATA_R = c; GPIO_PORTA_DATA_R = 0x40; delay7seg();
    GPIO_PORTB_DATA_R = d; GPIO_PORTA_DATA_R = 0x80; delay7seg();
}

/* ---------------- MAIN ---------------- */
int main(void)
{
    Init_Hardware();

    while (1)
    {
        int key_found = scankey_debounced();

        if (key_found && !key_active &&
            !pending_toggle && !pending_lock && !invalid_blink)
        {
            key_active = 1;

            /* -------- PIN CHANGE MODE -------- */
            if (pin_change_mode)
            {
                stored_pin[pin_index++] = key_to_seg(key);
                update_display_buffer(key);

                if (pin_index >= 4)
                {
                    pin_change_mode = 0;
                    pin_index = 0;
                    pending_lock = 1;
                    hold_counter = HOLD_TIME_MS;
                }
            }

            /* -------- NORMAL MODE -------- */
            else
            {
                if (safe_open)
                {
                    /* OPEN: # changes PIN, PIN closes safe */
                    if (key == '#')
                    {
                        pin_change_mode = 1;
                        pin_index = 0;
                        disp_buf[0] = disp_buf[1] =
                        disp_buf[2] = disp_buf[3] = SEG_DASH;
                    }
                    else
                    {
                        update_display_buffer(key);

                        if (disp_buf[0] != SEG_DASH &&
                            disp_buf[1] != SEG_DASH &&
                            disp_buf[2] != SEG_DASH &&
                            disp_buf[3] != SEG_DASH)
                        {
                            if (check_password())
                            {
                                pending_toggle = 1;
                                hold_counter = HOLD_TIME_MS;
                            }
                            else
                            {
                                /* ignore wrong PIN when open */
                                disp_buf[0] = disp_buf[1] =
                                disp_buf[2] = disp_buf[3] = SEG_DASH;
                            }
                        }
                    }
                }
                else
                {
                    /* CLOSED: accept PIN */
                    update_display_buffer(key);

                    if (disp_buf[0] != SEG_DASH &&
                        disp_buf[1] != SEG_DASH &&
                        disp_buf[2] != SEG_DASH &&
                        disp_buf[3] != SEG_DASH)
                    {
                        if (check_password())
                        {
                            pending_toggle = 1;
                            hold_counter = HOLD_TIME_MS;
                        }
                        else
                        {
                            invalid_blink = 1;
                            blink_counter = BLINK_COUNT * BLINK_TIME_MS;
                            blink_state = 0;
                        }
                    }
                }
            }
        }

        if (!key_found)
            key_active = 0;

        /* -------- Toggle Safe -------- */
        if (pending_toggle)
        {
            delayMs(1);
            if (--hold_counter <= 0)
            {
                toggle_safe();
                disp_buf[0] = disp_buf[1] =
                disp_buf[2] = disp_buf[3] = SEG_DASH;
                pending_toggle = 0;
            }
        }

        /* -------- Auto-lock after PIN change -------- */
        if (pending_lock)
        {
            delayMs(1);
            if (--hold_counter <= 0)
            {
                safe_open = 0;
                GPIO_PORTF_DATA_R = RED_LED;
                disp_buf[0] = disp_buf[1] =
                disp_buf[2] = disp_buf[3] = SEG_DASH;
                pending_lock = 0;
            }
        }

        /* -------- Invalid PIN Blink -------- */
        if (invalid_blink)
        {
            delayMs(1);
            if (--blink_counter <= 0)
            {
                GPIO_PORTF_DATA_R = RED_LED;
                disp_buf[0] = disp_buf[1] =
                disp_buf[2] = disp_buf[3] = SEG_DASH;
                invalid_blink = 0;
            }
            else if (blink_counter % BLINK_TIME_MS == 0)
            {
                GPIO_PORTF_DATA_R ^= RED_LED;
            }
        }

        Display_sevensegment(
            disp_buf[0],
            disp_buf[1],
            disp_buf[2],
            disp_buf[3]
        );
    }
}
