/*
 * demodulator.c
 *
 * Created: 15.06.2019
 *  Author: C. Hediger
 */ 

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "double_buffer_read_out.h"
#include "dma_config.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

volatile uint8_t ucActiveBuffer = DMA_EVT_GRP_BufferA;
volatile uint16_t ucActualBufferPos = 0;
volatile uint8_t ucStopDMA = 0;

void vWaitForBuffer(uint8_t bufferType);
uint8_t xGetNextValue();
uint8_t xSearchCarrierFreq();
uint8_t xCheckSquelch(uint8_t value);
uint8_t xCheckLevel(uint8_t value);
uint8_t xGetValueAtOffset(uint8_t offset);
uint8_t xGetNextSymbolPeak(uint8_t freq, int8_t error_correction);
void vSwitchBuffer();

enum {STATE_SEARCH, STATE_SEARCHHIGHPEAK, STATE_SEARCHLOWPEAK};
enum {HIGH_PEAK, LOW_PEAK, IDLE_SIGNAL, OUT_OF_RANGE};
	enum {LOCKED, SEARCHING};
	
#define SQUELCH_OFFSET	5
/*
#define ZERO_LEVEL		95
#define PEAK_OFFSET		80
*/

volatile uint8_t ZERO_LEVEL = 127;
volatile uint8_t PEAK_OFFSET = 70;
#define PEAK_TOLERANCE	6

volatile uint16_t ucPeakDelay = 0;
volatile uint16_t ucSampleCounter = 0;
volatile uint8_t ucLastSampleValue;
volatile uint8_t ucLockPosition = 0;

volatile uint8_t ucDataBuffer[32];
volatile uint16_t ucDataCounter = 0;
volatile uint8_t ucZero = 0;

volatile uint16_t ucErrorCount = 0;

volatile uint8_t bufferAReady = 0;
volatile uint8_t bufferBReady = 0;

volatile int8_t cCorrection = 0;

volatile uint8_t freq = 0;

//#define TEST_CASE 1;

/*
uint8_t testBuffer[] = {128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
	
	128,152,176,198,218,234,245,253,
	255,253,245,234,218,198,176,152,
	128,103,79,57,37,21,10,2,
	0,2,10,21,37,57,79,103,
	
	128,103,79,57,37,21,10,2,
	0,2,10,21,37,57,79,103,
	128,152,176,198,218,234,245,253,
	255,253,245,234,218,198,176,152,
	
	128,152,176,198,218,234,245,253,
	255,253,245,234,218,198,176,152,
	128,103,79,57,37,21,10,2,
	0,2,10,21,37,57,79,103,
	
	
	
	0,2,10,21,37,57,79,103,
	128,152,176,198,218,234,245,253,
	255,253,245,234,218,198,176,152,
	128,103,79,57,37,21,10,2,
	
	128,103,79,57,37,21,10,2,
	0,2,10,21,37,57,79,103,
	128,152,176,198,218,234,245,253,
	255,253,245,234,218,198,176,152,
	
	128,103,79,57,37,21,10,2,
	0,2,10,21,37,57,79,103,
	128,152,176,198,218,234,245,253,
	255,253,245,234,218,198,176,152,
	};
*/

// initialize USART0 with 115200 Baud.
void sys_InitUART0(void)
{
	//values calculated according to the Datasheets formula
	USARTC0.BAUDCTRLA = 0x17; USARTC0.BAUDCTRLB = 0xA4;
	// = 115200 Baud
	
	
	//Disable interrupts, just for safety
	USARTC0_CTRLA = 0;
	//8 data bits, no parity and 1 stop bit (8N1)
	USARTC0_CTRLC = USART_CHSIZE_8BIT_gc;
	
	//Enable receive and transmit
	USARTC0_CTRLB = USART_TXEN_bm | USART_RXEN_bm; // And enable high speed mode
	
	USARTC0.CTRLC = (uint8_t) USART_CHSIZE_8BIT_gc | USART_PMODE_DISABLED_gc | false;
	
	// USART0 PC3 TX, PC2 RX
	PORTC.DIRSET = PIN3_bm;
	PORTC.DIRCLR = PIN2_bm;
}

void sys_UARTSendChar(char c)
{
	while( !(USARTC0_STATUS & USART_DREIF_bm) ); //Wait until DATA buffer is empty
	USARTC0_DATA = c;
	//while( !(USARTC0_STATUS & USART_DREIF_bm) ); //Wait until DATA buffer is empty
	//_delay_us(1000);
}

void vTask_DMAHandler(void *pvParameters) 
{
	/*Do things and Stuff with DMA!*/
	PORTF.DIRSET = PIN1_bm; /*LED1*/
	PORTF.DIRSET = PIN2_bm; /*LED2*/
	
	static uint8_t ucState = SEARCHING;
	int8_t errorCorrection = 0;
	
	static uint8_t ucSymbolCounter = 0;
	static uint8_t ucDataByte = 0;
	uint8_t ucSymbol;
	

	
	sys_InitUART0();
	
	//xEventGroupClearBits(xDMAProcessEventGroup,DMA_EVT_GRP_BufferA|DMA_EVT_GRP_BufferB);
	
	while(bufferAReady != 1);
	//while(bufferBReady != 1);
	//ucStopDMA = 1;
	
	
	/*
	while(1)
	{
		if(ucActiveBuffer == DMA_EVT_GRP_BufferB)
		{
			for (ucDataCounter = 0; ucDataCounter != DMA_BUF_LEN; ucDataCounter++)
			{
				sys_UARTSendChar(buffer_a[ucDataCounter]);
			}
		}
		else
		{
			for (ucDataCounter = 0; ucDataCounter != DMA_BUF_LEN; ucDataCounter++)
			{
				sys_UARTSendChar(buffer_b[ucDataCounter]);
			}
		}
		
		vSwitchBuffer();
	}*/
	
	
	while(1)
	{
		
		switch (ucState)
		{
			case SEARCHING:
				freq = xSearchCarrierFreq();
				if( freq > 15) 
				{
					errorCorrection = -(ucSampleCounter - freq) + freq;
					ucState = LOCKED;
				}
			break;
			case LOCKED:
				ucSymbol = xGetNextSymbolPeak(freq, errorCorrection);
				if(ucSymbol == 0xFF)
				{
					//RESYNC
					ucState = SEARCHING;
					ucSymbolCounter = 0;
					ucDataByte = 0;
					ucErrorCount++;
				}
				else
				{
					ucDataByte |= ucSymbol;
					errorCorrection = 0;	
					if(ucSymbolCounter != 3) ucDataByte = ucDataByte << 2;
					ucSymbolCounter++;
					if(ucSymbolCounter == 4)
					{					
						ucSymbolCounter = 0;
						
						ucDataBuffer[ucDataCounter++] = ucDataByte;
						sys_UARTSendChar(ucDataByte);
						ucDataByte = 0;
						if(ucDataCounter == 32)
						{
							
							ucDataCounter = 0;
						}
						
						

						//SEND BYTE
					}
				}
			break;
		}
	}
}

int8_t xGetCorrection(uint8_t lastValue, uint8_t nextValue, uint8_t Value, uint8_t tolerance)
{
	int8_t tmp = 0;
	uint8_t level = 0;
	if(Value > 128)
	{		
		if(Value > 200) tmp = 4;
		if(Value > 210) tmp = 3;
		if(Value > 220) tmp = 2;
		if(Value > 225) tmp = 1; 
		if(Value > 230) tmp = 0; 
		
		if(nextValue > (Value + tolerance))
		{
			 tmp = -tmp;
		}
		if(nextValue < (Value - tolerance))
		{
			// tmp = -tmp;
		}
	}
	else
	{
		if(Value < 50) tmp = 4;
		if(Value < 48) tmp = 3;
		if(Value < 38) tmp = 2;
		if(Value < 28) tmp = 1;
		if(Value < 25) tmp = 0;
		
		if(nextValue < (Value - tolerance)) tmp =  -tmp;
		//if(nextValue > (Value + tolerance)) tmp = -tmp;
	}
	
	if(tmp > 0) freq ++;
	if(tmp < 0) freq --;
	
	sys_UARTSendChar(tmp+10);
	return tmp;	
}


uint8_t xGetNextSymbolPeak(uint8_t freq, int8_t error_correction)
{
	
	uint8_t ucValue;
	uint8_t lastValue;
	uint8_t nextValue;
	uint8_t signalState;

	lastValue = xGetValueAtOffset( (((freq*2)-2) + cCorrection) + error_correction);
	ucValue = xGetNextValue();
	nextValue = xGetNextValue(); 
	
	sys_UARTSendChar(ucValue);

	switch(xCheckLevel(ucValue))
	{
		case HIGH_PEAK:
			cCorrection = xGetCorrection(lastValue,nextValue,ucValue,PEAK_TOLERANCE);
			
			return 0x0;
		break;
		
		case LOW_PEAK:
			cCorrection = xGetCorrection(lastValue,nextValue,ucValue,PEAK_TOLERANCE);
			return 0x1;
		break;
		
		case IDLE_SIGNAL:
			if(lastValue > ucValue) signalState = 0x03;
			else signalState = 0x02;			
			return signalState;
		break;
		
		case OUT_OF_RANGE:
			sys_UARTSendChar(ucActualBufferPos);
			return 0xFF;		
		break;
	}
	
	
}


uint8_t xSearchCarrierFreq()
{
	uint8_t ucValue;
	static uint8_t ucState = STATE_SEARCH;
	static uint8_t ucHighValue = 0;

	ucValue = xGetNextValue();	
	switch (ucState)
	{
		case STATE_SEARCH:
			//Waiting for High-Trend
			//if( xCheckSquelch(ucValue) == HIGH_PEAK )
			//{
					ucState = STATE_SEARCHHIGHPEAK;
					ucLastSampleValue = ucValue;
					ucPeakDelay = 0;		
			//}
		break;
		
		case STATE_SEARCHHIGHPEAK:
			//if(ucValue > (ZERO_LEVEL+PEAK_OFFSET) )
			if(ucValue > (ZERO_LEVEL + SQUELCH_OFFSET))
			{	
				if(ucValue > (ucLastSampleValue)) 
				{
					ucSampleCounter = 0;
					ucLastSampleValue = ucValue;
					ucHighValue = ucValue;
				}
				else
				{
					if(ucValue > xGetNextValue())
					{
						if(ucHighValue != 0)
						{
							ucState = STATE_SEARCHLOWPEAK;
							ucLastSampleValue = ucHighValue;
						}
					}
				}
			}
		break;
		
		case STATE_SEARCHLOWPEAK:
			//if(ucValue < (ZERO_LEVEL-PEAK_OFFSET) )
			//{
			if(ucValue < (ZERO_LEVEL - SQUELCH_OFFSET))
			{
				if(ucValue < (ucLastSampleValue))
				{
					ucPeakDelay = ucSampleCounter;
					ucLastSampleValue = ucValue;
				}
				//}
				else
				{
					if(ucValue < xGetNextValue())
					{
						if(ucPeakDelay != 0)
						{
							//ZERO_LEVEL = (ucHighValue - ucLastSampleValue) / 2;
							//PEAK_OFFSET = ucHighValue-PEAK_TOLERANCE-ZERO_LEVEL;
							ucState = STATE_SEARCH;
							ucHighValue = 0;
							return ucPeakDelay;
							//ucState = STATE_SEARCHLOWPEAK;
						}
					}
				}
			}
		break;		
	}
	return 0;
}

uint8_t xCheckLevel(uint8_t value)
{
	if( (value >= (ZERO_LEVEL+PEAK_OFFSET)) ) return HIGH_PEAK;
	if( (value <= (ZERO_LEVEL-PEAK_OFFSET)) ) return LOW_PEAK;
	
	if( (value < (SQUELCH_OFFSET+ZERO_LEVEL)) && (value > (SQUELCH_OFFSET-ZERO_LEVEL)) ) return IDLE_SIGNAL;
	
	return OUT_OF_RANGE;
}

uint8_t xCheckSquelch(uint8_t value)
{
	if( (value > (SQUELCH_OFFSET+ZERO_LEVEL)) ) return HIGH_PEAK;
	if( (value < (SQUELCH_OFFSET+ZERO_LEVEL)) ) return LOW_PEAK;
	return IDLE_SIGNAL;	
}

uint8_t xGetValueAtOffset(uint8_t offset)
{
	uint8_t ucRemainder = 0;
	ucSampleCounter += offset;
	/*#ifdef TEST_CASE 
		ucActualBufferPos += offset;
		return testBuffer[ucActualBufferPos++];
	#elseif
	*/
	if((ucActualBufferPos + offset) < DMA_BUF_LEN-1)
	{
		ucActualBufferPos += offset;
		if(ucActiveBuffer == DMA_EVT_GRP_BufferA) return buffer_a[ucActualBufferPos++];
		else return buffer_b[ucActualBufferPos++];
	}
	else
	{
		ucRemainder = offset - (DMA_BUF_LEN - ucActualBufferPos);
		vSwitchBuffer();
		ucActualBufferPos = ucRemainder;
		if(ucActiveBuffer == DMA_EVT_GRP_BufferA) return buffer_a[ucActualBufferPos++];
		else return buffer_b[ucActualBufferPos++];
	}
	//#endif
}

uint8_t xGetNextValue()
{
	/*#ifdef TEST_CASE
		ucSampleCounter++;
		return testBuffer[ucActualBufferPos++];
	#elseif*/
	if(ucActualBufferPos == DMA_BUF_LEN - 1)
	{
		vSwitchBuffer();
		ucActualBufferPos = 0;
	}
	//#endif
	
	ucSampleCounter++;
	if(ucActiveBuffer == DMA_EVT_GRP_BufferA) return buffer_a[ucActualBufferPos++];
	else return buffer_b[ucActualBufferPos++];		
	
}

void vSwitchBuffer()
{
	if(ucActiveBuffer == DMA_EVT_GRP_BufferA)
	{
		while(bufferBReady != 1);
		ucActiveBuffer = DMA_EVT_GRP_BufferB;
		//bufferBReady = 0;
		bufferAReady = 0;
		//vWaitForBuffer(DMA_EVT_GRP_BufferB);
	}
	else
	{
		while(bufferAReady != 1);
		ucActiveBuffer = DMA_EVT_GRP_BufferA;
		bufferBReady = 0;
		//bufferAReady = 0;
		//vWaitForBuffer(DMA_EVT_GRP_BufferA);
	}
}

void vWaitForBuffer(uint8_t bufferType)
{
	xEventGroupWaitBits(
	xDMAProcessEventGroup,   /* The event group being tested. */
	bufferType, /* The bits within the event group to wait for. */
	pdTRUE,        /* Bits should be cleared before returning. */
	pdFALSE,       /* Don't wait for both bits, either bit will do. */
	portMAX_DELAY );/* Wait a maximum for either bit to be set. */
}