#include "global.h"

void irc_event(struct irc_session *session, const char *event, const char *origin, const char **params, unsigned int count){
	struct context *context = (struct context *)irc_get_ctx(session);
	struct message *message = calloc(1, sizeof(struct message));
	message->origin = origin ? strdup(origin) : NULL;
	message->event = strdup(event);
	message->params = calloc(count, sizeof(char *));
	unsigned int i;
	for( i = 0; i < count; ++i ){
		message->params[i] = strdup(params[i]);
	}
	g_async_queue_push(context->raw_messages, message);
}

void irc_numeric(struct irc_session *session, unsigned int event, const char *origin, const char **params, unsigned int count){
	struct context *context = (struct context *)irc_get_ctx(session);
	struct message *message = calloc(1, sizeof(struct message));
	message->origin = origin ? strdup(origin) : NULL;
	message->eventcode = event;
	message->params = calloc(count, sizeof(char *));
	unsigned int i;
	for( i = 0; i < count; ++i ){
		message->params[i] = strdup(params[i]);
	}
	g_async_queue_push(context->raw_messages, message);
}
