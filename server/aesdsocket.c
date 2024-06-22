#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/queue.h>
#include <time.h>
#include <stdint.h>

#define buffer_len 1024
#define DELAY_TIME 10
#define WAITING_CONNS 20
pthread_mutex_t file_mutex;
static int sockfd = -1;

timer_t timer_id = 0;

static char aesd_file[] = "/var/tmp/aesdsocketdata";

struct vin_thread_data
{
    int thread_completed;
    int vin_conn_fd;
};

struct vin_slist_node
{
    pthread_t vin_thread_id;
    struct vin_thread_data* thread_data;
    SLIST_ENTRY(vin_slist_node) entries;
};

SLIST_HEAD(vin_slist_head, vin_slist_node);
static struct vin_slist_head vin_head;

void vin_signal_handler(int sig) {
    // handle SIGINT and SIGTERM
	// if you are here, one of the above signals triggered
    struct vin_slist_node *sig_snode = NULL;

    if ((SIGINT == sig) || (SIGTERM == sig)) {	
        syslog(LOG_INFO, "\n=============Caught signal, set to start exiting\n");
        while(!SLIST_EMPTY(&vin_head)) {
            sig_snode = SLIST_FIRST(&vin_head);
            if (pthread_join(sig_snode->vin_thread_id, NULL) != 0)
            {
                syslog(LOG_ERR, "pthread_join Failed!\n");
                closelog();
                exit(-1);
            }
            SLIST_REMOVE_HEAD(&vin_head, entries);
            free(sig_snode->thread_data);
            free(sig_snode);
        }

        if (sockfd != -1) {
            close(sockfd);
            sockfd = -1;
        }

        if (remove(aesd_file) != 0) {
            syslog(LOG_ERR, "Failed to delete temp file");
            closelog();
            exit(-1);
        }
        
        if (timer_delete(timer_id) != 0) {
            syslog(LOG_ERR, "failed to delete timer");
            closelog();
            exit(-1);
        }
        
        closelog();
        exit(0);
    }
}



void* proc_thread_func(void* args)
{
    struct vin_thread_data* local_thread_data = (struct vin_thread_data *) args;
    char *vin_buffer = NULL;
    int thread_rec;
    FILE *thread_write_file = NULL;
    unsigned char exitflag;
    
	//syslog(LOG_ERR, "------------------pthread hallo---------");
 	// new attempt using recv

    
    thread_rec = pthread_mutex_lock(&file_mutex);
    if (thread_rec != 0) {
        syslog(LOG_ERR, "failed to lock mutex");
        close(local_thread_data->vin_conn_fd);
        local_thread_data->thread_completed = -1;
        return local_thread_data;
    } else {
        thread_write_file = fopen(aesd_file, "a");
        if (thread_write_file == NULL){
			syslog(LOG_ERR, "failed to open write file from thread");
            close(local_thread_data->vin_conn_fd);
            local_thread_data->thread_completed = -1;
            return local_thread_data;
        }
        
        vin_buffer = (char *)malloc(buffer_len);
        if (vin_buffer == NULL) {
            close(local_thread_data->vin_conn_fd);
            local_thread_data->thread_completed = -1;
            return local_thread_data;
        }
        
        // unsigned char exitflag, declared at top of function not to piss off valgrind
        exitflag = 0;
        ssize_t bytes_recv;
        while (!exitflag) {
            memset(vin_buffer, 0, buffer_len);
            bytes_recv = recv(local_thread_data->vin_conn_fd, vin_buffer, sizeof(vin_buffer), 0);
            if ((bytes_recv <= 0) || (strchr(vin_buffer, '\n') != NULL))
            {
                exitflag = 1;
            }
            
            if (bytes_recv > 0) {
                if (fwrite(vin_buffer, 1, bytes_recv, thread_write_file) != bytes_recv) {
                    syslog(LOG_ERR, "Write to File Failed!\n");
                    fclose(thread_write_file);
                    free(vin_buffer);
                    close(local_thread_data->vin_conn_fd);
                    local_thread_data->thread_completed = -1;
                    return local_thread_data;
                }
				//syslog(LOG_ERR, "------------------pthread has data---------%s", vin_buffer);
            }
        }
        fclose(thread_write_file);
        free(vin_buffer);
        
        thread_rec = pthread_mutex_unlock(&file_mutex);
        if (thread_rec != 0) {
            close(local_thread_data->vin_conn_fd);
            local_thread_data->thread_completed = -1;
            return local_thread_data;
        }
    }
    
    thread_rec = pthread_mutex_lock(&file_mutex);
    if (thread_rec != 0) {
        close(local_thread_data->vin_conn_fd);
        local_thread_data->thread_completed = -1;
        return local_thread_data;
    }
    else
    {
        thread_write_file = fopen(aesd_file, "r");
        if (thread_write_file == NULL) {
            syslog(LOG_ERR, "Failed to open write file from thread");
            close(local_thread_data->vin_conn_fd);
            local_thread_data->thread_completed = -1;
            return local_thread_data;
        }
            
        vin_buffer = (char *)malloc(buffer_len);
        if (vin_buffer == NULL) {
            close(local_thread_data->vin_conn_fd);
            local_thread_data->thread_completed = -1;
            return local_thread_data;
        }
            
        exitflag = 0;
        size_t bytes_read;
        while (!exitflag) {
            bytes_read = fread(vin_buffer, 1, sizeof(vin_buffer), thread_write_file);
            if ((bytes_read == 0) || (bytes_read < sizeof(vin_buffer))) {
                exitflag = 1;
            }
            
            if (bytes_read > 0) {
                if (send(local_thread_data->vin_conn_fd, vin_buffer, bytes_read, 0) != bytes_read) {
                    fclose(thread_write_file);
                    free(vin_buffer);
                    close(local_thread_data->vin_conn_fd);
                    local_thread_data->thread_completed = -1;
                    return local_thread_data;
                }
            }
        }
        fclose(thread_write_file);
        free(vin_buffer);
        
        thread_rec = pthread_mutex_unlock(&file_mutex);
        if (thread_rec != 0) {
            close(local_thread_data->vin_conn_fd);
            local_thread_data->thread_completed = -1;
            return local_thread_data;
        }
    }
    
    close(local_thread_data->vin_conn_fd);
    local_thread_data->thread_completed = 1;
   	// syslog(LOG_INFO, "Thread completed message tasks");
    
    return local_thread_data;
}


// who cares bits are not used
void time_keeper(int sig, siginfo_t *who, void *cares) {
    char timestamp_string[128] = { 0 };
    time_t localtimer;
    struct tm *localtime_info;
    FILE *time_write_file = NULL;
    int time_rec;
    
    if (SIGRTMIN == sig) {
        time_rec = pthread_mutex_lock(&file_mutex);
        if (time_rec != 0) {

            closelog();
            exit(-1);
        }
        else {
            time_write_file = fopen(aesd_file, "a");
            if (time_write_file == NULL) {
                // syslog(LOG_ERR, "Open File %s Failed!\n", aesd_file);
                closelog();
                exit(-1);
            }
        
            localtimer = time(NULL);
            localtime_info = localtime(&localtimer);
            if (localtime_info == NULL) {
                // syslog(LOG_ERR, "localtime\n");
                fclose(time_write_file);
                closelog();
                exit(-1);
            }
            
            if (strftime(timestamp_string, sizeof(timestamp_string), "timestamp:%Y-%m-%d %H:%M:%S\n", localtime_info) == 0) {
                // syslog(LOG_ERR, "strftime returned 0");
                fclose(time_write_file);
                closelog();
                exit(-1);
            }
            
            fprintf(time_write_file, "%s", timestamp_string);
            
            fclose(time_write_file);
            time_rec = pthread_mutex_unlock(&file_mutex);
        }
    }
}

int main(int argc, char *argv[]) {
	//bool exitflag = false;
	//bool daemonflag = false;
    struct addrinfo hints;
    struct addrinfo *result, *opAddrInfo;
    struct sockaddr peerAddr;
    socklen_t peerAddrSize;
    int main_rec;
    pthread_t new_thread_id;
    int connfd = -1;
    struct vin_slist_node *new_node = NULL;
    struct sigevent sev;
    struct sigaction sa;
    struct itimerspec its;
    int closedConnFound;
    //sigset_t set, oldSet;
    
    if ((signal(SIGINT, vin_signal_handler) == SIG_ERR) ||(signal(SIGTERM, vin_signal_handler) == SIG_ERR)) {
        syslog(LOG_ERR, "Register signal SIGINT & SIGTERM Failed!\n");
        closelog();
        exit(-1);
    }
  
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, "9000", &hints, &result) != 0) {
        syslog(LOG_ERR, "Getting Address Info Failed!\n");
        closelog();
        exit(-1);
    }
    
    opAddrInfo = result;
    while (opAddrInfo != NULL) {
        sockfd = socket(opAddrInfo->ai_family, opAddrInfo->ai_socktype, opAddrInfo->ai_protocol);
        if (sockfd == -1)
            opAddrInfo = opAddrInfo->ai_next;
        else
            break;
    }

    if (sockfd == -1) {
        syslog(LOG_ERR, "Creating Socket Failed!\n");
        freeaddrinfo(result);
        closelog();
        exit(-1);
    }
    
    int optionvalue = 1;
	if(setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, &optionvalue, sizeof(optionvalue)) == -1) {
        syslog(LOG_ERR, "Set Socket Option Failed!\n");
        freeaddrinfo(result);
        close(sockfd);
        sockfd = -1;
        closelog();
        exit(-1);
    }

    if (bind(sockfd, opAddrInfo->ai_addr, opAddrInfo->ai_addrlen) != 0) {
        syslog(LOG_ERR, "Binding to Socket Failed!\n");
        freeaddrinfo(result);
        close(sockfd);
        sockfd = -1;
        closelog();
        exit(-1);
    }
    freeaddrinfo(result);

    //daemon handling after binding
    // if ((argc == 2) && !strcmp(argv[1], "-d")) {
    //     daemon(0, 0);
    // }

    for (int cycle = 0; cycle < argc; ++cycle){
		if (strcmp(argv[cycle],"-d") == 0) {
			syslog(LOG_ERR, "----------------SETTING UP DAEMON");
			daemon(0,0);
		}
	}
    
    SLIST_INIT(&vin_head);
    
    openlog(NULL, 0, LOG_USER);

    //Setup timer
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timer_id;
    
    main_rec = timer_create(CLOCK_REALTIME, &sev, &timer_id);
    if (main_rec != 0) {
        syslog(LOG_ERR, "time_create for Timestamp Failed!\n");
        closelog();
        exit(-1);
    }
    
    //Establish handler for timer signal
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = time_keeper;
    
    //Init signal
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Sigaction for Timestamp Failed!\n");
        closelog();
        exit(-1);
    }

    //Start timer
    its.it_value.tv_sec = DELAY_TIME;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = DELAY_TIME;
    its.it_interval.tv_nsec = 0;
    main_rec = timer_settime(timer_id, 0, &its, NULL);
    if (main_rec != 0) {
        syslog(LOG_ERR, "timer_settime for Timestamp Failed!\n");
        closelog();
        exit(-1);
    }

    if (listen(sockfd, WAITING_CONNS) != 0) {
        syslog(LOG_ERR, "Setting Listen to Socket Failed!\n");
        close(sockfd);
        sockfd = -1;
        closelog();
        exit(-1);
    }

    while (1) {
        //attempt accept connection
        peerAddrSize = sizeof(peerAddr);
        connfd = accept(sockfd, (struct sockaddr *)&peerAddr, &peerAddrSize);
        if (connfd == -1) {
            syslog(LOG_ERR, "Accepting Socket Connection Failed!\n");
        }
        else {
            struct vin_thread_data* connData = malloc(sizeof(struct vin_thread_data));
            connData->thread_completed = 0;
            connData->vin_conn_fd = connfd;
            
            int main_rec = pthread_create(&new_thread_id, NULL, proc_thread_func, connData);
            if (main_rec != 0) {
                syslog(LOG_ERR, "pthread_create failed with error %d creating thread %lu!\n", main_rec, new_thread_id);
                close(sockfd);
                sockfd = -1;
                closelog();
                exit(-1);
            } else {
                new_node = malloc(sizeof(struct vin_slist_node));
                new_node->vin_thread_id = new_thread_id;
                new_node->thread_data = connData;
                SLIST_INSERT_HEAD(&vin_head, new_node, entries);
            }
        }
        
        if (!SLIST_EMPTY(&vin_head)) {
			// check and close all finished threads
            new_node = SLIST_FIRST(&vin_head);

            if ((new_node->thread_data->thread_completed == -1) || (new_node->thread_data->thread_completed == 1)) {
                pthread_join(new_node->vin_thread_id, NULL);
                SLIST_REMOVE_HEAD(&vin_head, entries);
                free(new_node->thread_data);
                free(new_node);
            }
            closedConnFound = 0;
            SLIST_FOREACH(new_node, &vin_head, entries) {
                if (new_node->thread_data->thread_completed != 0) {
					//syslog(LOG_ERR, "------------------nntid: ");
                    closedConnFound = 1;
                    break;
                }
            }
            if (closedConnFound == 1) {
                if ((new_node->thread_data->thread_completed == -1) || (new_node->thread_data->thread_completed == 1)) {
                    pthread_join(new_node->vin_thread_id, NULL);
                    SLIST_REMOVE(&vin_head, new_node, vin_slist_node, entries);
                    free(new_node->thread_data);
                    free(new_node);
                }
            }
        }
		//syslog(LOG_ERR, "-----------------end of the line loop deal");

    }

    return 0;
}