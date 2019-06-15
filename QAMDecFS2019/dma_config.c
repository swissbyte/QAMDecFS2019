
/*
 * dma_config.c
 *
 * Created: 08.06.2019 08:50:00
 *  Author: Tobias Liesching
 */ 

#include "stdint.h"
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"
#include "dma_config.h"
#include "double_buffer_read_out.h"

#include "FreeRTOS.h"
#include "task.h"
#include "errorHandler.h"

uint8_t buffer_length = 32;

//Initialisiert den ADC im Freerunning Mode
void sys_InitADC(void)
{
	
	// Free Running mode: On
	// Conversion mode: Unsigned, 8Bit
	ADCB.CTRLB=(ADCB.CTRLB & (~(ADC_CONMODE_bm | ADC_FREERUN_bm | ADC_RESOLUTION_gm))) | ADC_RESOLUTION_12BIT_gc; // | ADC_FREERUN_bm;
	// Reference 1V and configuration of prescaler to 256
	ADCB.PRESCALER=(ADCB.PRESCALER & (~ADC_PRESCALER_gm)) | ADC_PRESCALER_DIV32_gc; //clockdivider to 16 or 32, since adc max. is 2MHz
	ADCB.REFCTRL = ADC_REFSEL_INT1V_gc;			//internal 1V , maybe INTVCC to take for bigger measure range

	// Read and save the ADC offset using channel 0
	ADCB.CH0.CTRL=(ADCB.CH0.CTRL & (~(ADC_CH_START_bm | ADC_CH_GAIN_gm | ADC_CH_INPUTMODE_gm))) | ADC_CH_INPUTMODE_SINGLEENDED_gc;
	ADCB.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc ;	// PORTB:0
	// ADCB.CH0.INTCTRL = ADC_CH_INTMODE_COMPLETE_gc|ADC_CH_INTLVL0_bm;	
	
	//ADCB.CH1.CTRL=(ADCB.CH1.CTRL & (~(ADC_CH_START_bm | ADC_CH_GAIN_gm | ADC_CH_INPUTMODE_gm))) | ADC_CH_INPUTMODE_SINGLEENDED_gc;
	//ADCB.CH1.MUXCTRL = ADC_CH_MUXPOS_PIN1_gc ;	// PORTB:1	
	
	//ADCB.CH2.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_INTERNAL_gc;
	//ADCB.CH2.MUXCTRL = ADC_CH_MUXINT_TEMP_gc;  //Temp Mux
	
	ADCB.EVCTRL = ADC_EVSEL_7_gc | ADC_EVACT_CH0_gc;  /* First event triggers channel 0 */
	
	// Enable the ADC in order to read the offset
	ADCB.CTRLA|=ADC_ENABLE_bm;
}

void vInitDMA()
{
	//ADC8 PB0 Input
	PORTB.DIRCLR = PIN0_bm;
	PORTB.DIRCLR = PIN1_bm;
	// set TCC1 to 11024Hz overflow, actually 11019.2838Hz (-0.052% error)
	TCC1.CTRLA = 0; // stop if running
	TCC1.CNT = 0;
	TCC1.PER = 16000-1;

			
	sys_InitADC();
	//double_buff_synch();

	EVSYS.CH7MUX = EVSYS_CHMUX_TCC1_OVF_gc; // trigger on ch7


	// reset DMA controller
	DMA.CTRL = 0;
	DMA.CTRL = DMA_RESET_bm;
	while ((DMA.CTRL & DMA_RESET_bm) != 0);
	
	DMA.CTRL			= DMA_CH_ENABLE_bm | DMA_DBUFMODE_CH01_gc; // double buffered with channels 0 and 1
	
	//Bei Double Buffering wird automatisch aus Channel 0 und 1 ein "Pair" gebildet. 
	//Siehe dazu AVR1304.P8
	
	// channel 0
	// **** TODO: reset dma channels
	DMA.CH0.REPCNT		= 0;
	DMA.CH0.CTRLA		=  DMA_CH_BURSTLEN_2BYTE_gc | DMA_CH_SINGLE_bm | DMA_CH_REPEAT_bm; // ADC result is 1 byte (8 bit word)
	DMA.CH0.CTRLB		= DMA_CH_TRNINTLVL_LO_gc; // set interrupt when burs length reached
	DMA.CH0.ADDRCTRL	= DMA_CH_SRCRELOAD_BURST_gc | DMA_CH_SRCDIR_INC_gc | // reload source after every burst
	DMA_CH_DESTRELOAD_TRANSACTION_gc | DMA_CH_DESTDIR_INC_gc; // reload destination after every transaction
	DMA.CH0.TRIGSRC		= DMA_CH_TRIGSRC_ADCB_CH0_gc;	//DMA0 gets synched by ADCB CH0
	DMA.CH0.TRFCNT		= 32; // always the number of bytes, even if burst length > 1
	DMA.CH0.DESTADDR0	= (( (uint16_t) buffer_a) >> 0) & 0xFF;
	DMA.CH0.DESTADDR1	= (( (uint16_t) buffer_a) >> 8) & 0xFF;
	DMA.CH0.DESTADDR2	= 0;
	DMA.CH0.SRCADDR0	= (( (uint16_t) &ADCB.CH0.RES) >> 0) & 0xFF;
	DMA.CH0.SRCADDR1	= (( (uint16_t) &ADCB.CH0.RES) >> 8) & 0xFF;
	DMA.CH0.SRCADDR2	= 0;

	// channel 1
	DMA.CH1.REPCNT		= 0;
	DMA.CH1.CTRLA		= DMA_CH_BURSTLEN_2BYTE_gc | DMA_CH_SINGLE_bm | DMA_CH_REPEAT_bm; // ADC result is 1 byte (8 bit word)
	DMA.CH1.CTRLB		= DMA_CH_TRNINTLVL_LO_gc; // set interrupt when burs length reached
	DMA.CH1.ADDRCTRL	= DMA_CH_SRCRELOAD_BURST_gc | DMA_CH_SRCDIR_INC_gc | // reload source after every burst
	DMA_CH_DESTRELOAD_TRANSACTION_gc | DMA_CH_DESTDIR_INC_gc; // reload destination after every transaction
	DMA.CH1.TRIGSRC		= DMA_CH_TRIGSRC_ADCB_CH0_gc; //DMA1 gets synched by ADCB CH0
	DMA.CH1.TRFCNT		= 32; // always the number of bytes, even if burst length > 1
	DMA.CH1.DESTADDR0	= (( (uint16_t) buffer_b) >> 0) & 0xFF;
	DMA.CH1.DESTADDR1	= (( (uint16_t) buffer_b) >> 8) & 0xFF;
	DMA.CH1.DESTADDR2	= 0;
	DMA.CH1.SRCADDR0	= (( (uint16_t) &ADCB.CH0.RES) >> 0) & 0xFF;
	DMA.CH1.SRCADDR1	= (( (uint16_t) &ADCB.CH0.RES) >> 8) & 0xFF;
	DMA.CH1.SRCADDR2	= 0;


	
	DMA.CH0.CTRLA		|= DMA_CH_ENABLE_bm; //enable DMA channel 0
	DMA.CH1.CTRLA		|= DMA_CH_ENABLE_bm; //enable DMA channel 1
	

	/*Activate Timer */

			TCC1.CTRLA			= TC_CLKSEL_DIV1_gc; // start timer, and in turn ADC
			TCC1.INTCTRLA		= TC_OVFINTLVL_LO_gc;
	
	PMIC_CTRL = PMIC_LOLVLEN_bm;
	//sei();
}

ISR(TCC1_OVF_vect)
{
	static int count;
	if (count == 32)
	{
		count = 0;
	}
	count++;
}


ISR(DMA_CH0_vect)
	{
 		DMA.INTFLAGS = DMA_CH0TRNIF_bm;

		//Interrupt quittieren
		DMA.CH0.CTRLB |= 0x10;
		TCC1.INTFLAGS |= 0x01;
		//PORTF.OUTTGL = 0x01;
		//PORTF.OUT = 0x00;
		
		BaseType_t xHigherPriorityTaskWoken, xResult;

		/* xHigherPriorityTaskWoken must be initialised to pdFALSE. */
		xHigherPriorityTaskWoken = pdFALSE;

		/* Set bit 0 and bit 4 in xEventGroup. */
		xResult = xEventGroupSetBitsFromISR(
									xDMAProcessEventGroup,   /* The event group being updated. */
									DMA_EVT_GRP_BufferA, /* The bits being set. */
									&xHigherPriorityTaskWoken );

		/* Was the message posted successfully? */
		if( xHigherPriorityTaskWoken != pdFAIL )
		{
			/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
			switch should be requested.  The macro used is port specific and will
			be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
			the documentation page for the port being used. */
			//portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			//portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
		}	
	}

ISR(DMA_CH1_vect)
	//else if (!(DMA.INTFLAGS & DMA_CH0TRNIF_bm))
{
		//Interrupt quittieren
		DMA.CH1.CTRLB |= 0x10;
		TCC1.INTFLAGS |= 0x01;
		PORTF.OUTTGL = 0x01;

		
		BaseType_t xHigherPriorityTaskWoken, xResult;

		/* xHigherPriorityTaskWoken must be initialised to pdFALSE. */
		xHigherPriorityTaskWoken = pdFALSE;

		/* Set bit 0 and bit 4 in xEventGroup. */
		xResult = xEventGroupSetBitsFromISR(
									xDMAProcessEventGroup,   /* The event group being updated. */
									DMA_EVT_GRP_BufferB, /* The bits being set. */
									&xHigherPriorityTaskWoken );

		/* Was the message posted successfully? */
		if( xHigherPriorityTaskWoken != pdFAIL )
		{
			/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
			switch should be requested.  The macro used is port specific and will
			be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
			the documentation page for the port being used. */
			//portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
			//portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
		}	
}
