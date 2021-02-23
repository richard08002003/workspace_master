/*
 * DHT.c
 *
 *  Created on: 2018年1月30日
 *      Author: richard
 */

#include "DHT.h"

#define GPIO_BASE_OFFSET 0x200000
#define GPIO_LENGTH 4096

#define DHT_MAXCOUNT 32000
#define DHT_PULSES 41

static void busy_wait_milliseconds ( uint32_t millis ) ;
static void sleep_milliseconds ( uint32_t millis ) ;
static void set_max_priority ( void ) ;
static void set_default_priority ( void ) ;

static int pi_mmio_init ( void ) ;
static inline void pi_mmio_set_input ( const int gpio_number ) ;
static inline void pi_mmio_set_output ( const int gpio_number ) ;
static inline void pi_mmio_set_high ( const int gpio_number ) ;
static inline void pi_mmio_set_low ( const int gpio_number ) ;
static inline uint32_t pi_mmio_input ( const int gpio_number ) ;

// Busy wait delay for most accurate timing, but high CPU usage.
// Only use this for short periods of time (a few hundred milliseconds at most)!
static void busy_wait_milliseconds ( uint32_t millis ) {
	// Set delay time period.
	struct timeval deltatime ;
	deltatime.tv_sec = millis / 1000 ;
	deltatime.tv_usec = ( millis % 1000 ) * 1000 ;
	struct timeval walltime ;
	// Get current time and add delay to find end time.
	gettimeofday ( & walltime , NULL ) ;
	struct timeval endtime ;
	timeradd( & walltime , & deltatime , & endtime )
	;
	// Tight loop to waste time (and CPU) until enough time as elapsed.
	while ( timercmp( & walltime , & endtime , < ) ) {
		gettimeofday ( & walltime , NULL ) ;
	}
}

// General delay that sleeps so CPU usage is low, but accuracy is potentially bad.
static void sleep_milliseconds ( uint32_t millis ) {
	struct timespec sleep ;
	sleep.tv_sec = millis / 1000 ;
	sleep.tv_nsec = ( millis % 1000 ) * 1000000L ;
	while ( clock_nanosleep ( CLOCK_MONOTONIC , 0 , & sleep , & sleep ) && errno == EINTR )
		;
}

// Increase scheduling priority and algorithm to try to get 'real time' results.
static void set_max_priority ( void ) {
	struct sched_param sched ;
	memset ( & sched , 0 , sizeof ( sched ) ) ;
	// Use FIFO scheduler with highest priority for the lowest chance of the kernel context switching.
	sched.sched_priority = sched_get_priority_max ( SCHED_FIFO ) ;
	sched_setscheduler ( 0 , SCHED_FIFO , & sched ) ;
}

// Drop scheduling priority back to normal/default.
static void set_default_priority ( void ) {
	struct sched_param sched ;
	memset ( & sched , 0 , sizeof ( sched ) ) ;
	// Go back to default scheduler with default 0 priority.
	sched.sched_priority = 0 ;
	sched_setscheduler ( 0 , SCHED_OTHER , & sched ) ;
}

volatile uint32_t* pi_mmio_gpio = NULL ;
static int pi_mmio_init ( void ) {
	if ( pi_mmio_gpio == NULL ) {
		// Check for GPIO and peripheral addresses from device tree.
		// Adapted from code in the RPi.GPIO library at:
		//   http://sourceforge.net/p/raspberry-gpio-python/
		FILE *fp = fopen ( "/proc/device-tree/soc/ranges" , "rb" ) ;
		if ( fp == NULL ) {
			return MMIO_ERROR_OFFSET ;
		}
		fseek ( fp , 4 , SEEK_SET ) ;
		unsigned char buf [ 4 ] ;
		if ( fread ( buf , 1 , sizeof ( buf ) , fp ) != sizeof ( buf ) ) {
			return MMIO_ERROR_OFFSET ;
		}
		uint32_t peri_base = buf [ 0 ] << 24 | buf [ 1 ] << 16 | buf [ 2 ] << 8 | buf [ 3 ] << 0 ;
		uint32_t gpio_base = peri_base + GPIO_BASE_OFFSET ;
		fclose ( fp ) ;

		int fd = open ( "/dev/gpiomem" , O_RDWR | O_SYNC ) ;
		if ( fd == - 1 ) {
			// Error opening /dev/gpiomem.
			return MMIO_ERROR_DEVMEM ;
		}
		// Map GPIO memory to location in process space.
		pi_mmio_gpio = ( uint32_t* ) mmap ( NULL , GPIO_LENGTH , PROT_READ | PROT_WRITE , MAP_SHARED , fd , gpio_base ) ;
		close ( fd ) ;
		if ( pi_mmio_gpio == MAP_FAILED ) {
			// Don't save the result if the memory mapping failed.
			pi_mmio_gpio = NULL ;
			return MMIO_ERROR_MMAP ;
		}
	}
	return MMIO_SUCCESS ;
}

volatile uint32_t* pi_mmio_gpio ;
static inline void pi_mmio_set_input ( const int gpio_number ) {
	// Set GPIO register to 000 for specified GPIO number.
	* ( pi_mmio_gpio + ( ( gpio_number ) / 10 ) ) &= ~ ( 7 << ( ( ( gpio_number ) % 10 ) * 3 ) ) ;
}

static inline void pi_mmio_set_output ( const int gpio_number ) {
	// First set to 000 using input function.
	pi_mmio_set_input ( gpio_number ) ;
	// Next set bit 0 to 1 to set output.
	* ( pi_mmio_gpio + ( ( gpio_number ) / 10 ) ) |= ( 1 << ( ( ( gpio_number ) % 10 ) * 3 ) ) ;
}

static inline void pi_mmio_set_high ( const int gpio_number ) {
	* ( pi_mmio_gpio + 7 ) = 1 << gpio_number ;
}

static inline void pi_mmio_set_low ( const int gpio_number ) {
	* ( pi_mmio_gpio + 10 ) = 1 << gpio_number ;
}

static inline uint32_t pi_mmio_input ( const int gpio_number ) {
	return * ( pi_mmio_gpio + 13 ) & ( 1 << gpio_number ) ;
}


// Read DHT sensor connected to GPIO pin (using BCM numbering).  Humidity and temperature will be
// returned in the provided parameters. If a successfull reading could be made a value of 0
// (DHT_SUCCESS) will be returned.  If there was an error reading the sensor a negative value will
// be returned.  Some errors can be ignored and retried, specifically DHT_ERROR_TIMEOUT or DHT_ERROR_CHECKSUM.
int pi_dht_read ( int type , int pin , float* humidity , float* temperature ) {
	// Validate humidity and temperature arguments and set them to zero.
	if ( humidity == NULL || temperature == NULL ) {
		return DHT_ERROR_ARGUMENT ;
	}
	* temperature = 0.0f ;
	* humidity = 0.0f ;

	// Initialize GPIO library.
	if ( pi_mmio_init ( ) < 0 ) {
		return DHT_ERROR_GPIO ;
	}

	// Store the count that each DHT bit pulse is low and high.
	// Make sure array is initialized to start at zero.
	int pulseCounts [ DHT_PULSES * 2 ] = { 0 } ;

	// Set pin to output.
	pi_mmio_set_output ( pin ) ;

	// Bump up process priority and change scheduler to try to try to make process more 'real time'.
	set_max_priority ( ) ;

	// Set pin high for ~500 milliseconds.
	pi_mmio_set_high ( pin ) ;
	sleep_milliseconds ( 500 ) ;

	// The next calls are timing critical and care should be taken
	// to ensure no unnecssary work is done below.

	// Set pin low for ~20 milliseconds.
	pi_mmio_set_low ( pin ) ;
	busy_wait_milliseconds ( 20 ) ;

	// Set pin at input.
	pi_mmio_set_input ( pin ) ;
	// Need a very short delay before reading pins or else value is sometimes still low.
	for ( volatile int i = 0 ; i < 50 ; ++ i ) {
	}

	// Wait for DHT to pull pin low.
	uint32_t count = 0 ;
	while ( pi_mmio_input ( pin ) ) {
		if ( ++ count >= DHT_MAXCOUNT ) {
			// Timeout waiting for response.
			set_default_priority ( ) ;
			return DHT_ERROR_TIMEOUT ;
		}
	}

	// Record pulse widths for the expected result bits.
	for ( int i = 0 ; i < DHT_PULSES * 2 ; i += 2 ) {
		// Count how long pin is low and store in pulseCounts[i]
		while ( ! pi_mmio_input ( pin ) ) {
			if ( ++ pulseCounts [ i ] >= DHT_MAXCOUNT ) {
				// Timeout waiting for response.
				set_default_priority ( ) ;
				return DHT_ERROR_TIMEOUT ;
			}
		}
		// Count how long pin is high and store in pulseCounts[i+1]
		while ( pi_mmio_input ( pin ) ) {
			if ( ++ pulseCounts [ i + 1 ] >= DHT_MAXCOUNT ) {
				// Timeout waiting for response.
				set_default_priority ( ) ;
				return DHT_ERROR_TIMEOUT ;
			}
		}
	}

	// Done with timing critical code, now interpret the results.

	// Drop back to normal priority.
	set_default_priority ( ) ;

	// Compute the average low pulse width to use as a 50 microsecond reference threshold.
	// Ignore the first two readings because they are a constant 80 microsecond pulse.
	uint32_t threshold = 0 ;
	for ( int i = 2 ; i < DHT_PULSES * 2 ; i += 2 ) {
		threshold += pulseCounts [ i ] ;
	}
	threshold /= DHT_PULSES - 1 ;

	// Interpret each high pulse as a 0 or 1 by comparing it to the 50us reference.
	// If the count is less than 50us it must be a ~28us 0 pulse, and if it's higher
	// then it must be a ~70us 1 pulse.
	uint8_t data [ 5 ] = { 0 } ;
	for ( int i = 3 ; i < DHT_PULSES * 2 ; i += 2 ) {
		int index = ( i - 3 ) / 16 ;
		data [ index ] <<= 1 ;
		if ( pulseCounts [ i ] >= threshold ) {
			// One bit for long pulse.
			data [ index ] |= 1 ;
		}
		// Else zero bit for short pulse.
	}

	// Useful debug info:
	//printf("Data: 0x%x 0x%x 0x%x 0x%x 0x%x\n", data[0], data[1], data[2], data[3], data[4]);

	// Verify checksum of received data.
	if ( data [ 4 ] == ( ( data [ 0 ] + data [ 1 ] + data [ 2 ] + data [ 3 ] ) & 0xFF ) ) {
		if ( type == DHT11 ) {
			// Get humidity and temp for DHT11 sensor.
			* humidity = ( float ) data [ 0 ] ;
			* temperature = ( float ) data [ 2 ] ;
		} else if ( type == DHT22 ) {
			// Calculate humidity and temp for DHT22 sensor.
			* humidity = ( data [ 0 ] * 256 + data [ 1 ] ) / 10.0f ;
			* temperature = ( ( data [ 2 ] & 0x7F ) * 256 + data [ 3 ] ) / 10.0f ;
			if ( data [ 2 ] & 0x80 ) {
				* temperature *= - 1.0f ;
			}
		}
		return DHT_SUCCESS ;
	} else {
		return DHT_ERROR_CHECKSUM ;
	}
	return D_success ;
}
