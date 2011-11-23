#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <curl.h>
#include <glib.h>
#include <libconfig.h>
#include <libircclient.h>
#include <oauth.h>
#include "conf.h"
struct config_entry {
	int type;
	int element_type;
	int size;
	void *data;
};
