/*
 * QAMDecFS2019.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : chaos
 * Test Cedric
 */ 
// Projektleiter Philipp Eppler

//#include <avr/io.h>
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

#include "mem_check.h"

#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"
#include "protocolhandler.h"
#include "Menu_IMU.h"

#include "dma_config.h"		
#include "double_buffer_read_out.h"
#include "read_peaks.h"
#include "phase_detection.h"


extern void vApplicationIdleHook( void );


TaskHandle_t ProtocolHandlerTask;




void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
    resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();
	vInitDMA();	
	
	xTaskCreate( vProtocolHandlerTask, (const char *) "ProtocolHandlerTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	//xTaskCreate( vMenu, (const char *) "Menu", configMINIMAL_STACK_SIZE, NULL, 1, &xMenu);
	//xTaskCreate( vOutput, (const char *) "IMU", configMINIMAL_STACK_SIZE, NULL, 1, &xIO);
	//xTaskCreate( vTestpattern, (const char *) "IMU", configMINIMAL_STACK_SIZE, NULL, 1, &xTestpattern);
	//xTaskCreate( vRead_Peaks, (const char *) "read_Peaks", configMINIMAL_STACK_SIZE+100, NULL, 1, NULL);
	//xTaskCreate( vPhase_Detection, (const char *) "phase_detect", configMINIMAL_STACK_SIZE+10, NULL, 1, NULL);
	xTaskCreate( vTask_DMAHandler, (const char *) "dmaHandler", configMINIMAL_STACK_SIZE, NULL, 1, NULL);		
	xSignalProcessEventGroup = xEventGroupCreate();
	xPhaseDetectionEventGroup = xEventGroupCreate();
	xDMAProcessEventGroup = xEventGroupCreate();
	
	PORTF.DIRSET = PIN0_bm; /*LED1*/
	PORTE.DIRSET = PIN1_bm; /*LED1*/	
		
	xData = xQueueCreate( 10, sizeof(uint8_t) );
		
	xSettingKey = xSemaphoreCreateMutex(); //Create Lock
	xStatusKey = xSemaphoreCreateMutex(); //Create Lock
	
	vDisplayClear();
	vDisplayWriteStringAtPos(0,0,"FreeRTOS 10.0.1");
	vDisplayWriteStringAtPos(1,0,"EDUBoard 1.0");
	vDisplayWriteStringAtPos(2,0,"Template2");
	vDisplayWriteStringAtPos(3,0,"ResetReaso45n: %d", reason);
	vTaskStartScheduler();
	return 0;
}
