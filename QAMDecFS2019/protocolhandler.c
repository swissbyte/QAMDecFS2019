/*
 * protocolhandler.c
 *
 * Created: 25.05.2019 13:28:40
 *  Author: philippeppler
 */ 

#include "avr_compiler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "protocolhandler.h"
#include "string.h"

// KONSTANTEN
#define ANZSENDQUEUE					32
#define	PROTOCOLBUFFERSIZE				32
// xQuelle
#define PAKET_TYPE_ALDP					0x01
#define ALDP_SRC_UART					0x00
#define ALDP_SRC_I2C					0x01
#define ALDP_SRC_TEST					0x02
#define ALDP_SRC_ERROR					0xFF

// xSettings
#define Settings_QAM_Order				1<<0
#define Settings_Source_Bit1			1<<1
#define Settings_Source_Bit2			1<<2
#define Settings_Frequency				1<<3
// xStatus
#define Status_Error					1<<1
#define Status_Daten_ready				1<<2
#define Status_Daten_sending			1<<3
// xProtocolBufferStatus
#define BUFFER_A_freetouse				1<<0
#define BUFFER_B_freetouse				1<<1
// Buffer
#define ACTIVEBUFFER_A					0
#define ACTIVEBUFFER_B					1

EventGroupHandle_t xSettings;								// Settings vom GUI von Cedi
EventGroupHandle_t xStatus;									// auch irgendwas von Cedi

EventGroupHandle_t xProtocolBufferStatus;					// Eventbits for Buffer-Status

xQueueHandle xALDPQueue;									// Queue from Protocolhandler to Main-Task

//globale Variablen
uint8_t ucglobalProtocolBuffer_A[PROTOCOLBUFFERSIZE] = {8,1,6,3,4,5,6,7,8,66,8,1,6,1,2,3,4,5,6,66,8,0,6,6,7,8,9,10,11,66,8,1};	// Buffer_A from Demodulator to ProtocolTask
uint8_t ucglobalProtocolBuffer_B[PROTOCOLBUFFERSIZE] = {6,3,4,5,6,7,8,66,8,1,6,3,4,5,6,7,8,66,8,1,6,3,4,5,6,7,8,66,2,1,0,66};	// Buffer_B from Demodulator to ProtocolTask


void vProtocolHandlerTask(void *pvParameters) {
	(void) pvParameters;

	struct ALDP_t_class *xALDP_Paket;
	struct SLDP_t_class xSLDP_Paket;
	
	xProtocolBufferStatus = xEventGroupCreate();
	
	PORTF.DIRSET = PIN0_bm; /*LED1*/
	PORTF.OUT = 0x01;
	
	uint8_t ucBuffer_A_Position = 0;								// position inside the used buffer (A or B)
	uint8_t ucBuffer_B_Position = 0;								// position inside the used buffer (A or B)
	
	uint8_t ucactiveBuffer = ACTIVEBUFFER_A;
	
	uint8_t ucBufferSLDPpayloadInput[256]= {};
	uint8_t ucBufferSLDPpayloadInputCounter;
	
	xALDPQueue = xQueueCreate(ANZSENDQUEUE, sizeof(uint8_t));
	
	// Debugging
	xEventGroupSetBits(xProtocolBufferStatus, BUFFER_A_freetouse);
	xEventGroupSetBits(xProtocolBufferStatus, BUFFER_B_freetouse);
	
	
	

	for(;;) {
		
		PORTF.OUTTGL = 0x01;
		
		xSLDP_Paket.sldp_size = ucglobalProtocolBuffer_A[ucBuffer_A_Position];
			
		for (ucBufferSLDPpayloadInputCounter = 0; ucBufferSLDPpayloadInputCounter <= (xSLDP_Paket.sldp_size+1); ucBufferSLDPpayloadInputCounter++) {
				
			// Bufferhandler
			if (ucBuffer_A_Position >= (PROTOCOLBUFFERSIZE)) {
				ucactiveBuffer = ACTIVEBUFFER_B;
				ucBuffer_A_Position = 0;
			}
				
			if (ucBuffer_B_Position >= (PROTOCOLBUFFERSIZE)) {
				ucactiveBuffer = ACTIVEBUFFER_A;
				ucBuffer_B_Position = 0;
			}
				
				
			if (ucactiveBuffer == ACTIVEBUFFER_A) {
				xEventGroupWaitBits(xProtocolBufferStatus, BUFFER_A_freetouse, pdTRUE, pdFALSE, portMAX_DELAY);					// wait for Buffer A
				ucBufferSLDPpayloadInput[ucBufferSLDPpayloadInputCounter] = ucglobalProtocolBuffer_A[ucBuffer_A_Position];		
				ucBuffer_A_Position++;
				xEventGroupSetBits(xProtocolBufferStatus, BUFFER_A_freetouse);													// Buffer A release
			}
			else if (ucactiveBuffer == ACTIVEBUFFER_B) {
				xEventGroupWaitBits(xProtocolBufferStatus, BUFFER_B_freetouse, pdTRUE, pdFALSE, portMAX_DELAY);					// wait for Buffer A
				ucBufferSLDPpayloadInput[ucBufferSLDPpayloadInputCounter] = ucglobalProtocolBuffer_B[ucBuffer_B_Position];
				ucBuffer_B_Position++;
				xEventGroupSetBits(xProtocolBufferStatus, BUFFER_B_freetouse);													// Buffer A release
			}

		}

		ucBufferSLDPpayloadInputCounter--;	
			
			
			
		xSLDP_Paket.sldp_crc8 = ucBufferSLDPpayloadInput[ucBufferSLDPpayloadInputCounter];
		xSLDP_Paket.sldp_payload = &ucBufferSLDPpayloadInput[1];
			
		xALDP_Paket = (struct ALDP_t_class *)xSLDP_Paket.sldp_payload;
			
		
		
		uint8_t array[256]={};

		memcpy(array, xALDP_Paket->aldp_payload, xALDP_Paket->aldp_hdr_byte_2);










		vTaskDelay(50 / portTICK_RATE_MS);				// Delay 50ms
	}
}