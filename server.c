#include <stdio.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sys/sendfile.h>

#include "http.h"

#define PORT 8080
#define SA struct sockaddr 

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


char *find_type(char *buff) {
    char *type;
    char *temp;

    temp = malloc(strlen(buff));

    strcpy(temp, buff);
    strtok(temp, ".");
    type = strtok(NULL, ".");

    printf("Found type: %s\n", type);

    if(!strcmp(type, "jpg")) {
        return "image/jpg";
    } else if(!strcmp(type, "html")) {
        return "text/html";
    }
    
    return type;
}

// Driver function 
int main() 
{ 
    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
    char response[4096];

    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        handle_error("setsockopt(SO_REUSEADDR) failed");

    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        handle_error("Bind failed");
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 
  
    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd < 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else
        printf("server acccept the client...\n"); 
  
    memset(response,0,sizeof(response));

    while(1) {

        int n;
        n = read(connfd, response, 4096);
        if(n < 0) {
            printf("Error reading\n");
        } else {
            printf("Recieved:\n%s", response);
        }

        char *resource, *firstline;

        firstline = strtok(response, "\n");
        strtok(firstline, " ");
        resource = strtok(NULL, " ");

        char header[200];
        char requestedResource[200], filename[200];
        char type[20];
        int code;
        char message[20];

        printf("\n*****************\nRequested resource: %s\n", resource);

        if(!strcmp(resource, "/")) {
            strcpy(filename, "index.html");
        } else {
            strcpy(requestedResource, resource + 1);
            strcpy(filename, requestedResource);
        }

        int fd = open(filename, O_RDONLY);
        if(fd == -1) {
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

            int a = sendfile(connfd, fd, NULL, stat_buf.st_size);
            printf("\n%d\n*******************\n", a);
            close(fd);
        
    }

    close(connfd);
    close(sockfd);
}
