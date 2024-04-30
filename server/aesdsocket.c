// vincent anderson assignment 5 ecen 5305 aesdsocket.c code
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdbool.h>
#include <syslog.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define WAITING_CONNS 20
#define buff_file_path "/var/tmp/aesdsocketdata"

bool exitflag = false;
bool daemonflag = false;
int sockfd, connfd;
int bufffd;
socklen_t	conn_addr_size;
struct sockaddr_in my_addr, conn_addr;


int closingtime(int didgood){
	if(&sockfd>0){
		shutdown(sockfd,SHUT_RDWR);
		close(sockfd);	
	}
	if(&connfd>0){
		shutdown(connfd,SHUT_RDWR);
		close(connfd);
	}
	if(&bufffd>0){
		shutdown(bufffd,SHUT_RDWR);
		close(bufffd);
	}
	if(exitflag){
		syslog(LOG_DEBUG, "Caught signal, exiting");
	}  

	exit(didgood);
}

void signal_handler(int signum){
	// handle SIGINT and SIGTERM
	// if you are here, one of the above signals triggered
	exitflag=true;
}

int main(int argc, char const *argv[])
{
	openlog("aesdsocket",LOG_PERROR, LOG_USER);

	for (int cycle = 0; cycle < argc; ++cycle){
		if (strcmp(argv[cycle],"-d") == 0) {
			daemonflag = true;
			//syslog(LOG_DEBUG, "daemon on");
		}
	}


	// set up sigint and sigterm signal actions
	struct sigaction customsigaction;
	memset(&customsigaction, 0, sizeof(sigaction));
	customsigaction.sa_handler = &signal_handler;
	sigaction(SIGINT, &customsigaction, NULL);
	sigaction(SIGTERM, &customsigaction, NULL);

	// socket()
	sockfd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);

	if (sockfd == -1) {
		// socket unable to be created
		closingtime(-1);
	}


	// getaddrinfo
	memset(&my_addr,0,sizeof my_addr );
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(9000);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);


	
	// bind(sockfd, sockaddr-server,socklen_t)
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1) {
		// bind must have failed
		closingtime(-1);
	}

	if(daemonflag){
		// fork for child process and get rid of parent process
		if (daemon(0,0) == -1) {
			//daemon failed
			closingtime(-1);
		}
	}


	// listen(sockfd)
	if (listen(sockfd, WAITING_CONNS) ==-1){
		// fail to listen, error out
		closingtime(-1);
	}

	bufffd = open("/var/tmp/aesdsocketdata", O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (bufffd == -1) {
		// file could not be opened
		closingtime(-1);
	}

	while(!exitflag) {

		// accept(sockfd)
		conn_addr_size = sizeof(conn_addr);
		connfd = accept(sockfd, (struct sockaddr *) NULL, NULL);

		if (connfd==-1) {
			//unable to accept connection, error out
			closingtime(-1);
		}

		// log message to indicate successfull connection
		struct sockaddr peer_addr;
		socklen_t peer_length = 0;

		if (getpeername(connfd, &peer_addr,&peer_length) == -1) {
			// no peer available, throw error
			closingtime(-1);
		}

		struct sockaddr_in *peer_in_addr = (struct sockaddr_in *)&peer_addr;
		char connString[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &peer_in_addr->sin_addr, connString, sizeof connString);
		syslog(LOG_DEBUG, "Accepted connection from %s", connString); 

		


		exitflag = false;

	while(!exitflag) {
		char buffer[256000];
		char* segptr = buffer;
		ssize_t word_len = 0;
		word_len = recv(connfd, buffer, sizeof(buffer), 0);
		if(word_len <= 0) {
			syslog(LOG_DEBUG,"Closed connection from %s", connString);
			break;
		}
		ssize_t seg_length = 0;
		for(size_t index = 0; index < word_len; ++index) {

			++seg_length;

			if(buffer[index]=='\n') {
				write(bufffd, segptr, seg_length);

				//syslog(LOG_DEBUG,"Found end of word %d", written);
				struct stat sb;
				fstat(bufffd, &sb);
				char* filemap = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, bufffd, 0);
				send(connfd, filemap, sb.st_size, 0);
				//write(bufffd, filemap, sizeof filemap);
				//syslog(LOG_DEBUG, "%s\n", filemap);
				munmap(filemap, sb.st_size);
				//char *stringresult;
				//strcpy(stringresult, buffer+word_len);
				//fputs(stringresult,bufffd);
				segptr += seg_length;
				seg_length = 0;
			}


		}
		if (seg_length!=0) {
			write(bufffd, segptr, seg_length);
		}	
	}	


	}
	closingtime(1);
	return 0;
}