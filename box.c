/*
 * File:   box.c
 * Author: Maarten Hofman
 *
 * Created on June 6, 2026, 1:41 PM
 */

// PIC16F18115 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FEXTOSC = OFF    // External Oscillator Selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_32MHz// Reset Oscillator Selection bits (HFINTOSC (32 MHz))
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; i/o or oscillator function on OSC2)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config VDDAR = HI       // VDD Range Analog Calibration Selection bit (Internal analog systems are calibrated for operation between VDD = 2.3 - 5.5V)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)

// CONFIG2
#pragma config MCLRE = INTMCLR  // Master Clear Enable bit (If LVP = 0, MCLR is port-defined function; If LVP = 1, RA3 pin function is MCLR)
#pragma config PWRTS = PWRT_OFF // Power-up Timer Selection bits (PWRT is disabled)
#pragma config LPBOREN = OFF    // Low-Power BOR Enable bit (ULPBOR disabled)
#pragma config BOREN = OFF      // DISABLE BROWN OUT RESET
#pragma config DACAUTOEN = OFF  // DAC Buffer Automatic Range Select Enable bit (DAC Buffer reference range is determined by the REFRNG bit)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection bit (Brown-out Reset Voltage (VBOR) set to 1.9V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD module is disabled; ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = OFF    // PPSLOCKED One-Way Set Enable bit (The PPSLOCKED bit can be set and cleared as needed (unlocking sequence is required))
#pragma config STVREN = OFF     // DISABLE STACK OVERFLOW RESET
#pragma config DEBUG = OFF      // Background Debugger (Background Debugger disabled)

// CONFIG3
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = OFF        // WATCHDOG TIMER DISABLED
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT Input Clock Select bits (Software Control)

// CONFIG4
#pragma config BBSIZE = BB512   // Boot Block Size Selection bits (512 words boot block size)
#pragma config BBEN = OFF       // Boot Block Enable bit (Boot Block disabled)
#pragma config SAFEN = OFF      // Storage Area Flash (SAF) Enable bit (SAF disabled)
#pragma config WRTAPP = OFF     // Application Block Write Protection bit (Application Block is NOT write protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block is NOT write protected)
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration Register is NOT write protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM is NOT write protected)
#pragma config WRTSAF = OFF     // Storage Area Flash (SAF) Write Protection bit (SAF is NOT write protected)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (High Voltage on MCLR/Vpp must be used for programming)

// CONFIG5
#pragma config CP = OFF         // Program Flash Memory Code Protection bit (Program Flash Memory code protection is disabled)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM code protection is disabled)

/**
 * RA0: button in
 * RA1: MIDI out
 * RA3: MIDI in? (not set up yet)
 * RA5: analog in 
 */

#include <xc.h>

#define _XTAL_FREQ 32000000


volatile unsigned char queue[16];
volatile unsigned char queueread = 0;
volatile unsigned char queuewrite = 0;
volatile unsigned char queuesize = 0;
unsigned char led = 0;
unsigned char button = 0;
unsigned char stable = 0;
unsigned short longpress = 0;
unsigned char dial;
unsigned char ptr = 32;

// 64-byte direct mapping table optimized for the physical track geometry at 3.3V
// 255 = "In Dubio" transition dead zone
const unsigned char memory_map[64] = {
    131, 130, 130, 130, 255, 0,   0,   0,   // 0 - 7   (Pos 10 down to Pos 8)
    0,   0,   0,   0,   17,  17,  17,  17,  // 8 - 15  (Pos 8 and Pos 7)
    17,  17,  17, 255, 255, 102, 102, 102, // 16 - 23 (Dead zone and Pos 6)
    102, 102, 102, 102, 255, 85,  85,  85,  // 24 - 31 (Dead zone and Pos 5)
    85,  85,  85,  85,  255, 68,  68,  68,  // 32 - 39 (Dead zone and Pos 4)
    68,  68,  68,  68,  255, 255, 51,  51,  // 40 - 47 (Dead zone and Pos 3)
    51,  51,  51,  51,  255, 34,  34,  34,  // 48 - 55 (Dead zone and Pos 2)
    34,  34,  34,  255, 129, 129, 129, 128, // 56 - 63 (Pos 2 up to Pos 0)
};

const unsigned char CC_numbers[17] = {
  17, //Effect type
  18, //Effect depth
  19, //Effect rate
  79, //EG sustain level
  75, //EG decay time
  72, //EG release time
  83, //EG FEG-AEG balance
  73, //EG attack time
  71, //Filter resonance
  74, //Filter cutoff
  20, //Portamento
  76, //LFO speed
  77, //LFO depth
  78, //LFO assign
  80, //Oscillator type
  82, //Oscillator mod
  81 //Oscillator texture
};

unsigned char CC[17] = {
  127, //Effect type off
  0,   //Effect depth 0
  0,   //Effect rate 0
  120, //EG sustain level
  10,  //EG decay time
  0,   //EG release time
  64,  //EG FEG-AEG balance
  0,   //EG attack time
  10,  //Filter resonance
  100, //Filter cutoff
  0,   //Portamento
  64,  //LFO speed
  0,   //LFO depth
  64,  //LFO assign
  0,  //Oscillator type
  10,  //Oscillator mod
  10   //Oscillator texture
};

unsigned char read(unsigned char location) {
  // This code block will read 1 word (byte) of DFM
  NVMCON1bits.NVMREGS = 1; // Point to DFM
  NVMADRH = 0x0F0;
  NVMADRL = location;
  NVMCON1bits.RD = 1; // Initiate read cycle
  return NVMDATL;
}

void write(unsigned char location, unsigned char v) {
  // Wait for write to complete.
  while (NVMCON1bits.WR);
  NVMCON1bits.NVMREGS = 1; // Point to DFM
  NVMADRH = 0x0F0;
  NVMADRL = location;
  NVMDATL = v;
  NVMCON1bits.WREN = 1; // Allows program/erase cycles
  INTCONbits.GIE = 0; // Disable interrupts
  NVMCON2 = 0x55; // Perform required unlock sequence
  NVMCON2 = 0xAA;
  NVMCON1bits.WR = 1; // Begin program/erase cycle
  INTCONbits.GIE = 1; // Restore interrupt enable bit value
  NVMCON1bits.WREN = 0; // Disable program/erase
  NVMCON1bits.WRERR = 0; // Ignore errors
}

unsigned char getch() {
    unsigned char data;
    
    while(!queuesize) {
        // Wait until there is a message.
    }
    data = queue[queueread & 15];
    queueread++;
    queuesize--;
    return data;
}

inline unsigned char peek() {
    if (!queuesize) return 0;
    return queue[queueread & 15];

}

__interrupt() void incoming() {
    if (RC1STAbits.OERR) {
        led = !led;
        LATAbits.LATA2 = led;
        RC1STAbits.CREN = 0;
        RC1STAbits.CREN = 1;
        return;
    }    
    queue[queuewrite & 15] = RC1REG;
    queuewrite++;
    queuesize++;
}

void putch2(unsigned char data) {
  while(!PIR4bits.TX1IF) {          // wait until the transmitter is ready
  }
  TX1REG = data;                     // send one character
}


void init_uart(void) {
  SP1BRG = 15;                        // 3 at 8Mhz, 9 at 20Mhz (from 4 to 10), 15 at 32 Mhz
  SP1BRGH = 0;
  TX1STAbits.TXEN = 1;               // enable transmitter
  TX1STAbits.SYNC = 0;
  TX1STAbits.BRGH = 0;
  RC1STAbits.RX9 = 0;
  RC1STAbits.ADDEN = 0;
  RC1STAbits.SPEN = 1;               // enable serial port
  RC1STAbits.CREN = 1;               // enable receiver
  BAUD1CONbits.BRG16 = 0;            // Set at 1 Mhz with SP1BRG at 1
  BAUD1CONbits.ABDEN = 0;
  
  // Setup interrupt
  PIE4bits.RC1IE = 1;
  INTCONbits.PEIE = 1;
  INTCONbits.GIE = 1;
  RX1PPS = 3;
  RA1PPS = 0x13;  // RA1 is TX1
}

void checkButton() {
    switch(button) {
        case 0:
            if (PORTAbits.RA0) {
                stable = 0;
                return;
            }
            stable++;
            if (stable > 250) {
                longpress = 0;
                button = 1;
            }
            return;
        case 1:
            if (!PORTAbits.RA0) {
                stable = 0;
                longpress++;
                if (longpress > 65500) {
                    button = 2;
                    longpress = 0;
                }
                return;
            }
            stable++;
            if (stable > 250) button = 0;
            return;
        case 2:
            if (!PORTAbits.RA0) {
                stable = 0;
                return;
            }
            stable++;
            if (stable > 250) button = 0;
            return;
    }
}

unsigned short getAdc() {
    ADCON0bits.GO = 1;
    while(ADCON0bits.GO) {
        // DO nothing.
    }
    return ADRES;
}

void preset(unsigned char which) {
    if ((which == 128) || (which == 131)) {
      dial = getAdc() >> 10;
      which = memory_map[dial];
    }
    if ((which == 128) || (which == 131)) {
      dial = getAdc() >> 10;
      which = memory_map[dial];
    }    
    switch(which) {
        case 128:
            CC[0] = 0x5F;
            CC[1] = 0x4C;
            CC[2] = 0x7F;
            CC[3] = 0x72;
            CC[4] = 0x28;
            CC[5] = 0x59;
            CC[6] = 0x41;
            CC[7] = 0x37;
            CC[8] = 0x32;
            CC[9] = 0x32;
            CC[10] = 0x00;
            CC[11] = 0x1A;
            CC[12] = 0x0D;
            CC[13] = 0x5F;
            CC[14] = 0x40;
            CC[15] = 0x5A;
            CC[16] = 0x06;
            break;
        case 129:
            CC[0] = 0x00;
            CC[1] = 0x3E;
            CC[2] = 0x7F;
            CC[3] = 0x2C;
            CC[4] = 0x31;
            CC[5] = 0x00;
            CC[6] = 0x7F;
            CC[7] = 0x00;
            CC[8] = 0x00;
            CC[9] = 0x40;
            CC[10] = 0x00;
            CC[11] = 0x0C;
            CC[12] = 0x1A;
            CC[13] = 0x40;
            CC[14] = 0x00;
            CC[15] = 0x20;
            CC[16] = 0x35;
            break;
        case 130:  
            CC[0] = 0x20;
            CC[1] = 0x20;
            CC[2] = 0x17;
            CC[3] = 0x29;
            CC[4] = 0x6B;
            CC[5] = 0x46;
            CC[6] = 0x41;
            CC[7] = 0x21;
            CC[8] = 0x14;
            CC[9] = 0x33;
            CC[10] = 0x00;
            CC[11] = 0x1E;
            CC[12] = 0x15;
            CC[13] = 0x20;
            CC[14] = 0x20;
            CC[15] = 0x40;
            CC[16] = 0x04;
            break;
        case 131:
            CC[0] = 0x40;
            CC[1] = 0x22;
            CC[2] = 0x7F;
            CC[3] = 0x16;
            CC[4] = 0x3D;
            CC[5] = 0x3D;
            CC[6] = 0x4C;
            CC[7] = 0x00;
            CC[8] = 0x46;
            CC[9] = 0x2E;
            CC[10] = 0x00;
            CC[11] = 0x00;
            CC[12] = 0x00;
            CC[13] = 0x7F;
            CC[14] = 0x5F;
            CC[15] = 0x00;
            CC[16] = 0x42;
            break;
    }
}

void error() {
    RC1STAbits.CREN = 0;
    for (unsigned char i = 0; i < 3; i++) {
      led = !led;
      LATAbits.LATA2 = led;
      __delay_ms(250);
      led = !led;
      LATAbits.LATA2 = led;
      __delay_ms(250);
    }
    RC1STAbits.CREN = 1;
}

void main(void) {
    unsigned char value;
    unsigned char state;
    unsigned char cc;
    unsigned char prevButton = 0;
    
    init_uart();
    PORTA = 0x01;
    LATA = 0x01;
    ANSELA = 0x00;
    TRISA = 0x09;
    WPUAbits.WPUA0 = 1;
    WPUAbits.WPUA3 = 1;
    TRISAbits.TRISA0 = 1;
    TRISAbits.TRISA5 = 1;
    ANSELAbits.ANSELA5 = 1;

    ADCON0bits.FM = 0;
    ADCON0bits.CS = 1; // ADCRC Clock
    ADCON0bits.IC = 0; // Regular mode
    ADCON0bits.ON = 1; // Turn ADC on.
    ADPCH = 5;
    
    led = !led;
    LATAbits.LATA2 = led;
    __delay_ms(1000);
    led = !led;
    LATAbits.LATA2 = led;
    __delay_ms(1000);
    led = !led;
    LATAbits.LATA2 = led;

    while (1) {
        while (!queuesize) {
            checkButton();
            switch(button) {
                case 0:
                case 2:
                    led = 0;
                    LATAbits.LATA2 = led;
                    break;
                case 1:
                    led = 1;
                    LATAbits.LATA2 = led;
                    break;
            }
            if ((button == 0) && (button != prevButton)) {
                switch(prevButton) {
                    case 1:
                        // Load from memory into Yamaha.
                        dial = getAdc() >> 10;
                        ptr = memory_map[dial];
                        if (ptr == 255) {
                            error();
                            break;
                        }
                        if (ptr < 128) {
                            led = 1;
                            LATAbits.LATA2 = led;
                            for (unsigned char i = 0; i < 17; i++) {
                                CC[i] = read(ptr++);
                            }
                        } else {
                            preset(ptr);
                        }
                        RC1STAbits.CREN = 0;
                        putch2(0x92);
                        putch2(0x50);
                        putch2(0x50);
                        __delay_ms(1);
                        for (unsigned char i = 0; i < 17; i++) {
                            putch2(0xB2);
                            putch2(CC_numbers[i]);
                            putch2(CC[i]);
                            __delay_ms(1);
                        }
                        putch2(0x82);
                        putch2(0x50);
                        putch2(0x00);
                        RC1STAbits.CREN = 1;
                        led = 1;
                        LATAbits.LATA2 = led;
                        break;
                    case 2:
                        dial = getAdc() >> 10;
                        ptr = memory_map[dial];
                        if (ptr > 127) {
                            error();
                            break;
                        }
                        for (unsigned char i = 0; i < 17; i++) {
                            write(ptr++, CC[i]);
                        }
                        break;
                }
            }
            prevButton = button;
        }
        value = getch();  // Wait for next MIDI byte
        if (value >= 0x80) {
            // Status byte
            state = value;
        } else {
            switch(state & 0xF0) {
                case 0xF0:
                    break;
                case 0xB0: // Control Change
                    led = !led;
                    LATAbits.LATA2 = led;
                    cc = value;
                    value = getch();
                    //store received parameters in active RAM
                    for (unsigned char i = 0; i < 17; i++) {
                      if (cc == CC_numbers[i]) {
                        CC[i] = value;
                      }
                    }
                    break;
                case 0x90:
                    value = getch();                    
                    break;
                default:
                    getch();
                    break;
            }
        }
    }
}
