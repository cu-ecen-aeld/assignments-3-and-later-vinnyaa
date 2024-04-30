#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
	
	FILE *filething = fopen("aesdthing", "a+");
	char *stringthing = "teststring\n";
	
	fputs(stringthing, filething);
	fclose(filething);
}	