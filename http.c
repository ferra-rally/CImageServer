#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

//save the http method of the request in a result buffer
void find_method(char *header, char *result, size_t size)
{
	char tmp[size];
	char *saveptr;

	strncpy(tmp, header, size);

	strncpy(result, strtok_r(tmp, " ", &saveptr), size);
}

void find_line(char *header, char *target, char *result, size_t size)
{
	char *tmp;
	char tmpheader[size];
	char *saveptr;

	strncpy(tmpheader, header, size);

	tmp = strstr(tmpheader, target);

	if (tmp != NULL) {
		strncpy(result, strtok_r(tmp, "\n", &saveptr), size);
	} else {
		strncpy(result, "", size);
	}
}

void find_user_agent(char *header, char *result, size_t size)
{
	char user_agent[size];
	char *saveptr;
	find_line(header, "User-Agent: ", user_agent, size);
	user_agent[strlen(user_agent) - 1] = 0;
	strtok_r(user_agent, " ", &saveptr);
	strcpy(result, strtok_r(NULL, " ", &saveptr));

	char *tmp = strtok_r(NULL, " ", &saveptr);
	while (tmp != NULL) {
		strcat(result, "%20");
		strcat(result, tmp);
		tmp = strtok_r(NULL, " ", &saveptr);
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

float find_quality(char *buff, char *extension, size_t size)
{
	char header[size];
	char *tmp;
	char *imagetmp;
	char *target_quality;
	char *qstring;
	char *x;
	char *saveptr;

	strcpy(header, buff);

	if (strstr(header, "Accept: ") == NULL) {
		return 1;
	}

	char accept_string[size];
	find_line(header, "Accept: ", accept_string, size);

	tmp = strstr(accept_string, extension);

	if (tmp != NULL) {
		x = strstr(tmp, extension);
	} else if ((imagetmp = strstr(accept_string, "image/*")) != NULL) {
		x = imagetmp;
	} else {
		x = strstr(accept_string, "*/*");
		if (x == NULL) {
			return 1;
		}
	}

	target_quality = strtok_r(x, ",\n", &saveptr);

	qstring = strstr(target_quality, "q=");

	if (qstring != NULL) {
		qstring = qstring + 2;
		return atof(qstring);
	}

	return 1;
}

void parse_resource(char *buff, char *result, size_t size)
{
	char header[size];
	char *firstline;
	char *res;
	char *saveptr;

	strncpy(header, buff, size);

	firstline = strtok_r(header, "\n", &saveptr);
	if (firstline == NULL) {
		return;
	}

	strtok_r(firstline, " ", &saveptr);
	res = strtok_r(NULL, " ?", &saveptr);
	if (res == NULL) {
		return;
	}
	strncpy(result, res, size);
}

void find_type(char *buff, char *result, size_t size)
{
	char *type;
	char *saveptr;
	char temp[size];

	strncpy(temp, buff, size);
	strtok_r(temp, ".", &saveptr);
	type = strtok_r(NULL, ".", &saveptr);
	if (type == NULL) {
		//default if not found
		strcpy(result, "text/html");
		return;
	}

	if (!strcmp(type, "jpg")) {
		strcpy(result, "image/jpg");
	} else if (!strcmp(type, "html")) {
		strcpy(result, "text/html");
	} else if (!strcmp(type, "webp")) {
		strcpy(result, "image/webp");
	} else if (!strcmp(type, "jpeg")) {
		strcpy(result, "image/jpeg");
	} else if (!strcmp(type, "png")) {
		strcpy(result, "image/png");
	} else {
		strcpy(result, type);
	}
}
