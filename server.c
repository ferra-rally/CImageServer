#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <inttypes.h>

#include "http.h"
#include "list.h"

#define PORT 8080
#define SERVER_NAME "CServi"
#define SERVER_VERSION "0.2"

#ifdef IMAGE_CONVERTION
#include "jsmn.h"
#include "convert.h"
#define CACHE_LOCATION "imagecache"
#define SUPPORTED_CONVERSION_TYPES                                             \
	"image/jpg, image/jpeg, image/png, image/webp"
#define HTTP_PORT 80
#define RESPONSE_SIZE 4096
#define GET_STRING                                                             \
	"GET /api/v4/AQQNX4o8JjxVn2M_2Eg.json?user-agent=%s HTTP/1.1\r\nHost: cloud.51degrees.com\r\nUser-Agent: CServi/0.2 Linux x86_64\r\nAccept: application/json\r\nConnection: close\r\n\r\n"
#endif

FILE *logFile;

void logOnFile(int flag, char *msg)
{
	time_t rawtime;
	struct tm *info;
	char buffer[80];
	time(&rawtime);
	char tag[80];

	info = localtime(&rawtime);
	strftime(buffer, 80, "%x - %I:%M%p", info);

	switch (flag) {
	case 1:
		strcpy(tag, "ERROR");
		fprintf(logFile, "%s: %s: %s -- %s\n", tag, msg,
			strerror(errno), buffer);
		break;
	case 2:
		strcpy(tag, "PIPE");
		fprintf(logFile, "%s: %s -- %s\n", tag, msg, buffer);
		break;
	case 3:
		strcpy(tag, "CONNECTION");
		fprintf(logFile, "%s: %s -- %s\n", tag, msg, buffer);
		break;
	}

	fflush(logFile);
}

void handle_error(char *msg)
{
	logOnFile(1, msg);
	perror(msg);
}

void pipe_handler()
{
	logOnFile(2, "pipe handled\n");
}

void sigint_handler()
{
	fclose(logFile);
	exit(0);
}

#ifdef IMAGE_CONVERTION

static int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
	if (tok->type == JSMN_STRING &&
	    (int)strlen(s) == tok->end - tok->start &&
	    strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

int parse_json(char *json, char *dest)
{
	jsmn_parser p;
	jsmntok_t t[128];

	jsmn_init(&p);
	int r = jsmn_parse(&p, json, strlen(json), t, 128);
	int i, h, w;

	for (i = 1; i < r; i++) {
		if (jsoneq(json, &t[i], "screenpixelswidth") == 0) {
			w = 0;
			if (json + t[i + 1].start != NULL) {
				w = atoi(json + t[i + 1].start);
			}

			i++;
		} else if (jsoneq(json, &t[i], "screenpixelsheight") == 0) {
			h = 0;
			if (json + t[i + 1].start != NULL) {
				h = atoi(json + t[i + 1].start);
			}

			i++;
		}
	}

	sprintf(dest, "%d-%d", w, h);
	return 1;
}

void request_size(char *user_agent, char *result)
{
	int sock_ds;
	struct sockaddr_in server_addr;
	struct hostent *hp;

	if ((sock_ds = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		sprintf(result, "0-0");
		return;
	}

	memset(&server_addr, '\0', sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(HTTP_PORT);

	if ((hp = gethostbyname("cloud.51degrees.com")) == NULL) {
		sprintf(result, "0-0");
		return;
	}

	memcpy(&server_addr.sin_addr, hp->h_addr, 4);

	if (connect(sock_ds, (struct sockaddr *)&server_addr,
		    sizeof(server_addr)) == -1) {
		sprintf(result, "0-0");
		return;
	}

	char request[RESPONSE_SIZE];
	snprintf(request, RESPONSE_SIZE, GET_STRING, user_agent);
	//printf("%s\n", request);

	if (write(sock_ds, request, strlen(request) + 1) <= 0) {
		close(sock_ds);
		sprintf(result, "0-0");
		return;
	}

	char response[RESPONSE_SIZE];
	memset(response, 0, RESPONSE_SIZE);

	if (read(sock_ds, response, RESPONSE_SIZE) <= 0) {
		close(sock_ds);
		sprintf(result, "0-0");
		return;
	}

	close(sock_ds);

	//printf("%s\n", response);

	parse_json(strstr(response, "\r\n\r\n"), result);
}

#endif

void IP_logger(int fd)
{
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);
	getpeername(fd, (struct sockaddr *)&addr, &addr_size);
	char *dotAddr = strdup(inet_ntoa(addr.sin_addr));
	if (dotAddr == NULL)
		handle_error("strdup");

	char buffer[80];

	sprintf(buffer, "Client addr %s", dotAddr);

	logOnFile(3, buffer);
	//printf("Client addr %s\n", dotAddr);
	free(dotAddr);
}

void *thread_func(void *args)
{
	char request[4096];
	struct client *client = (struct client *)args;
	int connfd = client->conn_id;

	//printf("thread %lu alive\n", pthread_self());

	while (1) {
		memset(request, 0, sizeof(request));

		if (read(connfd, request, 4096) <= 0) {
			break;
		}
		//printf("************\n");
		//printf("Recieved:\n-%s-", request);

		size_t size = strlen(request) + 1;

		char resource[size];
		parse_resource(request, resource);

		char header[200];
		char requestedResource[200], filename[200];
		char type[20];
		int code;
#ifdef IMAGE_CONVERTION
		int w, h; //Requested quality, width and heigth of the screen
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
				find_user_agent(request, user_agent);
				char tmp[20];
				char *tmpw, *tmph, *saveptr;
				char filename_conv[512];
				struct stat sb;

				q = find_quality(request, type);
				request_size(user_agent, tmp);

				tmpw = strtok_r(tmp, "-", &saveptr);
				tmph = strtok_r(NULL, "-", &saveptr);

				w = atoi(tmpw);
				h = atoi(tmph);

				//printf("Quality for %s: %d-%d q = %d", filename, w, h, q);

				// Add condition for different cases
				//q = 0.1;

				if (h != 0 && w != 0) {
					if (q == 1) {
						sprintf(filename_conv,
							"%s/%d-%d-%s",
							CACHE_LOCATION, h, w,
							filename);
					} else {
						sprintf(filename_conv,
							"%s/%d-%d-%2.0f-%s",
							CACHE_LOCATION, h, w,
							q * 100, filename);
					}

					if (stat(filename_conv, &sb) == -1) {
						if (resize(filename,
							   filename_conv, w, h,
							   q * 100) !=
						    EXIT_SUCCESS)
							handle_error("resize");
					}

					close(fd);
					fd = open(filename_conv, O_RDONLY);
					if (fd == -1)
						handle_error("open");

				} else if (q != 1) {
					sprintf(filename_conv, "%s/%2.0f-%s",
						CACHE_LOCATION, q * 100,
						filename);

					if (stat(filename_conv, &sb) == -1) {
						if (change_quality(filename,
								   filename_conv,
								   q * 100) !=
						    EXIT_SUCCESS)
							handle_error(
								"change_quality");
					}

					close(fd);
					fd = open(filename_conv, O_RDONLY);
					if (fd == -1)
						handle_error("open");
				}
			}
#endif
		}

		struct stat stat_buf;
		fstat(fd, &stat_buf);

		sprintf(header,
			"HTTP/1.1 %d %s\r\nContent-length: %ld\r\nServer: %s version %s\r\nContent-Type: %s\r\n\r\n",
			code, message, stat_buf.st_size, SERVER_NAME,
			SERVER_VERSION, type);

		if (write(connfd, header, strlen(header)) <= 0) {
			close(fd);
			break;
		}

		char method[size];
		find_method(request, method);
		if (!strcmp(method, "GET")) {
			if (sendfile(connfd, fd, NULL, stat_buf.st_size) <= 0) {
				close(fd);
				break;
			}
		}
		close(fd);
	}
	if (close(connfd) < 0)
		handle_error("close");

	remove_node(client);
	pthread_exit(NULL);
}

uint16_t parse_input(int argc, char *argv[])
{
	if (argc < 2) {
		return PORT;
	} else if (argc > 2) {
		return 0;
	} else {
		char *end;
		intmax_t val = strtoimax(argv[1], &end, 10);
		if (errno == ERANGE || errno == EINVAL || val < 1 ||
		    val > UINT16_MAX || *end != '\0')
			return 0;
		return (uint16_t)val;
	}
}

int main(int argc, char *argv[])
{
	uint16_t port = parse_input(argc, argv);
	if (port == 0) {
		printf("Usage: command [port]\n");
		return -1;
	}

	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli;

	struct sigaction act;
	struct sigaction act1;

#ifdef IMAGE_CONVERTION

	struct stat st;
	int sr;
	if ((sr = stat(CACHE_LOCATION, &st)) == -1 && errno == ENOENT) {
		if (mkdir(CACHE_LOCATION, 0755) == -1) {
			perror("mkdir");
			exit(EXIT_FAILURE);
		}
	} else if (sr == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	} else if (!S_ISDIR(st.st_mode)) {
		printf("Remove file '%s' from folder containing server\n",
		       CACHE_LOCATION);
		return -1;
	}
#endif

	//setup logfile
	int lfd = open("logFile.log", O_WRONLY | O_CREAT | O_APPEND, 0644);

	if (lfd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	logFile = fdopen(lfd, "w");
	if (logFile == NULL) {
		perror("fdopen");
		exit(EXIT_FAILURE);
	}

	// Set SIGPIPE handler
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = pipe_handler;
	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGPIPE, &act, NULL) < 0) {
		handle_error("sigaction");
		exit(EXIT_FAILURE);
	}

	//set SIGINT handler
	memset(&act1, 0, sizeof(act1));
	act1.sa_sigaction = sigint_handler;
	act1.sa_flags = SA_SIGINFO;

	if (sigaction(SIGINT, &act1, NULL) < 0) {
		handle_error("sigaction");
		exit(EXIT_FAILURE);
	}

	// Socket creation and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd == -1) {
		handle_error("socket");
		exit(EXIT_FAILURE);
	} else {
		printf("Socket successfully created..\n");
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 },
		       sizeof(int)) < 0) {
		handle_error("setsockopt");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(struct sockaddr_in));

	// Assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	// Bind newly created socket to given IP and verification
	if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) !=
	    0) {
		handle_error("bind");
		exit(EXIT_FAILURE);
	} else {
		printf("Socket successfully binded..\n");
	}

	// Now server is ready to listen and verification
	if ((listen(sockfd, SOMAXCONN)) != 0) {
		//printf("Listen failed...\n");
		handle_error("listen");
		exit(EXIT_FAILURE);
	} else {
		printf("Server listening..\n");
	}
	len = sizeof(cli);

	// Initialize timeout structure to 300 seconds
	struct timeval timeout;
#ifdef IMAGE_CONVERTION
	timeout.tv_sec = 60;
#else
	timeout.tv_sec = 15;
#endif
	timeout.tv_usec = 0;

	pthread_attr_t attr;
	if (pthread_attr_init(&attr) != 0) {
		handle_error("pthread_attr_init");
		exit(EXIT_FAILURE);
	}
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
		handle_error("pthread_attr_setdetachstate");
		exit(EXIT_FAILURE);
	}

	while (1) {
		// Accept the data packet from client and verification
		connfd = accept(sockfd, (struct sockaddr *)&cli,
				(socklen_t *)&len);
		if (connfd < 0) {
			handle_error("accept");
		} else {
			IP_logger(connfd);
			//printf("server acccepted the client...\n");
		}

		// Set socket options
		if (setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO,
			       (void *)&timeout,
			       (socklen_t)sizeof(timeout)) == -1) {
			handle_error("setsockopt");
			exit(EXIT_FAILURE);
		}
		if (setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO,
			       (void *)&timeout,
			       (socklen_t)sizeof(timeout)) == -1) {
			handle_error("setsockopt");
			exit(EXIT_FAILURE);
		}

		// Pass connfd to thread
		pthread_t tid;
		if (pthread_create(&tid, &attr, thread_func,
				   (void *)append_node(connfd)) != 0)
			handle_error("pthread_create");
	}

	close(connfd);
	close(sockfd);

	return 0;
}
