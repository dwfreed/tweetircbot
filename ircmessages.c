#include "global.h"

void *ircmessages(void *args){
	struct irc_session *session = (struct irc_session *)args;
	struct context *context = irc_get_ctx(session);
	g_static_rw_lock_reader_lock(context->flags_lock);
	while( context->flags.run ){
		g_static_rw_lock_reader_unlock(context->flags_lock);
		struct message *message;
		GTimeVal wait_time;
		g_get_current_time(&wait_time);
		g_time_val_add(&wait_time, 1000000);
		while( (message = g_async_queue_timed_pop(context->raw_messages, &wait_time)) ){
			if( message->eventcode ){
				switch( message->eventcode ){
					case 354:
						if( strcmp(message->params[2], "0" ) ){
							g_hash_table_insert(context->nicks, strdup(message->params[1]), strdup(message->params[2]));
						} else {
							g_hash_table_remove(context->nicks, message->params[1]);
						}
						break;
				}
			} else {
				if( !strcmp(message->event, "JOIN") ){
					if( strcmp(message->params[1], "*") ){
						g_hash_table_insert(context->nicks, irc_target_get_nick(message->origin), strdup(message->params[1]));
					} else {
						char *nick = irc_target_get_nick(message->origin);
						g_hash_table_remove(context->nicks, nick);
						free(nick);
					}
				} else if( !strcmp(message->event, "ACCOUNT") ){
					if( strcmp(message->params[0], "*") ){
						g_hash_table_insert(context->nicks, irc_target_get_nick(message->origin), strdup(message->params[0]));
					} else {
						char *nick = irc_target_get_nick(message->origin);
						g_hash_table_remove(context->nicks, nick);
						free(nick);
					}
				}
				free(message->event);
			}
			free(message->origin);
			unsigned int i;
			for( i = 0; i < message->count; ++i ){
				free(message->params[i]);
			}
			free(message->params);
			free(message);
			g_get_current_time(&wait_time);
			g_time_val_add(&wait_time, 1000000);
		}
		g_static_rw_lock_reader_lock(context->flags_lock);
	}
	g_static_rw_lock_reader_unlock(context->flags_lock);
	return NULL;
}

void message_free(void *item){
	struct message *message = (struct message *)item;
	free(message->origin);
	if( !message->eventcode ){
		free(message->event);
	}
	unsigned int i;
	for( i = 0; i < message->count; ++i ){
		free(message->params[i]);
	}
	free(message->params);
	free(message);
}
