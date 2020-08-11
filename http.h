float find_quality(char *buff, char *extension);
void parse_resource(char *buff, char *result);
void find_type(char *buff, char *result);
void find_method(char *header, char *result);
int connection_status(char *header);
void find_line(char *header, char *target, char *result);
void find_user_agent(char *header, char *result);
