int compat_fromstring(char *string, int *compatibility);
int compat_tovalue(const char *name, int *compatibility);
struct SEE_string *compat_tostring(struct SEE_interpreter *interp,
	int compatibility);
