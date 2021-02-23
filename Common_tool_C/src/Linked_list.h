/*
 * Linked_list.h
 *
 *  Created on: 2017/5/10
 *      Author: richard
 */

#ifndef LINKED_LIST_H_
#define LINKED_LIST_H_

#include "Common_Tool.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

typedef struct _S_list S_list ;
struct _S_list {
	int ct ;
	int type ;			// node type
	void *p_data ;		// pointer to data
	int data_sz ;		// data sz
	S_list *next ;		// next node
} ;

/*
 * SYNOPSIS :
 * 		#include "Linked_list.h"
 * 		int Linked_list_add_node ( S_list **head , S_list* node ) ;
 *
 * DESCRIPTION :
 * 		新增新的節點到list之中
 *
 * PARAMETER :
 * 		 S_list **head  : Linked list的head
 * 		 S_list* node : 欲新增的節點
 *
 * RETURN VALUE :
 *		0 is success , others are fail
 *
 * NOTES :
 */
extern int Linked_list_add_node ( S_list **head , S_list *node ) ;

/*
 * SYNOPSIS :
 * 		#include "Linked_list.h"
 * 		S_list *Linked_list_search ( S_list *head , int ct ) ;
 *
 * DESCRIPTION :
 * 		搜尋list之中某節點
 *
 * PARAMETER :
 * 		 S_list **head  : Linked list的head
 * 		 int ct : 欲搜尋的ct
 *
 * RETURN VALUE :
 *		 返回S_list 的結構，NULL為無此節點
 *
 * NOTES :
 *
 */
extern S_list *Linked_list_search ( S_list *head , int ct ) ;

/*
 * SYNOPSIS :
 * 		#include "Linked_list.h"
 * 		int Linked_list_delete_node ( S_list **head , int ct ) ;
 *
 * DESCRIPTION :
 * 		刪除list之中某節點
 *
 * PARAMETER :
 * 		 S_list **head  : Linked list的head
 * 		 int ct : 欲刪除的ct
 *
 * RETURN VALUE :
 *		 0 is success , others are fail
 *
 * NOTES :
 */
extern int Linked_list_delete_node ( S_list **head , int ct ) ;

/*
 * SYNOPSIS :
 * 		#include "Linked_list.h"
 * 		int Linked_list_print ( S_list *head ) ;
 *
 * DESCRIPTION :
 * 		顯示list之中的資料
 *
 * PARAMETER :
 * 		 S_list **head  : Linked list的head
 *
 * RETURN VALUE :
 *		 0 is success , others are fail
 *
 * NOTES :
 */
extern int Linked_list_print ( S_list *head ) ;

/*
 * SYNOPSIS :
 * 		#include "Linked_list.h"
 * 		int Rtn_list_node_ct ( S_list *head ) ;
 *
 * DESCRIPTION :
 * 		回吐list之中節點數量
 *
 * PARAMETER :
 * 		 S_list **head  : Linked list的head
 *
 * RETURN VALUE :
 *		 list node number is success , 0 is fail
 *
 * NOTES :
 */
extern int Rtn_Linked_list_node_ct ( S_list *head ) ;


#if 0
// Linked list測試程式
int main(void) {

	S_list* head =NULL ;	// list head
	S_list node= {0} ;	// 要增加的資料

	// node.m_mlc_command....... ; >>　設定結構資料

	Linked_list_add_node(&head ,&node) ;

}
#endif

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "Common_Tool.h"


typedef struct _S_List S_List ;
typedef struct _S_list_node S_list_node ;

struct _S_List {
	void *i_private ;
	int (*Linked_list_add_node) ( S_list_node *node , S_list_node* node ) ;	// 加入節點
	S_list_node *(*Linked_list_search) ( S_list_node *node , int ct ) ;		// 搜尋節點
	int (*Linked_list_delete_node) ( S_list_node *node , int ct ) ;			// 刪除節點
	int (*Linked_list_print) ( S_list_node *node ) ;						// 顯示串列鍊結所有節點
	int (*Rtn_Linked_list_node_ct) ( S_list_node *node ) ;					// 回吐串列節點數量
};
#endif



#endif /* LINKED_LIST_H_ */
