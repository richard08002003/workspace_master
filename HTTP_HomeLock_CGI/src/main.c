/*
 * main.c
 *
 *  Created on: 2018年5月1日
 *      Author: richard
 */
#include "/media/richard/richard_hdd/workspace_NCHU/Common_tool_C/src/Setup.h"


 /************************************************
  *				URL Encode & Decode				*
  ************************************************/
 /* Converts a hex character to its integer value */
 char from_hex(char ch) {
   return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
 }

 /* Converts an integer value to its hex character*/
 char to_hex(char code) {
   static char hex[] = "0123456789abcdef";
   return hex[code & 15];
 }

 /* Returns a url-encoded version of str */
 /* IMPORTANT: be sure to free() the returned string after use */
 char *url_encode( char *str ) {
     char *pstr = str, *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;
     while (*pstr) {
         if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
             *pbuf++ = *pstr;
         else if (*pstr == ' ')
             *pbuf++ = '+';
         else
             *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15) ;
             pstr++ ;
     }
     *pbuf = '\0';
     return buf ;
 }

 /* Returns a url-decoded version of str */
 /* IMPORTANT: be sure to free() the returned string after use */
 char *url_decode(char *str) {
   char *pstr = str, *buf = malloc(strlen(str) + 1), *pbuf = buf;
   while (*pstr) {
     if (*pstr == '%') {
       if (pstr[1] && pstr[2]) {
         *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
         pstr += 2;
       }
     } else if (*pstr == '+') {
       *pbuf++ = ' ';
     } else {
       *pbuf++ = *pstr;
     }
     pstr++;
   }
   *pbuf = '\0';
   return buf;
 }

int main ( int argc , char *argv[ ] , char** envp ) {

	printf( "Expires: Sat, 01 Jan 2000 00:00:00 GMT\r\n" );
	printf( "Cache-Control: no-cache, no-cache\r\n" );
	printf( "Pragma: no-cache, no-cache\r\n" );
	printf( "Content-Type: text/html; charset=UTF8\r\n" );
	printf( "Connection: close\r\n\r\n" );


	system( "echo \"Begin\" > /tmp/http_cgi_log.txt" );

	char* content = NULL;
	int nContent_Length = 0;
	char *szRequest_Method = getenv( "REQUEST_METHOD" );
	if ( strcmp( szRequest_Method , "GET" ) == 0 ) {	// GET
		char *szQuery_String = getenv( "QUERY_STRING" );
		nContent_Length = strlen( szQuery_String );
		if ( 0 == nContent_Length ) {
			printf( "Get len is 0\n" );
			system( "echo \"Get len is 0\n\" >> /tmp/http_cgi_log.txt" );
			fflush( stdout );
			return -1;
		}

		content = (char*) calloc( nContent_Length + 1 , 1 );
		if ( NULL == content ) {
			printf( "Get calloc error\n" );
			system( "echo \"Get calloc error\n\" >> /tmp/http_cgi_log.txt" );
			fflush( stdout );
			return -2;
		}

		memcpy( content , szQuery_String , nContent_Length );

	} else if ( strcmp( szRequest_Method , "POST" ) == 0 ) {		// POST
		char *szContent_Length = getenv( "CONTENT_LENGTH" );
		nContent_Length = atoi( szContent_Length );
		if ( 0 == nContent_Length ) {
			printf( "Post len is 0\n" );
			system( "echo \"Post len is 0\n\" >> /tmp/http_cgi_log.txt" );
			fflush( stdout );
			return -3;
		}
		content = (char*) calloc( nContent_Length + 1 , 1 );
		if ( NULL == content ) {
			printf( "Get calloc error\n" );
			system( "echo \"Get calloc error\n\" >> /tmp/http_cgi_log.txt" );
			fflush( stdout );
			return -4;
		}

		fread( content , nContent_Length , 1 , stdin );
	} else {
		printf( "not support" );
		system( "echo \"not support\n\" >> /tmp/http_cgi_log.txt" );
		fflush( stdout );
		return -5;
	}

	char* p_new = url_decode( content );
	free( content );
	content = NULL;

	char buf[ strlen( p_new ) + 64 ];
	memset( buf , 0 , sizeof(buf) );
	snprintf( buf , sizeof(buf) , "echo \"input:%s\" >> /tmp/http_cgi_log.txt" , p_new );
	system( buf );


	system( "echo \"Will run IPC_Client\" >> /tmp/http_cgi_log.txt" );
//	IPC_Client( p_new );
	printf("%s" , p_new );

	free( p_new );
	p_new = NULL;
	return 0;
}

