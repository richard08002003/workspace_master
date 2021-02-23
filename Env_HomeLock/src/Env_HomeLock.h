/*
 * Env_HomeLock.h
 *
 *  Created on: 2018年1月30日
 *      Author: richard
 */

#ifndef SRC_ENV_HOMELOCK_H_
#define SRC_ENV_HOMELOCK_H_


#include "/media/richard/richard_hdd/workspace_NCHU/Common_tool_C/src/Setup.h"
//#include "/home/richard/Common_tool_C/src/Setup.h"


#include "Decode.h"

#define DF_Pgm_Name "Env_HomeLock"

/*
 * 2018/05/14
 * 修正取感測器資料失敗問題
 */
#define Ver "0002"

//#define Ver "0001"

extern int Pgm_Begin(void);



#endif /* SRC_ENV_HOMELOCK_H_ */
