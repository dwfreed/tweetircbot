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
				g_cond_timed_wait(context->pong_cond, context->pong_mutex, &wait_time);
				g_mutex_unlock(context->pong_mutex);
			} else {
				g_mutex_unlock(context->pong_mutex);
			}
		} else {
			g_cond_timed_wait(context->pong_cond, context->pong_mutex, &wait_time);
			g_mutex_unlock(context->pong_mutex);
		}
		g_static_rw_lock_reader_lock(context->flags_lock);
	}
	g_static_rw_lock_reader_unlock(context->flags_lock);
	return NULL;
}

int irc_send_raw_throttled(struct irc_session *session, const char *format, ...){
	struct context *context = irc_get_ctx(session);
	char *command;
	va_list va_arg_list;
	va_start(va_arg_list, format);
	if( vasprintf(&command, format, va_arg_list) < 0 ){
		return 1;
	}
	va_end(va_arg_list);
	g_async_queue_push(context->outgoing_messages, command);
	return 0;
}

int irc_cmd_channel_mode_throttled(struct irc_session *session, const char *channel, const char *mode){
	if( !channel ){
		return 1;
	}
	if( mode ){
		return irc_send_raw_throttled(session, "MODE %s %s", channel, mode);
	} else {
		return irc_send_raw_throttled(session, "MODE %s", channel);
	}
}

int irc_cmd_ctcp_reply_throttled(struct irc_session *session, const char *nick, const char *reply){
	if( !nick || !reply ){
		return 1;
	}
	return irc_send_raw_throttled(session, "NOTICE %s :\1%s\1", nick, reply);
}

int irc_cmd_ctcp_request_throttled(struct irc_session *session, const char *nch, const char *request){
	if( !nch || !request ){
		return 1;
	}
	return irc_send_raw_throttled(session, "PRIVMSG %s :\1%s\1", nch, request);
}

int irc_cmd_invite_throttled(struct irc_session *session, const char *nick, const char *channel){
	if( !channel || !nick ){
		return 1;
	}
	return irc_send_raw_throttled(session, "INVITE %s %s", nick, channel);
}

int irc_cmd_join_throttled(struct irc_session *session, const char *channel, const char *key){
	if( !channel ){
		return 1;
	}
	if( key ){
		return irc_send_raw_throttled(session, "JOIN %s :%s", channel, key);
	} else {
		return irc_send_raw_throttled(session, "JOIN %s", channel);
	}
}

int irc_cmd_kick_throttled(struct irc_session *session, const char *nick, const char *channel, const char *reason){
	if( !channel || !nick ){
		return 1;
	}
	if( reason ){
		return irc_send_raw_throttled(session, "KICK %s %s :%s", channel, nick, reason);
	} else {
		return irc_send_raw_throttled(session, "KICK %s %s", channel, nick);
	}
}

int irc_cmd_list_throttled(struct irc_session *session, const char *channel){
	if( channel ){
		return irc_send_raw_throttled(session, "LIST %s", channel);
	} else {
		return irc_send_raw_throttled(session, "LIST");
	}
}

int irc_cmd_me_throttled(struct irc_session *session, const char *nch, const char *text){
	if( !nch || !text ){
		return 1;
	}
	return irc_send_raw_throttled(session, "PRIVMSG %s :\1ACTION %s\1", nch, text);
}

int irc_cmd_msg_throttled(struct irc_session *session, const char *nch, const char *text){
	if( !nch || !text ){
		return 1;
	}
	char *temp_text = strdup(text), *new_text = temp_text;
	int length = strlen(new_text), max_text_length = 512 - (strlen(irc_get_nick(session)) + strlen(irc_get_username(session)) + strlen(irc_get_hostname(session)) + strlen(nch) + 17), ret_val = 0;
	while( length > 0 ){
		ret_val = irc_send_raw_throttled(session, "PRIVMSG %s :%s", nch, new_text);
		new_text = &new_text[max_text_length];
		length -= max_text_length;
	}
	free(temp_text);
	return ret_val;
}

int irc_cmd_msg_to_throttled(struct irc_session *session, const char *channel, const char *to, const char *text){
	if( !channel || !to || !text ){
		return 1;
	}
	char *new_text;
	if( asprintf(&new_text, "%s: %s", to, text) < 0 ){
		session->last_error = LIBIRCCLIENT_ERR_NOMEM;
		return 1;
	}
	int ret_val = irc_cmd_msg(session, channel, new_text);
	free(new_text);
	return ret_val;
}

int irc_cmd_names_throttled(struct irc_session *session, const char *channel){
	if( !channel ){
		return 1;
	}
	return irc_send_raw_throttled(session, "NAMES %s", channel);
}

int irc_cmd_nick_throttled(struct irc_session *session, const char *newnick){
	if( !newnick ){
		return 1;
	}
	return irc_send_raw_throttled(session, "NICK %s", newnick);
}

int irc_cmd_notice_throttled(struct irc_session *session, const char *nch, const char *text){
	if( !nch || !text ){
		return 1;
	}
	return irc_send_raw_throttled(session, "NOTICE %s :%s", nch, text);
}

int irc_cmd_part_throttled(struct irc_session *session, const char *channel, const char *reason){
	if( !channel ){
		return 1;
	}
	if( reason ){
		return irc_send_raw_throttled(session, "PART %s :%s", channel, reason);
	} else {
		return irc_send_raw_throttled(session, "PART %s", channel);
	}
}

int irc_cmd_quit_throttled(struct irc_session *session, const char *reason){
	if( reason ){
		return irc_send_raw_throttled(session, "QUIT :%s", reason);
	} else {
		return irc_send_raw_throttled(session, "QUIT");
	}
}

int irc_cmd_topic_throttled(struct irc_session *session, const char *channel, const char *topic){
	if( !channel ){
		return 1;
	}
	if( topic ){
		return irc_send_raw_throttled(session, "TOPIC %s :%s", channel, topic);
	} else {
		return irc_send_raw_throttled(session, "TOPIC %s", channel);
	}
}

int irc_cmd_user_mode_throttled(struct irc_session *session, const char *mode){
	if( mode ){
		return irc_send_raw_throttled(session, "MODE %s %s", session->nick, mode);
	} else {
		return irc_send_raw_throttled(session, "MODE %s", session->nick);
	}
}

int irc_cmd_whois_throttled(struct irc_session *session, const char *nick){
	if( !nick ){
		return 1;
	}
	return irc_send_raw_throttled(session, "WHOIS %1$s %1$s", nick);
}
