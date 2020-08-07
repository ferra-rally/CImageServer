#include <python3.8/Python.h>
#include <python3.8/pythonrun.h>
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
#include <arpa/inet.h>
#include <time.h>

#include "http.h"
#include "list.h"
#include "convert.h"

#define PORT 8080
#define SERVER_NAME "CServi"
#define SERVER_VERSION "0.1"

#define handle_error(msg) \
    do {logOnFile(1,msg);perror(msg); exit(EXIT_FAILURE); } while (0)

FILE *logFile;

void logOnFile(int flag, char *msg){
	time_t rawtime; 
	struct tm *info; 
	char buffer[80]; 
	time(&rawtime); 
	char tag[80];

	switch(flag){
		case 1:
			strcpy(tag, "ERROR");
			break;
		case 2:
			strcpy(tag, "PIPE");
			break;
		case 3:
			strcpy(tag, "CONNECTION");
			break;
	}

	info = localtime(&rawtime); 
	strftime(buffer,80,"%x - %I:%M%p",info); 
	fprintf(logFile, "%s: %s -- %s\n", tag,msg, buffer); 
	fflush(logFile);
}


void pipe_handle(int sig_num, siginfo_t *sig_info, void *context){
	
	logOnFile(2,"pipe handled\n");
	printf("PIPE\n");
}

void exec_pycode(char* ua)
{
    PyObject *pName, *pModule, *pDict, *pFunc, *pValue, *presult;

  Py_Initialize();
  PyObject* myModuleString = PyUnicode_FromString((char*)"resolveus");
  PyObject* myModule = PyImport_Import(myModuleString);
  if(myModule == NULL) {
      printf("NULL mymodule\n");
      return;
  }
  pDict = PyModule_GetDict(myModule);
  pFunc = PyDict_GetItemString(pDict, "resolve_ua");
  if(pFunc == NULL) {
      printf("No Function\n");
  }
  char* result;

if (PyCallable_Check(pFunc))
   {
       pValue=Py_BuildValue("(z)", ua);
       PyErr_Print();
       presult=PyObject_CallObject(pFunc,pValue);    
       result =PyUnicode_AsUTF8(presult);
       printf("Valore di ritorno in C %s\n", result);
       PyErr_Print();
   } else 
   {
       PyErr_Print();
   }

  Py_Finalize();
}

void IP_logger(int fd){

	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	int res = getpeername(fd, (struct sockaddr *)&addr, &addr_size);
	char *dotAddr = malloc(sizeof(char)*20);
	dotAddr = strdup(inet_ntoa(addr.sin_addr));
	char buffer[80];

	sprintf(buffer,"Client addr %s",dotAddr);

	logOnFile(3,buffer);
	printf("Client addr %s\n", dotAddr); // prints "10.0.0.1"
	free(dotAddr);
}


void *thread_func(void *args)
{
    char request[4096];
    struct client *client = (struct client *)args;
    int connfd = client->conn_id;

    printf("thread %lu alive\n", pthread_self());
    
    while(1) {
        memset(request, 0, sizeof(request));

        if (read(connfd, request, 4096) <= 0) {
            break;
        } else {
            printf("Recieved:\n%s", request);
        }

        printf("********************\nQuality: %f\n***************\n", find_quality(request, "image/webp"));
        exec_pycode("Mozilla/5.0 (Linux; Android 6.0.1; Nexus 6P Build/MMB29P) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.83 Mobile Safari/537.36");
        char *resource = parse_resource(request);

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

        if (write(connfd, header, strlen(header)) <= 0) {
            break;
        }

        if (!strcmp(find_method(request), "GET")) {
            if (sendfile(connfd, fd, NULL, stat_buf.st_size) <= 0) {
                break;
            }
        }
        printf("\n*******************\n");
        close(fd);
    }
    if (close(connfd) < 0) {
        printf("Error closing socket\n");
    }
    remove_node(client);
    pthread_exit(NULL);
}

int main() {
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    struct sigaction act;

    //set environment variable for python
    //system("export PYTHONPATH=$PYTHONPATH:pwd");



    //setup logfile
	int lfd = open("logFile.txt", O_WRONLY | O_CREAT | O_APPEND);
	if(lfd == -1){
		//non uso l' handle perchè nell handle scriviamo l' errore nel logfile 
		printf("Error opening logFile\n");
		exit(EXIT_FAILURE);
	}

	logFile = fdopen(lfd, "w");
	if(logFile == NULL){
		printf("Error fdopening logFile\n");
		exit(EXIT_FAILURE);
	}


	// Set SIGPIPE handler
	memset(&act, 0, sizeof(act));
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
        //printf("Listen failed...\n");
        handle_error("Listen failed...");
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
        	IP_logger(connfd);
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
