/*
 * dma_config.h
 *
 * Created: 08.06.2019
 *  Author: Tobias Liesching
 */ 


#ifndef DMA_CONFIG_H_
#define DMA_CONFIG_H_

void vInitDMA();

volatile uint8_t buffer_a[64];
volatile uint8_t buffer_b[64];
int xtest_array_length;
float xtest_array[96]; /*for testing*/
uint8_t buffer_length;



#endif /* DMA_H_ */