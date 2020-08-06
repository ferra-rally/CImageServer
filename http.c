#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

struct Header_line {
    char *line;
    struct Header_line *next;
};

char *find_line(char *header, char *target) {
    char *tmp, *dest;

    tmp = strstr(header, target);

    if(tmp != NULL) {
        dest = strtok(tmp, "\n");
        return dest;
    } else {
        return "";
    }
}

float find_quality(char *buff, char *extension) {
    char *header = malloc(strlen(buff));
    char *accept_string;
    char *tmp;
    char *target_quality;
    char *qstring;
    char *x;

    strcpy(header, buff);

    if (strstr(header, "Accept:") == NULL) {
        perror("Error: Header does not containt accept");
        return -1;
    }

    
    accept_string = find_line(header, "Accept: ");

    tmp = strstr(accept_string, extension);
    if(tmp == NULL) {
        x = strstr(accept_string, "*/*");
    } else {
        x = strstr(tmp, extension);
        printf("Not Default %s\n", x);
    }

    target_quality = strtok(x, ",\n");

    qstring = strstr(target_quality, "q=");

    if(qstring != NULL) {
        printf("TARGET: %s\n", qstring);
        qstring = qstring + 2;
        return atof(qstring);
    }
    
    return 1;
}

char *parse_resource(char *buff) {
    char *header = malloc(strlen(buff));
    char *resource;
    char *firstline;

    strcpy(header, buff);

    firstline = strtok(header, "\n");
    strtok(firstline, " ");
    resource = strtok(NULL, " ");

    return resource;
}

char *find_type(char *buff)
{
    char *type;
    char *temp;

    temp = malloc(strlen(buff));

    strcpy(temp, buff);
    strtok(temp, ".");
    type = strtok(NULL, ".");

    printf("Found type: %s\n", type);

    if (!strcmp(type, "jpg")) {
        return "image/jpg";
    } else if (!strcmp(type, "html")) {
        return "text/html";
    }
    
    return type;
}