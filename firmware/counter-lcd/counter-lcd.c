#include <counter-lcd.h>
#include <soft_lcd.c>

unsigned int t0of;  // T0 overflow, 256*presc counts
unsigned int t1of;  // T1 overflow, to keep track of time
unsigned int ready; // new measure is ready

#int_TIMER0
void TIMER0_isr(void) 
{
	t0of++;
}
 
#int_TIMER1
void TIMER1_isr(void) 
{
	t1of++;

	if (t1of < MIN_T1_OF) return;
	if (get_timer0() >= MIN_COUNT || 
	    t1of         >= MAX_T1_OF) 
	    ready = 1;
}

/* Display welcome message while counting for the first time */
void welcome_msg(lcd_t *lcd) 
{
	char lcd_lines[36];
	sprintf(lcd_lines, lcd_initm);
	lcd_pos(lcd,0,0);
	lcd_print(lcd, &lcd_lines);
}

/* Fill LCD from CPM.
   Calculate Sv/h, Sv/a and accumulated dose. 
   +--------------------+
   |Exp:   xxxxxxx CPM  |
   |Dosis:  xxx.xx uSv/h|
   |        xxx.xx mSv/a|
   |Acc:    xxx.xx mSv  |
   +--------------------+
*/
void fill_lcd(lcd_t *lcd, unsigned int32 count, unsigned int32 us) 
{
		unsigned int32 cpm;
		static unsigned int32 oldcpm = 0;
		static unsigned int32 totalcount = 0;
		float svh; char mult_svh;
		float sva; char mult_sva;
		float sv;  char mult_sv;
		char lcd_line[22]; // 20*4 lcd
		
		totalcount += count;
		
		/* Calculate CPM (exponential moving average) */
		cpm = 60*1e6*count/us;
		cpm = (ALPHA*cpm+(100-ALPHA)*oldcpm)/100;
		
		oldcpm = cpm;

		/* Exposure per Hour */
		mult_svh = 0xE4; // micro		
		svh = (float)cpm / CPM_PER_USVH;
		
		if (svh >= 1000) {
			svh /= 1000;
			mult_svh = 'm';
		}

		/* Exposure per Year */
		//24*365/1000 = 8.76
		mult_sva = 'm'; // mili
		sva = 8.76*(float)cpm / CPM_PER_USVH;
		
		if (sva >= 1000) {
			sva /= 1000;
			mult_sva = ' ';
		}
		
		/* Total dosage */
		mult_sv = 0xE4; // micro
		sv = 1. * totalcount / CPM_PER_USVH;
		
		if (sv >= 1000) {
			sv /= 1000;
			mult_sv = 'm';
		}
		
		/* Draw LCD lines */
		lcd_pos(lcd,0,0);
		sprintf(lcd_line, lcd_line1, cpm);
		lcd_print(lcd, &lcd_line);

		lcd_pos(lcd,1,0);
		sprintf(lcd_line, lcd_line2, svh, mult_svh);
		lcd_print(lcd, &lcd_line);

		lcd_pos(lcd,2,0);
		sprintf(lcd_line, lcd_line3, sva, mult_sva);
		lcd_print(lcd, &lcd_line);

		lcd_pos(lcd,3,0);
		sprintf(lcd_line, lcd_line4, sv,  mult_sv);
		lcd_print(lcd, &lcd_line);
}

void main() 
{
	lcd_t lcd;
	lcd_create(&lcd, 0x27, 4);

	/* Initialization */	
	setup_adc_ports(NO_ANALOGS);
	setup_timer_0(RTCC_EXT_L_TO_H|T0_DIV_1);
	setup_timer_1(T1_INTERNAL|T1_DIV_BY_8);
	enable_interrupts(INT_TIMER0);
	enable_interrupts(INT_TIMER1);
	enable_interrupts(GLOBAL);

	int initmsg = 1; //initial message is on lcd
	welcome_msg(&lcd);
	
	/* Main loop */
	while(TRUE)
	{
		unsigned int32 us;      // elapsed time
		unsigned int32 count;   // events counted
		
		/* Reset counters */
		ready = 0;
		t0of = 0;
		t1of = 0;
		
		/* Enable counters */
		set_timer1(0);
		enable_interrupts(INT_TIMER1);
		set_timer0(0);
		enable_interrupts(INT_TIMER0);

		/* Go! */
		input(PIN_COUNTER);
		
		while (!ready);
	
		/* Disable counters */
		output_low(PIN_COUNTER);
		clear_interrupt(INT_TIMER0);
		clear_interrupt(INT_TIMER1);
		
		/* Retrieve and display data */
		us    = (int32)t1of * 256*256*4+1;
		count = (int32)t0of * 256+get_timer0();

		if (initmsg) {
			lcd_clear(&lcd);
			initmsg = 0;
		}
		
		fill_lcd(&lcd, count, us);
	}

}
