#include <16F88.h>

#FUSES NOBROWNOUT            	//No brownout reset
#FUSES NOLVP                 	//No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
#FUSES PUT                   	//Power Up Timer
#FUSES NOMCLR                  	//Master Clear pin disabled

#use delay(internal=8MHz)

#define PIN_COUNTER PIN_A4
#define PIN_SDA     PIN_B1
#define PIN_SCL     PIN_B4
#define PIN_TX      PIN_B5

//#use rs232(baud=9600,parity=N,xmit=PIN_TX,rcv=PIN_B2,bits=8)
#use i2c(Master,slow,sda=PIN_SDA,scl=PIN_SCL)

/* Tube radiation sensitivity */
#define CPM_PER_USVH 315.0

/* Decay factor for exponential movil average (in %) */
#define ALPHA 60

/* Minimum count for a valid measure */
#define MIN_COUNT 50

/* Max time for a measure in T1 overflows (262ms each) */
#define MAX_T1_OF 19
#define MIN_T1_OF 4

const char lcd_initm[] = "Termo-Geiger   2019\n\nMidiendo...";
const char lcd_line1[] = "Exp:   %7Lu CPM  ";
const char lcd_line2[] = "Dosis:  %6.2g %cSv/h";
const char lcd_line3[] = "        %6.2g %cSv/a";
const char lcd_line4[] = "Acc:    %6.2g %cSv  ";

