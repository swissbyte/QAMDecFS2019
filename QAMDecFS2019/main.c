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
	
	xTaskCreate( vProtocolHandlerTask, (const char *) "ProtocolHandlerTask", configMINIMAL_STACK_SIZE+1000, NULL, 1, NULL);
	xTaskCreate( vMenu, (const char *) "Menu", configMINIMAL_STACK_SIZE, NULL, 1, &xMenu);
	xTaskCreate( vOutput, (const char *) "IMU", configMINIMAL_STACK_SIZE, NULL, 1, &xIO);
	xTaskCreate( vTestpattern, (const char *) "IMU", configMINIMAL_STACK_SIZE, NULL, 1, &xTestpattern);
	
		
	xData = xQueueCreate( 10, sizeof(uint8_t) );
		
	xSettingKey = xSemaphoreCreateMutex(); //Create Lock
	xStatusKey = xSemaphoreCreateMutex(); //Create Lock
	
	vDisplayClear();
	vDisplayWriteStringAtPos(0,0,"FreeRTOS 10.0.1");
	vDisplayWriteStringAtPos(1,0,"EDUBoard 1.0");
	vDisplayWriteStringAtPos(2,0,"Template");
	vDisplayWriteStringAtPos(3,0,"ResetReason: %d", reason);
	vTaskStartScheduler();
	return 0;
}
