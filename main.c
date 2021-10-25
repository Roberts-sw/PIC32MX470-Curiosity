/* main.c 
   ---------------------------------------------------- UTF-8, TAB-size 4 --
    board   -   PIC32MX470 Curiosity
    chip    -   PIC32MX470512H
    compiler-   XC32
    tool    -   PKOB (PIC-Kit On-Board)

Wijzigingen:
    RvL 23-10-2021  aanmaak
------------------------------------------------------------------------- */
//DEVCFG3
#pragma config USERID =     0xffff
#pragma config FSRSSEL =    PRIORITY_7      //shadow register used with prio 7
#pragma config PMDL1WAY =   ON              //only one reconfig
#pragma config IOL1WAY =    ON              //only one reconfig
#pragma config FUSBIDIO =   ON              //controlled by USB module
#pragma config FVBUSONIO =  ON              //controlled by USB module

//DEVCFG2: Frc/2 * 20/2 = 8M/2 * 20/2 = 40 MHz
#pragma config FPLLIDIV =   DIV_2           //(4M <= x <= 5M) x=OSC/2   (4MHz)
#pragma config FPLLMUL =    MUL_20          //F_PLL= x*20               (80MHz)
#pragma config UPLLIDIV =   DIV_2           //(USB input divider)
#pragma config UPLLEN =     OFF             //(USB PLL)
#pragma config FPLLODIV =   DIV_2           //SYSCLK= F_PLL/2           (40MHz)

//DEVCFG1
#pragma config FNOSC =      FRCPLL          //oscillator selection: Frc (8MHz)
#pragma config FSOSCEN =    OFF             //(Secondary Oscillator)
#pragma config IESO =       OFF             //(Internal/External Switch Over)
#pragma config POSCMOD =    OFF             //(Primary Oscillator)
#pragma config OSCIOFNC =   OFF             //clock out
#pragma config FPBDIV =     DIV_2           //PBCLK = SYSCLK/2
#pragma config FCKSM =      CSDCMD          //CLK: switch disabled, monitor disabled
#pragma config WDTPS =      PS1048576       //postscale = 1<<20
#pragma config WINDIS =     OFF             //WWDT disabled
#pragma config FWDTEN =     OFF             //WDT disabled
#pragma config FWDTWINSZ =  WINSZ_25        //25% window

//DEVCFG0
#pragma config DEBUG =      OFF             //(background debugger)
#pragma config JTAGEN =     OFF             //JTAG
#pragma config ICESEL =     ICS_PGx2        //PGED/PGEC channel 2
#pragma config PWP =        OFF             //(Program Write Protect)
#pragma config BWP =        OFF             //(Boot Write Protect)
#pragma config CP =         OFF             //(Code Protect)

#include <xc.h>

    /* ---------------------------------------------------------
    headers
    --------------------------------------------------------- */
#define LED_DDR             TRISE
//#define LED1              (1<<4)
#define LED2                (1<<6)
//#define LED3              (1<<7)

#define F_XTAL              20000000ul      //crystal, user-specified
#define F_SOSC              32768           //secondary, user-specified

    /* ---------------------------------------------------------
    private
    --------------------------------------------------------- */
static uint32_t CTR_PERIOD, SYSCLK;
static volatile uint32_t CTR_SAMPLE, Micros, Millis;

#include <sys/attribs.h>
void __ISR(_CORE_TIMER_VECTOR) CoreTimerHandler(void)
{   IFS0CLR=_IFS0_CTIF_MASK;
    _CP0_SET_COMPARE((CTR_SAMPLE=_CP0_GET_COMPARE() )+CTR_PERIOD);
    Micros+=1000;   Millis+=1;
}

    /* ---------------------------------------------------------
    protected
    --------------------------------------------------------- */
#define F_LPRC              31250           //LPRC = 31k25, fixed
#define F_FRC               8000000         //FRC = 8M, fixed
uint32_t SYSCLK_get (void)
{   uint32_t d,f,m,src=OSCCONbits.COSC;
    switch(src)
    {   case 0b111: f=F_FRC;
            d=OSCCONbits.FRCDIV;        //0..7
            goto div;
        case 0b110: f=F_FRC / 16;       break;
        case 0b101: f=F_LPRC;           break;
        case 0b100: f=F_SOSC;           break;
        case 0b010: f=F_XTAL;           break;
        case 0b011: if(1)   f = F_XTAL; else
        case 0b001: f=F_FRC;
            //input d
            d=DEVCFG2bits.FPLLIDIV+1;   //1..8
            f/=( d<=6 ? d : 2*d-4);     //7 8 ==> 10 12
            //PLL mul
            m=OSCCONbits.PLLMULT+15;    //15..22
            f*=(m!=22 ? m : 24);        //22 ==> 24
            //PLL d
            d=OSCCONbits.PLLODIV;       //0..7
div:        f>>=(d!=7 ? d : 8);         //7 ==> 8
            break;
        case 0b000: f=F_FRC;        break;
    }   return f;
}

    /* ---------------------------------------------------------
    public
    --------------------------------------------------------- */
void CTR_init (void)
{   SYSCLK=SYSCLK_get();
    CTR_PERIOD=SYSCLK/2/1000;   //1kHz CTR-interrupt
    IFS0CLR=_IFS0_CTIF_MASK;
    IPC0CLR=_IPC0_CTIP_MASK;
    IPC0SET=(7 << _IPC0_CTIP_POSITION); //pri=7
    IPC0CLR=_IPC0_CTIS_MASK;
    IPC0SET=(3 << _IPC0_CTIS_POSITION); //sub=3
    IEC0CLR=_IEC0_CTIE_MASK;
    IEC0SET=(1 << _IEC0_CTIE_POSITION); //enable
    _CP0_SET_COMPARE((CTR_SAMPLE=_CP0_GET_COUNT() )+CTR_PERIOD);
}
uint32_t micros (void)
{   __builtin_disable_interrupts();
    uint32_t res=Micros+(_CP0_GET_COMPARE()-CTR_SAMPLE)*1000/CTR_PERIOD;
    __builtin_enable_interrupts();
    return res;
}
uint32_t millis (void)  {   return Millis;  }

uint8_t const powered_on=1;
int main(void)
{   static uint32_t moment;

//  SYS_init();
    INTCONbits.MVEC = 1;        //BF881000:0000
    ANSELE=0x0000;              //BF886400:00F4
    TRISE&=~LED2;               //BF886410:xxxx

    CTR_init();                 //core timer
    __builtin_enable_interrupts();

    do{
        if(millis()-moment<-60000u) //max. interval 60s
        {   moment+=250;            //250ms
            LATE^=LED2;             //... toggle
        }
    } while(powered_on);
}


