/*
 * tasks.c
 *
 * Created: 08.06.2019 08:50:00
 *  Author: Tobias Liesching
 */ 

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "double_buffer_read_out.h"
#include "dma_config.h"
#include "errorHandler.h"

uint8_t count_buffer_a = 0;
uint8_t count_buffer_b = 0;
uint8_t position_array_a;
uint8_t position_array_b;

/* Test array : To use, add "xtest_array" instead uf "buffer_a" and "buffer_b in read_peaks.c file. Also adjust buffer_length in for loop
			int xtest_array_length = 96;
			int xtest_position_H = 5;
			int xtest_position_L = 15;
			
			//float xtest_array[96] = {127.000000,166.245158,201.648727,229.745158,247.784178,254.000000,247.784178,229.745158,
				//201.648727,201.648727,166.245158,127.000000, 87.754842, 52.351273, 24.254842, 6.215822, 0.000000,6.215822, 24.254842, 52.351273, 87.754842,127.000000,87.754842,
				//52.351273,24.254842,6.215822,0.000000,6.215822,24.254842,52.351273,87.754842,127.000000,166.245158,201.648727,229.745158,247.784178,254.000000,247.784178,229.745158,201.648727,166.245158,
				//0.000000,6.215822,24.254842,52.351273,87.754842,127.000000,166.245158,201.648727,229.745158,247.784178,254.000000,247.784178,229.745158,201.648727,166.245158,127.000000,87.754842,52.351273,
				//24.254842,6.215822,254.00000,0247.784178,229.745158,201.648727,166.245158,127.000000,87.754842,52.351273,24.254842,6.215822,0.000000,6.215822,24.254842,52.351273,87.754842,127.000000,166.245158,
				//201.648727,229.745158,247.784178,127.000000,166.245158,201.648727,229.745158,247.784178,254.000000,247.784178,229.745158,201.648727,166.245158,127.000000,87.754842,52.351273,24.254842,6.215822};
*/

void vTask_DMAHandler(void *pvParameters) 
{
	/*Do things and Stuff with DMA!*/
	
	EventBits_t uxBits;
	BaseType_t xResult;
	int i,n,no_signal_counter = 0;
	PORTF.DIRSET = PIN1_bm; /*LED1*/
	PORTF.DIRSET = PIN2_bm; /*LED2*/
	PORTE.DIRSET = PIN0_bm;
	PORTE.DIRSET = PIN1_bm;

	
	while(1)
	{
		uxBits = xEventGroupWaitBits(
		xDMAProcessEventGroup,   /* The event group being tested. */
		DMA_EVT_GRP_BufferA | DMA_EVT_GRP_BufferB, /* The bits within the event group to wait for. */
		pdTRUE,        /* Bits should be cleared before returning. */
		pdFALSE,       /* Don't wait for both bits, either bit will do. */
		portMAX_DELAY );/* Wait a maximum for either bit to be set. */
			
		//Check Event bits
		if(uxBits & DMA_EVT_GRP_BufferA)
		{
			//Do stuff with BufferA
			//buffer_a ....
			for (i=0;i<buffer_length;i++)//Detect signal 
			{
				if ((xtest_array_length < 135)&(xtest_array_length > 120))		/*after testing variable is buffer_a				// if value is under threshold, it shall stop calculating the signal*/
				{
					no_signal_counter++;							// wait 100 counts to make sure that signal has stopped
					if (no_signal_counter >=100)					// no signal stop calculating -> set event bits to 0
					{											// no signal stop calculating -> set event bits to 0
						xResult = xEventGroupClearBits(			// clear event bits
											xSignalProcessEventGroup,
											Process_Signal_BufferA|Process_Signal_BufferB
											);												
					}
				}
				else
				{
					xResult = xEventGroupSetBits(
					xSignalProcessEventGroup,		/* The event group being updated. */
					Process_Signal_BufferB			/* The bits being set. */
					);
					if( xResult & Process_Signal_BufferB )		//test if Eventgroup bit is set
					{
						//count_array_a++;
					}
					no_signal_counter = 0;
				}
			}
			count_buffer_a++;
			
			//Debug Output
			PORTF.OUT = (PORTF.OUT & (0xFF - 0x02));
			PORTF.OUT |= 0x04;	
		}
		
		else //When it was not DMA_EVT_GRP_BufferA, then it was probably B. Since we only use two bits!
		{						
			//Do stuff with BufferB
			//buffer_b ....
			for (i=0;i<buffer_length;i++)
			{				
				if (buffer_b[i] >= 20)
				{
				xResult = xEventGroupSetBits(
									xSignalProcessEventGroup,   /* The event group being updated. */
									Process_Signal_BufferA		/* The bits being set. */
									);	
				}
				if(xResult & Process_Signal_BufferA)			//test if Eventgroup bit is set
				{
					{
						//count_array_b++;
					}
					count_buffer_a = 0;
				}
				else											// if value is under threshold, it shall stop calculating the signal
				{
					no_signal_counter++;					
					if (no_signal_counter >=100)					// wait 100 counts to make sure that signal has stopped
					{		
											
						xResult = xEventGroupClearBits(			// clear event bits
												xSignalProcessEventGroup,
												Process_Signal_BufferA|Process_Signal_BufferB
												);										// no signal stop calculating -> set event bits to 0
						}
				}
			}
			count_buffer_b++;
			//Debug Output
			PORTF.OUT = (PORTF.OUT & (0xFF - 0x04));
			PORTF.OUT |= 0x02;
		}
	//vTaskDelay(100 / portTICK_RATE_MS);
	}
	
}