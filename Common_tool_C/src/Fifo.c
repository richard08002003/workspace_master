/*
 * Fifo.c
 *
 *  Created on: 2017年12月7日
 *      Author: richard
 */
#include "Fifo.h"

typedef struct _S_private_data {
	int i_fd ;
	Str mlc_read_data ;	// 讀到的資料
	int i_read_data_sz ;
} S_private_data ;

static int S_Mkfifo ( S_Fifo *obj , CStr path_name , mode_t mode ) ;
static int S_Open ( S_Fifo *obj , CStr path_name , int flags ) ;
static void S_Close ( S_Fifo *obj ) ;
static int S_Read ( S_Fifo *obj ) ;
static Str S_Rtn_read_data ( S_Fifo *obj ) ;
static int S_Write ( S_Fifo *obj , CStr msg ) ;

static int S_Mkfifo ( S_Fifo *obj , CStr path_name , mode_t mode ) {
	if ( ( ! obj ) || ( ! path_name ) || ( ! mode ) ) {
		return - 1 ;
	}
	int rtn = mkfifo ( path_name , mode ) ;
	if ( rtn != 0 ) {
		return - 2 ;
	}
	return D_success ;	// success
}

static int S_Open ( S_Fifo *obj , CStr path_name , int flags ) {
	if ( ( ! obj ) || ( ! path_name ) || ( ! flags ) ) {
		return - 1 ;
	}

printf ( ">> Will S_Open FIFO\n" ) ;
	S_private_data* p_private = obj->i_private ;
	int fd = open ( path_name , flags ) ;			// 有問題 //
	if ( fd == -1 ) {
printf ( ">> S_Open FIFO error\n" ) ;
		p_private->i_fd = fd ;
		return - 2 ;
	}
	p_private->i_fd = fd ;
printf ( ">> S_Open FIFO success\n" ) ;

	return D_success ;	// success
}

static void S_Close ( S_Fifo *obj ) {
	if ( ! obj ) {
		return ;
	}
	S_private_data* p_private = obj->i_private ;
	if ( p_private->i_fd > 0 ) {
		close ( p_private->i_fd ) ;
	}
	return ;
}

static int S_Read ( S_Fifo *obj ) {
	if ( ! obj ) {
		return - 1 ;
	}
	S_private_data* p_private = obj->i_private ;
	if ( p_private->i_fd < 0 ) {
		return - 2 ;
	}

	p_private->i_read_data_sz = 0 ; // 總量
	while ( 1 ) {
		char msg [ 8191 + 1 ] = { 0 } ;
		int rtn = read ( p_private->i_fd , msg , sizeof ( msg ) - 1 ) ;
		if ( rtn == 0 ) {
			break ;
		} else if ( rtn == - 1 ) {
			if ( p_private->i_read_data_sz > 0 ) {
				break ;
			}
			continue ;
		}

		p_private->mlc_read_data = ( char* ) realloc ( p_private->mlc_read_data , p_private->i_read_data_sz + rtn + 1 ) ;
		if ( p_private->mlc_read_data == NULL ) {
			return - 3 ;
		}
		memcpy ( p_private->mlc_read_data + p_private->i_read_data_sz , msg , rtn ) ;
		( p_private->mlc_read_data ) [ p_private->i_read_data_sz + rtn ] = 0 ;
		p_private->i_read_data_sz += rtn ;
	}

	return D_success ;	// success
}

static Str S_Rtn_read_data ( S_Fifo *obj ) {
	if ( ! obj ) {
		return NULL ;
	}
	S_private_data* p_private = obj->i_private ;
	return p_private->mlc_read_data ;
}

static int S_Write ( S_Fifo *obj , CStr msg ) {
	if ( ( ! obj ) || ( ! msg ) ) {
		return - 1 ;
	}
	S_private_data* p_private = obj->i_private ;
	if ( p_private->i_fd < 0 ) {
		return - 2 ;
	}
	int rtn = write ( p_private->i_fd , msg , strlen ( msg ) ) ;
	if ( rtn < 0 ) {
		return - 3 ;	// write error
	}
	return D_success ;	// success
}

S_Fifo *S_Fifo_New ( void ) {
	S_Fifo* p_tmp = ( S_Fifo* ) calloc ( 1 , sizeof(S_Fifo) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}
	S_private_data *i_private_data = ( S_private_data* ) calloc ( 1 , sizeof(S_private_data) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}
	i_private_data->i_fd = - 1 ;
	i_private_data->mlc_read_data = NULL ;
	i_private_data->i_read_data_sz = 0 ;
	p_tmp->i_private = ( void* ) i_private_data ;

	p_tmp->Mkfifo = S_Mkfifo ;
	p_tmp->Open = S_Open ;
	p_tmp->Close = S_Close ;
	p_tmp->Read = S_Read ;
	p_tmp->Rtn_read_data = S_Rtn_read_data ;
	p_tmp->Write = S_Write ;

	return p_tmp ;
}

void S_Fifo_Delete ( S_Fifo **obj ) {
	S_private_data* p_private = ( * obj )->i_private ;
	if ( p_private->i_fd > 0 ) {
		close ( p_private->i_fd ) ;
	}
	free ( p_private->mlc_read_data ) ;
	p_private->mlc_read_data = NULL ;

	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;
	if ( NULL != * obj ) {
		free ( * obj ) ;
		* obj = NULL ;
	}
	return ;
}
