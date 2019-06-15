
/*
 * read_peaks.c
 *
 * Created: 08.06.2019 08:50:00
 *  Author: Tobias Liesching
 */ 


#include "read_peaks.h"
#include "errorHandler.h"
#include "dma_config.h"

#define peak_array_length 2

uint8_t buffer_length;


void vRead_Peaks(void *pvParameters)
{
	int high_peak_a = 127;
	int high_peak = 127;
	int low_peak_a = 127;
	int low_peak = 127;
	int count_array_a = 0;
	int count_array_b = 0;
	int count_array_position_H = 0, count_array_position_L = 0;
	int position_high_peak_a, position_high_peak_b, position_low_peak_a, position_low_peak_b;
	int peak_array_H[64], peak_array_L[64];
	int sinus_status;		// 0 = rising up; 1 = rising down
	int count_after_peak;
	int i = 0;
	int flag_H;
	int flag_L;
	
	//int count_array_a = 0;
	EventBits_t uxBits;
	BaseType_t xResult;
	for (;;)
	{
		uxBits = xEventGroupWaitBits(
		xSignalProcessEventGroup,								/* The event group being tested. */
		Process_Signal_BufferA | Process_Signal_BufferB,		/* The bits within the event group to wait for. */
		pdTRUE,													/* Bits should be cleared before returning. */
		pdFALSE,												/* Don't wait for both bits, either bit will do. */
		portMAX_DELAY );										/* Wait a maximum for either bit to be set. */
		//process signal values
		if (uxBits & Process_Signal_BufferA)											/* if "BufferA" bit is set, read out bufferA*/
		{
			i = 0;
			for(i=0;i<buffer_length;i++)
			{
				/* HIGH PEAK A*/
				if(count_array_a >= peak_array_length)									/* check if a package is ready to transmit*/
				{
						position_array_H[count_array_position_H] =position_array_H[count_array_position_H];		/*debugging*/
						position_array_L[count_array_position_L] = position_array_L[count_array_position_L];	/*debugging*/
					count_array_a = 0;
					xResult = xEventGroupSetBits(										/* Set bit 0 and bit 4 in xEventGroup. */
					xPhaseDetectionEventGroup,											/* The event group being updated. */
					Process_Phase_detectionA											/* The bits being set. */
					);
				}
				else
				{
					if (buffer_a[i] > 127)
					{
						if (buffer_a[i] > high_peak)									/* after test use buffer_a		//		if buffer bigger than current high_peak*/
						{
							if (buffer_a[i] > 220)
							{
								high_peak = buffer_a[i];								// store new peak
								position_high_peak_a = i;								// store array position of new peak
								flag_H = 1;
								flag_L = 0;
							}
						}
						else
						{
							if (buffer_a[i] > 170 )
							{
								count_after_peak++;
							}
							else if(buffer_a[i] < 170)
							{
								if(flag_H == 1)
								{
									if (count_after_peak > 2)
									{
										peak_array_H[count_array_position_H] = high_peak;
										position_array_H[count_array_position_H] = position_high_peak_a;
										count_array_position_H++;
										count_array_a++;									/* Count up to activate event bit when 16 peaks got collected*/
										high_peak = 127;
										flag_H = 0;
										count_after_peak = 0;
									}
								}
							}
						}
					}
					// LOW PEAK	A
					else
					{
						if (buffer_a[i] < low_peak)										/*if buffer bigger than current high_peak*/
						{
							if (buffer_a[i] < 40)
							{
								low_peak = buffer_a[i];									/* store new peak*/
								position_low_peak_a = i;									/* store array position of new peak*/
								flag_L = 1;
								flag_H = 0;
							}
						}
						else
						{
							if (buffer_a[i] < 85)
							{
								count_after_peak++;
							}
							else if (buffer_a[i] > 88)
							{
								if (flag_L == 1)
								{
									if (count_after_peak > 2)
									{
										peak_array_L[count_array_position_L] = low_peak;
										position_array_L[count_array_position_L] = position_low_peak_a;
										count_array_position_L++;
										count_array_a++;										/* Count up to activate event bit when 16 peaks got collected*/
										low_peak = 127;
										flag_L = 0;
										count_after_peak = 0;
									}
								}
							}
						}
					}
				}
			}
		}
		else if (uxBits & Process_Signal_BufferB)
		{
			for (i=0;i<buffer_length;i++)													/* after test will be variable buffer_length*/
			{
				if(count_array_b >= peak_array_length)											/* check if package ready to send*/
				{
					position_array_H[count_array_position_H] =position_array_H[count_array_position_H];		/*debugging*/
					position_array_L[count_array_position_L] = position_array_L[count_array_position_L];	/*debugging*/
					count_array_b = 0;
					xResult = xEventGroupSetBits(												/* Set bit 0 and bit 4 in xEventGroup. */
					xPhaseDetectionEventGroup,													/* The event group being updated. */
					Process_Phase_detectionB													/* The bits being set. */
					);
				}
				else
				{
					/* HIGH PEAK B */
					if (buffer_b[i] > 127)													/* after test: use variable buffer_b */
					{
						if (buffer_b[i] > high_peak)											/*if buffer bigger than current high_peak*/
						{
							if (buffer_b[i] > 220)
							{
								high_peak = buffer_b[i];										/* store new peak*/
								position_high_peak_a = i;										/* store array position of new peak*/
								flag_H = 1;
								flag_L = 0;
							}
						}
						else
						{
							if (buffer_b[i] > 170 )											
							{
								count_after_peak++;
							}
							else if(buffer_b[i] < 170)
							{
								if(flag_H == 1)
								{
									if (count_after_peak > 2)
									{
										peak_array_H[count_array_position_H] = high_peak;
										position_array_H[count_array_position_H] = position_high_peak_a;
										count_array_position_H++;
										count_array_b++;										/* Count up to activate event bit when 16 peaks got collected*/
										high_peak = 127;
										flag_H = 0;
										count_after_peak = 0;
									}
								}
							}
						}
					}
					/* LOW PEAK	B*/
					else
					{
						if (buffer_b[i] < low_peak)											/*if buffer bigger than current high_peak*/
						{
							if (buffer_b[i] < 40)
							{
								low_peak = buffer_b[i];										/* store new peak*/
								position_low_peak_a = i;										/* store array position of new peak*/
								flag_L = 1;
								flag_H = 0;
							}
						}
						else
						{
							if (buffer_b[i] < 88)
							{
								count_after_peak++;
							}
							else if (buffer_b[i] > 85)
							{
								if (flag_L == 1)
								{
									if (count_after_peak > 2)
									{
										peak_array_L[count_array_position_L] = low_peak;
										position_array_L[count_array_position_L] = position_low_peak_a;
										count_array_position_L++;
										count_array_b++;										/* Count up to activate event bit when 16 peaks got collected*/
										low_peak = 127;
										flag_L = 0;
										count_after_peak = 0;
									}
								}
							}
						}
					}
				}
			}
		}
		//vTaskDelay(100 / portTICK_RATE_MS);
	}
	
}