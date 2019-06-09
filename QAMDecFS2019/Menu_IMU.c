/**
 * Menu_IMU.c
 *
 * This is the main Methode which generate the display output and organize the output.
 * @Author C. Häuptli
 */ 

 #include <avr/io.h>
 #include "Menu_IMU.h"
 

uint32_t ulStatus = 0;				//P-Resource Status
 



/**
* vMenu is responsible for the display output and handles the settings of the buttons
* @param args Unused
* @return Nothing
* @author C.Häuptli
*/

void vMenu(void *pvParameters) {
	(void) pvParameters;
	uint8_t ucSettings = 0;
	uint8_t ucSource = 0;
	uint8_t ucDisplayUpdateTimer= 0;
	uint8_t ucMode= 0;
	uint8_t ucData_to_send = 0;
	
	
	xQAMSettings.ucSettings = 0x02; //Start Settings for QAM

	
	for(;;) 
	{
		updateButtons();
	
	
		/*--------Button 1 is to select the settings--------*/	
		if (getButtonPress(eBUTTON1)== ePRESSED)
		{
			if (ucMode == 1)
			{
				if (ucSettings < 2)
				{
					ucSettings++;
				}
				else
				{
					ucSettings = 0;
				}
			}
		}
		
		/*--------Button 2 is to change the selected setting--------*/			
		if (getButtonPress(eBUTTON2)== ePRESSED)
		{
			if (ucMode == 1)
			{
				if (ucSettings == 0)
				{
					if(xSemaphoreTake(xSettingKey, portMAX_DELAY))
					{
						if (xQAMSettings.bits.bQAM_Order == 1)
						{
							xQAMSettings.bits.bQAM_Order = 0;
						}
						else
						{
							xQAMSettings.bits.bQAM_Order = 1;
						}
						xSemaphoreGive(xSettingKey);
					}
				}
				
				if (ucSettings == 1)
				{
					if (ucSource < 2 )
					{
						ucSource++;
					}
					else
					{
						ucSource = 0;
					}
					if(xSemaphoreTake(xSettingKey, portMAX_DELAY))
					{
						xQAMSettings.bits.bSource_IO = 0;
						xQAMSettings.bits.bSource_Test = 0;
						xQAMSettings.bits.bSource_UART = 0;
						switch (ucSource)
						{
							case 0:
								xQAMSettings.bits.bSource_IO = 1;
								vTaskResume(xIO);
								vTaskSuspend(xTestpattern);
								//vTaskSuspend(xUART)
								break;
							case 1:
								xQAMSettings.bits.bSource_Test = 1;
								vTaskResume(xTestpattern);
								vTaskSuspend(xIO);
								//vTaskSuspend(xUART);
								break;
							case 2:
								xQAMSettings.bits.bSource_UART = 1;
								//vTaskResume(xUART);
								vTaskSuspend(xIO);
								vTaskSuspend(xTestpattern);
								break;
							default:break;
						}
						xSemaphoreGive(xSettingKey);
					}
				}
				
				if (ucSettings == 2)
				{
					if(xSemaphoreTake(xSettingKey, portMAX_DELAY))
					{
						if(xQAMSettings.bits.bFrequency)
						{
							xQAMSettings.bits.bFrequency = 0;
						}
						else
						{
							xQAMSettings.bits.bFrequency = 1;
						}
						xSemaphoreGive(xSettingKey);
					}
				}
			}
		}
	
		/*--------Button 3 is to select the Mode --------*/
		if (getButtonPress(eBUTTON3)== ePRESSED)
		{
			if (ucMode < 2)
			{
				ucMode++;
			}
			else
			{
				ucMode = 0;
			}
			
		}

		/*--------Button 4 is not definded --------*/
		/*if (getButtonPress(BUTTON4)== PRESSED)
		{
			
		}
		*/
		
		/*------Displayoutput will update all 500ms--------*/		
		if (ucDisplayUpdateTimer > 50) 
		{
			ucDisplayUpdateTimer = 0;
			
			vDisplayClear();
			
			
			/*------Mode 0 will show the state--------*/	
			if (ucMode == 0)
			{
				vDisplayWriteStringAtPos(0,0,"QAMDec2019");
				vDisplayWriteStringAtPos(2,0,"Status");
				if(xSemaphoreTake(xStatusKey, 5 / portTICK_RATE_MS))
				{
					if((ulStatus && STATUS_ERROR)== STATUS_ERROR)
					{
						vDisplayWriteStringAtPos(2,14,"Ready");
					}
					else
					{
						vDisplayWriteStringAtPos(2,14,"Error");
					}
					xSemaphoreGive(xStatusKey);
				}
			}
			
			/*------Mode 1 will show the setting--------*/	
			if (ucMode == 1)
			{
				vDisplayWriteStringAtPos(0,0,"QAM Setting");
				vDisplayWriteStringAtPos(1,1,"QAMOrdnung");
				vDisplayWriteStringAtPos(2,1,"Output");
				vDisplayWriteStringAtPos(3,1,"Frequenz");
				if(xSemaphoreTake(xSettingKey, 5 / portTICK_RATE_MS))
				{
					if (xQAMSettings.bits.bQAM_Order)
					{
						vDisplayWriteStringAtPos(1,14,"16QAM");
					}
					else
					{
						vDisplayWriteStringAtPos(1,14,"4QAM");
					}
					
					if (xQAMSettings.bits.bSource_IO)
					{
						vDisplayWriteStringAtPos(2,14,"IO");
					}
					else if (xQAMSettings.bits.bSource_Test)
					{
						vDisplayWriteStringAtPos(2,14,"TEST");
					}
					else if (xQAMSettings.bits.bSource_UART)
					{
						vDisplayWriteStringAtPos(2,14,"UART");
					}
					
					if (xQAMSettings.bits.bFrequency)
					{
						vDisplayWriteStringAtPos(3,14,"100Hz");
					}
					else
					{
						vDisplayWriteStringAtPos(3,14,"1kHz");
					}
					xSemaphoreGive(xSettingKey);
				}
				vDisplayWriteStringAtPos(ucSettings+1,0,"-");	
			}
			
			/*------Mode 2 will show the data-------*/	
			if (ucMode == 2)
			{
				vDisplayWriteStringAtPos(0,0,"QAM Data");
				if(xQueuePeek(xData,&ucData_to_send,10/portTICK_RATE_MS))
				{
					vDisplayWriteStringAtPos(1,0,"Data: %d", ucData_to_send);
				}
				else
				{
					vDisplayWriteStringAtPos(1,0,"Data: No Data ");
				}
				
				
			}
				
		}
		ucDisplayUpdateTimer++;
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}

/**
* vOutput is to manage the I/O Output
* @param args Unused
* @return Nothing
* @author C.Häuptli
*/

void vOutput(void *pvParameters) {
	(void) pvParameters;
	
	PORTE.DIRSET = 0xFF;
	PORTE.OUTSET = 0xFF;

	uint8_t ucData_to_send = 0;
	
	for(;;) {
		
		if(xQueueReceive(xData,&ucData_to_send,(TickType_t)100))
		{
				if (ucData_to_send & 0x01)
				{
					PORTF.OUTSET = PIN0_bm;
				}
				else
				{
					PORTF.OUTCLR = PIN0_bm;
				}
				if (ucData_to_send & 0x02)
				{
					PORTF.OUTSET = PIN1_bm;
				}
				else
				{
					PORTF.OUTCLR = PIN1_bm;
				}
				if (ucData_to_send & 0x04)
				{
					PORTF.OUTSET = PIN2_bm;
				}
				else
				{
					PORTF.OUTCLR = PIN2_bm;
				}
				if (ucData_to_send & 0x08)
				{
					PORTF.OUTSET = PIN3_bm;
				}
				else
				{
					PORTF.OUTCLR = PIN3_bm;
				}
				if (ucData_to_send & 0x10)
				{
					PORTF.OUTSET = PIN4_bm;
				}
				else
				{
					PORTF.OUTCLR = PIN4_bm;
				}
				if (ucData_to_send & 0x20)
				{
					PORTF.OUTSET = PIN5_bm;
				}
				else
				{
					PORTF.OUTCLR = PIN5_bm;
				}
				if (ucData_to_send & 0x40)
				{
					PORTF.OUTSET = PIN6_bm;
				}
				else
				{
					PORTF.OUTCLR = PIN6_bm;
				}
				if (ucData_to_send & 0x80)
				{
					PORTF.OUTSET = PIN7_bm;
				}
				else
				{
					PORTF.OUTCLR = PIN7_bm;
				}
				
		}
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}

/**
* vTestpattern is to test the QAM connection
* @param args Unused
* @return Nothing
* @author C.Häuptli
*/

void vTestpattern(void *pvParameters){
	(void) pvParameters;
	
	uint8_t ucTestpattern = 0;
	vTaskSuspend(xTestpattern);
	for(;;) {
		
		if (xQueueReceive(xData,&ucTestpattern,10 / portTICK_RATE_MS))
		{
			if(ucTestpattern == 255)
			{
				if(xSemaphoreTake(xStatusKey, 5 / portTICK_RATE_MS))
				{
					ulStatus = !STATUS_ERROR;
					xSemaphoreGive(xStatusKey);
				}
			}
		}
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}

/**
* vUART is to send the data from the UART
* @param args Unused
* @return Nothing
* @author C.Häuptli
*/
/*
void vUART(void *pvParameters) {
	(void) pvParameters;

	uint8_t ucUART = 256;
	
	for(;;) {
		
		if (uxQueueMessagesWaiting(xData)< 2)
		{
			xQueueSendToBack(xData,&ucUART,portMAX_DELAY);
		}
	}
*/