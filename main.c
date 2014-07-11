#define F_CPU 16000000UL
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "ov7670.h"
static inline void serialWrB(uint8_t dat){
	while(!( UCSR0A & (1<<UDRE0)));//wait for byte to transmit
	UDR0=dat;
	while(!( UCSR0A & (1<<UDRE0)));//wait for byte to transmit
}
static void StringPgm(char * str){
	do{
		serialWrB(pgm_read_byte_near(str));
	}while(pgm_read_byte_near(++str));
}
static void captureImg(uint16_t wg,uint16_t hg){
	//first wait for vsync it is on pin 3 (counting from 0) portD
	uint16_t lg2;
	uint8_t x;
	/*StringPgm(PSTR("REG"));
	for(x=0;x<=0xC9;++x)
		serialWrB(rdReg(x));*/
	StringPgm(PSTR("RDY"));
	while(!(PIND&8));//wait for high
	while((PIND&8));//wait for low
	while(hg--){
		lg2=wg;
		while(lg2--){
			while((PIND&4));//wait for low
			UDR0=(PINC&15)|(PIND&240);
			while(!(PIND&4));//wait for high
		}
	}
}
int main(void){
	cli();//disable interupts
	/* Setup the 8mhz PWM clock 
	 * This will be on pin 11*/
	DDRB|=(1<<3);//pin 11
	ASSR &= ~(_BV(EXCLK) | _BV(AS2));
	TCCR2A=(1<<COM2A0)|(1<<WGM21)|(1<<WGM20);
	TCCR2B=(1<<WGM22)|(1<<CS20);
	OCR2A=0;//(F_CPU)/(2*(X+1))
	DDRC&=~15;//low d0-d3 camera
	DDRD&=~252;//d7-d4 and interupt pins
	_delay_ms(3000);
	//set up twi for 100khz
	TWSR&=~3;//disable prescaler for TWI
	TWBR=72;//set to 100khz
	//enable serial
	//UBRR0H = (unsigned char)(MYUBRR>>8);
	//UBRR0L = (unsigned char)MYUBRR&255;
	UBRR0H=0;
	UBRR0L=1;//3 = 0.5M 2M baud rate = 0 7 = 250k 207 is 9600 baud rate
	UCSR0A|=2;//double speed aysnc
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);//Enable receiver and transmitter
	UCSR0C=6;//async 1 stop bit 8bit char no parity bits
	camInit();
	setRes(vga);
	setColor(bayerRGB);
	wrReg(0x11,25);/* If you are not sure what value to use here for the divider
	* run the commeted out test below and pick the smallest value that gets a correct image */
	while (1){
		/* captureImg operates in bytes not pixels in some cases pixels are two bytes per pixel
		 * So for the width (if you were reading 640x480) you would put 1280 if you are reading yuv422 or rgb565 */
		/*uint8_t x=63;
		do{
			wrReg(0x11,x);
			_delay_ms(1000);
			captureImg(640,480);
		}while(--x);*/
		captureImg(640,480);
	}
}
