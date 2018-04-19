
#define F_CPU 20000000UL
#include <avr/io.h>
#include <avr/interrupt.h>	//割り込みのヘッダ
#include <util/delay.h>

#include "..\..\..\header\BIT_CTRL.cpp"
//#include "..\..\..\header\STRING_CTRL.cpp"
#include "..\..\..\header\UART.cpp"
#include "..\..\..\header\I2C_2class.cpp"
#include "..\..\..\header\MEASURE_PWM_TIME1.cpp"
#include "..\..\..\header\IR2302_TM0.cpp"
//#include "..\..\..\header\IR2302.cpp"

#define IN_PWM_PIN		PIND
#define IN_PWM_PIN_NUM	5
#define IN_I2C_PIN		PIND
#define IN_I2C_PIN_NUM	6
#define IN_UART_PIN		PIND
#define IN_UART_PIN_NUM	7

#define I2C_ADDRESS 10

int main(void){
	cbi(DDRD,7);
	sbi(PORTD,7);
	cbi(DDRD,6);
	sbi(PORTD,6);
	cbi(DDRD,5);
	sbi(PORTD,5);
	int16_t tm0Duty, tm2Duty;

	//ir2302Tm0.setTimer(64);
	gateDriveA.setIn1(&DDRB, &PORTB, 4);
	gateDriveA.setSd1(&DDRB, &PORTB, 3);
	gateDriveA.setIn2(&DDRB, &PORTB, 2);
	gateDriveA.setSd2(&DDRB, &PORTB, 1);

	//ir2302Tm2.setTimer(64);
	gateDriveB.setIn1(&DDRC, &PORTC, 3);
	gateDriveB.setSd1(&DDRC, &PORTC, 2);
	gateDriveB.setIn2(&DDRC, &PORTC, 1);
	gateDriveB.setSd2(&DDRC, &PORTC, 0);

	gateDriveA.begin();
	gateDriveB.begin();

	uart.begin(9600);
	uart.println("Start");

	if( checkbit(IN_PWM_PIN, IN_PWM_PIN_NUM)==0 ){
		uart.println("pwmMode");
		_delay_ms(1000);
		measureInt0.begin(10000,10000);
		measureInt1.begin(10000,10000);

		float int0HighTime,int1HighTime;
		while(1){
			if(measureInt0.highTimeUpdateFlag){
				int0HighTime = measureInt0.getHighUsec();
				if(900<=int0HighTime && int0HighTime<=2100){
					int0HighTime = margeNum(int0HighTime, 1000, 2000);
					if(1400<=int0HighTime && int0HighTime<=1600){	//NEUTRAL
						gateDriveA.setDutyByte( 0 );
					}
					else{	//TURN,RETURN
						tm0Duty = (int16_t)( 255*((int0HighTime-1500)/500) );
						gateDriveA.setDutyByte( tm0Duty );
					}
				}
				else if(2300<=int0HighTime && int0HighTime<=3000){	//BRAKE
					int0HighTime = margeNum(int0HighTime, 2400, 2900);
					tm0Duty = (int16_t)( 255*((int0HighTime-2400)/500) );
					gateDriveA.setDutyByte( tm0Duty , true);
				}
				else{
					gateDriveA.setDutyByte( 0 );
				}
			}

			if(measureInt1.highTimeUpdateFlag){
				int1HighTime = measureInt1.getHighUsec();
				if(900<=int1HighTime && int1HighTime<=2100){
					int1HighTime = margeNum(int1HighTime, 1000, 2000);
					if(1400<=int1HighTime && int1HighTime<=1600){
						gateDriveB.setDutyByte( 0 );
					}
					else{
						tm2Duty = (int16_t)( 255*((int1HighTime-1500)/500) );
						gateDriveB.setDutyByte( tm2Duty );
					}
				}
				else if(2300<=int1HighTime && int1HighTime<=3000){
					int1HighTime = margeNum(int1HighTime, 2400, 2900);
					tm2Duty = (int16_t)( 255*((int1HighTime-2400)/500) );
					gateDriveB.setDutyByte( tm2Duty , true);
				}
				else{
					gateDriveB.setDutyByte( 0 );
				}
			}
		}
	}
	else if( checkbit(IN_UART_PIN, IN_UART_PIN_NUM)==0 ){
		uart.println("uartMode");
	//	uart.begin(9600);
		char cmd[10];
		int8_t success=false;
		int16_t dutyByte=0;
		bool brakeFlag=false;
		while(1){
			success = uart.cmdCheckQueue("MD{01}{+-B}???", cmd);
			if(success==1){
				dutyByte = strToNum(cmd+4, 3);
				dutyByte = margeNum(dutyByte, -255, 255);
				switch(cmd[3]){
					case '+':
						brakeFlag=true;
					break;
					case '-':
						dutyByte *= -1;
						brakeFlag=true;
					break;
					case 'B':
						brakeFlag=true;
					break;
				}
				if(cmd[2]=='0'){
					gateDriveA.setDutyByte( dutyByte , brakeFlag);
				}
				else if(cmd[2]=='1'){
					gateDriveB.setDutyByte( dutyByte , brakeFlag);
				}
			}
		}
	}
	else if( checkbit(IN_I2C_PIN, IN_I2C_PIN_NUM)==0 ){
		uart.println("i2cMode");
		i2c.begin(I2C_ADDRESS);
		char cmd[10];
		int8_t success;
		int16_t dutyByte;
		bool brakeFlag;
		while(1){
			success = i2c.slave.cmdCheckQueue("MD{01}{+-B}???", cmd);
			if(success==1){
				dutyByte = strToNum(cmd+4, 3);
				dutyByte = margeNum(dutyByte, -255, 255);
				switch(cmd[3]){
					case '+':
					brakeFlag=true;
					break;
					case '-':
					dutyByte *= -1;
					brakeFlag=true;
					break;
					case 'B':
					brakeFlag=true;
					break;
				}
				if(cmd[2]=='0'){
					gateDriveA.setDutyByte( dutyByte , brakeFlag);
				}
				else if(cmd[2]=='1'){
					gateDriveB.setDutyByte( dutyByte , brakeFlag);
				}
			}
		}
	}
}

//*/