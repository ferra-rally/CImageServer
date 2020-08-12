int resize(char *oldname, char *newname, int width, int height, int quality);
int resize_to_bigger(char *oldname, char *newname, int width, int height,
		     int quality);
int resize_force(char *oldname, char *newname, int width, int height,
		 int quality);
int change_quality(char *oldname, char *newname, int quality);
