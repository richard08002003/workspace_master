/*
 * Gpio.c
 *
 *  Created on: 2018年3月12日
 *      Author: richard
 */

#include "Gpio.h"
#define DF_Gpio_Path "/sys/class/gpio"
// private data
typedef struct _S_Gpio_private {
	bool i_is_set_succ ;   			// true is success ;
	int i_force_pin ;				// gpio 腳位
	char i_pin_type [ 32 + 1 ] ;	// gpio型態

	char i_interrupt_path [ 256 + 1 ] ;
	int i_int_fd ;

	int i_write_before ;
	int i_write_now ;
	FILE* i_wr_fp ;
} S_Gpio_private ;

static int S_ForcePin ( S_Gpio *obj , int pin , CStr typeStr ) ;
static int S_ForcePinOut ( S_Gpio *obj , int pin ) ;
static int S_ForcePinIn ( S_Gpio *obj , int pin ) ;
static int S_ForcePinRising ( S_Gpio *obj , int pin ) ;
static int S_ForcePinFalling ( S_Gpio *obj , int pin ) ;
static int S_ForcePinBoth ( S_Gpio *obj , int pin ) ;
static int S_Read ( S_Gpio *obj ) ;
static int S_RtnPin ( S_Gpio *obj ) ;
static Str S_RtnType ( S_Gpio *obj ) ;
static int S_RtnInterruptFd ( S_Gpio *obj ) ;
static int S_Write ( S_Gpio *obj , int value ) ;		// 寫GPIO
static int S_RtnWriteBefore ( S_Gpio *obj ) ;
static int S_RtnWriteNow ( S_Gpio *obj ) ;

static int S_ForcePin ( S_Gpio *obj , int pin , CStr typeStr ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_Gpio_private *p_private = obj->i_private ;
	if ( 0 > pin ) {
		return -2 ;
	}
	p_private->i_force_pin = pin ;

	char path [ 128 + 1 ] = { 0 } ;
	snprintf ( path , sizeof ( path ) , "%s/export" , DF_Gpio_Path ) ;
	FILE *fp = fopen ( path , "w" ) ;	// 開檔，可寫
	if ( NULL == fp ) {
		return -3 ;
	}
	fprintf ( fp , "%i" , p_private->i_force_pin ) ;
	fflush ( fp ) ;
	fclose ( fp);


	// 設定 type //
	snprintf ( path , sizeof ( path ) , "%s/gpio%d/direction" , DF_Gpio_Path , p_private->i_force_pin ) ;
	fp = fopen ( path , "w" ) ;		// 開檔，可寫
	if ( NULL == fp ) {
		return -4 ;
	}


	bool edgeBol = false ;
	if ( ( strcmp ( typeStr , DF_CGpio_Type_Out ) == 0 ) || ( strcmp ( typeStr , DF_CGpio_Type_In ) == 0 ) ) {
		fprintf ( fp , "%s" , typeStr ) ;
	} else if ( ( strcmp ( typeStr , DF_CGpio_Type_Rising ) == 0 ) || ( strcmp ( typeStr , DF_CGpio_Type_Falling ) == 0 ) || ( strcmp ( typeStr , DF_CGpio_Type_Both ) == 0 ) ) {
		fprintf ( fp , "%s" , "in" ) ;
		edgeBol = true ;
	} else {
		fflush ( fp ) ;
		fclose ( fp ) ;
		return -5 ;
	}
	fflush ( fp ) ;
	fclose ( fp);

	if ( true == edgeBol ) {
		/* 要該腳位有拉到 edge 才有辦法寫入*/
		snprintf ( path , sizeof ( path ) , "%s/gpio%d/edge" , DF_Gpio_Path , p_private->i_force_pin ) ;
		fp = fopen ( path , "w" ) ;
		if ( NULL == fp ) {
			return -6 ;
		}

		fprintf ( fp , "%s" , typeStr ) ;
		fflush ( fp ) ;
		fclose ( fp);

		snprintf ( path , sizeof ( path ) , "%s/gpio%d/value" , DF_Gpio_Path , p_private->i_force_pin ) ;

		memset ( p_private->i_interrupt_path , 0 , sizeof ( p_private->i_interrupt_path ) ) ;
		snprintf ( p_private->i_interrupt_path , sizeof ( p_private->i_interrupt_path ) , "%s" , path ) ;
	}

	p_private->i_is_set_succ = true ;
	return D_success ;
}

static int S_ForcePinOut ( S_Gpio *obj , int pin ) {
	return obj->ForcePin ( obj , pin , "out" ) ;
}

static int S_ForcePinIn ( S_Gpio *obj , int pin ) {
	return obj->ForcePin ( obj , pin , "in" ) ;
}

static int S_ForcePinRising ( S_Gpio *obj , int pin ) {
	return obj->ForcePin ( obj , pin , "rising" ) ;
}


static int S_ForcePinFalling ( S_Gpio *obj , int pin ) {
	return obj->ForcePin ( obj , pin , "falling" ) ;
}

static int S_ForcePinBoth ( S_Gpio *obj , int pin ) {
	return obj->ForcePin ( obj , pin , "both" ) ;
}

// 讀取GPIO value
static int S_Read ( S_Gpio *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_Gpio_private *p_private = obj->i_private ;
	if ( false == p_private->i_is_set_succ ) {
		return - 2 ;
	}
	char fileName [ 127 + 1 ] = { 0 } ;
	snprintf ( fileName , sizeof ( fileName ) , "%s/gpio%d/value" , DF_Gpio_Path , p_private->i_force_pin ) ;

	FILE *fp = fopen ( fileName , "r+" ) ;
	if ( NULL == fp ) {
		return - 3 ;
	}

	fread ( fileName , 1 , sizeof ( fileName ) , fp ) ;
	fclose ( fp ) ;

	return atoi ( fileName ) ;
	return D_success ;
}

static int S_RtnPin ( S_Gpio *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_Gpio_private *p_private = obj->i_private ;
	return p_private->i_force_pin ;
}

static Str S_RtnType ( S_Gpio *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_Gpio_private *p_private = obj->i_private ;
	return p_private->i_pin_type ;
}

static int S_RtnInterruptFd ( S_Gpio *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_Gpio_private *p_private = obj->i_private ;
	if ( false == p_private->i_is_set_succ ) {
		return -2 ;
	}

	p_private->i_int_fd = open ( p_private->i_interrupt_path , O_RDONLY ) ;
	if ( -1 == p_private->i_int_fd ) {
		return -3 ;
	}
	return p_private->i_int_fd ;
}

static int S_Write ( S_Gpio *obj , int value ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_Gpio_private *p_private = obj->i_private ;

	if ( false == p_private->i_is_set_succ ) {
		return -2 ;
	}

	p_private->i_write_before = p_private->i_write_now ; // 寫入前的值
	p_private->i_write_now = value ;		   // 現在的值

	char fileName [ 256 + 1 ] = { 0 } ;
	snprintf ( fileName , sizeof ( fileName ) , "%s/gpio%d/value" , DF_Gpio_Path , p_private->i_force_pin ) ;

	FILE *fp = fopen ( fileName , "w" ) ;
	if ( NULL == fp ) {
		return -3 ;
	}
	fprintf ( fp , "%d" , p_private->i_write_now ) ;
	fflush ( fp ) ;
	fclose ( fp ) ;
	return D_success ;
}

static int S_RtnWriteBefore ( S_Gpio *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_Gpio_private *p_private = obj->i_private ;

	if ( false == p_private->i_is_set_succ ) {
		return - 2 ;
	}
	return p_private->i_write_before ;
}

static int S_RtnWriteNow ( S_Gpio *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_Gpio_private *p_private = obj->i_private ;

	if ( false == p_private->i_is_set_succ ) {
		return - 2 ;
	}
	return p_private->i_write_now ;
}

S_Gpio * S_Gpio_new ( void ) {
	S_Gpio* p_tmp = ( S_Gpio* ) calloc ( 1 , sizeof(S_Gpio) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}
	S_Gpio_private *i_private_data = ( S_Gpio_private* ) calloc ( 1 , sizeof(S_Gpio_private) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}

	// private
	i_private_data->i_is_set_succ = false ;
	i_private_data->i_force_pin = - 1 ;
	memset ( i_private_data->i_pin_type , 0 , sizeof ( i_private_data->i_pin_type ) ) ;
	memset ( i_private_data->i_interrupt_path , 0 , sizeof ( i_private_data->i_interrupt_path ) ) ;
	i_private_data->i_int_fd = - 1 ;
	i_private_data->i_write_before = - 1 ;
	i_private_data->i_write_now = - 1 ;
	i_private_data->i_wr_fp = NULL ;
	p_tmp->i_private = ( void* ) i_private_data ;

	// method
	p_tmp->ForcePin = S_ForcePin ;
	p_tmp->ForcePinOut = S_ForcePinOut ;
	p_tmp->ForcePinIn = S_ForcePinIn ;
	p_tmp->ForcePinRising = S_ForcePinRising ;
	p_tmp->ForcePinFalling = S_ForcePinFalling ;
	p_tmp->ForcePinBoth = S_ForcePinBoth ;
	p_tmp->Read = S_Read ;
	p_tmp->RtnPin = S_RtnPin ;
	p_tmp->RtnType = S_RtnType ;

	p_tmp->RtnInterruptFd = S_RtnInterruptFd ;
	p_tmp->Write = S_Write ;
	p_tmp->RtnWriteBefore = S_RtnWriteBefore ;
	p_tmp->RtnWriteNow = S_RtnWriteNow ;
	return p_tmp ;
}
void S_Gpio_Delete ( S_Gpio** obj ) {
//	S_Gpio_private* p_private = ( * obj )->i_private ;
	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;
	free ( * obj ) ;
	* obj = NULL ;
	return ;
}
