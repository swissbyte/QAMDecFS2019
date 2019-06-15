/*
 * read_peaks.h
 *
 * Created: 08.06.2019
 *  Author: Tobias Liesching
 */ 
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "double_buffer_read_out.h"
#include "dma_config.h"


#ifndef READ_PEAKS_H_
#define READ_PEAKS_H_
#define Process_Phase_detectionA ( 1 << 0 )
#define Process_Phase_detectionB ( 1 << 1 )

int position_array_H[64];
int position_array_L[64];

void vRead_Peaks(void *pvParameters);

EventGroupHandle_t xSignalProcessEventGroup;
EventGroupHandle_t xPhaseDetectionEventGroup;

#endif /* READ_PEAKS_H_ */