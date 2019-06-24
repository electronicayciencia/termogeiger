#include <12F683.h>
#DEVICE ADC=8

#FUSES PUT                   	//Power Up Timer
#FUSES NOMCLR                  	//Master Clear pin disabled

#define RND_PIN  PIN_A1 // Random seed
#define OUT_PIN  PIN_A2 // Green led
#define LED_PIN  PIN_A5 // Red led / RS232
#define IN_PIN   PIN_A3 // NTC
#define AUX_PIN  PIN_A0 // Aux functions/PGD

#use delay(internal=8MHz)
#use rs232(baud=9600,parity=N,xmit=LED_PIN,rcv=PIN_A1,bits=8,stream=PORT1)

/* Const stores values in eeprom, slower to retrieve */
unsigned int8 app_table_x[] = 
{64,144,188,221,245};

unsigned int32 app_table_a[] =
{366,1229,24762,329418,10681344};

unsigned int32 app_table_b[] =
{1463,29082,1723485,30361104,1174248960};
