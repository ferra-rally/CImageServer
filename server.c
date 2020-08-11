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
#include "jsmn.h"

#define PORT 8080
#define SERVER_NAME "CServi"
#define SERVER_VERSION "0.2"

#ifdef IMAGE_CONVERTION
#define CACHE_LOCATION "imagecache"
#define SUPPORTED_CONVERSION_TYPES "image/jpg, image/jpeg, image/png"
#endif


//TODO exmaple string - remove this
static const char *JSON_STRING =
    "{\"device\":[\"screenpixelswidth\":1440, \"screenpixelsheight\":2560]}";

//TO THIS

FILE *logFile;

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

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

void  handle_error(char *msg){
    logOnFile(1,msg);
    perror(msg); 
    exit(EXIT_FAILURE);
}

void pipe_handle(/*int sig_num, siginfo_t *sig_info, void *context*/){
	
	logOnFile(2,"pipe handled\n");
	printf("PIPE\n");
}

#ifdef IMAGE_CONVERTION
char* exec_pycode(char* ua) {
    PyObject *pName, *pModule, *pDict, *pFunc, *pValue, *presult;
    char *result;

  Py_Initialize();
  pName = PyUnicode_FromString((char*)"resolveus");
  pModule = PyImport_Import(pName);
  if(pModule == NULL) {
      printf("NULL mymodule\n");
      return "";
  }
  pDict = PyModule_GetDict(pModule);
  pFunc = PyDict_GetItemString(pDict, "resolve_ua");
  if(pFunc == NULL) {
      printf("No Function\n");
  }
  

if (PyCallable_Check(pFunc))
   {
       pValue=Py_BuildValue("(z)", ua);
       PyErr_Print();
       presult=PyObject_CallObject(pFunc,pValue);    
       result = PyUnicode_AsUTF8(presult);
       //printf("Valore di ritorno in C %s\n", result);
       PyErr_Print();
   } else {
       PyErr_Print();
   }

  Py_Finalize();
  return result;
}
#endif

int parse_json(char *json, char *dest) {

    jsmn_parser p;
    jsmntok_t t[128]; /* We expect no more than 128 JSON tokens */

    jsmn_init(&p);
    int r = jsmn_parse(&p, json, strlen(json), t, 128);
    int i, h, w;

    for (i = 1; i < r; i++) {
        if (jsoneq(json, &t[i], "screenpixelswidth") == 0) {
            w = 0;
            if(JSON_STRING + t[i + 1].start != NULL) {
                w = atoi(json + t[i + 1].start);
            }

            i++;
        } else if (jsoneq(json, &t[i], "screenpixelsheight") == 0) {
            h = 0;
            if(json + t[i + 1].start != NULL) {
                h = atoi(json + t[i + 1].start);
            }

            i++;
        }
    }

    sprintf(dest, "%d-%d", w, h);
    return 1;
}

void IP_logger(int fd){

	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	getpeername(fd, (struct sockaddr *)&addr, &addr_size);
	char *dotAddr = malloc(sizeof(char)*20);
	dotAddr = strdup(inet_ntoa(addr.sin_addr));
	char buffer[80];

	sprintf(buffer,"Client addr %s",dotAddr);

	logOnFile(3,buffer);
	printf("Client addr %s\n", dotAddr); 
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
            printf("************\n");
            printf("Recieved:\n-%s-", request);
        }

        size_t size = strlen(request) + 1;
        
        char resource[size];
        parse_resource(request, resource);

        char header[200];
        char requestedResource[200], filename[200];
        char type[20];
        int code;
#ifdef IMAGE_CONVERTION
        int w, h;    //Requested quality, width and heigth of the screen
        float q;
#endif
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
            find_type(filename, type);
#ifdef IMAGE_CONVERTION
            if (strstr(SUPPORTED_CONVERSION_TYPES, type) != NULL) {
                char user_agent[size];
                find_line(request, "User-Agent: ", user_agent);
                char *tmp, *tmpw, *tmph;
                char filename_conv[512];
                struct stat sb;
            
                q = find_quality(request, type);
                tmp = exec_pycode(user_agent);

                tmpw = strtok(tmp, "-");
                tmph = strtok(NULL, "-");

                w = atoi(tmpw);
                h = atoi(tmph);

                //printf("Quality for %s: %d-%d q = %d", filename, w, h, q);

                // Add condition for different cases
                //q = 0.1;

                if(h != 0 && w != 0) {
                    if(q == 1) {
                        sprintf(filename_conv, "%s/%d-%d-%s", CACHE_LOCATION, h, w, filename);
                    } else {
                        sprintf(filename_conv, "%s/%d-%d-%2.0f-%s", CACHE_LOCATION, h, w, q * 100, filename);
                    }
                    
                    if (stat(filename_conv, &sb) == -1) {
                        if(resize(filename, filename_conv, w, h, q*100) != EXIT_SUCCESS) perror("Error resizing image\n");
                    }

                    close(fd);
                    fd = open(filename_conv, O_RDONLY);
                    if(fd == -1) {
                        perror("File not found\n");
                    }
                } else if(q != 1) {
                    sprintf(filename_conv, "%s/%2.0f-%s", CACHE_LOCATION, q * 100, filename);
                    
                    if (stat(filename_conv, &sb) == -1) {
                        if (change_quality(filename, filename_conv, q*100) != EXIT_SUCCESS) perror("Error changing quality of image\n");
                    }

                    close(fd);
                    fd = open(filename_conv, O_RDONLY);
                    if(fd == -1) {
                        perror("File not found\n");
                    }
                }
            }
#endif
        }
    
        struct stat stat_buf;
        fstat(fd, &stat_buf);

        sprintf(header, "HTTP/1.1 %d %s\r\nContent-length: %ld\r\nServer: %s version %s\r\nContent-Type: %s\r\n\r\n", code, message, stat_buf.st_size, SERVER_NAME, SERVER_VERSION, type);

        if (write(connfd, header, strlen(header)) <= 0) {
            break;
        }

        char method[size];
        find_method(request, method);
        if (!strcmp(method, "GET")) {
            if (sendfile(connfd, fd, NULL, stat_buf.st_size) <= 0) {
                break;
            }
        }
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

    /*
    char json[20];
    parse_json(JSON_STRING, json);
    printf("Example JSON: %s\n", json);
    */
#ifdef IMAGE_CONVERTION

    // Set environment variable for python
    char pythonpath[512];
    char *p = secure_getenv("PYTHONPATH");
    if (p)
        snprintf(pythonpath, 512, "PYTHONPATH=%s:pwd", secure_getenv("PYTHONPATH"));
    else
        snprintf(pythonpath, 512, "PYTHONPATH=:pwd");
    if (putenv(pythonpath) == -1) {
        printf("Error setting PYTHONPATH environment variable\n");
        exit(EXIT_FAILURE);
    }

    struct stat st;
    int sr;
    if ((sr = stat(CACHE_LOCATION, &st)) == -1 && errno == ENOENT) {
        if (mkdir(CACHE_LOCATION, 0755) == -1)
            handle_error("mkdir");
    } else if (sr == -1) {
        handle_error("stat");
    } else if (!S_ISDIR(st.st_mode)) {
        printf("Remove file '%s' from folder containing server\n", CACHE_LOCATION);
        return -1;
    }
#endif

    //setup logfile
	int lfd = open("logFile.log", O_WRONLY | O_CREAT | O_APPEND, 0644);

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
