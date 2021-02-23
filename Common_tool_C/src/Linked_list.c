/*
 * Linked_list.c
 *
 *  Created on: 2017/5/10
 *      Author: richard
 */
#include "Linked_list.h"

#if 1
int Linked_list_add_node ( S_list **head , S_list *node ) {
	if ( ( ! head ) || ( ! node ) ) {
		return - 1 ;
	}
	S_list *new_node = ( S_list* ) calloc ( 1 , sizeof(S_list) ) ;
	if ( ! new_node ) {
		return - 2 ;
	}

	new_node->ct = node->ct ;
	new_node->type = node->type ;
	new_node->data_sz = node->data_sz ;
	new_node->p_data = node->p_data ;

	if ( ! ( * head ) ) {
		( * head ) = new_node ;
		return D_success ;
	}

	S_list *ptr = * head ;
	while ( ptr->next ) {
		ptr = ptr->next ;
	}

	ptr->next = new_node ;
	return D_success ;
}

S_list *Linked_list_search ( S_list *head , int ct ) {
	if ( ( ! head ) || ( ! ct ) ) {
		return NULL ;
	}
	while ( head ) {
		if ( ct == head->ct ) {
			return head ;
		}
		head = head->next ;
	}
	return NULL ;
}

int Linked_list_delete_node ( S_list **head , int ct ) {
	if ( ( ! head ) || ( ! * head ) || ( ! ct ) ) {
		return - 1 ;
	}

	S_list *p_node = Linked_list_search ( * head , ct ) ;	// 搜尋節點
	if ( NULL == p_node ) {
		return - 2 ;
	}

	if ( p_node == * head ) {	// 若欲刪除的節點為head
		S_list *p_new_head = ( * head )->next ;	// 下個節點為新head

		// free data
		p_node->p_data = NULL ;

		free ( p_node ) ;
		p_node = NULL ;

		* head = p_new_head ;
		return D_success ;
	}

	S_list *p_tmp = * head ;
	while ( p_tmp ) {
		if ( p_tmp->next == p_node ) {

			p_tmp->next = p_node->next ;

			// free data
			p_node->p_data = NULL ;

			free ( p_node ) ;
			p_node = NULL ;

			return 0 ;
		}
		p_tmp = p_tmp->next ;
	}

	return D_success ;
}

int Linked_list_print ( S_list *head ) {
	if ( NULL == head ) {
		printf("list is NULL\n") ;
		return - 1 ;
	}
	S_list *ptr = head ;
	while ( NULL != ptr ) {
		printf ( "ptr->ct = %d, " , ptr->ct ) ;
		printf ( "ptr->type = %02d, " , ptr->type ) ;
		printf ( "ptr->data_sz = %d\n" , ptr->data_sz ) ;
		ptr = ptr->next ;
	}
	return D_success ;
}

int Rtn_Linked_list_node_ct ( S_list *head ) {
	if ( ! head ) {
		return 0 ;
	}
	S_list *ptr = head ;
	int ct = 0 ;
	while ( NULL != ptr ) {
		ct ++ ;
		ptr = ptr->next ;
	}
	return ct ;
}
#endif


#if 0
// 2017/08/17 Richard
typedef struct _S_private_data {
	S_list_node *head ;			// list 的頭
//	S_list *search_node ;	// search 到的節點
	int list_node_num ;		// list 的數量

} S_private_data ;

// node
struct _S_list_node {
	int ct ;
	int type ;			// node type
	void *p_data ;		// pointer to data
	int data_sz ;		// data sz
	S_list_node *next ;		// next node
} ;
#endif
