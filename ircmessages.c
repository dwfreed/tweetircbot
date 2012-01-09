#include "global.h"

char isadmin(struct irc_session *session, char *origin){
	struct context *context = irc_get_ctx(session);
	char *nick = irc_target_get_nick(origin);
	char *account = g_hash_table_lookup(context->nicks, nick);
	free(nick);
	if( account ){
		int admins_size, i = 0;
		char found = FALSE;
		g_static_rw_lock_reader_lock(context->config_lock);
		GList *admins = config_get_array(context->config, "bot.admins", &admins_size);
		while( i < admins_size ){
			char *admin = config_array_get_string(admins, i++);
			if( !strcasecmp(admin, account) ){
				found = TRUE;
				break;
			}
		}
		g_static_rw_lock_reader_unlock(context->config_lock);
		return found;
	} else {
		return 0;
	}
}

char ischanop(struct irc_session *session, char *channel, char *origin){
	struct context *context = irc_get_ctx(session);
	char *nick = irc_target_get_nick(origin);
	char *account = g_hash_table_lookup(context->nicks, nick);
	free(nick);
	if( account ){
		int chanops_size, i = 0;
		char found = FALSE;
		char *chanop_setting_path = g_strdup_printf("channels.%s.ops", channel);
		g_static_rw_lock_reader_lock(context->config_lock);
		GList *chanops = config_get_array(context->config, chanop_setting_path, &chanops_size);
		while( i < chanops_size ){
			char *chanop = config_array_get_string(chanops, i++);
			if( !strcasecmp(chanop, account) ){
				found = TRUE;
				break;
			}
		}
		g_static_rw_lock_reader_unlock(context->config_lock);
		g_free(chanop_setting_path);
		return found;
	} else {
		return FALSE;
	}
}

void commands(struct irc_session *session, struct message *message, char *command_parts[]){
	struct context *context = irc_get_ctx(session);
	char *nick = irc_target_get_nick(message->origin);
	char authorized = FALSE;
	if( !strcasecmp(command_parts[0], "quit") ){
		if( isadmin(session, message->origin) ){
			g_static_rw_lock_writer_lock(context->flags_lock);
			context->flags.run = 0;
			g_static_rw_lock_writer_unlock(context->flags_lock);
			irc_cmd_quit(session, "Bye!");
			authorized = TRUE;
		}
	}
	if( !authorized ){
		irc_cmd_notice(session, nick, "You are not authorized to perform this command.");
	}
	free(nick);
}

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
				if( !strcasecmp(message->event, "JOIN") ){
					char *nick = irc_target_get_nick(message->origin);
					if( !strcmp(nick, irc_get_nick(session)) ){
						irc_send_raw(session, "WHO %s %%na", message->params[0]);
						free(nick);
					} else {
						if( strcmp(message->params[1], "*") ){
							g_hash_table_insert(context->nicks, nick, strdup(message->params[1]));
						} else {
							g_hash_table_remove(context->nicks, nick);
							free(nick);
						}
					}
				} else if( !strcasecmp(message->event, "ACCOUNT") ){
					char *nick = irc_target_get_nick(message->origin);
					if( strcmp(message->params[0], "*") ){
						g_hash_table_insert(context->nicks, nick, strdup(message->params[0]));
					} else {
						g_hash_table_remove(context->nicks, nick);
						free(nick);
					}
				} else if( !strcasecmp(message->event, "PRIVMSG") ){
					char *nick = irc_get_nick(session);
					size_t nick_length = strlen(nick);
					char **text_parts = g_strsplit(message->params[1], " ", 0);
					if( !strcmp(message->params[0], nick) ){
						commands(session, message, text_parts);
					} else if( !( strncmp(text_parts[0], nick, nick_length) || strncmp(text_parts[0] + nick_length, ":", 1) ) ){
						commands(session, message, text_parts + 1);
					}
					g_strfreev(text_parts);
				} else if( !strcasecmp(message->event, "CONNECT") ){
					irc_send_raw(session, "CAP REQ :account-notify extended-join");
					int channels_size;
					g_static_rw_lock_reader_lock(context->config_lock);
					GList *channels = config_get_array(context->config, "bot.channels", &channels_size);
					char join_command[511] = "JOIN ";
					int i = 0;
					while( i < channels_size ){
						while( strlen(join_command) < 505 ){
							char *channel = config_array_get_string(channels, i);
							if( strlen(join_command) + strlen(channel) + 1 > 505 ){
								break;
							} else {
								if( strcmp(join_command, "JOIN ") ){
									strcat(join_command, ",");
								}
								strcat(join_command, channel);
								++i;
							}
							if( i >= channels_size ){
								break;
							}
						}
						irc_send_raw(session, join_command, NULL);
						strcpy(join_command, "JOIN ");
					}
					g_static_rw_lock_reader_unlock(context->config_lock);
				} else if( !strcasecmp(message->event, "ERROR") ){
					irc_disconnect(session);
					g_static_rw_lock_writer_lock(context->flags_lock);
					context->flags.run = 0;
					g_static_rw_lock_writer_unlock(context->flags_lock);
				}
			}
			message_free(message);
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
