/*
 * dma_config.h
 *
 * Created: 08.06.2019
 *  Author: Tobias Liesching
 */ 


#ifndef DMA_CONFIG_H_
#define DMA_CONFIG_H_

void vInitDMA();

#define DMA_BUF_LEN	128

extern volatile uint8_t buffer_a[];
extern volatile uint8_t buffer_b[];

extern volatile uint8_t ucStopDMA;

extern volatile uint8_t bufferAReady;
extern volatile uint8_t bufferBReady;

//int xtest_array_length;
//float xtest_array[96]; /*for testing*/
//uint8_t buffer_length;



#endif /* DMA_H_ */