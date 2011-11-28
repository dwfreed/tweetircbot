#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <curl/curl.h>
#include <glib.h>
#include <libconfig.h>
#include <libircclient.h>
#include <oauth.h>
#include "conf.h"
#include "callbacks.h"
struct config_entry {
	int type;
	int element_type;
	int size;
	void *data;
};
