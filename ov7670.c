#define F_CPU 16000000UL
#include <stdint.h>
#include <avr/io.h>
#include <util/twi.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "ov7670.h"
#include "ov7670_regs.h"
static void error_led(void){
	DDRB|=32;//make sure led is output
	while(1){//wait for reset
		PORTB^=32;// toggle led
		_delay_ms(100);
	}
}
static void twiStart(void){
	TWCR=_BV(TWINT)| _BV(TWSTA)| _BV(TWEN);//send start
	while(!(TWCR & (1<<TWINT)));//wait for start to be transmitted
	if((TWSR & 0xF8)!=TW_START)
		error_led();
}
static void twiWriteByte(uint8_t DATA,uint8_t type){
	TWDR = DATA;
	TWCR = _BV(TWINT) | _BV(TWEN);
	while (!(TWCR & (1<<TWINT))) {}
	if ((TWSR & 0xF8) != type)
		error_led();
}
static void twiAddr(uint8_t addr,uint8_t typeTWI){
	TWDR = addr;//send address
	TWCR = _BV(TWINT) | _BV(TWEN);		/* clear interrupt to start transmission */
	while ((TWCR & _BV(TWINT)) == 0);	/* wait for transmission */
	if ((TWSR & 0xF8) != typeTWI)
		error_led();
}
void wrReg(uint8_t reg,uint8_t dat){
	//send start condition
	twiStart();
	twiAddr(camAddr_WR,TW_MT_SLA_ACK);
	twiWriteByte(reg,TW_MT_DATA_ACK);
	twiWriteByte(dat,TW_MT_DATA_ACK);
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);//send stop
	_delay_ms(1);
}
static uint8_t twiRd(uint8_t nack){
	if (nack){
		TWCR=_BV(TWINT) | _BV(TWEN);
		while ((TWCR & _BV(TWINT)) == 0);	/* wait for transmission */
		if ((TWSR & 0xF8) != TW_MR_DATA_NACK)
			error_led();
		return TWDR;
	}else{
		TWCR=_BV(TWINT) | _BV(TWEN) | _BV(TWEA);
		while ((TWCR & _BV(TWINT)) == 0) ; /* wait for transmission */
		if ((TWSR & 0xF8) != TW_MR_DATA_ACK)
			error_led();
		return TWDR;
	}
}
uint8_t rdReg(uint8_t reg){
	uint8_t dat;
	twiStart();
	twiAddr(camAddr_WR,TW_MT_SLA_ACK);
	twiWriteByte(reg,TW_MT_DATA_ACK);
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);//send stop
	_delay_ms(1);
	twiStart();
	twiAddr(camAddr_RD,TW_MR_SLA_ACK);
	dat=twiRd(1);
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);//send stop
	_delay_ms(1);
	return dat;
}
static void wrSensorRegs8_8(const struct regval_list reglist[]){
	uint8_t reg_addr,reg_val;
	const struct regval_list *next = reglist;
	while ((reg_addr != 0xff) | (reg_val != 0xff)){
		reg_addr = pgm_read_byte(&next->reg_num);
		reg_val = pgm_read_byte(&next->value);
		wrReg(reg_addr, reg_val);
		next++;
	}
}
void setColor(uint8_t color){
	switch(color){
		case yuv422:
			wrSensorRegs8_8(yuv422_ov7670);
		break;
		case rgb565:
			wrSensorRegs8_8(rgb565_ov7670);
			{uint8_t temp=rdReg(0x11);
			_delay_ms(1);
			wrReg(0x11,temp);}//according to the linux kernel driver rgb565 PCLK needs re-writting
		break;
		case bayerRGB:
			wrSensorRegs8_8(bayerRGB_ov7670);
		break;
	}
}
void setRes(uint8_t res){
	switch(res){
		case vga:
			wrReg(REG_COM3,0);	// REG_COM3
			wrSensorRegs8_8(vga_ov7670);
		break;
		case qvga:
			wrReg(REG_COM3,4);	// REG_COM3 enable scaling
			wrSensorRegs8_8(qvga_ov7670);
		break;
		case qqvga:
			wrReg(REG_COM3,4);	// REG_COM3 enable scaling
			wrSensorRegs8_8(qqvga_ov7670);
		break;
	}
}
void camInit(void){
	wrReg(0x12, 0x80);
	_delay_ms(100);
	wrSensorRegs8_8(ov7670_default_regs);
	wrReg(REG_COM10,32);//pclk does not toggle on HBLANK
}
