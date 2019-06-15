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
volatile uint8_t ucActualBufferPos = 0;

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
	
#define SQUELCH_OFFSET	10
#define ZERO_LEVEL		128
#define PEAK_OFFSET		90
#define PEAK_TOLERANCE	8

volatile uint8_t ucPeakDelay = 0;
volatile uint8_t ucSampleCounter = 0;
volatile uint8_t ucLastSampleValue;
volatile uint8_t ucLockPosition = 0;


//#define TEST_CASE 1;

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

void vTask_DMAHandler(void *pvParameters) 
{
	/*Do things and Stuff with DMA!*/
	PORTF.DIRSET = PIN1_bm; /*LED1*/
	PORTF.DIRSET = PIN2_bm; /*LED2*/
	uint8_t freq = 0;
	static uint8_t ucState = SEARCHING;
	int8_t errorCorrection = 0;
	
	static uint8_t ucSymbolCounter = 0;
	static uint8_t ucDataByte = 0;
	uint8_t ucSymbol;
	
	while(1)
	{
		PORTF.OUTTGL = 0x04;
		
		switch (ucState)
		{
			case SEARCHING:
				freq = xSearchCarrierFreq();
				if(freq) 
				{
					errorCorrection = -(ucSampleCounter - freq);
					ucState = LOCKED;
				}
			break;
			case LOCKED:
				ucSymbol = xGetNextSymbolPeak(16, errorCorrection);
				if(ucSymbol == 0xFF)
				{
					//RESYNC
					ucState = SEARCHING;
					ucSymbolCounter = 0;
					ucDataByte = 0;
				}
				else
				{
					ucDataByte |= xGetNextSymbolPeak(16, errorCorrection);
					errorCorrection = 0;	
					if(ucSymbolCounter != 3) ucDataByte = ucDataByte << 2;
					ucSymbolCounter++;
					if(ucSymbolCounter == 4)
					{					
						ucSymbolCounter = 0;
						//SEND BYTE
					}
				}
			break;
		}
	}
}

int8_t xGetCorrection(uint8_t lastValue, uint8_t nextValue, uint8_t Value, uint8_t tolerance)
{
	if(Value > 128)
	{		
		if(nextValue > (Value + tolerance)) return 2;
		if(nextValue < (Value - tolerance)) return -2;
	}
	else
	{
		if(nextValue < (Value - tolerance)) return 2;
		if(nextValue > (Value + tolerance)) return -2;
	}
	return 0;	
}


uint8_t xGetNextSymbolPeak(uint8_t freq, int8_t error_correction)
{
	static int8_t cCorrection = 0;
	uint8_t ucValue;
	uint8_t lastValue;
	uint8_t nextValue;
	uint8_t signalState;

	lastValue = xGetValueAtOffset( (((freq*2)-2) + cCorrection) + error_correction);
	ucValue = xGetNextValue();
	nextValue = xGetNextValue(); 
	
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
			return 0xFF;		
		break;
	}
}

/*
uint8_t xGetNextSymbol(uint8_t ucFreqTicks)
{
	static uint8_t ucLastPeakLow = 0;
	static uint8_t ucLastPeakHigh = 0;
	static int8_t cCorrection = 0;
	uint8_t ucValue;
	uint8_t ucSignalState;
	
	
	ucValue = xGetValueAtOffset(ucFreqTicks);
	
	//Wait for the next Period!
	while(ucSampleCounter != (ucFreqTicks * 2) + cCorrection)
	{
		ucValue = xGetNextValue();
	}
	ucSampleCounter = 0;
	
	ucSignalState = xCheckLevel(ucValue);
	
	switch (ucSignalState)
	{
		case HIGH_PEAK:
			ucLastPeakHigh = ucValue;
			return 0x0;
		break;
		case LOW_PEAK:
			ucLastPeakLow = ucValue;
			return 0x1;
		break;
		case IDLE_SIGNAL:
			while(ucSampleCounter != ucFreqTicks)
			{
				ucValue = xGetNextValue();
			}
			ucSignalState = xCheckLevel(ucValue);
			if(ucSignalState == HIGH_PEAK) return 0x2;
			if(ucSignalState == LOW_PEAK) return 0x3;
			
			//wait additional 16 Ticks 
		break;
		case OUT_OF_RANGE:
			return 0xFF;
		break;

	}
	
	
	//if(ucValue > () )	
}
*/

uint8_t xSearchCarrierFreq()
{
	uint8_t ucValue;
	static uint8_t ucState = STATE_SEARCH;

	ucValue = xGetNextValue();	
	switch (ucState)
	{
		case STATE_SEARCH:
			//Waiting for High-Trend
			if( xCheckSquelch(ucValue) == HIGH_PEAK )
			{
					ucState = STATE_SEARCHHIGHPEAK;
					ucLastSampleValue = 0;
					ucPeakDelay = 0;		
			}
		break;
		
		case STATE_SEARCHHIGHPEAK:
			if(ucValue > (ZERO_LEVEL+PEAK_OFFSET) )
			{ 
				if(ucValue > ucLastSampleValue) 
				{
					ucSampleCounter = 0;
					ucLastSampleValue = ucValue;
				}
			}
			else
			{
				if(ucLastSampleValue != 0)
				{
					ucState = STATE_SEARCHLOWPEAK;					
				}				
			}
		break;
		
		case STATE_SEARCHLOWPEAK:
			if(ucValue < (ZERO_LEVEL-PEAK_OFFSET) )
			{
				if(ucValue < ucLastSampleValue)
				{
					ucPeakDelay = ucSampleCounter;
					ucLastSampleValue = ucValue;
				}
			}
			else
			{
				if(ucPeakDelay != 0)
				{
					return ucPeakDelay;
					//ucState = STATE_SEARCHLOWPEAK;
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
	#ifdef TEST_CASE 
		ucActualBufferPos += offset;
		return testBuffer[ucActualBufferPos++];
	#elseif
	if((ucActualBufferPos + offset) < DMA_BUF_LEN)
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
	#endif
}

uint8_t xGetNextValue()
{
	#ifdef TEST_CASE
		ucSampleCounter++;
		return testBuffer[ucActualBufferPos++];
	#elseif
	if(ucActualBufferPos == DMA_BUF_LEN - 1)
	{
		vSwitchBuffer();
		ucActualBufferPos = 0;
	}
	#endif
	
	ucSampleCounter++;
	if(ucActiveBuffer == DMA_EVT_GRP_BufferA) return buffer_a[ucActualBufferPos++];
	else return buffer_b[ucActualBufferPos++];		
	
}

void vSwitchBuffer()
{
	if(ucActiveBuffer == DMA_EVT_GRP_BufferA)
	{
		ucActiveBuffer = DMA_EVT_GRP_BufferB;
		vWaitForBuffer(DMA_EVT_GRP_BufferB);
	}
	else
	{
		ucActiveBuffer = DMA_EVT_GRP_BufferA;
		vWaitForBuffer(DMA_EVT_GRP_BufferA);
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