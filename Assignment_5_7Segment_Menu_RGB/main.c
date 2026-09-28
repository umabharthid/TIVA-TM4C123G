#include <stdint.h>
#include "drivers/sevenseg.h"
#include "inc/tm4c123gh6pm.h"

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
#define RED     0x02
#define BLUE    0x04
#define GREEN   0x08
#define YELLOW  0x0A
#define MAGENTA 0x06
#define CYAN    0x0C
#define WHITE   0x0E
#define SEG_S   0x6D
#define SEG_C   0x39   // a f e d
#define SEG_t   0x78   // f e d g
#define SEG_P   0x73   // a b f g e
#define SEG_E   0x79   // a f g e d
#define SEG_n   0x54   // c e g
#define SEG_r   0x50   // e g
#define SW1        4  // PF4
#define SW2        0   // PF0
#define SEG_dot 0x80


typedef enum { pressed, notpressed } event_t;
typedef enum { Sel_color, Sel_speed, En, Stop, Rst  } mode_t;

//extern volatile uint64_t system_time;

event_t button_SW1 = notpressed;
event_t button_SW2 = notpressed;

mode_t mode = Rst;
uint8_t dispbuf[4] = {SEG_r, SEG_S, SEG_dot, SEG_dot};
uint8_t map[7] = {SEG_1, SEG_2, SEG_3, SEG_4, SEG_5, SEG_6, SEG_7};
uint8_t color_map[7] = { RED, BLUE, GREEN, YELLOW, MAGENTA, CYAN, WHITE};
uint8_t i = 0, speed=0;
uint32_t status;
uint32_t delay=63000;
uint8_t color = 0x00;
uint16_t speed_table[7] = {2000,1500,1000,800,500,300,100};
uint64_t last_toggle = 0;

/* simple delay */
void delayMs(int n)
{
    int i,j;
    for(i=0;i<n;i++)
        for(j=0;j<3180;j++);
}
void GPIO_init()
{
    //Enable clock on port F
        SYSCTL_RCGCGPIO_R |= (1<<5);
        while(!(SYSCTL_PRGPIO_R & (1<<5)));
        GPIO_PORTF_LOCK_R = 0x4C4F434B;
        GPIO_PORTF_CR_R |= (1<<SW1)|(1<<SW2);

        //Make LED pins as output and switches as input
        GPIO_PORTF_AFSEL_R &= ~((1<<3)|(1<<1)|(1<<SW1)|(1<<SW2)|(1<<2));
        GPIO_PORTF_AMSEL_R &= ~((1<<3)|(1<<1)|(1<<SW1)|(1<<SW2)|(1<<2));

        GPIO_PORTF_DIR_R |= (1<<3)|(1<<1)|(1<<2);
        GPIO_PORTF_DIR_R &= ~((1<<SW1)|(1<<SW2));
        GPIO_PORTF_DEN_R |= (1<<3)|(1<<1)|(1<<SW1)|(1<<SW2)|(1<<2);
        GPIO_PORTF_PUR_R |= (1<<SW1)|(1<<SW2);  //Need to enable internal pull up for input pins

        //Interrupt config
        //Edge sensitive
        GPIO_PORTF_IS_R &= ~((1 << SW1)|(1 << SW2));   // 0 = Edge

        //Single edge
        GPIO_PORTF_IBE_R &= ~((1 << SW1)|(1 << SW2));  // 0 = Not both edges

        //Falling edge
        GPIO_PORTF_IEV_R &= ~((1 << SW1)|(1 << SW2));  // 0 = Falling edge

        //Clear prior interrupt
        GPIO_PORTF_ICR_R |= ((1 << SW1)|(1 << SW2));

        //Unmask interrups for SW1 and SW2
        GPIO_PORTF_IM_R |= (1 << SW1)|(1 << SW2);

        //Enable IRQ 30 in NVIC
        NVIC_EN0_R |= (1 << 30);
       // __enable_irq();
        __asm(" CPSIE I ");

}

void GPIOF_Handler()
{



        status = GPIO_PORTF_MIS_R;//First read so you can clear it ASAP
        GPIO_PORTF_ICR_R = status;//interrupt cleared already
        GPIO_PORTF_IM_R &= ~status;//Mask them temporarily
        delayMs(40);

        if(!(GPIO_PORTF_DATA_R & (1<<SW2))) {button_SW2=pressed;}
        if(!(GPIO_PORTF_DATA_R & (1<<SW1))) {button_SW1=pressed;}
        //the following two statements are always written after the if block.. they need to get executed irrepspective
        GPIO_PORTF_ICR_R |= ((1 << SW1)|(1 << SW2));//interrupt cleared again
        GPIO_PORTF_IM_R |=(1 << SW1)|(1 << SW2);


}

int main(void)
{
    sevenseg_init();
    GPIO_init();

    while(1)
    {
        if(button_SW2 == pressed)//Change mode
                            {
                                switch(mode)
                                        {
                                            case (Rst):{mode=Sel_color;dispbuf[2] = SEG_dot;dispbuf[3] = SEG_dot; break;}
                                            case (Sel_color):{mode=Sel_speed;dispbuf[2]=map[i]; GPIO_PORTF_DATA_R &= ~(0x0E); color=color_map[i]; break;}
                                            case (Sel_speed):{mode = En;dispbuf[3] = map[i]; speed = i; break;}
                                            case (En):{mode = Stop;GPIO_PORTF_DATA_R &= ~(0x0E);break;}
                                            case (Stop):{mode = Rst;break;}

                                        }
                                button_SW2 = notpressed; i=0;
                            }
        if(button_SW1 == pressed)//Change mode
                                    {
                                        i++; if (i==7){i=0;}
                                        button_SW1 = notpressed;
                                    }
        switch(mode)//I want to loop/increment number here and map it to the mode
            {
            case (Rst):{  sevenseg_set_digits(SEG_r, SEG_S, SEG_dot, SEG_dot); break;}
            case (Sel_color):{ sevenseg_set_digits(SEG_S, SEG_C, map[i], dispbuf[3]); break;}
            case (Sel_speed):{ sevenseg_set_digits(SEG_S, SEG_P, dispbuf[2], map[i]);break;}
            case (En):
                {
                sevenseg_set_digits(SEG_E, SEG_n, dispbuf[2], dispbuf[3]);
                if(system_time - last_toggle > speed_table[speed])
                {
                    GPIO_PORTF_DATA_R ^= color;
                    last_toggle = system_time;
                }
                break;
                }
            case (Stop):{ sevenseg_set_digits(SEG_S, SEG_t, dispbuf[2], dispbuf[3]);break;GPIO_PORTF_DATA_R &= ~(0x0E);}
            }
    }
}
