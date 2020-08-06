#include <stdio.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sys/sendfile.h>
#include <signal.h>

#include "http.h"
#include "list.h"

#define PORT 8080

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

void pipe_handle(int sig_num, siginfo_t *sig_info, void *context){
	printf("PIPE\n");
}

void *thread_func(void *args)
{
    char response[4096];
    struct client *client = (struct client *)args;
    int connfd = client->conn_id;

    printf("thread %lu alive\n", pthread_self());
    
    while(1) {
        memset(response, 0, sizeof(response));
        int n;
        n = read(connfd, response, 4096);
        if (n < 0) {
            printf("Error reading\n");
        } else {
            printf("Recieved:\n%s", response);
        }

        printf("********************\nQuality: %f\n***************\n", find_quality(response, "image/webp"));
        char *resource = parse_resource(response);

        char header[200];
        char requestedResource[200], filename[200];
        char type[20];
        int code;
        char message[20];

        if (!strcmp(resource, "/")) {
            strcpy(filename, "index.html");
        } else {
            strcpy(requestedResource, resource + 1);
            strcpy(filename, requestedResource);
        }

        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            fd = open("404.html", O_RDONLY);
            code = 404;
            strcpy(message, "Not Found");
            strcpy(type, "text/html");
        } else {
            code = 200;
            strcpy(message, "OK");
            strcpy(type, find_type(filename));
        }
    
        struct stat stat_buf;
        fstat(fd, &stat_buf);

        sprintf(header, "HTTP/1.1 %d %s\r\nContent-length: %ld\r\nContent-Type: %s\r\n\r\n", code, message, stat_buf.st_size, type);

        write(connfd, header, strlen(header));

        if(!strcmp(find_method(response), "GET")) {
            sendfile(connfd, fd, NULL, stat_buf.st_size);
        }
        printf("\n*******************\n");
        close(fd);
    }
}

int main() {
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    struct sigaction act;

	// Set SIGPIPE handler
	memset(&act,'\0', sizeof(act));
	act.sa_sigaction = pipe_handle;
	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGPIPE, &act, NULL) < 0) {
		handle_error("sigaction");
	}

    // Socket creation and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        handle_error("setsockopt(SO_REUSEADDR) failed");

    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    } else {
        printf("Socket successfully created..\n");
    }
    memset(&servaddr, 0, sizeof(struct sockaddr_in));
  
    // Assign IP, PORT 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
  
    // Bind newly created socket to given IP and verification 
    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
        handle_error("Bind failed");
        exit(0);
    } else {
        printf("Socket successfully binded..\n");
    }
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    } else {
        printf("Server listening..\n");
    }
    len = sizeof(cli);

    // Initialize timeout structure to 300 seconds
	struct timeval timeout;
	timeout.tv_sec = 300;
	timeout.tv_usec = 0;

	pthread_attr_t attr;
	if (pthread_attr_init(&attr) != 0)
		handle_error("pthread_attr_init");
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0)
		handle_error("pthread_attr_setdetachstate");

	struct sockaddr_in client_addr;

    while (1) {
        // Accept the data packet from client and verification 
        connfd = accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len);
        if (connfd < 0) {
            printf("server acccept failed...\n");
            exit(0);
        } else {
            printf("server acccept the client...\n");
        }

        // Set socket options
		if (setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout,
					(socklen_t)sizeof(timeout)) == -1)
			handle_error("setsockopt");
		if (setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout,
					(socklen_t)sizeof(timeout)) == -1)
			handle_error("setsockopt");

        // Pass connfd to thread
		pthread_t tid;
		if (pthread_create(&tid, &attr, thread_func, (void *) append_node(connfd)) != 0)
			handle_error("pthread_create");

    }

    close(connfd);
    close(sockfd);

    return 0;
}
