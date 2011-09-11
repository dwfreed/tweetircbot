/*
 * Copyright 2011 by Douglas Freed <dwfreed@mtu.edu>
 * Derived from work by Kaleb Elwert <kelwert@mtu.edu>
 *
 * This work is licensed under the Creative Commons Attribution-Share-Alike 3.0 license
 */
#include "global.h"

int load_config(GHashTable *config_table){
	config_t config;
	config_init(&config);
	if( !config_read_file(&config, "wolfbot.conf") ){
		fprintf(stderr, "%s:%d: error: %s", config_error_file(&config), config_error_line(&config), config_error_text(&config));
		config_destroy(&config);
		return 0;
	} else {
		config_parse("", config_table, config_root_setting(&config));
		config_destroy(&config);
		return 1;
	}
}

void config_parse(char *path, GHashTable *config, config_setting_t *parent){
	int i;
	for( i = 0; i < config_setting_length(parent); ++i ){
		config_setting_t *element = config_setting_get_elem(parent, i);
		char *name = config_setting_name(element);
		char *title;
		if( config_setting_is_root(parent) ){
			title = g_strdup(name);
		} else {
			title = g_strdup_printf("%s.%s", path, name);
		}
		struct config_entry *entry = calloc(1, sizeof(struct config_entry));
		entry->type = config_setting_type(element);
		switch( entry->type ){
			case CONFIG_TYPE_GROUP:
				free(entry);
				config_parse(title, config, element);
				break;
			case CONFIG_TYPE_STRING:
				entry->size = 1;
				entry->data = strdup(config_setting_get_string(element));
				g_hash_table_insert(config, strdup(title), entry);
				break;
			case CONFIG_TYPE_BOOL:
				entry->size = 1;
				entry->data = malloc(sizeof(int));
				*((int *)entry->data) = config_setting_get_bool(element);
				g_hash_table_insert(config, strdup(title), entry);
				break;
			case CONFIG_TYPE_INT:
				entry->size = 1;
				entry->data = malloc(sizeof(int));
				*((int *)entry->data) = config_setting_get_int(element);
				g_hash_table_insert(config, strdup(title), entry);
				break;
			case CONFIG_TYPE_INT64:
				entry->size = 1;
				entry->data = malloc(sizeof(long long));
				*((long long *)entry->data) = config_setting_get_int64(element);
				g_hash_table_insert(config, strdup(title), entry);
				break;
			case CONFIG_TYPE_FLOAT:
				entry->size = 1;
				entry->data = malloc(sizeof(double));
				*((double *)entry->data) = config_setting_get_float(element);
				g_hash_table_insert(config, strdup(title), entry);
				break;
			case CONFIG_TYPE_ARRAY:
				entry->size = config_setting_length(element);
				if( entry->size ){
					config_setting_t *firstentry = config_setting_get_elem(element, 0);
					entry->element_type = config_setting_type(firstentry);
					int j;
					switch( entry->element_type ){
						case CONFIG_TYPE_STRING:
							entry->data = malloc(entry->size * sizeof(char *));
							for( j = 0; j < entry->size; ++j ){
								((char **)entry->data)[j] = strdup(config_setting_get_string_elem(element, j));
							}
							break;
						case CONFIG_TYPE_BOOL:
							entry->data = malloc(entry->size * sizeof(int));
							for( j = 0; j < entry->size; ++j ){
								((int *)entry->data)[j] = config_setting_get_bool_elem(element, j);
							}
							break;
						case CONFIG_TYPE_INT:
							entry->data = malloc(entry->size * sizeof(int));
							for( j = 0; j < entry->size; ++j ){
								((int *)entry->data)[j] = config_setting_get_int_elem(element, j);
							}
							break;
						case CONFIG_TYPE_INT64:
							entry->data = malloc(entry->size * sizeof(long long));
							for( j = 0; j < entry->size; ++j ){
								((long long *)entry->data)[j] = config_setting_get_int64_elem(element, j);
							}
							break;
						case CONFIG_TYPE_FLOAT:
							entry->data = malloc(entry->size * sizeof(double));
							for( j = 0; j < entry->size; ++j ){
								((double *)entry->data)[j] = config_setting_get_float_elem(element, j);
							}
							break;
					}
				} else {
					entry->data = malloc(0);
				}
				g_hash_table_insert(config, strdup(title), entry);
				break;
		}
		g_free(title);
	}
}

char *config_get_string(GHashTable *config, char *path){
	struct config_entry *value = (struct config_entry *)g_hash_table_lookup(config, path);
	if( value ){
		if( value->type == CONFIG_TYPE_STRING ){
			return (char *)value->data;
		} else {
			errno = EINVAL;
			return NULL;
		}
	} else {
		errno = ENOENT;
		return NULL;
	}
}

int config_get_int(GHashTable *config, char *path){
	struct config_entry *value = (struct config_entry *)g_hash_table_lookup(config, path);
	if( value ){
		if( value->type == CONFIG_TYPE_INT || value->type == CONFIG_TYPE_BOOL ){
			return *(int *)value->data;
		} else {
			errno = EINVAL;
			return 0;
		}
	} else {
		errno = ENOENT;
		return 0;
	}
}

long long config_get_int64(GHashTable *config, char *path){
	struct config_entry *value = (struct config_entry *)g_hash_table_lookup(config, path);
	if( value ){
		if( value->type == CONFIG_TYPE_INT64 ){
			return *(long long *)value->data;
		} else {
			errno = EINVAL;
			return 0;
		}
	} else {
		errno = ENOENT;
		return 0;
	}
}

double config_get_float(GHashTable *config, char *path){
	struct config_entry *value = (struct config_entry *)g_hash_table_lookup(config, path);
	if( value ){
		if( value->type == CONFIG_TYPE_FLOAT ){
			return *(double *)value->data;
		} else {
			errno = EINVAL;
			return 0.0;
		}
	} else {
		errno = ENOENT;
		return 0.0;
	}
}

void *config_get_array(GHashTable *config, char *path, int *element_type, int *size){
	struct config_entry *value = (struct config_entry *)g_hash_table_lookup(config, path);
	if( value ){
		if( value->type == CONFIG_TYPE_ARRAY ){
			if( element_type ){
				*element_type = value->element_type;
			}
			if( size ){
				*size = value->size;
			}
			return value->data;
		} else {
			errno = EINVAL;
			return NULL;
		}
	} else {
		errno = ENOENT;
		return NULL;
	}
}

void config_free(void *item){
	struct config_entry *config_item = (struct config_entry *)item;
	if( config_item->type == CONFIG_TYPE_ARRAY ){
		if( config_item->element_type == CONFIG_TYPE_STRING ){
			int i;
			for( i = 0; i < config_item->size; ++i ){
				free(((void **)config_item->data)[i]);
			}
		}
		free(config_item->data);
	} else {
		free(config_item->data);
	}
	free(config_item);
}
