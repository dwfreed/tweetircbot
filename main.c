#include "global.h"

int main(int argc __attribute__((__unused__)), char *argv[]){
	int retval = 0;
	struct rlimit limits;
	getrlimit(RLIMIT_AS, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_AS, &limits);
	getrlimit(RLIMIT_CORE, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_CORE, &limits);
	getrlimit(RLIMIT_CPU, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_CPU, &limits);
	getrlimit(RLIMIT_DATA, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_DATA, &limits);
	getrlimit(RLIMIT_FSIZE, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_FSIZE, &limits);
	getrlimit(RLIMIT_NOFILE, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_NOFILE, &limits);
	getrlimit(RLIMIT_NPROC, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_NPROC, &limits);
	getrlimit(RLIMIT_SIGPENDING, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_SIGPENDING, &limits);
	getrlimit(RLIMIT_STACK, &limits);
	limits.rlim_cur = limits.rlim_max;
	setrlimit(RLIMIT_STACK, &limits);
	struct irc_callbacks *callbacks = (struct irc_callbacks *)malloc(sizeof(struct irc_callbacks));
	callbacks->event_channel = irc_event;
	callbacks->event_channel_notice = irc_event;
	callbacks->event_connect = irc_event;
	callbacks->event_ctcp_action = irc_event;
	callbacks->event_ctcp_rep = irc_event;
	callbacks->event_ctcp_req = irc_event;
	callbacks->event_invite = irc_event;
	callbacks->event_join = irc_event;
	callbacks->event_kick = irc_event;
	callbacks->event_mode = irc_event;
	callbacks->event_nick = irc_event;
	callbacks->event_notice = irc_event;
	callbacks->event_numeric = irc_numeric;
	callbacks->event_part = irc_event;
	callbacks->event_privmsg = irc_event;
	callbacks->event_quit = irc_event;
	callbacks->event_topic = irc_event;
	callbacks->event_umode = irc_event;
	callbacks->event_unknown = irc_event;
	struct irc_session *session = irc_create_session(callbacks);
	free(callbacks);
	irc_option_set(session, LIBIRCCLIENT_OPTION_DEBUG);
	g_thread_init(NULL);
	struct context *context = (struct context *)calloc(1, sizeof(struct context));
	context->config = g_hash_table_new_full(g_str_hash, g_str_equal, free, config_free);
	if( load_config(context->config) ){
		if( config_get_boolean(context->config, "bot.sasl") ){
			irc_option_set(session, LIBIRCCLIENT_OPTION_SASL);
		}
		context->flags.run = TRUE;
		context->flags.restart = FALSE;
		context->flags.follow_config_changed = FALSE;
		context->flags.pong_received = FALSE;
		GStaticRWLock flags_lock = G_STATIC_RW_LOCK_INIT;
		context->flags_lock = &flags_lock;
		context->pong_cond = g_cond_new();
		context->pong_mutex = g_mutex_new();
		GStaticRWLock config_lock = G_STATIC_RW_LOCK_INIT;
		context->config_lock = &config_lock;
		context->nicks = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
		context->raw_tweets = g_async_queue_new_full(free);
		context->raw_messages = g_async_queue_new_full(message_free);
		context->outgoing_messages = g_async_queue_new_full(free);
		context->channel_queues = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
		irc_set_ctx(session, context);
		GError *error = NULL;
		GThread *ircmessage_thread;
		if( !(ircmessage_thread = g_thread_create(ircmessages, session, TRUE, &error)) ){
			fprintf(stderr, "Error starting IRC message handling thread: %s", error->message);
			g_error_free(error);
			retval = 1;
		} else {
			GThread *throttler_thread;
			if( !(throttler_thread = g_thread_create(throttler, session, TRUE, &error)) ){
				fprintf(stderr, "Error starting throttler thread: %s", error->message);
				g_error_free(error);
				retval = 1;
			} else {
				g_static_rw_lock_reader_lock(context->config_lock);
				if( config_get_boolean(context->config, "bot.ipv6") ){
					irc_connect6(session, config_get_string(context->config, "bot.server"), config_get_int(context->config, "bot.port"), config_get_string(context->config, "bot.password"),  config_get_string(context->config, "bot.nick"), config_get_string(context->config, "bot.user"), config_get_string(context->config, "bot.name"));
				} else {
					irc_connect(session, config_get_string(context->config, "bot.server"), config_get_int(context->config, "bot.port"), config_get_string(context->config, "bot.password"),  config_get_string(context->config, "bot.nick"), config_get_string(context->config, "bot.user"), config_get_string(context->config, "bot.name"));
				}
				g_static_rw_lock_reader_unlock(context->config_lock);
				irc_run(session);
				g_static_rw_lock_reader_lock(context->flags_lock);
				if( context->flags.run ){
					g_static_rw_lock_reader_unlock(context->flags_lock);
					g_static_rw_lock_writer_lock(context->flags_lock);
					context->flags.run = FALSE;
					g_static_rw_lock_writer_unlock(context->flags_lock);
				} else {
					g_static_rw_lock_reader_unlock(context->flags_lock);
				}
				g_thread_join(throttler_thread);
				g_thread_join(ircmessage_thread);
		}
		g_hash_table_destroy(context->channel_queues);
		g_async_queue_unref(context->outgoing_messages);
		g_async_queue_unref(context->raw_messages);
		g_async_queue_unref(context->raw_tweets);
		g_hash_table_destroy(context->nicks);
		g_mutex_free(context->pong_mutex);
		g_cond_free(context->pong_cond);
	} else {
		retval = 1;
	}
	g_hash_table_destroy(context->config);
	irc_destroy_session(session);
	if( context->flags.restart ){
		free(context);
		execvp(argv[0], argv);
	}
	free(context);
	return retval;
}
