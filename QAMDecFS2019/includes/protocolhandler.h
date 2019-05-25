/*
 * protocolhandlerh.h
 *
 * Created: 25.05.2019 13:29:00
 *  Author: philippeppler
 */ 


#ifndef PROTOCOLHANDLERH_H_
#define PROTOCOLHANDLERH_H_

struct ALDP_t_class
{
	uint8_t aldp_hdr_byte_1;
	uint8_t aldp_hdr_byte_2;
	uint8_t aldp_payload[];
};

struct SLDP_t_class
{
	uint8_t sldp_size;
	uint8_t *sldp_payload;
	uint8_t sldp_crc8;
};

void vProtocolHandlerTask(void *pvParameters);



#endif /* PROTOCOLHANDLERH_H_ */