// vincent anderson assignment 2 ecen5305 writer.c
// started way too late on finished on way too much caffeine

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdarg.h>



int main( int argc, char *argv[] ){


// verify correct number of arguments, should be 2 arguments
if ( argc != 3 ) {
	// t
	openlog("vinny-aa writer.c errlog: ",0,LOG_ERR);
	syslog(LOG_ERR, "Incorrect number of arguments");
	closelog();
	printf("Incorrect number of arguments entered\n");
	// debug statement, commented out now
	//printf("%d: %s \n",argc, argv[3]);
	exit(1);
}


// capture text to write
char *writestring;
writestring = argv[2];


// opening with w+ will open with write access
// if the file doesn't exist, w+ will create 
FILE *writefile = fopen( argv[1], "w+" );

if( writefile  != NULL ) {
	// file is open or created
	fprintf(writefile, "%s\n", writestring);
	fclose(writefile);
	openlog("vinny-aa writer.c logs:",0,LOG_DEBUG);
	syslog(LOG_DEBUG,"Writing %s to %s", argv[2], argv[1]);
	closelog();

} else { 
	// file could not be written / opened
	openlog("vinny-aa writer.c errlog: ",0,LOG_ERR);
	syslog(LOG_ERR, "File could not be written");
	closelog();
	printf("File could not be written.\n");
	exit(1);
}




return 0;   
}