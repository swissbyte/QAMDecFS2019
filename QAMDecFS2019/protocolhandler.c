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
#include "semphr.h"

/* Constants */
#define ANZ_SEND_QUEUE					32
#define	PROTOCOL_BUFFER_SIZE			32
#define SLDP_PAYLOAD_MAX_SIZE			256
/* xQuelle */
#define PAKET_TYPE_ALDP					0x01
#define ALDP_SRC_UART					0x00
#define ALDP_SRC_I2C					0x01
#define ALDP_SRC_TEST					0x02
#define ALDP_SRC_ERROR					0xFF

/* xSettings */
#define Settings_QAM_Order				1<<0
#define Settings_Source_Bit1			1<<1
#define Settings_Source_Bit2			1<<2
#define Settings_Frequency				1<<3
/* xStatus */
#define Status_Error					1<<1
#define Status_Daten_ready				1<<2
#define Status_Daten_sending			1<<3
/* xProtocolBufferStatus*/
#define BUFFER_A_FreeToUse				1<<0
#define BUFFER_B_FreeToUse				1<<1
/* Buffer */
#define ACTIVEBUFFER_A					0
#define ACTIVEBUFFER_B					1

EventGroupHandle_t xSettings;								// Settings vom GUI von Cedi
EventGroupHandle_t xStatus;									// auch irgendwas von Cedi


xQueueHandle xALDPQueue;									// Queue from Protocolhandler to Main-Task

SemaphoreHandle_t xGlobalProtocolBuffer_A_Key;			//A-Resource for ucGlobalProtocolBuffer_A
SemaphoreHandle_t xGlobalProtocolBuffer_B_Key;			//A-Resource for ucGlobalProtocolBuffer_B

/* global Variables */
uint8_t ucGlobalProtocolBuffer_A[ PROTOCOL_BUFFER_SIZE ] = {};	// Buffer_A from Demodulator to ProtocolTask
uint8_t ucGlobalProtocolBuffer_B[ PROTOCOL_BUFFER_SIZE ] = {};	// Buffer_B from Demodulator to ProtocolTask

/* Debug ( Testpattern )
uint8_t ucGlobalProtocolBuffer_A[ PROTOCOL_BUFFER_SIZE ] = {8,1,6,3,4,5,6,7,8,42,8,1,6,1,2,3,4,5,6,16,8,0,6,6,7,8,9,10,11,101,8,1};	// Buffer_A from Demodulator to ProtocolTask
uint8_t ucGlobalProtocolBuffer_B[ PROTOCOL_BUFFER_SIZE ] = {6,3,4,5,6,7,8,66,8,1,6,3,4,5,6,7,8,66,8,1,6,3,4,5,6,7,8,66,2,1,0,66};	// Buffer_B from Demodulator to ProtocolTask
*/

void vProtocolHandlerTask( void *pvParameters ) {
	( void ) pvParameters;

	struct ALDP_t_class *xALDP_Paket;
	struct SLDP_t_class xSLDP_Paket;
	
	uint8_t ucBuffer_A_Position = 0;								// position inside the used buffer ( A or B )
	uint8_t ucBuffer_B_Position = 0;								// position inside the used buffer ( A or B )
	
	uint8_t ucActiveBuffer = ACTIVEBUFFER_A;
	
	uint8_t ucBufferSLDPpayloadInput[ SLDP_PAYLOAD_MAX_SIZE ]= {};		// could be done better maybe
	uint8_t ucBufferSLDPpayloadInputCounter;
	uint8_t ucValidData = pdFALSE;
	uint32_t ulCRC_ErrorCounter = 0;
	
	xGlobalProtocolBuffer_A_Key = xSemaphoreCreateMutex();
	xGlobalProtocolBuffer_B_Key = xSemaphoreCreateMutex();
	
	xALDPQueue = xQueueCreate( ANZ_SEND_QUEUE, sizeof( uint8_t ) );

	
/* Debugging
	PORTF.DIRSET = PIN0_bm; //LED1
	PORTF.OUT = 0x01;
*/	

	for( ;; ) {
/* Debugging		
		PORTF.OUTTGL = 0x01;
*/


/* Searching active buffer for SLDPsize not 0 (zero) */		
		if( ucActiveBuffer == ACTIVEBUFFER_A ) {
			if( xSemaphoreTake( xGlobalProtocolBuffer_A_Key, ( 50 / portTICK_RATE_MS ) ) ) {
				if( ucGlobalProtocolBuffer_A[ ucBuffer_A_Position ] != 0) {
					xSLDP_Paket.sldp_size = ucGlobalProtocolBuffer_A[ ucBuffer_A_Position ];
					ucValidData = pdTRUE;
				}
				else {
					ucBuffer_A_Position++;
					ucValidData = pdFALSE;
				}
				xSemaphoreGive(xGlobalProtocolBuffer_A_Key);
			}
		}
		else if ( ucActiveBuffer == ACTIVEBUFFER_B ) {
			if( xSemaphoreTake( xGlobalProtocolBuffer_B_Key, ( 50 / portTICK_RATE_MS ) ) ) {
				if( ucGlobalProtocolBuffer_B[ ucBuffer_B_Position ] != 0) {
					xSLDP_Paket.sldp_size = ucGlobalProtocolBuffer_B[ ucBuffer_B_Position ];
					ucValidData = pdTRUE;
				}
				else {
					ucBuffer_B_Position++;
					ucValidData = pdFALSE;
				}
				xSemaphoreGive(xGlobalProtocolBuffer_A_Key);
			}
		}

/* Valid Data in buffer? */
		if( ucValidData == pdTRUE) {
		
/* Copy Data from global Buffer into local Buffer */

			for ( ucBufferSLDPpayloadInputCounter = 0; ucBufferSLDPpayloadInputCounter <= ( xSLDP_Paket.sldp_size+1 ); ucBufferSLDPpayloadInputCounter++ ) {
				
/* Bufferhandler */
				if ( ucBuffer_A_Position >= ( PROTOCOL_BUFFER_SIZE ) ) {
					ucBuffer_A_Position = 0;
					xSemaphoreGive(xGlobalProtocolBuffer_A_Key);
					ucActiveBuffer = ACTIVEBUFFER_B;
					xSemaphoreTake( xGlobalProtocolBuffer_B_Key, portMAX_DELAY );
				}
				
				if ( ucBuffer_B_Position >= ( PROTOCOL_BUFFER_SIZE ) ) {
					ucBuffer_B_Position = 0;
					xSemaphoreGive(xGlobalProtocolBuffer_B_Key);
					ucActiveBuffer = ACTIVEBUFFER_A;
					xSemaphoreTake( xGlobalProtocolBuffer_B_Key, portMAX_DELAY );
				}
				
/* write from Doublebuffer into Protocolbuffer */
				if ( ucActiveBuffer == ACTIVEBUFFER_A ) {
					ucBufferSLDPpayloadInput[ ucBufferSLDPpayloadInputCounter ] = ucGlobalProtocolBuffer_A[ ucBuffer_A_Position ];		
					ucBuffer_A_Position++;
				}
				else if ( ucActiveBuffer == ACTIVEBUFFER_B ) {
					ucBufferSLDPpayloadInput[ ucBufferSLDPpayloadInputCounter ] = ucGlobalProtocolBuffer_B[ ucBuffer_B_Position ];
					ucBuffer_B_Position++;
				}

			}

			ucBufferSLDPpayloadInputCounter--;	
			
/* fill SLDP and ALDP */
			xSLDP_Paket.sldp_crc8 = ucBufferSLDPpayloadInput[ ucBufferSLDPpayloadInputCounter ];									// TODO: CRC8 überprüfen
			xSLDP_Paket.sldp_payload = &ucBufferSLDPpayloadInput[ 1 ];
			
			xALDP_Paket = ( struct ALDP_t_class * )xSLDP_Paket.sldp_payload;

/* Debug
			uint8_t array[ 256 ]={};
			memcpy( array, xALDP_Paket->aldp_payload, xALDP_Paket->aldp_hdr_byte_2 );
*/
			
/* calculating CRC8 */
			
			uint8_t ucCRC8 = 0;
			ucCRC8 = xCRC_calc(ucCRC8, xSLDP_Paket.sldp_size );
			for ( uint8_t i = 0; i < xSLDP_Paket.sldp_size; i++ ) {
				uint8_t ucCRCSend = xSLDP_Paket.sldp_payload[ i ];
				ucCRC8 = xCRC_calc(ucCRC8, ucCRCSend );
			}

/* checking CRC8 with calculation */

			if(ucCRC8 == xSLDP_Paket.sldp_crc8) {
				
/* write ALDP Payload into ALDP-Queue */
				for ( uint8_t i = 0; i < xALDP_Paket->aldp_hdr_byte_2; i++ ) {
					uint8_t ucSendChar = xSLDP_Paket.sldp_payload[ i+2 ];
					xQueueSend( xALDPQueue, &ucSendChar, portMAX_DELAY );
				}				
			}
			else {
				ulCRC_ErrorCounter++;
			}
		}
	}
}

/* CRC8 Function (ROM=39 / RAM=4 / Average => 196_Tcy / 24.5_us for 8MHz clock)    https://www.ccsinfo.com/forum/viewtopic.php?t=37015 (Original code by T. Scott Dattalo) */
uint8_t xCRC_calc( uint8_t uiCRC, uint8_t uiCRC_data )
{
	uint8_t i = (uiCRC_data ^ uiCRC) & 0xff;
	uiCRC = 0;
	if(i & 1)
	uiCRC ^= 0x5e;
	if(i & 2)
	uiCRC ^= 0xbc;
	if(i & 4)
	uiCRC ^= 0x61;
	if(i & 8)
	uiCRC ^= 0xc2;
	if(i & 0x10)
	uiCRC ^= 0x9d;
	if(i & 0x20)
	uiCRC ^= 0x23;
	if(i & 0x40)
	uiCRC ^= 0x46;
	if(i & 0x80)
	uiCRC ^= 0x8c;
	return(uiCRC);
}