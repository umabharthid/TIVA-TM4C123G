#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

/* ---------- FUNCTION DECLARATIONS ---------- */
void I2C0_Init(void);
void UART0_Init(void);
void UART0_WriteChar(char c);
void UART0_WriteString(char *s);
void UART0_WriteFloat(float num);
void UART0_WriteInt(int num);

void SysTick_Init(void);
void delay_ms(int n);

void I2C_WriteReg(uint8_t reg, uint8_t data);
void I2C_ReadMulti(uint8_t reg, uint8_t *data, uint8_t len);

void MPU_Init(void);
void MPU_Calibrate(void);
void MPU_ReadGyro(float *gx, float *gy, float *gz);

/* ---------- GLOBAL OFFSETS ---------- */
float gx_off = 0, gy_off = 0, gz_off = 0;

/* ---------- MAIN ---------- */
int main(void)
{
    float gx, gy, gz;

    I2C0_Init();
    UART0_Init();
    SysTick_Init();

    MPU_Init();
    delay_ms(100);

    UART0_WriteString("Calibrating...\r\n");
    MPU_Calibrate();
    UART0_WriteString("Done!\r\n");

    while(1)
    {
        MPU_ReadGyro(&gx, &gy, &gz);

        UART0_WriteString("RollRate=");
        UART0_WriteFloat(gx);

        UART0_WriteString(" PitchRate=");
        UART0_WriteFloat(gy);

        UART0_WriteString(" YawRate=");
        UART0_WriteFloat(gz);

        UART0_WriteString("\r\n");

        delay_ms(20);
    }
}

/* ---------- SYSTICK INIT (1ms) ---------- */
void SysTick_Init(void)
{
    NVIC_ST_CTRL_R = 0;
    NVIC_ST_RELOAD_R = 80000 - 1;  // 1 ms @ 80 MHz
    NVIC_ST_CURRENT_R = 0;
    NVIC_ST_CTRL_R = 5;            // enable + system clock
}

/* ---------- DELAY ---------- */
void delay_ms(int n)
{
    for(int i = 0; i < n; i++)
    {
        while((NVIC_ST_CTRL_R & 0x10000) == 0);
    }
}

/* ---------- MPU INIT ---------- */
void MPU_Init(void)
{
    delay_ms(100);

    I2C_WriteReg(0x6B, 0x01);  // Wake + PLL
    delay_ms(10);

    I2C_WriteReg(0x1A, 0x03);  // LPF ~44 Hz (better for testing)
    I2C_WriteReg(0x1B, 0x08);  // ±500 dps
}

/* ---------- READ GYRO ---------- */
void MPU_ReadGyro(float *gx, float *gy, float *gz)
{
    uint8_t data[6];
    int16_t raw_gx, raw_gy, raw_gz;

    I2C_ReadMulti(0x43, data, 6);

    raw_gx = (data[0] << 8) | data[1];
    raw_gy = (data[2] << 8) | data[3];
    raw_gz = (data[4] << 8) | data[5];

    *gx = (raw_gx - gx_off) / 65.5f;
    *gy = (raw_gy - gy_off) / 65.5f;
    *gz = (raw_gz - gz_off) / 65.5f;
}

/* ---------- CALIBRATION ---------- */
void MPU_Calibrate(void)
{
    float sum_gx = 0, sum_gy = 0, sum_gz = 0;
    uint8_t data[6];

    for(int i = 0; i < 2000; i++)
    {
        I2C_ReadMulti(0x43, data, 6);

        int16_t gx = (data[0] << 8) | data[1];
        int16_t gy = (data[2] << 8) | data[3];
        int16_t gz = (data[4] << 8) | data[5];

        sum_gx += gx;
        sum_gy += gy;
        sum_gz += gz;

        delay_ms(1);
    }

    gx_off = sum_gx / 2000.0f;
    gy_off = sum_gy / 2000.0f;
    gz_off = sum_gz / 2000.0f;
}

/* ---------- I2C WRITE ---------- */
void I2C_WriteReg(uint8_t reg, uint8_t data)
{
    I2C0_MSA_R = (0x68 << 1);
    I2C0_MDR_R = reg;
    I2C0_MCS_R = 0x03;
    while(I2C0_MCS_R & 0x01);

    I2C0_MDR_R = data;
    I2C0_MCS_R = 0x05;
    while(I2C0_MCS_R & 0x01);
}

/* ---------- I2C READ MULTI ---------- */
void I2C_ReadMulti(uint8_t reg, uint8_t *data, uint8_t len)
{
    int i;

    I2C0_MSA_R = (0x68 << 1);
    I2C0_MDR_R = reg;
    I2C0_MCS_R = 0x03;
    while(I2C0_MCS_R & 0x01);

    I2C0_MSA_R = (0x68 << 1) | 1;

    for(i = 0; i < len; i++)
    {
        if(i == 0)
            I2C0_MCS_R = 0x0B;
        else if(i == len - 1)
            I2C0_MCS_R = 0x05;
        else
            I2C0_MCS_R = 0x09;

        while(I2C0_MCS_R & 0x01);
        data[i] = I2C0_MDR_R;
    }
}

/* ---------- I2C INIT ---------- */
void I2C0_Init(void)
{
    SYSCTL_RCGCI2C_R |= 1;
    SYSCTL_RCGCGPIO_R |= 0x02;
    while((SYSCTL_PRGPIO_R & 0x02) == 0);

    GPIO_PORTB_AFSEL_R |= 0x0C;
    GPIO_PORTB_ODR_R |= 0x08;
    GPIO_PORTB_DEN_R |= 0x0C;
    GPIO_PORTB_PCTL_R |= 0x00003300;

    I2C0_MCR_R = 0x10;
    I2C0_MTPR_R = 19;   // safer ~100 kHz
}

/* ---------- UART INIT ---------- */
void UART0_Init(void)
{
    SYSCTL_RCGCUART_R |= 1;
    SYSCTL_RCGCGPIO_R |= 1;
    while((SYSCTL_PRGPIO_R & 1) == 0);

    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_PCTL_R |= 0x00000011;
    GPIO_PORTA_DEN_R |= 0x03;

    UART0_CTL_R = 0;
    UART0_IBRD_R = 8;
    UART0_FBRD_R = 44;
    UART0_LCRH_R = 0x60;
    UART0_CC_R = 0;
    UART0_CTL_R = 0x301;
}

/* ---------- UART FUNCTIONS ---------- */
void UART0_WriteChar(char c)
{
    while(UART0_FR_R & 0x20);
    UART0_DR_R = c;
}

void UART0_WriteString(char *s)
{
    while(*s)
        UART0_WriteChar(*s++);
}

void UART0_WriteFloat(float num)
{
    if(num < 0)
    {
        UART0_WriteChar('-');
        num = -num;
    }

    int int_part = (int)num;
    int frac = (int)((num - int_part) * 100);

    UART0_WriteInt(int_part);
    UART0_WriteChar('.');
    if(frac < 10) UART0_WriteChar('0');
    UART0_WriteInt(frac);
}

void UART0_WriteInt(int num)
{
    char buf[10];
    int i = 0;

    if(num < 0)
    {
        UART0_WriteChar('-');
        num = -num;
    }

    do {
        buf[i++] = num % 10 + '0';
        num /= 10;
    } while(num);

    while(i--)
        UART0_WriteChar(buf[i]);
}
