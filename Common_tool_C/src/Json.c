/*
 * Json.c
 *
 *  Created on: 2015/7/7
 *      Author: richard
 */
/**JSON Parser*/
#include "Json.h"

typedef struct stuJson S_Json;
// Json struct
struct stuJson {
	void (*JsonFirstDecodeMethod) ( S_Json* ) ;
	void (*UnJsonFuncInitialMethod) ( S_Json** ) ;

	char *inputStr ;
	char *willSearchStr ;

	char *rtnPtrOut ;
	int rtnPtrOutSz ;
	int rtnOutSz ;
	int rtnMethodStatus ;
};

#define      DF_ERR_PARAMETER 		( -1 )
#define      DF_JSON_ERR_QMARKS 	( -2 )
#define      DF_JSON_ERR_FORMAT    	( -3 )
#define      DF_JSON_ERR_SEARCH    	( -4 )
#define      DF_JSON_ERR_COLON    	( -5 )
#define      DF_JSON_ERR_COMMA    	( -6 )
#define      DF_JSON_ERR_BUFFER    	( -7 )
#define      DF_JSON_ERR_OBJ   		( -8 )
#define      DF_JSON_ERR_ARY   		( -9 )

#define 	 JSON_FP_ERR			( -10 )
#define 	 JSON_VALUE_ERR			( -11 )

#define      DF_SUCCESS   			( 1 )
#define      DF_JSON_VALUE   		( 2 )
#define      DF_JSON_STRING   		( 3 )
#define      DF_JSON_OBJECT   		( 4 )
#define      DF_JSON_ARRAY   		( 5 )

#define      TRUE   1

static int CompileJsonFormat ( char *str ) {
	if ( NULL == str ) {
		return DF_ERR_PARAMETER;
	}

	char* ptrBrckts = str;
	int bigLeft = 0X00;    // {
	int medLeft = 0X00;    // [
	int qttnMrk = 0X00;    // "
	int firstQttnMrk = 0X00;    // ",¬ÝÀÉ®×€º¬O§_Š³ "
	int contI = 0X00;
	int firstTag = 0X00;   //­Y¬O¶}ÀY ¬° '{' or '[' «hªí¥Ü pass

	for ( ; ptrBrckts[ contI ] ; ++contI ) {
		if ( '\"' == ptrBrckts[ contI ] ) {
			++qttnMrk;
			firstQttnMrk = 1;
		} else if ( ('{' == ptrBrckts[ contI ]) && (0X00 == (qttnMrk & 0X01)) ) {
			if ( (0 < contI) && (0 == bigLeft) && (0 == firstTag) ) { //¶}ÀY«D '{'
				return DF_JSON_ERR_OBJ;   //€j¬Až¹Š³°ÝÃD
			}
			firstTag = 1;   //ªí¥Ü¶}ÀY pass
			++bigLeft;
		} else if ( ('}' == ptrBrckts[ contI ]) && (0X00 == (qttnMrk & 0X01)) ) {
			--bigLeft;
		} else if ( ('[' == ptrBrckts[ contI ]) && (0X00 == (qttnMrk & 0X01)) ) {
			if ( (0 < contI) && (0 == medLeft) && (0 == firstTag) ) { //¶}ÀY«D '['
				return DF_JSON_ERR_ARY;
			}
			firstTag = 1;   //ªí¥Ü¶}ÀY pass
			++medLeft;
		} else if ( (']' == ptrBrckts[ contI ]) && (0X00 == (qttnMrk & 0X01)) ) {
			--medLeft;
		}
		/* §PÂ_¬Až¹³¡¥÷ */
		if ( 0 > bigLeft ) {
			return DF_JSON_ERR_OBJ;
		}
		if ( 0 > medLeft ) {
			return DF_JSON_ERR_ARY;
		}
	}
	if ( 0 != bigLeft ) {   //{} €£§å°t
		return DF_JSON_ERR_OBJ;

	} else if ( 0 != medLeft ) {   //[] €£§å°t
		return DF_JSON_ERR_ARY;

	} else if ( (0X00 != (qttnMrk & 0X01)) || (0 == firstQttnMrk) ) {  // " «DÂùŒÆ
		return DF_JSON_ERR_QMARKS;
	} else if ( 0 == firstTag ) {   //µL°}ŠC»Pª«¥ó
		return DF_JSON_ERR_FORMAT;
	}
	return DF_SUCCESS;
}

/*
 * ¥D­n¹BŠæµ{Š¡
 * -1 °ÑŒÆ¿ù»~
 * -2 ®æŠ¡¿ù»~
 * -3 ·jŽM€£šì©Ò­n§äªºŠrŠêŠWºÙ
 */
static void SearchJson ( S_Json *obj ) {
	/* °ÑŒÆÀË¬d */
	int boolLen = (NULL == obj);
	boolLen |= (NULL == obj->inputStr);
	boolLen |= (NULL == obj->willSearchStr);
	boolLen |= (NULL == obj->rtnPtrOut);
	boolLen |= (0 >= obj->rtnPtrOutSz);
	if ( 1 == boolLen ) {
		obj->rtnMethodStatus = DF_ERR_PARAMETER;
		return;
	}
	/* END °ÑŒÆÀË¬d */
	memset( obj->rtnPtrOut , 0 , obj->rtnPtrOutSz );   // 2014 02 14

	char sourStrTmp[ strlen( obj->inputStr ) + 1 ];
	snprintf( sourStrTmp , sizeof(sourStrTmp) , "%s" , obj->inputStr );

	int checkFormat = CompileJsonFormat( sourStrTmp ); /////////////////////
	if ( DF_SUCCESS != checkFormat ) {
		obj->rtnMethodStatus = checkFormat;
		return;
	}

	char searStrTmp[ 1 + strlen( obj->willSearchStr ) + 1 + 1 ];
	snprintf( searStrTmp , sizeof(searStrTmp) , "\"%s\"" , obj->willSearchStr ); //±N·jŽMŠr¥[€WÂù€Þž¹ ""
	char *sPtr = strstr( sourStrTmp , searStrTmp );
	if ( NULL == sPtr ) {
		obj->rtnMethodStatus = DF_JSON_ERR_SEARCH;
		return;
	}
	sPtr += strlen( searStrTmp );   //§äšì«á·Ç³Æ§ìšä­È

	/* ±N©ÒŠ³ ascii žü€J */
	char strCmp[ 126 + 1 ] = { 0 };   //©ÒŠ³ ascii ¥þ³¡¯Ç€J,€£§tµ²§ôŠr€ž

	int contI = 0x00;   //ary
	int num = 0x00;   //ascii
	for ( ; num < sizeof(strCmp) ; ) {
		if ( 0x20 != (++num) ) {   //±Æ°£ªÅ¥Õ
			strCmp[ contI++ ] = num;
		}
	}
	/* END ±N©ÒŠ³ ascii žü€J */

	sPtr = strpbrk( sPtr , strCmp );
	if ( ':' != *sPtr ) {
		obj->rtnMethodStatus = DF_JSON_ERR_COLON;
		return;
	}
	sPtr += 1;
	sPtr = strpbrk( sPtr , strCmp );
	if ( ('0' <= *sPtr) && ('9' >= *sPtr) ) {   //¥Nªí¬°ŒÆ­È
		char *ePtr = strpbrk( sPtr + 1 , ",]}" );
		if ( NULL == ePtr ) {
			obj->rtnMethodStatus = DF_JSON_ERR_COMMA;
			return;
		}
		*ePtr = 0;
		obj->rtnMethodStatus = DF_JSON_VALUE;

		obj->rtnOutSz = snprintf( obj->rtnPtrOut , obj->rtnPtrOutSz , "%d" , atoi( sPtr ) );   //¶¶«Kšú±o€j€p
		if ( obj->rtnPtrOutSz <= obj->rtnOutSz ) {
			obj->rtnMethodStatus = DF_JSON_ERR_BUFFER;
		}
		return;
	} else if ( '\"' == *sPtr ) {   //ªí¥Ü¬OŠrŠê
		sPtr += 1;
		char* ePtr = strpbrk( sPtr , "\"" );
		if ( NULL == ePtr ) {
			obj->rtnMethodStatus = DF_JSON_ERR_QMARKS;
			return;
		}
		*ePtr = 0;
		obj->rtnMethodStatus = DF_JSON_STRING;

		obj->rtnOutSz = snprintf( obj->rtnPtrOut , obj->rtnPtrOutSz , "%s" , sPtr ); //¶¶«Kšú±o€j€p
		if ( obj->rtnPtrOutSz <= obj->rtnOutSz ) {
			obj->rtnMethodStatus = DF_JSON_ERR_BUFFER;
		}
		return;
	} else if ( '{' == *sPtr ) {
		char *sPtrTmp = sPtr;
		int left = 0;
		int right = 0;
		while ( 1 ) {
			char *chrTmp = strpbrk( sPtrTmp , "{}" );
			if ( '{' == *chrTmp ) {
				++left;
			} else if ( '}' == *chrTmp ) {
				++right;
				if ( left < right ) {
					obj->rtnMethodStatus = DF_JSON_ERR_OBJ;
					return;
				} else if ( (left == right) ) {   //¶ÇŠ^ª«¥ó
					*(chrTmp + 1) = 0;
					obj->rtnMethodStatus = DF_JSON_OBJECT;

					obj->rtnOutSz = snprintf( obj->rtnPtrOut , obj->rtnPtrOutSz , "%s" , sPtr );   //¶¶«Kšú±o€j€p
					if ( obj->rtnPtrOutSz <= obj->rtnOutSz ) {
						obj->rtnMethodStatus = DF_JSON_ERR_BUFFER;
					}
					return;
				}
			} else if ( NULL == chrTmp ) {
				obj->rtnMethodStatus = DF_JSON_ERR_OBJ;
				return;
			}
			sPtrTmp = (chrTmp + 1);
		}
	} else if ( '[' == *sPtr ) {
		char *sPtrTmp = sPtr;
		int left = 0;
		int right = 0;
		while ( 1 ) {
			char *chrTmp = strpbrk( sPtrTmp , "[]" );
			if ( '[' == *chrTmp ) {
				++left;
			} else if ( ']' == *chrTmp ) {
				++right;
				if ( left < right ) {
					obj->rtnMethodStatus = DF_JSON_ERR_ARY;
					return;
				} else if ( (left == right) ) {   //¶ÇŠ^°}ŠC
					*(chrTmp + 1) = 0;
					obj->rtnMethodStatus = DF_JSON_ARRAY;

					obj->rtnOutSz = snprintf( obj->rtnPtrOut , obj->rtnPtrOutSz , "%s" , sPtr );   //¶¶«Kšú±o€j€p
					if ( obj->rtnPtrOutSz <= obj->rtnOutSz ) {
						obj->rtnMethodStatus = DF_JSON_ERR_BUFFER;
					}
					return;
				}
			} else {
				obj->rtnMethodStatus = DF_JSON_ERR_ARY;
				return;
			}
			sPtrTmp = (chrTmp + 1);
		}
	} else {
		obj->rtnMethodStatus = DF_JSON_ERR_FORMAT;   //®æŠ¡¿ù»~
		return;
	}
	obj->rtnMethodStatus = DF_SUCCESS;
	return;
}

static void UnJsonFuncInitial ( S_Json** obj ) {
	if ( NULL != *obj ) {
		free( *obj );
	}
	*obj = NULL;
	return;
}

static S_Json * JsonFuncInitial ( void ) {
	S_Json *ptr = (S_Json *) calloc( 1 , sizeof(S_Json) );
	if ( NULL == ptr ) {
		return NULL;
	}

	ptr->JsonFirstDecodeMethod = SearchJson;
	ptr->UnJsonFuncInitialMethod = UnJsonFuncInitial;
	return ptr;
}


//JSON Value
int Json_value ( char* input , char *search_key , char *bufferptr , int buffersz ) {
	S_Json* obj = JsonFuncInitial( ); //初始化
	obj->inputStr = input; //資料來源

	char json_value[ 10240 ] = { 0 };
	obj->willSearchStr = search_key;
	obj->rtnPtrOut = json_value;
	obj->rtnPtrOutSz = sizeof(json_value);
	obj->JsonFirstDecodeMethod( obj );

	if ( DF_SUCCESS > obj->rtnMethodStatus ) {
		fprintf( stderr , "Json error :%d\n" , obj->rtnMethodStatus );
 		if ( obj->rtnMethodStatus == -4 ) {
 			fprintf( stderr , "\"%s\" Json Key Can Not Search\n" , search_key );
			fflush( stderr );
		} else if ( obj->rtnMethodStatus == -3 ) {
 			fprintf( stderr , "%s" , "Json Fromat of Value is Error\n" );
			fflush( stderr );
		} else if ( obj->rtnMethodStatus == -8 ) {
 			fprintf( stderr , "%s" , "Fromat of Data is Not JSON\n" );
			fflush( stderr );
		} else if ( obj->rtnMethodStatus == -5 ) {
 			fprintf( stderr , "\"%s\" is Not Json Key\n" , search_key );
			fflush( stderr );
		}

		return JSON_VALUE_ERR;
	}

	char *nptr = strrchr( json_value , '\n' );
	if ( nptr != NULL ) {
		*nptr = 0;
	}
	char *rptr = strrchr( json_value , '\r' );
	if ( rptr != NULL ) {
		*rptr = 0;
	}

	snprintf( bufferptr , buffersz , "%s" , json_value );

	memset( json_value , 0 , sizeof(json_value) );
	obj->UnJsonFuncInitialMethod( &obj ); //解構

	return D_success ;
}

/*
 * 確認是否為JSON格式
 * return -1 : Not Json Format
 * return 0 : Is Json Format
 * */
int checkJsonFormat ( char* input ) {
	int inputSz = strlen ( input ) ;
	int si = 0 , curly_bracket = 0 , double_quotes = 0 ;

	for ( si = 0 ; si < inputSz ; si ++ ) {
		if ( input [ si ] == '{' ) {
			curly_bracket += 1 ;
		} else if ( input [ si ] == '}' ) {
			curly_bracket -= 1 ;
		} else if ( input [ si ] == '\"' ) {
			double_quotes ++ ;
		}
	}

	if ( ( curly_bracket != 0 ) || ( ( double_quotes & 0x01 ) != 0 ) ) {
		return - 1 ;
	}

	return D_success ; // JSON 格式 > return D_success//
}

