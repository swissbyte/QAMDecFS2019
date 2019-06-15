/*
 * double_buffer_read_out.h
 *
 * Created: 08.06.2019
 *  Author: Tobias Liesching
 */ 


#ifndef DOUBLE_BUFFER_READ_OUT_H_
#define DOUBLE_BUFFER_READ_OUT_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

void vTask_DMAHandler(void *pvParameters);
void vRead_DMA(void *pvParameters);

#define DMA_EVT_GRP_BufferA  0x01
#define DMA_EVT_GRP_BufferB  0x02

extern EventGroupHandle_t xDMAProcessEventGroup;


#endif /* TASKS_H_ */