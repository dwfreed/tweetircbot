struct message {
	char *origin;
	char *event;
	unsigned int eventcode;
	char **params;
	unsigned int count;
};
void *ircmessages(void *args);
