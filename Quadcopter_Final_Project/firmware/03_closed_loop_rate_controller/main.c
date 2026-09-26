#include <stdint.h>
#include <math.h>
#include "inc/tm4c123gh6pm.h"

/* ---------- GLOBALS ---------- */
float gx_off=0, gy_off=0, gz_off=0;

/* ---------- CONTROL ---------- */
float Kp_roll  = 1.5f;
float Kp_pitch = 1.5f;
float Kp_yaw   = 1.0f;

float InputRoll, InputPitch, InputYaw;
float MotorInput1 = 1000, MotorInput2 = 1000, MotorInput3 = 1000, MotorInput4 = 1000;

/* ---------- RC INPUT ---------- */
volatile uint32_t rise_time = 0;
volatile uint32_t pulse_width = 1000;
volatile uint32_t last_valid = 1000;

/* ---------- TIMERS ---------- */
uint32_t loop_4ms = 0;
uint32_t loop_20ms = 0;

/* ---------- SYSTICK ---------- */
void SysTick_Init(void){
    NVIC_ST_CTRL_R=0;
    NVIC_ST_RELOAD_R=0xFFFFFF;
    NVIC_ST_CURRENT_R=0;
    NVIC_ST_CTRL_R=0x05;
}
void LED_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x20; // Port F
    while((SYSCTL_PRGPIO_R & 0x20)==0);

    GPIO_PORTF_DIR_R |= 0x0E;  // PF1,2,3 output
    GPIO_PORTF_DEN_R |= 0x0E;
}
uint32_t micros(void){
    return (0xFFFFFF-NVIC_ST_CURRENT_R)/16;
}
void delay_ms(int n){
    uint32_t s=micros();
    while(micros()-s < n*1000);
}



/* ---------- PWM ---------- */
void PWM0_Init(void)
{
    SYSCTL_RCGCPWM_R |= 0x01;
    SYSCTL_RCGCGPIO_R |= 0x02;
    while((SYSCTL_PRGPIO_R & 0x02)==0);

    SYSCTL_RCC_R |= (1<<20);
    SYSCTL_RCC_R &= ~(0x7<<17);
    SYSCTL_RCC_R |= (0x6<<17);

    GPIO_PORTB_AFSEL_R |= 0xF0;
    GPIO_PORTB_PCTL_R &= ~0xFFFF0000;
    GPIO_PORTB_PCTL_R |=  0x44440000;
    GPIO_PORTB_DEN_R  |= 0xF0;

    PWM0_0_CTL_R = 0;
    PWM0_1_CTL_R = 0;

    PWM0_0_LOAD_R = 5000 - 1;
    PWM0_1_LOAD_R = 5000 - 1;

    PWM0_0_GENA_R = 0x8C;
    PWM0_0_GENB_R = 0x80C;
    PWM0_1_GENA_R = 0x8C;
    PWM0_1_GENB_R = 0x80C;

    PWM0_0_CTL_R |= 1;
    PWM0_1_CTL_R |= 1;

    PWM0_ENABLE_R |= 0x0F;
}

/* ---------- RC INPUT INIT ---------- */
void RC_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x04;
    while((SYSCTL_PRGPIO_R & 0x04)==0);

    GPIO_PORTC_DIR_R &= ~0x10;   // PC4 input
    GPIO_PORTC_DEN_R |=  0x10;

    GPIO_PORTC_IS_R  &= ~0x10;
    GPIO_PORTC_IBE_R |=  0x10;   // both edges

    GPIO_PORTC_ICR_R |=  0x10;
    GPIO_PORTC_IM_R  |=  0x10;

    NVIC_EN0_R |= (1 << 2);
}

/* ---------- RC ISR ---------- */
void GPIOPortC_Handler(void)
{
    uint32_t now = micros();
    uint8_t pin = GPIO_PORTC_DATA_R & 0x10;

    GPIO_PORTC_ICR_R = 0x10;

    if(pin)
    {
        rise_time = now;
    }
    else
    {
        uint32_t width = now - rise_time;

        if(width >= 900 && width <= 2100)
        {
            pulse_width = width;
            last_valid  = width;
        }
        else
        {
            pulse_width = last_valid;
        }
    }
}

/* ---------- SPI ---------- */
void SPI0_Init(void){
    SYSCTL_RCGCSSI_R|=1;
    SYSCTL_RCGCGPIO_R|=1;
    while((SYSCTL_PRGPIO_R&1)==0);

    GPIO_PORTA_AFSEL_R|=0x34;
    GPIO_PORTA_PCTL_R|=0x00202200;
    GPIO_PORTA_DEN_R|=0x3C;

    GPIO_PORTA_DIR_R|=0x08;
    GPIO_PORTA_DATA_R|=0x08;

    SSI0_CR1_R=0;
    SSI0_CC_R=0;
    SSI0_CPSR_R=32;
    SSI0_CR0_R=0x0007;
    SSI0_CR1_R|=0x02;
}

uint8_t SPI_Transfer(uint8_t d){
    while((SSI0_SR_R&2)==0);
    SSI0_DR_R=d;
    while(SSI0_SR_R&0x10);
    return SSI0_DR_R;
}

/* ---------- MPU ---------- */
void MPU_Write(uint8_t reg,uint8_t data){
    GPIO_PORTA_DATA_R&=~0x08;
    SPI_Transfer(reg&0x7F);
    SPI_Transfer(data);
    GPIO_PORTA_DATA_R|=0x08;
}

void MPU_ReadMulti(uint8_t reg,uint8_t *buf,int n){
    GPIO_PORTA_DATA_R&=~0x08;
    SPI_Transfer(reg|0x80);
    for(int i=0;i<n;i++) buf[i]=SPI_Transfer(0);
    GPIO_PORTA_DATA_R|=0x08;
}

void MPU_Init(void){
    delay_ms(100);
    MPU_Write(0x6B,0x01);
    MPU_Write(0x1A,0x05);
    MPU_Write(0x1B,0x08);
}

void MPU_Calibrate(void){
    int32_t sx=0, sy=0, sz=0;
    uint8_t d[6];

    for(int i=0;i<2000;i++){

        // 🔴 RED ON during sampling
        GPIO_PORTF_DATA_R = 0x02;

        MPU_ReadMulti(0x43,d,6);

        sx += (int16_t)((d[0]<<8)|d[1]);
        sy += (int16_t)((d[2]<<8)|d[3]);
        sz += (int16_t)((d[4]<<8)|d[5]);

        delay_ms(1);

        // 🟢 GREEN blink toggle
        GPIO_PORTF_DATA_R ^= 0x08;
    }

    gx_off = sx/2000.0f;
    gy_off = sy/2000.0f;
    gz_off = sz/2000.0f;

    // 🟢 GREEN ON = calibration done
    GPIO_PORTF_DATA_R = 0x08;
}

/* ---------- MAIN ---------- */
int main(void){
    SysTick_Init();
    LED_Init();
    PWM0_Init();
    SPI0_Init();
    RC_Init();        // 🔥 added
    MPU_Init();
    MPU_Calibrate();

    loop_4ms = micros();
    loop_20ms = micros();

    while(1){

        if(micros() - loop_4ms >= 4000){
            loop_4ms += 4000;

            uint8_t d[6];
            int16_t gx,gy,gz;

            MPU_ReadMulti(0x43,d,6);
            gx=(d[0]<<8)|d[1];
            gy=(d[2]<<8)|d[3];
            gz=(d[4]<<8)|d[5];

            float RateRoll  = (gx-gx_off)/65.5f;
            float RatePitch = (gy-gy_off)/65.5f;
            float RateYaw   = (gz-gz_off)/65.5f;

            InputRoll  = Kp_roll  * (-RateRoll);
            InputPitch = Kp_pitch * (-RatePitch);
            InputYaw   = Kp_yaw   * (-RateYaw);

            float thr = pulse_width;

            if(thr > 1800) thr = 1800;

            MotorInput1 = 1.024f*(thr-InputRoll-InputPitch-InputYaw);
            MotorInput2 = 1.024f*(thr-InputRoll+InputPitch+InputYaw);
            MotorInput3 = 1.024f*(thr+InputRoll+InputPitch-InputYaw);
            MotorInput4 = 1.024f*(thr+InputRoll-InputPitch+InputYaw);

            if(MotorInput1 < 1000) MotorInput1 = 1000;
            if(MotorInput2 < 1000) MotorInput2 = 1000;
            if(MotorInput3 < 1000) MotorInput3 = 1000;
            if(MotorInput4 < 1000) MotorInput4 = 1000;

            if(MotorInput1 > 2000) MotorInput1 = 2000;
            if(MotorInput2 > 2000) MotorInput2 = 2000;
            if(MotorInput3 > 2000) MotorInput3 = 2000;
            if(MotorInput4 > 2000) MotorInput4 = 2000;
        }

        if(micros() - loop_20ms >= 20000){
            loop_20ms += 20000;

            PWM0_0_CMPA_R = 5000 - (MotorInput1/4);
            PWM0_0_CMPB_R = 5000 - (MotorInput2/4);
            PWM0_1_CMPA_R = 5000 - (MotorInput3/4);
            PWM0_1_CMPB_R = 5000 - (MotorInput4/4);
        }
    }
}
