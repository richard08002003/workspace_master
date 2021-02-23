/*
 * main.c
 *
 *  Created on: 2018年3月12日
 *      Author: richard
 */

#include "/media/richard/richard_hdd/workspace_NCHU/Common_tool_C/src/Setup.h"



int main ( ) {
	S_Gpio *gpio22 = S_Gpio_new ( ) ;
	if ( NULL == gpio22 ) {
		return - 1 ;
	}

	S_Gpio *gpio23 = S_Gpio_new ( ) ;
	if ( NULL == gpio23 ) {
		S_Gpio_Delete ( & gpio22 ) ;
		return - 2 ;
	}

	S_Gpio *gpio24 = S_Gpio_new ( ) ;
	if ( NULL == gpio24 ) {
		S_Gpio_Delete ( & gpio22 ) ;
		S_Gpio_Delete ( & gpio23 ) ;
		return - 3 ;
	}

	S_Gpio *gpio25 = S_Gpio_new ( ) ;
	if ( NULL == gpio25 ) {
		S_Gpio_Delete ( & gpio22 ) ;
		S_Gpio_Delete ( & gpio23 ) ;
		S_Gpio_Delete ( & gpio24 ) ;
		return - 4 ;
	}

	gpio22->ForcePinOut ( gpio22 , 22 ) ;
	sleep(1);
	gpio23->ForcePinOut ( gpio23 , 23 ) ;
	sleep(1);
	gpio24->ForcePinOut ( gpio24 , 24 ) ;
	sleep(1);
	gpio25->ForcePinOut ( gpio25 , 25 ) ;



	while ( 1 ) {
		gpio22->Write ( gpio22 , D_low ) ;
		gpio23->Write ( gpio23 , D_low ) ;
		gpio24->Write ( gpio24 , D_low ) ;
		gpio25->Write ( gpio25 , D_low ) ;
		sleep ( 3 ) ;

		gpio22->Write ( gpio22 , D_hight ) ;
		gpio23->Write ( gpio23 , D_hight ) ;
		gpio24->Write ( gpio24 , D_hight ) ;
		gpio25->Write ( gpio25 , D_hight ) ;
		sleep ( 3 ) ;
	}




	S_Gpio_Delete ( & gpio22 ) ;
	S_Gpio_Delete ( & gpio23 ) ;
	S_Gpio_Delete ( & gpio24 ) ;
	S_Gpio_Delete ( & gpio25 ) ;
	return 0 ;
}
