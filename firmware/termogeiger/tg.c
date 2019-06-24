#include <tg.h>

unsigned int32 xs; // random variable

#INT_TIMER0
void TIMER0_isr(void) 
{
	output_low(OUT_PIN);
	output_low(AUX_PIN);
	disable_interrupts(INT_TIMER0);
}

/* Returns 16bit "random" number using 32 bit XorShift RNG */
long rand16b() {
	xs ^= xs << 13;
	xs ^= xs >> 17;
	xs ^= xs << 5;

    return xs>>16; // 32 -> 16bits
}

/* Returns a value between 0-65535 as a function of ADC level */
unsigned int16 apertura(int8 x) {
	unsigned int32 b = 0;
	unsigned int32 acc = 0;
	unsigned char  i = 0;

	if (x == 0)   return 0;
	if (x >= 255) return 0xFFFF;

	while (app_table_x[i] < x) {i++;}

	acc = app_table_a[i];
	b   = app_table_b[i];

	acc *= x;
	acc >>=1;
	acc -= b;
	acc >>=11;

	if (acc > 0xFFFF) acc = 0xFFFF;

	return acc;
}

/* Seed random generator. Overwrite global var XS. */
void randomize() {
	setup_adc_ports(PIN_A1);
	set_adc_channel(1);
	setup_adc(ADC_CLOCK_DIV_16);
	delay_us(20);
	xs = 0;

	int i=32;
	while (i--) {
		int a = read_adc();
		a &= 1;
		xs <<= 1;
		xs |= a;
	}
	
	if (xs == 0) xs = 1;
	
	printf("Random seed: %Lx\r\n", xs);
}

/* Write apertura values from ADC. */
void print_apertura() {
	for (unsigned long i = 0; i <= 255; i++) {
		printf("%Lu;%Lu\r\n",i,apertura(i));
	}
}

/* --- */
void main () {
	
	/**TEST**/
	/*int32 a;
	char i = 1;
	int32 *aa = app_table_a;
	a=*(aa+3);
	a=apertura(1);
	delay_ms(10);*/
	
	// Setup ADC, timer and interrups
	setup_adc_ports(IN_PIN);
	setup_adc_ports(RND_PIN);
	setup_timer_0(RTCC_INTERNAL|RTCC_DIV_4|RTCC_8_bit);		//512us overflow
	enable_interrupts(GLOBAL);

	puts("Termo-Geiger. Electronica y ciencia. 2019");
	/*print_apertura();*/

	randomize();

	/* Main loop */
	set_adc_channel(3);
	setup_adc(ADC_CLOCK_DIV_16);
	delay_us(20);

	while(true) {
		unsigned long rnd,app;
		unsigned int adc;
		
		/* Read ADC while calculating random number */
		read_adc(ADC_START_ONLY);
		rnd = rand16b();
		adc = read_adc(ADC_READ_ONLY);
		
		/* Calculate opening from ADC value */
		app = apertura(adc);
		//printf("ADC: %3u, App: %5Lu, Rnd: %5Lu\r\n", adc, app, rnd);
		if (rnd < app) {
			output_high(OUT_PIN);
			output_high(AUX_PIN);
			set_timer0(0);
			clear_interrupt(INT_TIMER0);
			enable_interrupts(INT_TIMER0);
		}
	}
}


