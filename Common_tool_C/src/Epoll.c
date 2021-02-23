/*
 * Epoll.c
 *
 *  Created on: 2017/5/28
 *      Author: richard
 */

#include "Epoll.h"

#if 1
typedef struct _S_private_data {
	int i_epl_fd ;		// epoll fd
	int i_epl_t ;		// epoll_wait times
	int i_epl_n ;		// epoll_wait num

//	struct epoll_event *iEvnts ;
} S_private_data ;

static void S_Epoll_Setup_wait_time_ms ( S_Epoll *obj , int ms ) ;
static void S_Epoll_Setup_wait_num ( S_Epoll *obj , int num ) ;
static int S_Epoll_CreateEpoll ( S_Epoll *obj ) ;
static int S_Rtn_epl_fd ( S_Epoll *obj ) ;	// 2017/12/19
static int S_Rtn_epl_t ( S_Epoll *obj ) ;	// 2017/12/19
static int S_Rtn_epl_n ( S_Epoll *obj ) ;	// 2017/12/19
static int S_Epoll_CloseEpoll ( S_Epoll*obj ) ;
static int S_Epoll_WaitEpoll ( S_Epoll *obj , void*i_events ) ;	// 2017/08/08
static int S_Epoll_AddEpollCtl ( S_Epoll*obj , int fd , E_epl_evnt Mod ) ;
static int S_Epoll_ModEpollCtl ( S_Epoll *obj , int fd , E_epl_evnt Mod ) ;
static int S_Epoll_DelEpollCtl ( S_Epoll *obj , int fd ) ;
static int S_Epoll_detect_hup ( S_Epoll *obj , int sock_fd ) ;

static void S_Epoll_Setup_wait_time_ms ( S_Epoll *obj , int ms ) {
	if ( ( ! obj ) || ( ! ms )  ) {
		return ;
	}
	S_private_data* p_private = obj->i_private ;
	p_private->i_epl_t = ms ;
	return ;
}

static void S_Epoll_Setup_wait_num ( S_Epoll *obj , int num ) {
	if ( ( ! obj ) || ( ! num )  ) {
		return ;
	}
	S_private_data* p_private = obj->i_private ;
	p_private->i_epl_n = num ;
	return ;
}

static int S_Epoll_CreateEpoll ( S_Epoll *obj ) {
	if ( ! obj ) {
		return - 1 ;
	}
	S_private_data* p_private = obj->i_private ;
	if ( 0 >= p_private->i_epl_n ) {
		return - 2 ;
	}

	p_private->i_epl_fd = epoll_create ( p_private->i_epl_n ) ;
	if ( p_private->i_epl_fd < 0 ) {
		return - 3 ;
	}

	return D_success ;	// success
}

static int S_Rtn_epl_fd ( S_Epoll *obj ) {
	if ( ! obj ) {
		return - 1 ;
	}
	S_private_data* p_private = obj->i_private ;
	return p_private->i_epl_fd ;
}

static int S_Rtn_epl_t ( S_Epoll *obj ) {
	if ( ! obj ) {
		return - 1 ;
	}
	S_private_data* p_private = obj->i_private ;
	return p_private->i_epl_t ;
}

static int S_Rtn_epl_n ( S_Epoll *obj ) {
	if ( ! obj ) {
		return - 1 ;
	}
	S_private_data* p_private = obj->i_private ;
	return p_private->i_epl_n ;
}

static int S_Epoll_CloseEpoll ( S_Epoll *obj ) {
	if ( ! obj ) {
		return - 1 ;
	}
	S_private_data* p_private = obj->i_private ;
	if ( - 1 < p_private->i_epl_fd ) {
		close ( p_private->i_epl_fd ) ;
		p_private->i_epl_fd = - 1 ;
	}
	return D_success ;	// success
}

static int S_Epoll_WaitEpoll ( S_Epoll *obj , void *i_events ) {
	if ( ! obj ) {
		return - 1 ;
	}
	S_private_data* p_private = obj->i_private ;
	return epoll_wait ( p_private->i_epl_fd , i_events , p_private->i_epl_n , p_private->i_epl_t ) ;
}

static int S_Epoll_AddEpollCtl ( S_Epoll*obj , int fd , E_epl_evnt Mod ) {
	if ( ( ! obj ) || ( ! fd ) || ( ! Mod ) ) {
		return - 1 ;
	}
	S_private_data* p_private = obj->i_private ;
	struct epoll_event evnt = { 0 } ;
	evnt.data.fd = fd ;
	evnt.events = Mod ;
	if ( - 1 == epoll_ctl ( p_private->i_epl_fd , EPOLL_CTL_ADD , fd , & evnt ) ) {
		return - 2 ;
	}
	return D_success ;	// success
}

static int S_Epoll_ModEpollCtl ( S_Epoll *obj , int fd , E_epl_evnt Mod ) {
	if ( ( ! obj ) || ( ! fd ) || ( ! Mod ) ) {
		return - 1 ;
	}
	S_private_data* p_private = obj->i_private ;
	struct epoll_event evnt = { 0 } ;
	evnt.data.fd = fd ;
	evnt.events = Mod ;
	if ( - 1 == epoll_ctl ( p_private->i_epl_fd , EPOLL_CTL_MOD , evnt.data.fd , & evnt ) ) {
		return - 2 ;
	}
	return D_success ;	// success
}

static int S_Epoll_DelEpollCtl ( S_Epoll *obj , int fd ) {
	if ( ( ! obj ) || ( ! fd ) ) {
		return - 1 ;
	}
	S_private_data* p_private = obj->i_private ;
	struct epoll_event evnt = { 0 } ;
	evnt.data.fd = fd ;
	evnt.events = EPOLLHUP ;
	if ( - 1 == epoll_ctl ( p_private->i_epl_fd , EPOLL_CTL_DEL , evnt.data.fd , & evnt ) ) {
		return - 2 ;
	}
	return D_success ;	// success
}




// ********** 為了用來偵測是否斷線 ********** //
// 初始化EPOLL
static int init_epoll ( int fd , enum EPOLL_EVENTS Mod ) ;
static int epoll_wait_event ( struct epoll_event* events , int fd ) ;
static int init_epoll ( int fd , enum EPOLL_EVENTS Mod ) {
	int efd = epoll_create ( 500 ) ;	// 建立Epoll
	if ( efd < 0 ) {
		return - 1 ;
	}
	struct epoll_event event = { 0 } ;
	event.data.fd = fd ;   	// 加入需要監控的fd
	event.events = Mod ;  	// EPOLLIN:可以讀
	int ectl = epoll_ctl ( efd , EPOLL_CTL_ADD , fd , & event ) ; // EPOLL ADD FD
	if ( ectl < 0 ) {
		return - 2 ;
	}
	return efd ;
}
// EPOLL Wait EPOLLHUP 事件
static int epoll_wait_event ( struct epoll_event* events , int fd ) {
	int n = epoll_wait( fd , events , MAXEVENTS , -1 );
	if ( n < 0 ) {
		return -1;
	}
	if ( EPOLLHUP & events->events ) {
		// 斷線
		return 1;
	}
	return D_success ;	// success
}
// ********** End of 為了用來偵測是否斷線 ********** //

// 偵測是否斷線
static int S_Epoll_detect_hup ( S_Epoll *obj , int fd ) {
	if ( ( ! obj ) || ( ! fd )) {
		return - 2 ;
	}
	// 偵測Client是否斷線，斷線則不讀資料Respon
	int check_fd = init_epoll ( fd , EPOLLOUT ) ;
	struct epoll_event check_events = { 0 } ;
	epoll_wait_event ( & check_events , check_fd ) ;
	close ( check_fd ) ;
	if ( EPOLLHUP & check_events.events ) {
perror ( "TCP is EPOLLHUP\n" ) ;
		return - 1 ;
	}
	return D_success ;	// success, not hup
}


S_Epoll * S_Epoll_New ( void ) {
	S_Epoll* p_tmp = ( S_Epoll* ) calloc ( 1 , sizeof(S_Epoll) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}

	S_private_data *i_private_data = ( S_private_data* ) calloc ( 1 , sizeof(S_private_data) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}
	i_private_data->i_epl_fd = - 1 ;
	i_private_data->i_epl_t = - 1 ;
	i_private_data->i_epl_n = 100 ;
//	i_private_data->iEvnts = NULL ;
	p_tmp->i_private = ( void* ) i_private_data ;

	p_tmp->Setup_wait_time_ms = S_Epoll_Setup_wait_time_ms ;
	p_tmp->Setup_wait_num = S_Epoll_Setup_wait_num ;
	p_tmp->Create = S_Epoll_CreateEpoll ;
	p_tmp->Rtn_epl_fd = S_Rtn_epl_fd ;		// 2017/12/19
	p_tmp->Rtn_epl_t = S_Rtn_epl_t ;			// 2017/12/19
	p_tmp->Rtn_epl_n = S_Rtn_epl_n ;			// 2017/12/19
	p_tmp->Close = S_Epoll_CloseEpoll ;
	p_tmp->Wait_Epoll = S_Epoll_WaitEpoll ;
//	p_tmp->Rtn_wait = S_Rtn_wait;

	p_tmp->Add_EpollCtl = S_Epoll_AddEpollCtl ;
	p_tmp->Mod_EpollCtl = S_Epoll_ModEpollCtl ;
	p_tmp->Del_EpollCtl = S_Epoll_DelEpollCtl ;

	p_tmp->Detect_hup = S_Epoll_detect_hup ;	// 2017/05/28 偵測斷線
	return p_tmp ;
}

void S_Epoll_Delete ( S_Epoll **obj ) {
//	S_private_data* p_private = ( * obj )->i_private ;
	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;
	if ( NULL != * obj ) {
		free ( * obj ) ;
		* obj = NULL ;
	}
	return ;
}

#endif



#if 0
void S_Epoll_Setup_wait_time_ms ( S_Epoll*obj , int ms ) {
	obj->i_epl_t = ms ;
	return ;
}

void S_Epoll_Setup_wait_num ( S_Epoll*obj , int num ) {
	obj->i_epl_n = num ;
	return ;
}

static int S_Epoll_CreateEpoll ( S_Epoll *obj ) {
	if ( ! obj ) {
		return - 1 ;
	}
	obj->i_epl_fd = epoll_create ( obj->i_epl_n ) ;
	if ( obj->i_epl_fd < 0 ) {
		return - 2 ;
	}

	return D_success ;	// success
}

static int S_Epoll_CloseEpoll ( S_Epoll*obj ) {
	if ( ! obj ) {
		return - 1 ;
	}
	if ( - 1 < obj->i_epl_fd ) {
		close ( obj->i_epl_fd ) ;
		obj->i_epl_fd = - 1 ;
	}
	return D_success ;	// success
}

static int S_Epoll_WaitEpoll ( S_Epoll*obj , void*iEvnts ) {
	if ( ! obj ) {
		return - 1 ;
	}
	return epoll_wait ( obj->i_epl_fd , iEvnts , obj->i_epl_n , obj->i_epl_t ) ;
}

static int S_Epoll_AddEpollCtl ( S_Epoll*obj , int fd , enum EPOLL_EVENTS Mod ) {
	if ( ! obj ) {
		return - 1 ;
	}
	struct epoll_event evnt = { 0 } ;
	evnt.data.fd = fd ;
	evnt.events = Mod ;
	if ( - 1 == epoll_ctl ( obj->i_epl_fd , EPOLL_CTL_ADD , fd , & evnt ) ) {
		return -2 ;
	}
	return 0 ;	// success
}

static int S_Epoll_ModEpollCtl ( S_Epoll*obj , int fd , enum EPOLL_EVENTS Mod ) {
	if ( ! obj ) {
		return - 1 ;
	}
	struct epoll_event evnt = { 0 } ;
	evnt.data.fd = fd ;
	evnt.events = Mod ;
	if ( - 1 == epoll_ctl ( obj->i_epl_fd , EPOLL_CTL_MOD , evnt.data.fd , & evnt ) ) {
		return -2 ;
	}
	return 0 ;	// success
}

static int S_Epoll_DelEpollCtl ( S_Epoll*obj , int fd ) {
	if ( ! obj ) {
		return - 1 ;
	}
	struct epoll_event evnt = { 0 } ;
	evnt.data.fd = fd ;
	evnt.events = EPOLLHUP ;
	if ( - 1 == epoll_ctl ( obj->i_epl_fd , EPOLL_CTL_DEL , evnt.data.fd , & evnt ) ) {
		return -2 ;
	}
	return 0 ;	// success
}

S_Epoll * S_Epoll_New ( void ) {
	S_Epoll* p_epl = ( S_Epoll* ) calloc ( 1 , sizeof(S_Epoll) ) ;
	if ( NULL == p_epl ) {	// calloc fail
		return NULL ;
	}
	p_epl->i_epl_fd = - 1 ;
	p_epl->i_epl_t = - 1 ;
	p_epl->i_epl_n = 100 ;
	p_epl->Epoll_Setup_wait_time_ms = S_Epoll_Setup_wait_time_ms ;
	p_epl->Epoll_Setup_wait_num = S_Epoll_Setup_wait_num ;
	p_epl->Epoll_CreateEpoll = S_Epoll_CreateEpoll ;
	p_epl->Epoll_CloseEpoll = S_Epoll_CloseEpoll ;
	p_epl->Epoll_WaitEpoll = S_Epoll_WaitEpoll ;

	p_epl->Epoll_AddEpollCtl = S_Epoll_AddEpollCtl ;
	p_epl->Epoll_ModEpollCtl = S_Epoll_ModEpollCtl ;
	p_epl->Epoll_DelEpollCtl = S_Epoll_DelEpollCtl ;

	return p_epl ;
}

void S_Epoll_Delete ( S_Epoll** obj ) {
	if ( NULL != * obj ) {
		free ( * obj ) ;
	}
	* obj = NULL ;

	return ;
}

// 初始化EPOLL
int init_epoll ( int fd , enum EPOLL_EVENTS Mod ) {
	int efd = epoll_create ( 500 ) ;	// 建立Epoll
	if ( efd < 0 ) {
		MyLog ( "%s" , "epoll_create error" ) ;
		return -1 ;
	}

	struct epoll_event event = { 0 } ;
	event.data.fd = fd ;   //加入需要監控的fd
	event.events = Mod ;  // EPOLLIN:可以讀
	int ectl = epoll_ctl ( efd , EPOLL_CTL_ADD , fd , & event ) ; // EPOLL ADD FD
	if ( ectl < 0 ) {
		MyLog ( "%s" , "epoll_ctl error" ) ;
		return -2 ;
	}

	return efd ;
}

// EPOLL Wait EPOLLHUP 事件
int epoll_wait_event ( struct epoll_event* events , int fd ) {
	int n = epoll_wait ( fd , events , MAXEVENTS , - 1 ) ;
	if ( n < 0 ) {
		return - 1 ;
	}
	if ( EPOLLHUP & events->events ) {
		// 斷線
		return 1 ;
	}
	return 0 ;
}
#endif
