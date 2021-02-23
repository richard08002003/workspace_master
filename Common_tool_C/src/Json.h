/*
 * Json.h
 *
 *  Created on: 2017年8月8日
 *      Author: richard
 */

#ifndef INC_JSON_H_
#define INC_JSON_H_

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "Common_Tool.h"

extern int Json_value ( Str input , char *search_key , char *bufferptr , int buffersz );
extern int checkJsonFormat ( char* input );


#endif /* INC_JSON_H_ */
