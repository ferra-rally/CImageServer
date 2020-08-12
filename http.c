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

void find_user_agent(char *header, char *result)
{
	size_t size = strlen(header) + 1;
	char user_agent[size];
	find_line(header, "User-Agent: ", user_agent);
	user_agent[strlen(user_agent) - 1] = 0;
	strtok(user_agent, " ");
	strcpy(result, strtok(NULL, " "));

	char *tmp = strtok(NULL, " ");
	while (tmp != NULL) {
		strcat(result, "%20");
		strcat(result, tmp);
		tmp = strtok(NULL, " ");
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
	}

	target_quality = strtok(x, ",\n");

	qstring = strstr(target_quality, "q=");

	if (qstring != NULL) {
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
	char *res;

	strncpy(header, buff, size);

	firstline = strtok(header, "\n");
	if (firstline == NULL) {
		return;
	}

	strtok(firstline, " ");
	res = strtok(NULL, " ?");
	if (res == NULL) {
		return;
	}
	strncpy(result, res, size);
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

	if (!strcmp(type, "jpg")) {
		strncpy(result, "image/jpg", size);
	} else if (!strcmp(type, "html")) {
		strncpy(result, "text/html", size);
	} else {
		strncpy(result, type, size);
	}
}
