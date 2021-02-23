/*
 * My_Sqlite3.c
 *
 *  Created on: 2017年11月15日
 *      Author: richard
 */
#include "My_Sqlite3.h"

typedef struct _S_private_data {
	sqlite3 *i_db ;			// db handle
	sqlite3_stmt *i_stmt;


	Str i_err_msg ;			// 錯誤信息
	Str mlc_callback_msg ;	// callback function message

} S_private_data ;

// 僅內部使用 //
static int S_Open_db ( S_Sqlite3 *obj , CStr db_path ) ;
static int S_Close_db ( S_Sqlite3 *obj ) ;
static int S_Insert_into_table ( S_Sqlite3 *obj , CStr table_name , CStr fieldname , CStr value ) ;
static int S_Select_from ( S_Sqlite3 *obj , CStr data , CStr table_name ) ;
static int S_Select_from_where ( S_Sqlite3 *obj , CStr table_name , CStr data , CStr where ) ;
static int S_Delete_from_where ( S_Sqlite3 *obj , CStr table_name , CStr where ) ;
static int S_Update_set_where ( S_Sqlite3 *obj , CStr table_name , CStr field_name_value , CStr where ) ;
static Str S_Rtn_err_msg ( S_Sqlite3 *obj ) ;

static Str S_Rtn_callback_msg ( S_Sqlite3 *obj ) ;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for(i=0; i<argc; i++){
	   printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
//   printf("=========\n");
   return 0;
}
// 僅內部使用 //

static int S_Open_db ( S_Sqlite3 *obj , CStr db_path ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! db_path ) ) {
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;
	int rtn = sqlite3_open ( db_path , & p_private->i_db ) ;
	if ( SQLITE_OK != rtn ) {
		return - 2 ;
	}
	return D_success ;
}
static int S_Close_db ( S_Sqlite3 *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;
	if ( NULL == p_private->i_db ) {
		return D_success ;
	}
	int rtn = sqlite3_close ( p_private->i_db ) ;
	if ( SQLITE_BUSY == rtn ) {
		return - 2 ;
	}
	return D_success ;
}

/* INSERT INTO <Table Name> (<Field 1>, <Field 2>, ......... <Field n>) VALUES (<Value 1>, <Value 2>, .......... <Value n>);
 * ex:"insert into Table (id, name) values (1,"Light Sensor");"
 *    "insert into Table (id, name) values (2,"Temperature Sensor");"
 */
#define DF_Insert	"INSERT INTO %s (%s) VALUES (%s);"
static int S_Insert_into_table ( S_Sqlite3 *obj , CStr table_name , CStr fieldname , CStr value ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! table_name )|| ( ! fieldname )|| ( ! value ) ) {
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;

	char insert_str [ strlen ( DF_Insert ) + strlen ( table_name ) + strlen ( fieldname ) + strlen ( value ) + 1 ] ;
	memset ( insert_str , 0 , sizeof ( insert_str ) ) ;
	snprintf ( insert_str , sizeof ( insert_str ) , DF_Insert , table_name , fieldname , value ) ;

	int rtn  = sqlite3_exec ( p_private->i_db , insert_str , NULL , 0 , & p_private->i_err_msg ) ;
	if ( rtn != SQLITE_OK ) {
		fprintf ( stderr , "SQL error: %s\n" , p_private->i_err_msg ) ;
		sqlite3_free ( p_private->i_err_msg ) ;
		return - 2 ;
	} else {
		fprintf ( stdout , "Records created successfully\n" ) ;
	}

	return D_success ;
}

/* SELECT * FROM <Table Name>;
 * ex:"select * from Table;"
 */
#define DF_Select_from	"SELECT %s from %s;"
static int S_Select_from ( S_Sqlite3 *obj , CStr data , CStr table_name ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! data ) || ( ! table_name ) ) {
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;
	char select_str [ strlen ( DF_Select_from ) + strlen ( data ) + strlen ( table_name ) + 1 ] ;
	memset ( select_str , 0 , sizeof ( select_str ) ) ;
	snprintf ( select_str , sizeof ( select_str ) , DF_Select_from , data , table_name ) ;

	int rtn  = sqlite3_exec ( p_private->i_db , select_str , callback , 0 , & p_private->i_err_msg ) ;
	if ( rtn != SQLITE_OK ) {
		fprintf ( stderr , "SQL error: %s\n" , p_private->i_err_msg ) ;
		sqlite3_free ( p_private->i_err_msg ) ;
		return - 2 ;
	}

	return D_success ;
}

/* SELECT * FROM <Table Name>;
 * ex:"select * from Table where id=1;"
 */
static int S_Select_from_where ( S_Sqlite3 *obj , CStr table_name , CStr data , CStr where ) {

	return D_success ;
}

/* DELETE FROM <Table Name> WHERE <Field1>
 * delete from Table where id=2;
 */
static int S_Delete_from_where ( S_Sqlite3 *obj , CStr table_name , CStr where ) {

	return D_success ;
}

/* UPDATE <Table Name> SET <Field And Value> WHERE <Field And Value>
 * ex:"update Table set name='Humidity Sensor' where name='Light Sensor';"
 */
static int S_Update_set_where ( S_Sqlite3 *obj , CStr table_name , CStr field_name_value , CStr where ) {

	return D_success ;
}
static Str S_Rtn_err_msg ( S_Sqlite3 *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = obj->i_private ;
	return p_private->i_err_msg ;
}

static Str S_Rtn_callback_msg ( S_Sqlite3 *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = obj->i_private ;
	return p_private->mlc_callback_msg ;
}

S_Sqlite3 *S_Sqlite3_tool_New ( void ) {
	S_Sqlite3 *p_tmp = ( S_Sqlite3* ) calloc ( 1 , sizeof(S_Sqlite3) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}
	S_private_data *i_private_data = ( S_private_data* ) calloc ( 1 , sizeof(S_private_data) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}

	i_private_data->i_db = NULL ;
	i_private_data->i_stmt = NULL ;
	i_private_data->i_err_msg = NULL ;
	i_private_data->mlc_callback_msg = NULL ;
	p_tmp->i_private = i_private_data ;

	p_tmp->Open_db = S_Open_db ;
	p_tmp->Close_db = S_Close_db ;
	p_tmp->Insert_into_table = S_Insert_into_table ;
	p_tmp->Select_from = S_Select_from ;
	p_tmp->Select_from_where = S_Select_from_where ;
	p_tmp->Delete_from_where = S_Delete_from_where ;
	p_tmp->Update_set_where = S_Update_set_where ;

	p_tmp->Rtn_err_msg = S_Rtn_err_msg ;
	p_tmp->Rtn_callback_msg = S_Rtn_callback_msg ;

	return p_tmp ;
}
void S_Sqlite3_tool_Delete ( S_Sqlite3 **obj ) {
	S_private_data *p_private = ( * obj )->i_private ;
	if ( p_private->i_db != NULL ) {
		sqlite3_close ( p_private->i_db ) ;
	}

	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;

	free ( * obj ) ;
	* obj = NULL ;
	return ;
}
