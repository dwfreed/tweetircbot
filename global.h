#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <curl/curl.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include <oauth.h>
#include "libircclient.h"
#include "conf.h"
#include "callbacks.h"
#include "ircmessages.h"
#include "throttler.h"
struct context {
	struct {
		unsigned int run : 1;
		unsigned int restart : 1;
		unsigned int follow_config_changed : 1;
		unsigned int pong_received : 1;
	} flags;
	GStaticRWLock *flags_lock;
	GCond *pong_cond;
	GMutex *pong_mutex;
	GHashTable *config;
	GStaticRWLock *config_lock;
	GHashTable *nicks;
	GAsyncQueue *raw_tweets;
	GAsyncQueue *raw_messages;
	GAsyncQueue *outgoing_messages;
	GHashTable *channel_queues;
};
