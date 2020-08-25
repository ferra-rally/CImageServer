#pragma once

float find_quality(char *buff, char *extension, size_t size);
void parse_resource(char *buff, char *result, size_t size);
void find_type(char *buff, char *result, size_t size);
void find_method(char *header, char *result, size_t size);
int connection_status(char *header, size_t size);
void find_line(char *header, char *target, char *result, size_t size);
void find_user_agent(char *header, char *result, size_t size);
