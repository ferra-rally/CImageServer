#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int find_quality(char *buff) {
    return 1;
}

char *parse_resource(char *buff) {
    char *header = malloc(strlen(buff));
    char *resource;
    char *firstline;

    strcpy(header, buff);

    printf("=========================\n");
    firstline = strtok(header, "\n");
    strtok(firstline, " ");
    resource = strtok(NULL, " ");

    printf("Resource: %s\n", resource);
    printf("=========================\n");

    return resource;
}