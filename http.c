#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void find_method(char *header, char *result)
{
    size_t size = strlen(header) + 1;
    char tmp[size];

    strncpy(tmp, header, size);

    strncpy(result, strtok(tmp, " "), size);
}

void find_line(char *header, char *target, char *result)
{
    char *tmp;
    size_t size = strlen(header) + 1;
    char tmpheader[size];

    strncpy(tmpheader, header, size);

    tmp = strstr(tmpheader, target);

    if (tmp != NULL) {
        strncpy(result, strtok(tmp, "\n"), size);
    } else {
        strncpy(result, "", size);
    }
}

int connection_status(char *header)
{
    if (strstr(header, "Connection: keep-alive") != NULL) {
        return 1;
    } else {
        return 0;
    }
}

float find_quality(char *buff, char *extension)
{
    size_t size = strlen(buff) + 1;
    char header[size];
    char *tmp;
    char *target_quality;
    char *qstring;
    char *x;

    strcpy(header, buff);

    if (strstr(header, "Accept: ") == NULL) {
        perror("Error: Header does not containt accept");
        return -1;
    }

    
    char accept_string[size];
    find_line(header, "Accept: ", accept_string);

    tmp = strstr(accept_string, extension);
    if (tmp == NULL) {
        x = strstr(accept_string, "*/*");
    } else {
        x = strstr(tmp, extension);
        printf("Not Default %s\n", x);
    }

    target_quality = strtok(x, ",\n");

    qstring = strstr(target_quality, "q=");

    if (qstring != NULL) {
        printf("TARGET: %s\n", qstring);
        qstring = qstring + 2;
        return atof(qstring);
    }
    
    return 1;
}

void parse_resource(char *buff, char *result)
{
    size_t size = strlen(buff) + 1;
    char header[size];
    char *firstline;

    strncpy(header, buff, size);

    firstline = strtok(header, "\n");
    strtok(firstline, " ");
    strncpy(result, strtok(NULL, " ?"), size);
    //strtok(NULL, " ?");
}

void find_type(char *buff, char *result)
{
    size_t size = strlen(buff) + 1;
    char *type;
    char temp[size];

    strncpy(temp, buff, size);
    strtok(temp, ".");
    type = strtok(NULL, ".");

    printf("Found type: %s\n", type);

    if (!strcmp(type, "jpg")) {
        strncpy(result, "image/jpg", size);
    } else if (!strcmp(type, "html")) {
        strncpy(result, "text/html", size);
    } else {
        strncpy(result, type, size);
    }
}