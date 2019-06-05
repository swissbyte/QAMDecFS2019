/*
 * ButtonHandler.c
 *
 * Created: 21.06.2017 12:50:56
 *  Author: mburger
 */ 
 #include <avr/io.h>
 #include "ButtonHandler.h"

 #define BUTTON1_VALUE (PORTF.IN & PIN4_bm) 
 #define BUTTON2_VALUE (PORTF.IN & PIN5_bm) 
 #define BUTTON3_VALUE (PORTF.IN & PIN6_bm) 
 #define BUTTON4_VALUE (PORTF.IN & PIN7_bm) 

 #define BUTTON_PRESS_SHORT			10
 #define BUTTON_PRESS_LONG			500

 /** 
 This is the initialization of the Button main
 @author C.Häuptli
 */

 void initButtons(void) {
	PORTF.DIRCLR = PIN4_bm; //SW1
	PORTF.DIRCLR = PIN5_bm; //SW2
	PORTF.DIRCLR = PIN6_bm; //SW3
	PORTF.DIRCLR = PIN7_bm; //SW4
 }

 button_press_t b1Status;
 button_press_t b2Status;
 button_press_t b3Status;
 button_press_t b4Status;

/** 
 This is the main updateButton function
 @author C.Häuptli
 */
 void updateButtons(void) {
	static uint16_t usButton1Counter = 0;
	static uint16_t usButton2Counter = 0;
	static uint16_t usButton3Counter = 0;
	static uint16_t usButton4Counter = 0;
	if(BUTTON1_VALUE == 0) 
	{
		if(usButton1Counter < 60000) 
		{
			usButton1Counter++;
		}
	} 
	else 
	{
		if(usButton1Counter > (BUTTON_PRESS_SHORT / (1000/BUTTON_UPDATE_FREQUENCY_HZ))) 
		{
			//Button was pressed Short	
			b1Status = ePRESSED;
		}
		else 
		{
			b1Status = eNOT_PRESSED;
		}
		usButton1Counter = 0;
	}
	if(BUTTON2_VALUE == 0) {
		if(usButton2Counter < 60000) {
			usButton2Counter++;
		}
	} 
	else 
	{
		if(usButton2Counter > (BUTTON_PRESS_SHORT / (1000/BUTTON_UPDATE_FREQUENCY_HZ))) 
		{
				//Button was pressed Short
				b2Status = ePRESSED;
		
		} 
		else 
		{
			b2Status = eNOT_PRESSED;			
		}
		usButton2Counter = 0;
	}
	if(BUTTON3_VALUE == 0) 
	{
		if(usButton3Counter < 60000)
		 {
			usButton3Counter++;
		}
	}
	else 
	{
		if(usButton3Counter > (BUTTON_PRESS_SHORT / (1000/BUTTON_UPDATE_FREQUENCY_HZ))) 
		{
			//Button was pressed Short
			b3Status = ePRESSED;
			
		} 
		else 
		{
			b3Status = eNOT_PRESSED;
		}
		usButton3Counter = 0;
	}
	if(BUTTON4_VALUE == 0)
	 {
		if(usButton4Counter < 60000)
		 {
			usButton4Counter++;
		}
	} 
	else 
	{
		if(usButton4Counter > (BUTTON_PRESS_SHORT / (1000/BUTTON_UPDATE_FREQUENCY_HZ))) 
		{
				//Button was pressed Short
				b4Status = ePRESSED;
	
		} 
		else
		{
			b4Status = eNOT_PRESSED;
		}
		usButton4Counter = 0;
	}
 }

/** 
 This is the main updateButton function
 @author C.Häuptli
 */

 button_press_t getButtonPress(button_t button) {
	switch(button) {
		case eBUTTON1:
			return b1Status;
		break;
		case eBUTTON2:
			return b2Status;
		break;
		case eBUTTON3:
			return b3Status;
		break;
		case eBUTTON4:
			return b4Status;
		break;
		
	}
	return eNOT_PRESSED;
 }