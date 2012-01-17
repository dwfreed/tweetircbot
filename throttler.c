#include "global.h"

void *throttler(void *args){
	struct irc_session *session = (struct irc_session *)args;
	struct context *context = irc_get_ctx(session);
	g_static_rw_lock_reader_lock(context->flags_lock);
	while( context->flags.run ){
		g_static_rw_lock_reader_unlock(context->flags_lock);
		GTimeVal wait_time;
		g_get_current_time(&wait_time);
		g_time_val_add(&wait_time, 1000000);
		g_mutex_lock(context->pong_mutex);
		if( context->flags.pong_received ){
			char *message;
			g_get_current_time(&wait_time);
			g_time_val_add(&wait_time, 1000000);
			if( ( message = g_async_queue_timed_pop(context->outgoing_messages, &wait_time) ) ){
				irc_send_raw(session, message, NULL);
				free(message);
				int i;
				for( i = 0; i < 3; ++i ){
					if( ( message = g_async_queue_try_pop(context->outgoing_messages) ) ){
						irc_send_raw(session, message, NULL);
						free(message);
					} else {
						break;
					}
				}
				context->flags.pong_received = FALSE;
				irc_send_raw(session, "PING %s", irc_get_nick(session));
				g_get_current_time(&wait_time);
				g_time_val_add(&wait_time, 1000000);
				if( g_cond_timed_wait(context->pong_cond, context->pong_mutex, &wait_time) ){
					g_mutex_unlock(context->pong_mutex);
				}
			} else {
				g_mutex_unlock(context->pong_mutex);
			}
		} else if( g_cond_timed_wait(context->pong_cond, context->pong_mutex, &wait_time) ){
			g_mutex_unlock(context->pong_mutex);
		}
		g_static_rw_lock_reader_lock(context->flags_lock);
	}
	g_static_rw_lock_reader_unlock(context->flags_lock);
	return NULL;
}
