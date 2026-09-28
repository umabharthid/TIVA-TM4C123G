#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "inc/tm4c123gh6pm.h"

/* ─────────────────────────────────────────────────────────
 * ENUMS
 * ───────────────────────────────────────────────────────── */
typedef enum {
    STATE_IDLE     = 0,
    STATE_RUNNING  = 1,
    STATE_PAUSED   = 2,
    STATE_ERROR    = 3,
    STATE_FINISHED = 4
} MachineState;

typedef enum {
    PHASE_PRE  = 0,
    PHASE_WASH = 1,
    PHASE_SPIN = 2
} WashPhase;

/* ─────────────────────────────────────────────────────────
 * GLOBALS
 * ───────────────────────────────────────────────────────── */
volatile uint32_t ms_count  = 0;
volatile uint32_t ms_tick   = 0;
volatile uint32_t debounce  = 0;

volatile uint8_t displayBuffer[2] = {0x3F, 0x3F};
volatile uint8_t dispDigit = 0;
volatile uint32_t disp_tick = 0;

volatile uint8_t lcd_busy         = 0;
volatile uint8_t lcd_needs_update = 0;
volatile uint8_t lcd_error_active = 0;
char lcd_err_row0[17] = "";
char lcd_err_row1[17] = "";

MachineState machineState = STATE_IDLE;
WashPhase    washPhase    = PHASE_PRE;

int doorOpen  = 1;
int detergent = 0;
int washtimer = 0;

uint32_t totalTime_ms   = 0;
uint32_t wash_thresh_ms = 0;
uint32_t spin_thresh_ms = 0;

const uint8_t SEGMAP[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

char rx_buf[32];
int  rx_idx = 0;

const char KEYMAP[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

/* +++LAB08: debug flag — write 0x01 to its address to enable,
             0x00 to disable. Use 'peek debug' to find address. */
volatile uint8_t debug_flag = 0;

/* +++LAB08: macro — replaces bare UART_TxStr in phase messages */
#define DEBUG_PRINT(s)  do { if(debug_flag) UART_TxStr(s); } while(0)

/* +++LAB08: LCD message buffer — address defined by linker script.
             Paste into LCD row 1 via: poke str <addr> Hello!     */
extern uint8_t lcd_msg_buf[32];

/* ─────────────────────────────────────────────────────────
 * LED MASKS
 * ───────────────────────────────────────────────────────── */
#define LED_RED    0x02u
#define LED_GREEN  0x08u
#define LED_WHITE  0x0Eu
#define LED_ALL    0x0Eu
#define LED_OFF    0x00u

/* ─────────────────────────────────────────────────────────
 * FUNCTION PROTOTYPES
 * ───────────────────────────────────────────────────────── */
void Init_PWM(void);
void Init_MotorDir(void);
void Init_SysTick(void);
void Init_GPIO(void);
void Init_UART0(void);
void Init_Keypad(void);
void motor_on(int s);
void motor_off(void);
void UART_TxChar(char c);
void UART_TxStr(const char *s);
void UART_TxUInt(uint32_t n);
char UART_RxNB(void);
void update_7seg(uint32_t secs);
void RefreshDisplay(void);
void set_leds(uint8_t mask);
void print_status(void);
void LCD_Init(void);
void LCD_delayMs(int n);
void LCD_Command(unsigned char cmd);
void LCD_Data(unsigned char data);
void LCD_String(const char *str);
void LCD_Num2(uint32_t n);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_StringPad(const char *str, int width);
void LCD_Update(void);
void LCD_ShowLinkerMsg(void);          /* +++LAB08 */
void cmd_start(void);
void cmd_abort(void);
void cmd_pause(void);
void cmd_resume(void);
void cmd_door_open(void);
void cmd_door_close(void);
void cmd_load_dt(void);
void cmd_set_timer(int t);
void update_running(void);
char GetKey(void);
void handle_key(char k);
void process_uart(void);
/* +++LAB08 helpers */
void UART_TxHexByte(uint8_t b);
void UART_TxHexWord(uint32_t w);
uint32_t parse_hex(const char *s);
void cmd_peek(uint32_t addr, uint32_t n);
void cmd_poke_hex(uint32_t addr, const char *hex_str);
void cmd_poke_str(uint32_t addr, const char *str);

/* ─────────────────────────────────────────────────────────
 * MAIN
 * ───────────────────────────────────────────────────────── */
int main(void) {
    Init_PWM();
    Init_MotorDir();
    Init_SysTick();
    Init_GPIO();
    Init_UART0();
    Init_Keypad();
    motor_off();
    LCD_Init();
    LCD_Update();

    __asm(" CPSIE I");

    UART_TxStr("\r\n===== Washing Machine Controller =====\r\n");
    UART_TxStr("UART cmds : set timer XX | load dt | door open | door close\r\n");
    UART_TxStr("            start | pause | resume | abort | status | reset\r\n");
    UART_TxStr("Keypad    : 1=time  2=door  3=detergent  D=start  *=abort  B=pause/resume\r\n");
    UART_TxStr("Switches  : SW1=abort  SW2=pause/resume\r\n");
    /* +++LAB08: extra banner */
    UART_TxStr("Debug cmds : peek <addr> <n> | poke hex <addr> <hexdata>\r\n");
    UART_TxStr("             poke str <addr> <text> | sim sw1|sw2|run <xx>|phase <0-2>\r\n");
    UART_TxStr("             debug on|off | lcd msg <text> | addrs\r\n\r\n");

    while(1) {
        char c = UART_RxNB();
        if(c) {
            if(c == '\r' || c == '\n') {
                if(rx_idx > 0) process_uart();
            } else if(rx_idx < 31) {
                rx_buf[rx_idx++] = c;
                UART_TxChar(c);
            }
        }

        char key = GetKey();
        if(key) handle_key(key);

        if(machineState == STATE_RUNNING) update_running();

        if(lcd_needs_update) {
            lcd_needs_update = 0;
            LCD_Update();
        }

        /* +++LAB08: show any string poked into lcd_msg_buf */
        LCD_ShowLinkerMsg();

        if(machineState == STATE_RUNNING) {
            if((ms_tick / 500) & 1) GPIO_PORTF_DATA_R |=  LED_GREEN;
            else                    GPIO_PORTF_DATA_R &= ~LED_GREEN;
        }
    }
}

/* ─────────────────────────────────────────────────────────
 * INIT FUNCTIONS  (unchanged from lab07)
 * ───────────────────────────────────────────────────────── */
void Init_PWM(void) {
    SYSCTL_RCGCPWM_R  |= 0x01u;
    SYSCTL_RCGCGPIO_R |= 0x10u;
    SYSCTL_RCC_R      &= ~0x00100000u;
    while(!(SYSCTL_PRGPIO_R & 0x10u));
    GPIO_PORTE_AFSEL_R |= 0x20u;
    GPIO_PORTE_PCTL_R   = (GPIO_PORTE_PCTL_R & ~0x00F00000u) | 0x00400000u;
    GPIO_PORTE_DEN_R   |= 0x20u;
    PWM0_2_CTL_R  = 0;
    PWM0_2_GENB_R = 0x0000080Cu;
    PWM0_2_LOAD_R = 16000u - 1u;
    PWM0_2_CMPB_R = 16000u;
    PWM0_2_CTL_R  = 1u;
    PWM0_ENABLE_R |= 0x20u;
}

void Init_MotorDir(void) {
    SYSCTL_RCGCGPIO_R |= 0x01u;
    while(!(SYSCTL_PRGPIO_R & 0x01u));
    GPIO_PORTA_DIR_R |= 0x0Cu;
    GPIO_PORTA_DEN_R |= 0x0Cu;
    GPIO_PORTA_DATA_R = 0x04u;
}

void Init_SysTick(void) {
    NVIC_ST_CTRL_R    = 0;
    NVIC_ST_RELOAD_R  = 16000u - 1u;
    NVIC_ST_CURRENT_R = 0;
    NVIC_ST_CTRL_R    = 0x07u;
}

void Init_GPIO(void) {
    SYSCTL_RCGCGPIO_R |= 0x23u;
    while(!(SYSCTL_PRGPIO_R & 0x23u));
    GPIO_PORTF_LOCK_R  = 0x4C4F434Bu;
    GPIO_PORTF_CR_R   |= 0x1Fu;
    GPIO_PORTF_DIR_R   = (GPIO_PORTF_DIR_R & ~0x11u) | 0x0Eu;
    GPIO_PORTF_DEN_R  |= 0x1Fu;
    GPIO_PORTF_PUR_R  |= 0x11u;
    GPIO_PORTF_DATA_R &= ~LED_ALL;
    GPIO_PORTF_IM_R   &= ~0x11u;
    GPIO_PORTF_IS_R   &= ~0x11u;
    GPIO_PORTF_IBE_R  &= ~0x11u;
    GPIO_PORTF_IEV_R  &= ~0x11u;
    GPIO_PORTF_ICR_R  |=  0x11u;
    GPIO_PORTF_IM_R   |=  0x11u;
    NVIC_EN0_R |= (1u << 30);
    SYSCTL_RCGCGPIO_R |= 0x02u;
    while(!(SYSCTL_PRGPIO_R & 0x02u));
    GPIO_PORTB_DIR_R  =  0xFFu;
    GPIO_PORTB_DEN_R  =  0xFFu;
    GPIO_PORTB_DATA_R =  0x00u;
    GPIO_PORTA_DIR_R  |=  0x30u;
    GPIO_PORTA_DEN_R  |=  0x30u;
    GPIO_PORTA_DATA_R &= ~0x30u;
    GPIO_PORTA_DIR_R  |= 0xC0u;
    GPIO_PORTA_DEN_R  |= 0xC0u;
    GPIO_PORTA_DATA_R &= ~0xC0u;
}

void Init_UART0(void) {
    SYSCTL_RCGCUART_R |= 0x01u;
    SYSCTL_RCGCGPIO_R |= 0x01u;
    while(!(SYSCTL_PRUART_R & 0x01u));
    GPIO_PORTA_AFSEL_R |= 0x03u;
    GPIO_PORTA_PCTL_R   = (GPIO_PORTA_PCTL_R & ~0xFFu) | 0x11u;
    GPIO_PORTA_DEN_R   |= 0x03u;
    UART0_CTL_R  &= ~0x01u;
    UART0_IBRD_R  = 8u;
    UART0_FBRD_R  = 44u;
    UART0_LCRH_R  = 0x70u;
    UART0_CTL_R  |= 0x301u;
}

void Init_Keypad(void) {
    SYSCTL_RCGCGPIO_R |= 0x14u;
    while((SYSCTL_PRGPIO_R & 0x14u) != 0x14u);
    GPIO_PORTE_DIR_R |= 0x0Fu;
    GPIO_PORTE_DEN_R |= 0x0Fu;
    GPIO_PORTE_ODR_R |= 0x0Fu;
    GPIO_PORTC_DIR_R &= ~0xF0u;
    GPIO_PORTC_DEN_R |= 0xF0u;
    GPIO_PORTC_PUR_R |= 0xF0u;
}

/* ─────────────────────────────────────────────────────────
 * SYSTICK HANDLER  (unchanged)
 * ───────────────────────────────────────────────────────── */
void SysTick_Handler(void) {
    ms_tick++;
    if(machineState == STATE_RUNNING && ms_count > 0) ms_count--;
    if(debounce > 0) debounce--;
    disp_tick++;
    if(disp_tick >= 5) {
        disp_tick = 0;
        RefreshDisplay();
    }
}

/* ─────────────────────────────────────────────────────────
 * MOTOR  (unchanged)
 * ───────────────────────────────────────────────────────── */
void motor_off(void) {
    PWM0_ENABLE_R     &= ~0x20u;
    GPIO_PORTA_DATA_R &= ~0x0Cu;
}

void motor_on(int s) {
    switch(s) {
        case 0:  PWM0_2_CMPB_R = 12000u; break;
        case 1:  PWM0_2_CMPB_R =  8000u; break;
        case 2:  PWM0_2_CMPB_R =  2000u; break;
        default: motor_off(); return;
    }
    GPIO_PORTA_DATA_R = (GPIO_PORTA_DATA_R & ~0x0Cu) | 0x04u;
    PWM0_ENABLE_R     |= 0x20u;
}

/* ─────────────────────────────────────────────────────────
 * UART HELPERS  (unchanged)
 * ───────────────────────────────────────────────────────── */
void UART_TxChar(char c)       { while(UART0_FR_R & 0x20u); UART0_DR_R = (uint32_t)c; }
void UART_TxStr(const char *s) { while(*s) UART_TxChar(*s++); }
char UART_RxNB(void)           { if(UART0_FR_R & 0x10u) return 0; return (char)UART0_DR_R; }

void UART_TxUInt(uint32_t n) {
    char buf[12]; int i = 11;
    buf[i] = '\0';
    if(n == 0) { UART_TxChar('0'); return; }
    while(n && i > 0) { buf[--i] = (char)('0' + n % 10); n /= 10; }
    UART_TxStr(&buf[i]);
}

/* +++LAB08: hex printing helpers */
void UART_TxHexByte(uint8_t b) {
    const char hex[] = "0123456789ABCDEF";
    UART_TxChar(hex[b >> 4]);
    UART_TxChar(hex[b & 0x0F]);
}

void UART_TxHexWord(uint32_t w) {
    int i;
    for(i = 28; i >= 0; i -= 4)
        UART_TxChar("0123456789ABCDEF"[(w >> i) & 0xF]);
}

/* ─────────────────────────────────────────────────────────
 * DISPLAY HELPERS  (unchanged)
 * ───────────────────────────────────────────────────────── */
void set_leds(uint8_t mask) {
    GPIO_PORTF_DATA_R = (GPIO_PORTF_DATA_R & ~LED_ALL) | (mask & LED_ALL);
}

void update_7seg(uint32_t secs) {
    if(secs > 99) secs = 99;
    displayBuffer[0] = SEGMAP[secs / 10];
    displayBuffer[1] = SEGMAP[secs % 10];
}

void RefreshDisplay(void) {
    if(lcd_busy) return;
    GPIO_PORTA_DATA_R &= ~0x30u;
    GPIO_PORTB_DATA_R  = displayBuffer[dispDigit];
    if(dispDigit == 0)  GPIO_PORTA_DATA_R |= 0x20u;
    else                GPIO_PORTA_DATA_R |= 0x10u;
    dispDigit ^= 1;
}

void print_status(void) {
    UART_TxStr("[STATUS] ");
    const char *sname[] = {"IDLE","RUNNING","PAUSED","ERROR","FINISHED"};
    const char *pname[] = {"PRE_WASH","WASH","SPIN"};
    UART_TxStr(sname[machineState]);
    UART_TxStr(" | Phase:");
    UART_TxStr(pname[washPhase]);
    UART_TxStr(" | Remain:");
    UART_TxUInt(ms_count / 1000);
    UART_TxStr("s | Door:");
    UART_TxStr(doorOpen ? "OPEN" : "CLOSED");
    UART_TxStr(" | Det:");
    UART_TxStr(detergent ? "YES" : "NO");
    UART_TxStr(" | Timer:");
    UART_TxUInt((uint32_t)washtimer);
    UART_TxStr("s\r\n");
}

/* ─────────────────────────────────────────────────────────
 * LCD HELPERS  (unchanged except DEBUG_PRINT in LCD_Update)
 * ───────────────────────────────────────────────────────── */
void LCD_delayMs(int n) {
    volatile int i, j;
    for(i = 0; i < n; i++)
        for(j = 0; j < 3180; j++);
}

void LCD_Command(unsigned char cmd) {
    GPIO_PORTA_DATA_R &= ~0x40u;
    GPIO_PORTB_DATA_R  = cmd;
    GPIO_PORTA_DATA_R |=  0x80u;
    LCD_delayMs(1);
    GPIO_PORTA_DATA_R &= ~0x80u;
    LCD_delayMs(2);
}

void LCD_Data(unsigned char data) {
    GPIO_PORTA_DATA_R |=  0x40u;
    GPIO_PORTB_DATA_R  = data;
    GPIO_PORTA_DATA_R |=  0x80u;
    LCD_delayMs(1);
    GPIO_PORTA_DATA_R &= ~0x80u;
    LCD_delayMs(1);
}

void LCD_String(const char *str)      { while(*str) LCD_Data((unsigned char)*str++); }
void LCD_Num2(uint32_t n)             { if(n>99)n=99; LCD_Data('0'+n/10); LCD_Data('0'+n%10); }
void LCD_SetCursor(uint8_t row, uint8_t col) {
    LCD_Command((row == 0 ? 0x80u : 0xC0u) + col);
}
void LCD_StringPad(const char *str, int width) {
    int i = 0;
    while(*str && i < width) { LCD_Data((unsigned char)*str++); i++; }
    while(i < width)          { LCD_Data(' '); i++; }
}

void set_lcd_error(const char *row0, const char *row1) {
    int i;
    for(i=0; i<16 && row0[i]; i++) lcd_err_row0[i]=row0[i];
    while(i<16) lcd_err_row0[i++]=' ';
    lcd_err_row0[16]='\0';
    for(i=0; i<16 && row1[i]; i++) lcd_err_row1[i]=row1[i];
    while(i<16) lcd_err_row1[i++]=' ';
    lcd_err_row1[16]='\0';
    lcd_error_active = 1;
    lcd_needs_update = 1;
}

void clear_lcd_error(void) {
    lcd_error_active = 0;
    lcd_err_row0[0]  = '\0';
    lcd_err_row1[0]  = '\0';
}

void LCD_Init(void) {
    LCD_delayMs(20);
    LCD_Command(0x38);
    LCD_Command(0x0Cu);
    LCD_Command(0x06u);
    LCD_Command(0x01u);
    LCD_delayMs(2);
}

void LCD_Update(void) {
    lcd_busy = 1;
    GPIO_PORTA_DATA_R &= ~0x30u;

    const char *phases[] = {"PRE_WASH", "WASH    ", "SPIN    "};

    if(lcd_error_active) {
        LCD_SetCursor(0, 0); LCD_StringPad(lcd_err_row0, 16);
        LCD_SetCursor(1, 0); LCD_StringPad(lcd_err_row1, 16);
        lcd_busy = 0;
        return;
    }

    switch(machineState) {
        case STATE_IDLE:
        case STATE_FINISHED:
            LCD_SetCursor(0, 0);
            LCD_String("Door:");
            LCD_Data(doorOpen ? 'O' : 'C');
            LCD_String(" Det:");
            LCD_Data(detergent ? 'Y' : 'N');
            LCD_StringPad("", 5);
            LCD_SetCursor(1, 0);
            LCD_String("Timer: ");
            LCD_Num2((uint32_t)washtimer);
            LCD_StringPad(machineState==STATE_FINISHED?" DONE   ":"        ", 8);
            break;

        case STATE_RUNNING:
            LCD_SetCursor(0, 0);
            LCD_String("RUN ");
            LCD_Num2((uint32_t)washtimer);
            LCD_String("s ");
            LCD_StringPad(phases[washPhase], 8);
            LCD_SetCursor(1, 0);
            LCD_String("Left: ");
            LCD_Num2(ms_count / 1000);
            LCD_StringPad("s      ", 9);
            break;

        case STATE_PAUSED:
            LCD_SetCursor(0, 0);
            LCD_String("PAUSED ");
            LCD_Num2((uint32_t)washtimer);
            LCD_String("s ");
            LCD_StringPad(phases[washPhase], 6);
            LCD_SetCursor(1, 0);
            LCD_String("Left: ");
            LCD_Num2(ms_count / 1000);
            LCD_StringPad("s      ", 9);
            break;

        case STATE_ERROR:
            LCD_SetCursor(0, 0); LCD_StringPad("ERROR E3:       ", 16);
            LCD_SetCursor(1, 0); LCD_StringPad("Door opened!    ", 16);
            break;
    }

    lcd_busy = 0;
}

/* +++LAB08: write lcd_msg_buf to LCD row 1 when non-empty.
   Called every main loop iteration — very fast when buf is empty. */
void LCD_ShowLinkerMsg(void) {
    if(lcd_msg_buf[0] == 0) return;

    lcd_busy = 1;
    GPIO_PORTA_DATA_R &= ~0x30u;

    LCD_SetCursor(1, 0);
    int i;
    for(i = 0; i < 16 && lcd_msg_buf[i]; i++)
        LCD_Data(lcd_msg_buf[i]);
    while(i++ < 16) LCD_Data(' ');

    for(i = 0; i < 32; i++) lcd_msg_buf[i] = 0;

    lcd_busy = 0;
}

/* ─────────────────────────────────────────────────────────
 * +++LAB08: PEEK / POKE CORE
 * ───────────────────────────────────────────────────────── */
uint32_t parse_hex(const char *s) {
    uint32_t val = 0;
    while(*s) {
        char c = *s++;
        uint8_t nib;
        if     (c>='0'&&c<='9') nib=(uint8_t)(c-'0');
        else if(c>='a'&&c<='f') nib=(uint8_t)(c-'a'+10);
        else if(c>='A'&&c<='F') nib=(uint8_t)(c-'A'+10);
        else break;
        val = (val << 4) | nib;
    }
    return val;
}

void cmd_peek(uint32_t addr, uint32_t n) {
    if(n == 0 || n > 256) { UART_TxStr("[PEEK] count 1-256\r\n"); return; }

    UART_TxStr("[PEEK] 0x"); UART_TxHexWord(addr);
    UART_TxStr(" len="); UART_TxUInt(n); UART_TxStr("\r\n");

    volatile uint8_t *ptr = (volatile uint8_t *)addr;
    uint32_t row, col;
    for(row = 0; row < n; row += 16) {
        UART_TxStr("  0x"); UART_TxHexWord(addr + row); UART_TxStr("  ");
        for(col = 0; col < 16; col++) {
            if(row+col < n) { UART_TxHexByte(ptr[row+col]); UART_TxChar(' '); }
            else              UART_TxStr("   ");
            if(col == 7) UART_TxChar(' ');
        }
        UART_TxStr(" |");
        for(col = 0; col < 16 && (row+col) < n; col++) {
            uint8_t b = ptr[row+col];
            UART_TxChar((b>=0x20 && b<0x7F)?(char)b:'.');
        }
        UART_TxStr("|\r\n");
    }
}

void cmd_poke_hex(uint32_t addr, const char *hex_str) {
    volatile uint8_t *ptr = (volatile uint8_t *)addr;
    uint32_t count = 0;
    const char *p = hex_str;
    while(p[0] && p[1]) {
        uint8_t hi, lo; char c;
        c=p[0];
        if     (c>='0'&&c<='9') hi=(uint8_t)(c-'0');
        else if(c>='a'&&c<='f') hi=(uint8_t)(c-'a'+10);
        else if(c>='A'&&c<='F') hi=(uint8_t)(c-'A'+10);
        else break;
        c=p[1];
        if     (c>='0'&&c<='9') lo=(uint8_t)(c-'0');
        else if(c>='a'&&c<='f') lo=(uint8_t)(c-'a'+10);
        else if(c>='A'&&c<='F') lo=(uint8_t)(c-'A'+10);
        else break;
        ptr[count++] = (uint8_t)((hi<<4)|lo);
        p += 2;
        if(*p == ' ') p++;
    }
    UART_TxStr("[POKE] wrote "); UART_TxUInt(count);
    UART_TxStr(" byte(s) to 0x"); UART_TxHexWord(addr); UART_TxStr("\r\n");
}

void cmd_poke_str(uint32_t addr, const char *str) {
    volatile uint8_t *ptr = (volatile uint8_t *)addr;
    uint32_t count = 0;
    while(*str && count < 255) ptr[count++] = (uint8_t)(*str++);
    ptr[count] = 0;
    UART_TxStr("[POKE] wrote str ("); UART_TxUInt(count);
    UART_TxStr(" chars) to 0x"); UART_TxHexWord(addr); UART_TxStr("\r\n");
}

/* ─────────────────────────────────────────────────────────
 * STATE MACHINE COMMANDS  (unchanged from lab07)
 * ───────────────────────────────────────────────────────── */
void cmd_start(void) {
    if(machineState == STATE_RUNNING || machineState == STATE_PAUSED) {
        UART_TxStr("[WARN] Already running or paused. Abort first.\r\n"); return;
    }
    if(machineState == STATE_ERROR) {
        UART_TxStr("[WARN] ERROR state. Abort to reset first.\r\n"); return;
    }
    int err = 0;
    if(washtimer == 0) {
        UART_TxStr("[ERROR E2] No timer set!\r\n");
        set_leds(LED_RED); set_lcd_error("ERROR E2:", "Set timer first!"); err=1;
    }
    if(doorOpen) {
        UART_TxStr("[ERROR E0] Door is OPEN!\r\n");
        set_leds(LED_RED); set_lcd_error("ERROR E0:", "Close door first"); err=1;
    }
    if(!detergent) {
        UART_TxStr("[ERROR E1] No detergent!\r\n");
        set_leds(LED_RED); set_lcd_error("ERROR E1:", "Load detergent! "); err=1;
    }
    if(err) return;

    clear_lcd_error();
    totalTime_ms   = (uint32_t)washtimer * 1000UL;
    wash_thresh_ms = totalTime_ms * 80UL / 100UL;
    spin_thresh_ms = totalTime_ms * 20UL / 100UL;
    ms_count       = totalTime_ms;
    washPhase      = PHASE_PRE;
    machineState   = STATE_RUNNING;
    motor_on(0);
    set_leds(LED_OFF);
    UART_TxStr("[INFO] Wash cycle STARTED. Phase: PRE_WASH\r\n");
    print_status();
    lcd_needs_update = 1;
}

void cmd_abort(void) {
    if(machineState == STATE_IDLE || machineState == STATE_FINISHED) {
        UART_TxStr("[WARN] Nothing to abort.\r\n"); return;
    }
    motor_off();
    displayBuffer[0] = 0x3F; displayBuffer[1] = 0x3F;
    ms_count     = 0;
    machineState = STATE_IDLE;
    washPhase    = PHASE_PRE;
    set_leds(LED_OFF);
    clear_lcd_error();
    UART_TxStr("[INFO] Wash cycle ABORTED. Machine reset to IDLE.\r\n");
    lcd_needs_update = 1;
}

void cmd_pause(void) {
    if(machineState != STATE_RUNNING) {
        UART_TxStr("[WARN] Machine is not running.\r\n"); return;
    }
    motor_off();
    machineState = STATE_PAUSED;
    set_leds(LED_OFF);
    UART_TxStr("[INFO] Wash cycle PAUSED. Time remaining: ");
    UART_TxUInt(ms_count / 1000); UART_TxStr("s\r\n");
    lcd_needs_update = 1;
}

void cmd_resume(void) {
    if(machineState != STATE_PAUSED) {
        UART_TxStr("[WARN] Machine is not paused.\r\n"); return;
    }
    machineState = STATE_RUNNING;
    switch(washPhase) {
        case PHASE_PRE:  motor_on(0); break;
        case PHASE_WASH: motor_on(1); break;
        case PHASE_SPIN: motor_on(2); break;
    }
    UART_TxStr("[INFO] Wash cycle RESUMED.\r\n");
    print_status();
    lcd_needs_update = 1;
}

void cmd_door_open(void) {
    if(machineState == STATE_RUNNING || machineState == STATE_PAUSED) {
        motor_off();
        machineState = STATE_ERROR;
        set_leds(LED_RED);
        set_lcd_error("ERROR E3:", "Door opened!    ");
        UART_TxStr("[ERROR E3] Door opened during cycle! EMERGENCY STOP.\r\n");
        UART_TxStr("           Use 'abort' to reset, then close door and restart.\r\n");
        return;
    }
    doorOpen = 1;
    clear_lcd_error();
    UART_TxStr("[INFO] Door OPENED.\r\n");
    lcd_needs_update = 1;
}

void cmd_door_close(void) {
    doorOpen = 0;
    UART_TxStr("[INFO] Door CLOSED.\r\n");
    lcd_needs_update = 1;
}

void cmd_load_dt(void) {
    detergent = 1;
    UART_TxStr("[INFO] Detergent LOADED.\r\n");
    lcd_needs_update = 1;
}

void cmd_set_timer(int t) {
    if(machineState != STATE_IDLE) {
        UART_TxStr("[WARN] Cannot change timer unless IDLE.\r\n");
        set_lcd_error("WARN:", "Machine not IDLE"); return;
    }
    if(t != 5 && t != 15 && t != 30) {
        UART_TxStr("[WARN] Valid times: 5, 15, 30 seconds only.\r\n"); return;
    }
    washtimer = t;
    update_7seg((uint32_t)t);
    UART_TxStr("[INFO] Timer set to "); UART_TxUInt((uint32_t)t); UART_TxStr(" seconds.\r\n");
    lcd_needs_update = 1;
}

/* ─────────────────────────────────────────────────────────
 * RUNNING UPDATE  (DEBUG_PRINT replaces bare UART_TxStr for
 * phase change messages — only prints when debug_flag=1)
 * ───────────────────────────────────────────────────────── */
void update_running(void) {
    static uint32_t last_sec = 0xFFFFFFFFu;
    uint32_t cur = ms_count;

    if(washPhase == PHASE_PRE && cur <= wash_thresh_ms) {
        washPhase = PHASE_WASH;
        motor_on(1);
        DEBUG_PRINT("[INFO] Phase: WASH (medium speed)\r\n"); /* +++LAB08 */
        print_status();
        lcd_needs_update = 1;
    } else if(washPhase == PHASE_WASH && cur <= spin_thresh_ms) {
        washPhase = PHASE_SPIN;
        motor_on(2);
        DEBUG_PRINT("[INFO] Phase: SPIN (high speed)\r\n");   /* +++LAB08 */
        print_status();
        lcd_needs_update = 1;
    }

    if(cur == 0) {
        motor_off();
        displayBuffer[0] = 0x3F; displayBuffer[1] = 0x3F;
        detergent    = 0;
        machineState = STATE_FINISHED;
        washPhase    = PHASE_PRE;
        set_leds(LED_WHITE);
        lcd_needs_update = 1;
        UART_TxStr("[INFO] *** Wash cycle COMPLETE! *** White LED on.\r\n");
        UART_TxStr("           Press D (keypad) or send 'reset' to go back to IDLE.\r\n");
        return;
    }

    uint32_t sec = cur / 1000;
    if(sec != last_sec) {
        last_sec = sec;
        update_7seg(sec);
        lcd_needs_update = 1;
    }
}

/* ─────────────────────────────────────────────────────────
 * KEYPAD  (unchanged)
 * ───────────────────────────────────────────────────────── */
char GetKey(void) {
    if(debounce > 0) return 0;
    for(int r = 0; r < 4; r++) {
        GPIO_PORTE_DATA_R |=  0x0Fu;
        GPIO_PORTE_DATA_R &= ~(1u << r);
        for(volatile int d = 0; d < 100; d++);
        for(int c = 0; c < 4; c++) {
            if(!(GPIO_PORTC_DATA_R & (1u << (c+4)))) {
                debounce = 200u;
                while(!(GPIO_PORTC_DATA_R & (1u << (c+4))));
                return KEYMAP[r][c];
            }
        }
    }
    return 0;
}

void handle_key(char k) {
    UART_TxStr("[KEY] "); UART_TxChar(k); UART_TxStr("\r\n");

    switch(k) {
        case '1':
            if(machineState != STATE_IDLE) {
                UART_TxStr("[WARN] Cannot change timer unless IDLE.\r\n");
                set_lcd_error("WARN:", "Machine not IDLE"); break;
            }
            if     (washtimer==0)  washtimer=5;
            else if(washtimer==5)  washtimer=15;
            else if(washtimer==15) washtimer=30;
            else                   washtimer=0;
            update_7seg((uint32_t)washtimer);
            clear_lcd_error();
            lcd_needs_update = 1;
            UART_TxStr("[KEY] Timer → ");
            if(washtimer==0) UART_TxStr("CLEARED\r\n");
            else { UART_TxUInt((uint32_t)washtimer); UART_TxStr("s\r\n"); }
            break;

        case '2':
            if(doorOpen) cmd_door_close(); else cmd_door_open();
            break;

        case '3': cmd_load_dt();  break;
        case '*': cmd_abort();    break;

        case 'D':
            if(machineState == STATE_IDLE)
                cmd_start();
            else if(machineState == STATE_FINISHED) {
                machineState = STATE_IDLE;
                set_leds(LED_OFF);
                UART_TxStr("[INFO] Reset to IDLE. Ready for next wash.\r\n");
                lcd_needs_update = 1;
            } else {
                UART_TxStr("[WARN] Use * to abort current cycle first.\r\n");
            }
            break;

        case 'B':
            if(machineState == STATE_RUNNING)     cmd_pause();
            else if(machineState == STATE_PAUSED) cmd_resume();
            break;

        default: break;
    }
}

/* ─────────────────────────────────────────────────────────
 * UART COMMAND PROCESSOR
 * Lab07 commands first, then +++LAB08 block in the final else
 * ───────────────────────────────────────────────────────── */
void process_uart(void) {
    rx_buf[rx_idx] = '\0';
    rx_idx = 0;
    UART_TxStr("\r\n");

    /* ── Lab07 commands (unchanged) ── */
    if     (strcmp(rx_buf,"start")      ==0) cmd_start();
    else if(strcmp(rx_buf,"abort")      ==0) cmd_abort();
    else if(strcmp(rx_buf,"pause")      ==0) cmd_pause();
    else if(strcmp(rx_buf,"resume")     ==0) cmd_resume();
    else if(strcmp(rx_buf,"door open")  ==0) cmd_door_open();
    else if(strcmp(rx_buf,"door close") ==0) cmd_door_close();
    else if(strcmp(rx_buf,"load dt")    ==0) cmd_load_dt();
    else if(strcmp(rx_buf,"status")     ==0) print_status();
    else if(strncmp(rx_buf,"set timer ",10)==0) cmd_set_timer(atoi(&rx_buf[10]));
    else if(strcmp(rx_buf,"reset")==0) {
        cmd_abort();
        detergent=0; doorOpen=1; washtimer=0;
        update_7seg(0);
        set_leds(LED_OFF);
        UART_TxStr("[INFO] Full reset complete.\r\n");
        lcd_needs_update = 1;
    }

    /* ── +++LAB08 commands ── */

    /* peek <hex_addr> <n>  |  peek debug|state|lcd */
    else if(strncmp(rx_buf,"peek ",5)==0) {
        char *p = rx_buf + 5;

        if(strcmp(p,"debug")==0) {
            UART_TxStr("[INFO] &debug_flag = 0x"); UART_TxHexWord((uint32_t)&debug_flag);
            UART_TxStr("  value = 0x"); UART_TxHexByte(debug_flag); UART_TxStr("\r\n");
        }
        else if(strcmp(p,"state")==0) {
            const char *sn[]={"IDLE","RUNNING","PAUSED","ERROR","FINISHED"};
            const char *pn[]={"PRE_WASH","WASH","SPIN"};
            UART_TxStr("[INFO] state=");    UART_TxStr(sn[machineState]);
            UART_TxStr(" phase=");          UART_TxStr(pn[washPhase]);
            UART_TxStr(" ms_count=");       UART_TxUInt(ms_count);
            UART_TxStr(" debug_flag=0x");   UART_TxHexByte(debug_flag);
            UART_TxStr("\r\n");
            UART_TxStr("  &machineState=0x"); UART_TxHexWord((uint32_t)&machineState);
            UART_TxStr("  &washPhase=0x");    UART_TxHexWord((uint32_t)&washPhase);
            UART_TxStr("  &ms_count=0x");     UART_TxHexWord((uint32_t)&ms_count);
            UART_TxStr("\r\n");
        }
        else if(strcmp(p,"lcd")==0) {
            UART_TxStr("[INFO] &lcd_msg_buf = 0x");
            UART_TxHexWord((uint32_t)lcd_msg_buf); UART_TxStr("\r\n");
            cmd_peek((uint32_t)lcd_msg_buf, 32);
        }
        else {
            uint32_t addr = parse_hex(p);
            while(*p && *p!=' ') p++;
            while(*p==' ') p++;
            uint32_t n = (uint32_t)atoi(p);
            if(n==0) n=16;
            cmd_peek(addr, n);
        }
    }

    /* poke hex <hex_addr> <hexdata> */
    else if(strncmp(rx_buf,"poke hex ",9)==0) {
        char *p = rx_buf + 9;
        uint32_t addr = parse_hex(p);
        while(*p && *p!=' ') p++;
        while(*p==' ') p++;
        cmd_poke_hex(addr, p);
        lcd_needs_update = 1;   /* state var might have changed */
    }

    /* poke str <hex_addr> <ascii text> */
    else if(strncmp(rx_buf,"poke str ",9)==0) {
        char *p = rx_buf + 9;
        uint32_t addr = parse_hex(p);
        while(*p && *p!=' ') p++;
        while(*p==' ') p++;
        cmd_poke_str(addr, p);
        if(addr == (uint32_t)lcd_msg_buf) lcd_needs_update = 1;
    }

    /* sim sw1 | sim sw2 | sim phase <0|1|2> */
    else if(strncmp(rx_buf,"sim ",4)==0) {
        char *p = rx_buf + 4;
        if(strcmp(p,"sw1")==0) {
            UART_TxStr("[SIM] SW1 (abort)\r\n");
            if(machineState==STATE_RUNNING||machineState==STATE_PAUSED||machineState==STATE_ERROR)
                cmd_abort();
            else UART_TxStr("[SIM] Nothing to abort\r\n");
        }
        else if(strcmp(p,"sw2")==0) {
            UART_TxStr("[SIM] SW2 (pause/resume)\r\n");
            if(machineState==STATE_RUNNING)     cmd_pause();
            else if(machineState==STATE_PAUSED) cmd_resume();
            else UART_TxStr("[SIM] Not running or paused\r\n");
        }
        else if(strncmp(p,"phase ",6)==0) {
            if(machineState!=STATE_RUNNING) {
                UART_TxStr("[SIM] phase: machine not RUNNING\r\n");
            } else {
                int ph = atoi(p+6);
                if(ph<0||ph>2) { UART_TxStr("[SIM] phase 0=PRE 1=WASH 2=SPIN\r\n"); }
                else {
                    washPhase = (WashPhase)ph;
                    switch(washPhase){case PHASE_PRE:motor_on(0);break;case PHASE_WASH:motor_on(1);break;case PHASE_SPIN:motor_on(2);break;}
                    const char *pn[]={"PRE_WASH","WASH","SPIN"};
                    UART_TxStr("[SIM] Forced phase → "); UART_TxStr(pn[washPhase]); UART_TxStr("\r\n");
                    lcd_needs_update = 1;
                }
            }
        }
        else if(strncmp(p, "run ", 4) == 0) {
            int t = atoi(p + 4);
            if(t <= 0 || t > 99) {
                UART_TxStr("[SIM] run: seconds must be 1-99\r\n");
            } else if(machineState == STATE_RUNNING || machineState == STATE_PAUSED) {
                UART_TxStr("[SIM] run: abort current cycle first\r\n");
            } else {
                /* Set ALL timing variables exactly as cmd_start() does */
                washtimer      = t;
                totalTime_ms   = (uint32_t)t * 1000UL;
                wash_thresh_ms = totalTime_ms * 80UL / 100UL;
                spin_thresh_ms = totalTime_ms * 20UL / 100UL;
                ms_count       = totalTime_ms;
                washPhase      = PHASE_PRE;
                machineState   = STATE_RUNNING;
                motor_on(0);
                set_leds(LED_OFF);
                update_7seg((uint32_t)t);
                UART_TxStr("[SIM] Machine RUNNING with ");
                UART_TxUInt((uint32_t)t);
                UART_TxStr("s timer. Now sim sw1/sw2/phase work.\r\n");
                lcd_needs_update = 1;
            }
        }
        else UART_TxStr("[WARN] sim: sw1 | sw2 | run <xx> | phase <0|1|2>\r\n");
    }

    /* debug on | debug off */
    else if(strcmp(rx_buf,"debug on")==0)  { debug_flag=1; UART_TxStr("[INFO] Debug ON\r\n");  }
    else if(strcmp(rx_buf,"debug off")==0) { debug_flag=0; UART_TxStr("[INFO] Debug OFF\r\n"); }

    /* lcd msg <text>  — shortcut for poke str to lcd_msg_buf */
    else if(strncmp(rx_buf,"lcd msg ",8)==0) {
        cmd_poke_str((uint32_t)lcd_msg_buf, rx_buf+8);
        lcd_needs_update = 1;
    }

    /* addrs — dump all key variable addresses */
    else if(strcmp(rx_buf,"addrs")==0) {
        UART_TxStr("[ADDRS]\r\n");
        UART_TxStr("  debug_flag   0x"); UART_TxHexWord((uint32_t)&debug_flag);   UART_TxStr("\r\n");
        UART_TxStr("  machineState 0x"); UART_TxHexWord((uint32_t)&machineState); UART_TxStr("\r\n");
        UART_TxStr("  washPhase    0x"); UART_TxHexWord((uint32_t)&washPhase);    UART_TxStr("\r\n");
        UART_TxStr("  ms_count     0x"); UART_TxHexWord((uint32_t)&ms_count);     UART_TxStr("\r\n");
        UART_TxStr("  washtimer    0x"); UART_TxHexWord((uint32_t)&washtimer);    UART_TxStr("\r\n");
        UART_TxStr("  doorOpen     0x"); UART_TxHexWord((uint32_t)&doorOpen);     UART_TxStr("\r\n");
        UART_TxStr("  detergent    0x"); UART_TxHexWord((uint32_t)&detergent);    UART_TxStr("\r\n");
        UART_TxStr("  lcd_msg_buf  0x"); UART_TxHexWord((uint32_t)lcd_msg_buf);   UART_TxStr("\r\n");
    }

    else {
        UART_TxStr("[WARN] Unknown command.\r\n");
        UART_TxStr("  Lab07: set timer XX | load dt | door open | door close\r\n");
        UART_TxStr("         start | pause | resume | abort | status | reset\r\n");
        UART_TxStr("  Lab08: peek <addr> <n> | peek debug|state|lcd\r\n");
        UART_TxStr("         poke hex <addr> <data> | poke str <addr> <text>\r\n");
        UART_TxStr("         sim sw1|sw2|phase <0-2> | debug on|off\r\n");
        UART_TxStr("         lcd msg <text> | addrs\r\n");
    }
}

/* ─────────────────────────────────────────────────────────
 * GPIO PORT F ISR  (unchanged)
 * ───────────────────────────────────────────────────────── */
void GPIOPortF_Handler(void) {
    uint32_t status = GPIO_PORTF_MIS_R;
    if(status & 0x10u) {
        GPIO_PORTF_ICR_R = 0x10u;
        if(machineState==STATE_RUNNING||machineState==STATE_PAUSED||machineState==STATE_ERROR)
            cmd_abort();
    }
    if(status & 0x01u) {
        GPIO_PORTF_ICR_R = 0x01u;
        if(machineState==STATE_RUNNING)     cmd_pause();
        else if(machineState==STATE_PAUSED) cmd_resume();
    }
}
