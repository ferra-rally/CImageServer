#include <stdio.h>
#include <stdlib.h>
#include "convert.h"

int resize(char *oldname, char *newname, int width, int height, int quality) {
    char cmd[512];
    sprintf(cmd, "convert %s -resize %dx%d -quality %d %s", oldname, width, height, quality, newname);
    return system(cmd);
}

int resize_to_bigger(char *oldname, char *newname, int width, int height, int quality) {
    char cmd[512];
    sprintf(cmd, "convert %s -resize %dx%d^ -quality %d %s", oldname, width, height, quality, newname);
    return system(cmd);
}

int resize_force(char *oldname, char *newname, int width, int height, int quality) {
    char cmd[512];
    sprintf(cmd, "convert %s -resize %dx%d! -quality %d %s", oldname, width, height, quality, newname);
    return system(cmd);
}