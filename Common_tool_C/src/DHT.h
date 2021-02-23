/*
 * DHT.h
 *
 *  Created on: 2018年1月30日
 *      Author: richard
 */

#ifndef SRC_DHT_H_
#define SRC_DHT_H_

#include "Common_Tool.h"

// About Common DHT Read //
// Define errors and return values.
#define DHT_ERROR_TIMEOUT 	-1
#define DHT_ERROR_CHECKSUM 	-2
#define DHT_ERROR_ARGUMENT 	-3
#define DHT_ERROR_GPIO 		-4
#define DHT_SUCCESS 		 0

// Define sensor types.
#define DHT11 				11
#define DHT22 				22
#define AM2302 				22

// About MMIO //
#define MMIO_SUCCESS 		 0
#define MMIO_ERROR_DEVMEM 	-1
#define MMIO_ERROR_MMAP 	-2
#define MMIO_ERROR_OFFSET 	-3

extern int pi_dht_read ( int sensor , int pin , float* humidity , float* temperature ) ;

#endif /* SRC_DHT_H_ */
