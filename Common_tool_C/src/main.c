/*
 * main.c
 *
 *  Created on: 2017年11月15日
 *      Author: richard
 */

#include "Setup.h"

int main ( void ) {
#if 0
	int fd [ 2 ] ;
	pid_t fpid ;		// fork 用 //

	if ( ( pipe ( fd ) ) < 0 ) {
		perror ( "pipe" ) ;
		exit ( 1 ) ;
	}

	if ( ( fpid = fork ( ) ) < 0 ) {
		perror ( "fork" ) ;
		exit ( 1 ) ;
	}

	if ( fpid == 0 ) {
		/* Child process */
		close ( fd [ 0 ] ) ;
		char data [ ] = "Hello, world!" ;
		printf ( "Child Write Data : %s, Length : %d\n" , data , ( int ) strlen ( data ) ) ;
		write ( fd [ 1 ] , data , ( strlen ( data ) + 1 ) ) ;
		exit ( 0 ) ;
	} else {
		/* Parent process */
		close ( fd [ 1 ] ) ;
		char readbuffer [ 80 ] ;
		int nbytes = read ( fd [ 0 ] , readbuffer , sizeof ( readbuffer ) ) ;
		if ( nbytes < 0 ) {
			printf ( "Error read()" ) ;
		} else {
			printf ( "Parent Read Data : %s, fd = %d \n" , readbuffer , nbytes ) ;
			write ( STDOUT_FILENO , readbuffer , nbytes ) ;
			int aaa = 9000 ;
			printf ( "aaa = %d\n" , aaa ) ;
		}
	}
#endif
	return 0 ;
}
