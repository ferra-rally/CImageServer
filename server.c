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
#define MAX 80 
#define PORT 8080
#define SA struct sockaddr 

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)
  
// Driver function 
int main() 
{ 
    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
    char response[4096];

    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
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

    int n;
    n = read(connfd, response, 4096);
    if(n < 0) {
        printf("Error reading\n");
    } else {
        printf("Recieved:\n%s", response);
    }

    char *resource, *firstline;

    firstline = strtok(response, "\n");
    printf("First line: %s\n", firstline);
    strtok(firstline, " ");
    resource = strtok(NULL, " ");

    printf("\n*****************\nRequested resource: %s\n", resource);

    char page[200];

    if(!strcmp("/", resource) || !strcmp("/index.html", resource)) {
        //sprintf(string, "<head>\n<title>This is a big discovery</title>\n</head>\n<body><h1>Benvenuti in questo fantastico server</h1></body>");
        

        int fd = open("index.html", O_RDONLY);

        FILE *file = fdopen(fd, "r");
        fseek(file, 0L, SEEK_END);
        int size = ftell(file);
        fseek(file, 0L, SEEK_SET);

        char *string = malloc(size + 1);
        fread(string, 1, size, file);
        fclose(file);

        sprintf(page, "HTTP/1.1 200 OK\nContent-length: %d\nContent-Type: text/html\n\r\n\r%s", size, string);

        close(fd);

        write(connfd, page, strlen(page));
    }

    // After chatting close the socket 
    close(connfd);
    close(sockfd);
}
