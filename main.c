#include <xc.h>
#include <cp0defs.h>
#include <sys/attribs.h>
#include <stdint.h>
#include <stdio.h>
#include <proc/p32mz2048efm100.h>
#include <stdbool.h>

/* manual mode 
#pragma config FPLLIDIV = DIV_3         // System PLL Input Divider (3x Divider)
#pragma config FPLLRNG = RANGE_5_10_MHZ // System PLL Input Range (5-10 MHz Input)
#pragma config FPLLICLK = PLL_POSC      // System PLL Input Clock Selection (POSC is input to the System PLL)
#pragma config FPLLMULT = MUL_50        // System PLL Multiplier (PLL Multiply by 50)
#pragma config FPLLODIV = DIV_2         // System PLL Output Clock Divider (2x Divider)
#pragma config POSCMOD = EC             // Primary Oscillator Configuration (External clock mode)
#pragma config FNOSC = SPLL             // Oscillator Selection Bits (System PLL)
#pragma config DMTCNT =     DMT9
 */


/*** DEVCFG0 ***/

#pragma config DEBUG =      OFF
#pragma config JTAGEN =     OFF
#pragma config ICESEL =     ICS_PGx2
#pragma config TRCEN =      OFF
#pragma config BOOTISA =    MIPS32
#pragma config FECCCON =    OFF_UNLOCKED
#pragma config FSLEEP =     OFF
#pragma config DBGPER =     PG_ALL
#pragma config SMCLR =      MCLR_NORM
#pragma config SOSCGAIN =   GAIN_LEVEL_3
#pragma config SOSCBOOST =  ON
#pragma config POSCGAIN =   GAIN_LEVEL_3
#pragma config POSCBOOST =  ON
#pragma config EJTAGBEN =   NORMAL
#pragma config CP =         OFF

/*** DEVCFG1 ***/

#pragma config FNOSC =      SPLL
#pragma config DMTINTV =    WIN_127_128
#pragma config FSOSCEN =    OFF
#pragma config IESO =       OFF
#pragma config POSCMOD =    EC
#pragma config OSCIOFNC =   OFF
#pragma config FCKSM =      CSECME
#pragma config WDTPS =      PS32768
#pragma config WDTSPGM =    STOP
#pragma config FWDTEN =     OFF
#pragma config WINDIS =     NORMAL
#pragma config FWDTWINSZ =  WINSZ_25
#pragma config DMTCNT =     DMT9
#pragma config FDMTEN =     OFF
/*** DEVCFG2 ***/

#pragma config FPLLIDIV =   DIV_1
#pragma config FPLLRNG =    RANGE_8_16_MHZ
#pragma config FPLLICLK =   PLL_FRC
#pragma config FPLLMULT =   MUL_50
#pragma config FPLLODIV =   DIV_2
#pragma config UPLLFSEL =   FREQ_24MHZ
/*** DEVCFG3 ***/

#pragma config USERID =     0xffff
#pragma config FMIIEN =     OFF
#pragma config FETHIO =     ON
#pragma config PGL1WAY =    ON
#pragma config PMDL1WAY =   ON
#pragma config IOL1WAY =    ON
#pragma config FUSBIDIO =   OFF

/*** BF1SEQ0 ***/

#pragma config TSEQ =       0xffff
#pragma config CSEQ =       0x0000

#define SYS_FREQ 200000000 // Running at 200MHz

void set_performance_mode()
{   
	unsigned int cp0;
	
    // Unlock Sequence
    asm volatile("di"); // Disable all interrupts
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;  

    // PB1DIV
    // Peripheral Bus 1 cannot be turned off, so there's no need to turn it on
    PB1DIVbits.PBDIV = 1; // Peripheral Bus 1 Clock Divisor Control (PBCLK1 is SYSCLK divided by 2)

    // PB2DIV
    PB2DIVbits.ON = 1; // Peripheral Bus 2 Output Clock Enable (Output clock is enabled)
    PB2DIVbits.PBDIV = 1; // Peripheral Bus 2 Clock Divisor Control (PBCLK2 is SYSCLK divided by 2)

    // PB3DIV
    PB3DIVbits.ON = 1; // Peripheral Bus 2 Output Clock Enable (Output clock is enabled)
    PB3DIVbits.PBDIV = 1; // Peripheral Bus 3 Clock Divisor Control (PBCLK3 is SYSCLK divided by 2)

    // PB4DIV
    PB4DIVbits.ON = 1; // Peripheral Bus 4 Output Clock Enable (Output clock is enabled)
    while (!PB4DIVbits.PBDIVRDY); // Wait until it is ready to write to
    PB4DIVbits.PBDIV = 0; // Peripheral Bus 4 Clock Divisor Control (PBCLK4 is SYSCLK divided by 1)

    // PB5DIV
    PB5DIVbits.ON = 1; // Peripheral Bus 5 Output Clock Enable (Output clock is enabled)
    PB5DIVbits.PBDIV = 1; // Peripheral Bus 5 Clock Divisor Control (PBCLK5 is SYSCLK divided by 2)

    // PB7DIV
    PB7DIVbits.ON = 1; // Peripheral Bus 7 Output Clock Enable (Output clock is enabled)
    PB7DIVbits.PBDIV = 0; // Peripheral Bus 7 Clock Divisor Control (PBCLK7 is SYSCLK divided by 1)

    // PB8DIV
    PB8DIVbits.ON = 1; // Peripheral Bus 8 Output Clock Enable (Output clock is enabled)
    PB8DIVbits.PBDIV = 1; // Peripheral Bus 8 Clock Divisor Control (PBCLK8 is SYSCLK divided by 2)

    // PRECON - Set up prefetch
    PRECONbits.PFMSECEN = 0; // Flash SEC Interrupt Enable (Do not generate an interrupt when the PFMSEC bit is set)
    PRECONbits.PREFEN = 0b11; // Predictive Prefetch Enable (Enable predictive prefetch for any address)
    PRECONbits.PFMWS = 0b010; // PFM Access Time Defined in Terms of SYSCLK Wait States (Two wait states)

    // Set up caching
    cp0 = _mfc0(16, 0);
    cp0 &= ~0x07;
    cp0 |= 0b011; // K0 = Cacheable, non-coherent, write-back, write allocate
    _mtc0(16, 0, cp0);  

    // Lock Sequence
    SYSKEY = 0x33333333;
    asm volatile("ei"); // Enable all interrupts
}


static void init_uart_debug(void)
{
    /* curiosity board - U1 RX=RD10 TX=RD15 */
    U1RXR = 0b0011; // RPD10
    RPD15R = 0b0001; // RPD15
    
    uint32_t pbClk = SYS_FREQ / 2;
    
    U1MODE = 0; // disable autobaud, TX and RX enabled only, 8N1, idle=HIGH
    U1BRG = pbClk / (16 * 115200) - 1;//PB_CLK / (16 * 9600) - 1;
    U1STA = 0;
    U1STAbits.URXEN = 1; // Enable the RX pin
    U1STAbits.UTXEN = 1; // Enable the TX pin
    U1MODEbits.PDSEL = 0; // PDSEL controls how many data bits and how many parity bits we want, this is the default of 8-bit data, no parity bits that most terminals use
    U1MODEbits.STSEL = 0; // STSEL controls how many stop bits we use, let's use the default of 1
    U1MODEbits.ON = 1; // Turn on the UART 3 peripheral (no interrupts)
}

static void delay_us(unsigned int us)
{
    // Convert microseconds us into how many clock ticks it will take
	us *= SYS_FREQ / 1000000 / 2; // Core Timer updates every 2 ticks       
    _CP0_SET_COUNT(0); // Set Core Timer count to 0    
    while (us > _CP0_GET_COUNT()); // Wait until Core Timer count reaches the number we calculated earlier
}

static void delay_ms(int ms)
{
    delay_us(ms * 1000);
}

void _mon_putc(char c)
 {
   while (U1STAbits.UTXBF)
   {
       ;; // wait for current transmission
   }
   U1TXREG = c;
}

int main(void) {
    set_performance_mode();
    
    uint32_t pbClk = SYS_FREQ / 2;
    uint32_t i;
    ANSELA = 0x00;
    TRISA = 0x00;
    ANSELB = 0x00;
    TRISB = 0x00;
    ANSELC = 0x00;
    TRISC = 0x00;
    ANSELD = 0x00;
    TRISD = 0x00;
    ANSELE = 0x00;
    TRISE = 0x00;
    ANSELF = 0x00;
    TRISF = 0x00;
    LATA = 0;
    LATB = 0xFF;
    LATC = 0;
    LATD = 0;
    LATE = 0;
    LATF = 0;
    
    init_uart_debug();

    /* RE3 = led1 */
    TRISEbits.TRISE3 = 0;
    /* turn on */
    // LATEbits.LATE3 = 1;
    
    /* set RPE3 PPS to OC5 out */
    RPE3R = 0b1011; /* OC5 */

    /* set RPF5 to OC4 out */
    RPF5R = 0b1011;
    
     
     /*
     RB0 = Blue
     RB1 = Green
     RB5 = Red 
    */
    bool up = true;
    const uint32_t rate = 15;
    PORTBbits.RB0 = 0;
    
    printf("started up boys\r\n");
    
    while (1) { 
        
    }
}
