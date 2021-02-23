/*
 * Gpio.h
 *
 *  Created on: 2018年3月12日
 *      Author: richard
 */

#ifndef SRC_GPIO_H_
#define SRC_GPIO_H_

#include "Common_Tool.h"
#include <stdio.h>
#include <stdlib.h>

#define D_hight ( 1 )
#define D_low   ( 0 )

#define DF_CGpio_Type_Out      "out"
#define DF_CGpio_Type_In       "in"
#define DF_CGpio_Type_Rising   "rising"
#define DF_CGpio_Type_Falling  "falling"
#define DF_CGpio_Type_Both     "both"

typedef struct _S_Gpio S_Gpio ;
struct _S_Gpio {
	void *i_private ;

	int (*ForcePin) ( S_Gpio *obj , int pin , CStr typeStr ) ;
	int (*ForcePinOut) ( S_Gpio *obj , int pin ) ;
	int (*ForcePinIn) ( S_Gpio *obj , int pin ) ;
	int (*ForcePinRising) ( S_Gpio *obj , int pin ) ;
	int (*ForcePinFalling) ( S_Gpio *obj , int pin ) ;
	int (*ForcePinBoth) ( S_Gpio *obj , int pin ) ;

	int (*Read) ( S_Gpio *obj ) ;
	int (*RtnPin) ( S_Gpio *obj ) ;
	Str (*RtnType) ( S_Gpio *obj ) ;

	int (*RtnInterruptFd) ( S_Gpio *obj ) ;

	int (*Write) ( S_Gpio *obj , int value ) ;		// 寫GPIO
	int (*RtnWriteBefore) ( S_Gpio *obj ) ;
	int (*RtnWriteNow) ( S_Gpio *obj ) ;

} ;
S_Gpio * S_Gpio_new ( void ) ;
void S_Gpio_Delete ( S_Gpio** obj ) ;

#if 0 // Test
int main ( ) {
	S_Gpio *gpio_obj = S_Gpio_new ( ) ;
	if ( NULL == gpio_obj ) {
		return -1 ;
	}
	gpio_obj->ForcePinOut ( gpio_obj , 4 ) ;

	while ( 1 ) {
		gpio_obj->Write ( D_low ) ;
		sleep ( 3 ) ;
		gpio_obj->Write ( D_hight ) ;
		sleep ( 3 ) ;
	}

	return 0 ;
}
#endif


#endif /* SRC_GPIO_H_ */
