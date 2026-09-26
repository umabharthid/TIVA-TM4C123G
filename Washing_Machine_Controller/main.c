#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "inc/tm4c123gh6pm.h"
#include "lcd.h"
#include "uart.h"

// ================= 7 SEG =================
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

uint8_t seg_map[10] = {SEG_0,SEG_1,SEG_2,SEG_3,SEG_4,SEG_5,SEG_6,SEG_7,SEG_8,SEG_9};

uint32_t tick = 0;
uint16_t tens = SEG_0, ones = SEG_0;

// ================= STATES =================
typedef enum {
    IDLE, PREWASH, WASH, SPIN, PAUSED, COMPLETE, ERROR
} state_t;

state_t state = IDLE;
state_t prev_state = 255;

// ================= ERROR =================
typedef enum {
    NO_ERROR, ERR_DOOR, ERR_DETERGENT, ERR_TIME
} error_t;

error_t current_error = NO_ERROR;

// ================= FLAGS =================
uint8_t door_closed = 0;
uint8_t detergent_loaded = 0;
uint8_t paused = 0;

// ================= TIME =================
uint32_t total_time = 0;
uint32_t remaining_time = 0;
uint32_t last_sec_tick = 0;
uint32_t last_led_toggle = 0;

// ================= LED =================
#define RED   (1<<1)
#define BLUE  (1<<2)
#define GREEN (1<<3)
#define WHITE (RED|BLUE|GREEN)

// ================= DELAY =================
void delayMs(int n){
    int i,j;
    for(i=0;i<n;i++)
        for(j=0;j<3180;j++);
}

// ================= PORT INIT =================
void port_init()
{
    SYSCTL_RCGC2_R |= 0x23;
    delayMs(1);

    GPIO_PORTA_DIR_R |= 0xF0;
    GPIO_PORTA_DEN_R |= 0xF0;

    GPIO_PORTB_DIR_R |= 0xFF;
    GPIO_PORTB_DEN_R |= 0xFF;

    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R |= (1<<0);

    GPIO_PORTF_DIR_R |= (RED|GREEN|BLUE);
    GPIO_PORTF_DEN_R |= (RED|GREEN|BLUE);

    GPIO_PORTF_DIR_R &= ~((1<<0)|(1<<4));
    GPIO_PORTF_DEN_R |= (1<<0)|(1<<4);
    GPIO_PORTF_PUR_R |= (1<<0)|(1<<4);
}

// ================= SYSTICK =================
void SysTick_Handler()
{
    tick++;

    static uint8_t digit = 0;

    GPIO_PORTA_DATA_R &= ~0xF0;

    if(digit == 0){
        GPIO_PORTB_DATA_R = ones;
        GPIO_PORTA_DATA_R |= 0x10;
    } else {
        GPIO_PORTB_DATA_R = tens;
        GPIO_PORTA_DATA_R |= 0x20;
    }

    digit ^= 1;
}

// ================= TIME =================
void update_time()
{
    if(tick - last_sec_tick >= 1000)
    {
        last_sec_tick = tick;

        if(state != PAUSED && state != IDLE && state != COMPLETE && state != ERROR)
        {
            if(remaining_time > 0)
                remaining_time--;
        }
    }
}

// ================= DISPLAY =================
void update_display()
{
    tens = seg_map[remaining_time / 10];
    ones = seg_map[remaining_time % 10];
}

// ================= LED =================
void update_led()
{
    static uint8_t led_state = 0;

    switch(state)
    {
        case IDLE:
            GPIO_PORTF_DATA_R = 0;
            break;

        case PREWASH:
            if(tick - last_led_toggle >= 500){
                last_led_toggle = tick;
                led_state ^= GREEN;
                GPIO_PORTF_DATA_R = led_state;
            }
            break;

        case WASH:
            GPIO_PORTF_DATA_R = GREEN;
            break;

        case SPIN:
            if(tick - last_led_toggle >= 200){
                last_led_toggle = tick;
                led_state ^= GREEN;
                GPIO_PORTF_DATA_R = led_state;
            }
            break;

        case COMPLETE:
            GPIO_PORTF_DATA_R = WHITE;
            break;

        case ERROR:
            if(tick - last_led_toggle >= 400){
                last_led_toggle = tick;
                led_state ^= RED;
                GPIO_PORTF_DATA_R = led_state;
            }
            break;
    }
}

// ================= SWITCH =================
void check_switch()
{
    static uint32_t last_sw = 0;

    if(tick - last_sw < 200) return;

    // SW1 → ABORT (🔥 FIXED)
    if((GPIO_PORTF_DATA_R & (1<<4)) == 0)
    {
        last_sw = tick;

        state = IDLE;
        remaining_time = 0;
        paused = 0;

        // 🔥 RESET CONDITIONS (FIX)
        door_closed = 0;
        detergent_loaded = 0;
        total_time = 0;

        prev_state = 255;
    }

    // SW2 → PAUSE / RESUME
    if((GPIO_PORTF_DATA_R & (1<<0)) == 0)
    {
        last_sw = tick;

        paused ^= 1;

        if(paused)
        {
            state = PAUSED;
        }
        else
        {
            float progress = (float)(total_time - remaining_time) / total_time;

            if(progress < 0.2)
                state = PREWASH;
            else if(progress < 0.8)
                state = WASH;
            else
                state = SPIN;
        }

        prev_state = 255;
    }
}

// ================= UART =================
void handle_uart()
{
    uart_update();
    if(!uart_command_ready()) return;

    char *cmd = uart_get_buffer();

    if(strcmp(cmd,"door_close")==0){
        door_closed = 1;
        if(state == ERROR && current_error == ERR_DOOR){
            state = IDLE;
            current_error = NO_ERROR;
        }
        prev_state = 255;
    }
    else if(strcmp(cmd,"door_open")==0){
        door_closed = 0;
        prev_state = 255;
    }
    else if(strcmp(cmd,"load_dt")==0){
        detergent_loaded = 1;
        if(state == ERROR && current_error == ERR_DETERGENT){
            state = IDLE;
            current_error = NO_ERROR;
        }
        prev_state = 255;
    }
    else if(strcmp(cmd,"time 5")==0){
        total_time = 5;
        if(state == ERROR && current_error == ERR_TIME){
            state = IDLE;
            current_error = NO_ERROR;
        }
        prev_state = 255;
    }
    else if(strcmp(cmd,"time 15")==0){
        total_time = 15;
        if(state == ERROR && current_error == ERR_TIME){
            state = IDLE;
            current_error = NO_ERROR;
        }
        prev_state = 255;
    }
    else if(strcmp(cmd,"time 30")==0){
        total_time = 30;
        if(state == ERROR && current_error == ERR_TIME){
            state = IDLE;
            current_error = NO_ERROR;
        }
        prev_state = 255;
    }
    else if(strcmp(cmd,"start")==0)
    {
        if(!door_closed){
            current_error = ERR_DOOR;
            state = ERROR;
        }
        else if(!detergent_loaded){
            current_error = ERR_DETERGENT;
            state = ERROR;
        }
        else if(total_time == 0){
            current_error = ERR_TIME;
            state = ERROR;
        }
        else{
            current_error = NO_ERROR;
            remaining_time = total_time;
            state = PREWASH;
        }
        prev_state = 255;
    }

    uart_clear_command();
}

// ================= FSM =================
void washing_fsm()
{
    static uint32_t complete_time = 0;

    if(state == PAUSED) return;

    float progress = (float)(total_time - remaining_time) / total_time;

    if(state == PREWASH && progress >= 0.2)
        state = WASH;
    else if(state == WASH && progress >= 0.8)
        state = SPIN;
    else if(state == SPIN && remaining_time == 0)
        state = COMPLETE;

    if(state == COMPLETE)
    {
        if(complete_time == 0)
            complete_time = tick;

        if(tick - complete_time >= 5000)
        {
            state = IDLE;
            door_closed = 0;
            detergent_loaded = 0;
            remaining_time = 0;
            complete_time = 0;
            prev_state = 255;
        }
    }
    else
    {
        complete_time = 0;
    }
}

// ================= LCD =================
void update_lcd()
{
    if(state == prev_state) return;

    prev_state = state;

    lcd_clear();
    lcd_set_cursor(0,0);

    switch(state)
    {
        case IDLE:

            if(!door_closed)
                lcd_print_string("Close Door   ");

            else if(!detergent_loaded)
                lcd_print_string("Load Dt      ");

            else if(total_time == 0)
                lcd_print_string("Set Time     ");

            else
                lcd_print_string("Press Start  ");

            break;

        case PREWASH: lcd_print_string("PREWASH      "); break;
        case WASH: lcd_print_string("WASH         "); break;
        case SPIN: lcd_print_string("SPIN         "); break;
        case PAUSED: lcd_print_string("PAUSED       "); break;
        case COMPLETE: lcd_print_string("DONE         "); break;

        case ERROR:
            switch(current_error)
            {
                case ERR_DOOR: lcd_print_string("DOOR OPEN    "); break;
                case ERR_DETERGENT: lcd_print_string("NO DT        "); break;
                case ERR_TIME: lcd_print_string("SET TIME     "); break;
                default: lcd_print_string("ERROR        "); break;
            }
            break;
    }
}

// ================= MAIN =================
int main(void)
{
    uart_init();
    lcd_init();
    lcd_clear();

    port_init();

    NVIC_ST_CTRL_R = 0;
    NVIC_ST_RELOAD_R = 15999;
    NVIC_ST_CURRENT_R = 0;
    NVIC_ST_CTRL_R = 0x07;

    while(1)
    {
        handle_uart();
        check_switch();
        update_time();
        washing_fsm();
        update_display();
        update_led();
        update_lcd();
    }
}
