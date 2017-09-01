#include <avr/io.h>
#include <string.h>
#include "io.c"
#include "timer.h"

#define period 1000
#define note 261.63

unsigned char lightPatterns[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02};
unsigned char maxPatterns = 8;
unsigned char nextPattern = 0;
const unsigned char colon = ':';
unsigned char button;
unsigned char statechanging = 0;
unsigned char alarmOn;
unsigned char hour = 0;
unsigned char min = 0;
unsigned char sec = 0;
unsigned char aHour = 0;
unsigned char aMin = 0;
unsigned char aSec = 0;
unsigned char time[32] = "00:00:00        Alarm:  11:11:11";
unsigned char alarm[8];												

// helper functions
void updateTime();
void alarmTime();

enum States {init, setHour, setMin, setSec, incHour, incMin, incSec,
			 AlarmHour, AlarmMin, AlarmSec, incAHour, incAMin, incASec, release} state;

enum AlarmStates {buzzOn, buzzOff, snooze, alarmOff} alarmstate;
enum LightStates {initlight, light, lightsnooze, lightOff} lightstate;


void set_PWM(double frequency) {
	static double current_frequency;
	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; }
		else { TCCR3B |= 0x03; }
		if (frequency < 0.954) { OCR3A = 0xFFFF; }
		else if (frequency > 31250) { OCR3A = 0x0000; }
		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }
		TCNT3 = 0;
		current_frequency = frequency;
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

void Second_Tick()							// updates time every second.
{
	if (button == 4)
	{
		statechanging = 1;
	}
	sec++;
	if (sec > 59)
	{
		min++;
		sec = 0;
		if (min > 59)
		{
			hour++;
			if (hour > 23)
			{
				hour = 0;
			}
			min = 0;
		}
	}
		updateTime();						// updates time and also checks if alarm is same as time.
		LCD_DisplayString(1, time);
}

void Time_Tick()
{
	switch (state)				// State Transitions
	{
		case init:
			if (statechanging == 1)
			{
				state = setHour;
			}
			else
			{
				state = init;
			}
			break;
		case setHour:			// pause the time until done.
			if (button == 0)
			{
				state = setHour;
			}
			else if (button == 1)
			{
				state = setMin;
			}
			else if (button == 2)
			{
				state = incHour;
			}
			else if (button == 4)
			{
				state = AlarmHour;
			}
			else if (button == 0x08)
			{
				state = init;
			}
			break;
		case setMin:
			if (button == 0)
			{
				state = setMin;
			}
			else if (button == 1)
			{
				state = setSec;
			}
			else if (button == 2)
			{
				state = incMin;
			}
			else if (button == 4)
			{
				state = AlarmHour;
			}
			else if (button == 0x08)
			{
				state = init;
			}
			break;
		case setSec:
			if (button == 0)
			{
				state = setSec;
			}
			else if (button == 1)
			{
				state = AlarmHour;
			}
			else if (button == 2)
			{
				state = incSec;
			}
			else if (button == 4)
			{
				state = AlarmHour;
			}
			else if (button == 0x08)
			{
				state = init;
			}
			break;
		case incHour:
			state = setHour;
			break;
		case incMin:
			state = setMin;
			break;
		case incSec:
			state = setSec;
			break;
		case AlarmHour:
			if (button == 0)
			{
				state = AlarmHour;
			}
			else if (button == 1)
			{
				state = AlarmMin;
			}
			else if (button == 2)
			{
				state = incAHour;
			}
			else if (button == 4)
			{
				state = setHour;
			}
			else if (button == 0x08)
			{	
				state = init;
			}
			break;
		case AlarmMin:
			if (button == 0)
			{
				state = AlarmMin;
			}
			else if (button == 1)
			{
				state = AlarmSec;
			}
			else if (button == 2)
			{
				state = incAMin;
			}
			else if (button == 4)
			{
				state = setHour;
			}
			else if (button == 0x08)
			{	
				state = init;
			}
			break;
		case AlarmSec:
			if (button == 0)
			{
				state = AlarmSec;
			}
			else if (button == 1)
			{	
				state = init;
			}
			else if (button == 2)
			{
				state = incASec;
			}
			else if (button == 4)
			{
				state = setHour;
			}
			else if (button == 0x08)
			{
				state = release;
			}
			break;
		case incAHour:
			state = AlarmHour;
			break;
		case incAMin:
			state = AlarmMin;
			break;
		case incASec:
			state = AlarmSec;
			break;
		case release:
			if (button != 0)
			{
				state = release;
			}
			else
			{
				state = init;
			}
			break;
		default:
			state = init;
			break;
	}
	
	switch(state)				// State Actions
	//init, setHour, setMin, setSec, incHour, incMin, incSec,
	//AlarmHour, AlarmMin, AlarmSec, incAHour, incAMin, incASec
	{
		case init:
			statechanging = 0;
			break;
		case incHour:
			if (hour >= 23)
			{
				hour = 0;
			}
			else
			{
				hour++;
			}
			updateTime();						// updates time and also checks if alarm is same as time.
			LCD_DisplayString(1, time);
			break;
		case incMin:
			if (min >= 59)
			{
				min = 0;
			}
			else
			{
				min++;
			}
			updateTime();						// updates time and also checks if alarm is same as time.
			LCD_DisplayString(1, time);
			break;
		case incSec:
			if (sec >= 59)
			{
				sec = 0;
			}
			else
			{
				sec++;
			}
			updateTime();						// updates time and also checks if alarm is same as time.
			LCD_DisplayString(1, time);
			break;
		case incAHour:
			if (aHour >= 23)
			{
				aHour = 0;
			}
			else
			{
				aHour++;
			}
			updateTime();						// updates time and also checks if alarm is same as time.
			LCD_DisplayString(1, time);
			break;
		case incAMin:
			if (aMin >= 59)
			{
				aMin = 0;
			}
			else
			{
				aMin++;
			}
			updateTime();						// updates time and also checks if alarm is same as time.
			LCD_DisplayString(1, time);
			break;
		case incASec:
			if(aSec >= 59)
			{
				aSec = 0;
			}
			else
			{
				aSec++;	
			}
			updateTime();						// updates time and also checks if alarm is same as time.
			LCD_DisplayString(1, time);
			break;
		default:
			break;
	}
}

void LightTick()
{
	switch (lightstate)
	{
		case initlight:
			if (button == 0x08)
			{
				lightstate = lightOff;
			}
			else if (button == 0x10)
			{
				lightstate = lightsnooze;
			}
			else
			{
				lightstate = light;
			}
			break;
		case light:
			if (button == 0x08)
			{
				lightstate = lightOff;
			}
			else if (button == 0x10)
			{
				lightstate = lightsnooze;
			}
			else
			{
				lightstate = (nextPattern > maxPatterns - 2) ? initlight : light;
			}
			break;
		case lightsnooze:
			lightstate = initlight;
			break;
		case lightOff:
			lightstate = initlight;
			break;
		default:
			break;
	}
	switch (lightstate)
	{
		case initlight:
			nextPattern = 0;
			PORTB = lightPatterns[nextPattern];
			break;
		case light:
			nextPattern++;
			PORTB = lightPatterns[nextPattern];
			break;
		case lightsnooze:
			PORTB = 0;
			alarmOn = 0;
			updateTime();
			LCD_DisplayString(1, time);
			break;
		case lightOff:
			PORTB = 0;
			alarmOn = 0;
			break;
	}
}

void AlarmTick()
{
	switch (alarmstate)
	{
		case buzzOn:
			if (button == 0x10)
			{
				alarmstate = snooze;
			}
			else if (button == 0x08)
			{
				alarmstate = alarmOff;
			}
			else
			{
				alarmstate = buzzOff;
			}
			break;
		case buzzOff:
			if (button == 0x10)
			{
				alarmstate = snooze;
			}
			else if (button == 0x08)
			{
				alarmstate = alarmOff;
			}
			else
			{
				alarmstate = buzzOn;
			}
			break;
		case snooze:
			alarmstate = buzzOn;
			break;
		case alarmOff:
			alarmstate = buzzOn;
			break;
		default:
			break;
	}
	switch (alarmstate)
	{
		case buzzOn:
			set_PWM(note);
			break;
		case buzzOff:
			set_PWM(0);
			break;
		case snooze:
			set_PWM(0);
			if(aMin >= 55)
			{
				aMin = 10 - (60 - aMin);
			}
			else
			{
				aMin += 10;
			}
			alarmOn = 0;
			break;
		case alarmOff:
			set_PWM(0);
			alarmOn = 0;
			break;
	}
}

int main(void)
{
    DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	unsigned long secondTickCounter = 1000;
	unsigned char timeTickCounter = 150;
	unsigned char alarmTickCounter = 250;
	
	PWM_on();
	set_PWM(0);
	LCD_init();
	TimerSet(1);
	TimerOn();
	LCD_DisplayString(1, time);
	
    while (1) 
    {
		button = ~PINA & 0x1F;
		if (statechanging == 1 && timeTickCounter >= 150)
		{
			if (alarmOn == 1)
			{
				alarmOn = 0;
			}
			Time_Tick();
			timeTickCounter = 0;
		}
		else if (statechanging == 0 && secondTickCounter >= 1000)
		{
			alarmTime();
			Second_Tick();
			secondTickCounter = 0;
		}
		if (alarmOn == 1 && alarmTickCounter >= 250)
		{
			LightTick();
			AlarmTick();
			alarmTickCounter = 0;
		}
		secondTickCounter++;
		timeTickCounter++;
		alarmTickCounter++;
		while (!TimerFlag);
		TimerFlag = 0;
    }
}

// helper functions
void updateTime()
{
	time[0] = '0' + hour/10;
	time[1] = '0' + hour%10;
	time[3] = '0' + min/10;
	time[4] = '0' + min%10;
	time[6] = '0' + sec/10;
	time[7] = '0' + sec%10;
	time[24] = '0' + aHour/10;
	time[25] = '0' + aHour%10;
	time[27] = '0' + aMin/10;
	time[28] = '0' + aMin%10;
	time[30] = '0' + aSec/10;
	time[31] = '0' + aSec%10;
	time[2] = time[5] = time[26] = time[29] = ':';
}

void alarmTime()
{
	for (int i = 0; i < 8; i++)
	{
		alarm[i] = time[i+24];
	}
	
	if (alarmOn == 0)
	{
		for (int i = 0; i < 8; i++)
		{
			if (alarm[i] != time[i])
			{
				alarmOn = 0;
				return;
			}
		}
			alarmOn = 1;
	}
	return;
}
//****NOTES****
/*										// example code for strcat in AVR
unsigned char* x = "\0";
unsigned char z[15] = "Hello ";
unsigned char y[15] = "World";
strcat(x, z);
strcat(x, y);
LCD_DisplayString(1, x);
*/

/*
hours/10 = first digit of hours
hours%10 = second digit of hours
etc
*/