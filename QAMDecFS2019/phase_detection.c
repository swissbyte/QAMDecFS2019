
/*
 * read_peaks.h
 *
 * Created: 08.06.2019
 *  Author: Tobias Liesching
 */ 
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"
#include "semphr.h"
#include "mem_check.h"

#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

#include "read_peaks.h"

#define	PROTOCOL_BUFFER_SIZE		32

uint8_t ucGlobalProtocolBuffer_A[ PROTOCOL_BUFFER_SIZE ] = {}; // Buffer_A from Demodulator to ProtocolTask
uint8_t ucGlobalProtocolBuffer_B[ PROTOCOL_BUFFER_SIZE ] = {}; // Buffer_B from Demodulator to ProtocolTask

SemaphoreHandle_t xGlobalProtocolBuffer_A_Key;	//A-Resource for ucGlobalProtocolBuffer_A
SemaphoreHandle_t xGlobalProtocolBuffer_B_Key;	//A-Resource for ucGlobalProtocolBuffer_B


void vPhase_Detection(void *pvParameters) // becomes peak array and position -> detects the phase shift
{
	unsigned long int control_result;
	int uxBits = 0;
	int xreference_H,xreference_L,periode_rms;
	int	check_phase_1, check_phase_2;
	int phase_H[32], phase_L[32];
	int phase_detect_results;
	int check_peak_position;
	int i = 0, j = 0;
	uint16_t ucqambit , ucvalue , uci, ucj, ucl, ucm, uca=0, ucb=0, ucx=0, ucy=0;
	char xOutput1, xOutput2;
	for (;;)
	{
		uxBits = xEventGroupWaitBits(
		xPhaseDetectionEventGroup,														/* The event group being tested. */
		Process_Phase_detectionA | Process_Phase_detectionB,							/* The bits within the event group to wait for. */
		pdTRUE,																			/* Bits should be cleared before returning. */
		pdFALSE,																		/* Don't wait for both bits, either bit will do. */
		portMAX_DELAY );																/* Wait a maximum for either bit to be set. */


		/*Phase Detection */
		
		if ((uxBits & Process_Phase_detectionA)||(uxBits & Process_Phase_detectionB))	/* if Bit phase detection is set -> read out peak array*/
		{
			xreference_H = position_array_H[0]*4;
			xreference_L = position_array_L[0]*4/3;
			periode_rms = position_array_H[0] + position_array_L[0];									/*(peak position of referencsignal added together will give the number of sampels for one sinus periode;*/
				
			check_phase_1 = xreference_L - xreference_H;								/*check differences between phase_H and phase_L to prove that no reading error happened*/
			check_phase_2 = xreference_H - xreference_L;
				
			for (i=0; i<PROTOCOL_BUFFER_SIZE; i++)															/*original variable after testing: peak_array_length*/
			{
		
					position_array_H[i] = position_array_H[i]; /* Debugging*/
					position_array_L[i] = position_array_L[i]; /* Debugging*/
					phase_H[i] = (periode_rms*(j+1) - position_array_H[i]);					/* calculates the distance from high peak to start of next periode.*/
					phase_L[i] = (periode_rms*(j+1) - position_array_L[i]);					/* calculates the distance from low peak to start of next periode.*/
					// Phase High detect
					j++;
					if (j == 5)
					{
						j = 0;
					}

			
			
			}
				if (( check_phase_1 < 2 )||( check_phase_2 < 2 ))						/* check if theres no read out error with comparing low peak and high peak position of reference periode.*/
				{
				
					 if(xSemaphoreTake( xGlobalProtocolBuffer_A_Key, portMAX_DELAY )) /* has to get deactivated, if only demodulating code has to be tested */					{
						for (uci = 0; uci < 8; uci++)									/*For Schleife mit 8 Wiederholungen für 8 Byte */
						{
							for (ucj = 0; ucj < 4; ucj++)								/*For Schleife mit 4 Wiederholungen für 8 Bits (1Byte)*/
							{
								ucvalue = phase_H[ucy];									/* result will get calculated from high peak array (low peak array is only used for checking deviations */
								ucy++;
								if((ucvalue < 10) && (ucvalue > 1))	/* 180° */
								{
									ucqambit = 0;
									check_peak_position = ucvalue * 4;
								}
								else if((ucvalue < 15) && (ucvalue > 9)) /* 270° */
								{
									ucqambit = 1;
									check_peak_position = ucvalue * 2;
								}
								else if((ucvalue < 19) && (ucvalue > 14)) /* 0° */
								{
									ucqambit = 2;
									check_peak_position = ucvalue * 4 / 3;
								}
								else if((ucvalue < 24) && (ucvalue > 18)) /* 90° */
								{
									ucqambit = 3;
									check_peak_position = ucvalue;
								}
								xOutput1 = (xOutput1 << 2) | (ucqambit & 0x03);
							}
							ucGlobalProtocolBuffer_A[uca] = xOutput1;
							uca++;
							xOutput1 = 0;
						}
						ucx=0;
						uca=0;
					 xSemaphoreGive(xGlobalProtocolBuffer_A_Key);			/* has to get deactivated, if only demodulating code has to be tested */
					}
					
					if(xSemaphoreTake( xGlobalProtocolBuffer_B_Key, portMAX_DELAY ))
					{
						for (ucl = 0; ucl < 8; ucl++)
						{
							for (ucm = 0; ucm < 4; ucm++)
							{
								ucvalue = phase_H[ucy];									/* result will get calculated from high peak array (low peak array is only used for checking deviations */
								ucy++;
								if((ucvalue < 10) && (ucvalue > 1))	/* 180° */
								{
									ucqambit = 0;
									check_peak_position = ucvalue * 4;
								}
								else if((ucvalue < 15) && (ucvalue > 9)) /* 270° */
								{
									ucqambit = 1;
									check_peak_position = ucvalue * 2;
								}
								else if((ucvalue < 19) && (ucvalue > 14)) /* 0° */
								{
									ucqambit = 2;
									check_peak_position = ucvalue * 4 / 3;
								}
								else if((ucvalue < 23) && (ucvalue > 18)) /* 90° */
								{
									ucqambit = 3;
									check_peak_position = ucvalue;
								}
								xOutput2 = (xOutput2 << 2) | (ucqambit & 0x03);
							}
							ucGlobalProtocolBuffer_B[ucb] = xOutput2;
							ucb++;
							xOutput2 = 0;
						}
						ucy=0;
						ucb=0;
						xSemaphoreGive(xGlobalProtocolBuffer_B_Key);
					}	
				}
				
				/* read out check */
				else																	/* reference signal check failed */
				{
					/*read error is occured, phase_H and phase_L do not correspond to each other */		
					/* make a error function here */	
				}
					
				
				if(check_peak_position > periode_rms)
				{
					periode_rms = periode_rms + 1;										/*if check variable bigger than periode, then decrease periode*/
				}
				else if (check_peak_position < periode_rms)
				{
					periode_rms = periode_rms - 1;										/*if prüf variable bigger than periode, then increase periode*/
				}
		}
		
		
		
		//vTaskDelay(100 / portTICK_RATE_MS);
	}
}