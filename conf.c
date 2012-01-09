/*
 * Copyright 2011 by Douglas Freed <dwfreed@mtu.edu>
 * Derived from work by Kaleb Elwert <kelwert@mtu.edu>
 *
 * This work is licensed under the Creative Commons Attribution-Share-Alike 3.0 license
 */
#include "global.h"

int load_config(GHashTable *config_table){
	g_type_init();
	JsonParser *parser = json_parser_new();
	GError *error = NULL;
	if( !json_parser_load_from_file(parser, "twitterbot.conf", &error) ){
		fprintf(stderr, "Error parsing configuration file: %s\n", error->message);
		g_error_free(error);
		g_object_unref(parser);
		return 0;
	} else {
		JsonReader *reader = json_reader_new(json_parser_get_root(parser));
		config_parse("", config_table, reader);
		g_object_unref(reader);
		g_object_unref(parser);
		return 1;
	}
}

void config_parse_array(JsonReader *reader, GList **array){
	int i;
	for( i = 0; i < json_reader_count_elements(reader); ++i ){
		json_reader_read_element(reader, i);
		if( !json_reader_get_null_value(reader) ){
			struct config_entry *entry = (struct config_entry *)malloc(sizeof(struct config_entry));
			if( json_reader_is_object(reader) ){
				GHashTable *config = g_hash_table_new_full(g_str_hash, g_str_equal, free, config_free);
				config_parse("", config, reader);
				entry->type = TYPE_OBJECT;
				entry->size = 1;
				entry->data = config;
			} else if( json_reader_is_array(reader) ){
				GList *sub_array = NULL;
				config_parse_array(reader, &sub_array);
				sub_array = g_list_reverse(sub_array);
				entry->type = TYPE_ARRAY;
				entry->size = json_reader_count_elements(reader);
				entry->data = sub_array;
			} else {
				JsonNode *value = json_reader_get_value(reader);
				entry->size = 1;
				switch( json_node_get_value_type(value) ){
					case G_TYPE_INT64:
						entry->type = TYPE_INTEGER;
						entry->data = malloc(sizeof(long long));
						*((long long *)entry->data) = json_node_get_int(value);
						break;
					case G_TYPE_BOOLEAN:
						entry->type = TYPE_BOOLEAN;
						entry->data = malloc(sizeof(int));
						*((int *)entry->data) = json_node_get_boolean(value);
						break;
					case G_TYPE_DOUBLE:
						entry->type = TYPE_DOUBLE;
						entry->data = malloc(sizeof(double));
						*((double *)entry->data) = json_node_get_double(value);
						break;
					case G_TYPE_STRING:
						entry->type = TYPE_STRING;
						entry->data = strdup(json_node_get_string(value));
						break;
				}
			}
			*array = g_list_prepend(*array, entry);
		} else {
			*array = g_list_prepend(*array, NULL);
		}
		json_reader_end_element(reader);
	}
}

void config_parse(char *path, GHashTable *config, JsonReader *reader){
	int i;
	for( i = 0; i < json_reader_count_members(reader); ++i ){
		json_reader_read_element(reader, i);
		char *name;
		if( strcmp(path, "") ){
			name = g_strdup_printf("%s.%s", path, json_reader_get_member_name(reader));
		} else {
			name = g_strdup(json_reader_get_member_name(reader));
		}
		if( !json_reader_get_null_value(reader) ){
			if( json_reader_is_object(reader) ){
				config_parse(name, config, reader);
			} else if( json_reader_is_array(reader) ){
				GList *array = NULL;
				config_parse_array(reader, &array);
				array = g_list_reverse(array);
				struct config_entry *entry = (struct config_entry *)malloc(sizeof(struct config_entry));
				entry->type = TYPE_ARRAY;
				entry->size = json_reader_count_elements(reader);
				entry->data = array;
				g_hash_table_insert(config, strdup(name), entry);
			} else {
				JsonNode *value = json_reader_get_value(reader);
				struct config_entry *entry = (struct config_entry *)malloc(sizeof(struct config_entry));
				entry->size = 1;
				switch( json_node_get_value_type(value) ){
					case G_TYPE_INT64:
						entry->type = TYPE_INTEGER;
						entry->data = malloc(sizeof(long long));
						*((long long *)entry->data) = json_node_get_int(value);
						break;
					case G_TYPE_BOOLEAN:
						entry->type = TYPE_BOOLEAN;
						entry->data = malloc(sizeof(int));
						*((int *)entry->data) = json_node_get_boolean(value);
						break;
					case G_TYPE_DOUBLE:
						entry->type = TYPE_DOUBLE;
						entry->data = malloc(sizeof(double));
						*((double *)entry->data) = json_node_get_double(value);
						break;
					case G_TYPE_STRING:
						entry->type = TYPE_STRING;
						entry->data = strdup(json_node_get_string(value));
						break;
				}
				g_hash_table_insert(config, strdup(name), entry);
			}
		} else {
			g_hash_table_insert(config, strdup(name), NULL);
		}
		g_free(name);
		json_reader_end_element(reader);
	}
}

char *config_get_string(GHashTable *config, char *path){
	struct config_entry *value = (struct config_entry *)g_hash_table_lookup(config, path);
	if( value ){
		if( value->type == TYPE_STRING ){
			errno = 0;
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

long long config_get_int(GHashTable *config, char *path){
	struct config_entry *value = (struct config_entry *)g_hash_table_lookup(config, path);
	if( value ){
		if( value->type == TYPE_INTEGER ){
			errno = 0;
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
		if( value->type == TYPE_DOUBLE ){
			errno = 0;
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

int config_get_boolean(GHashTable *config, char *path){
	struct config_entry *value = (struct config_entry *)g_hash_table_lookup(config, path);
	if( value ){
		if( value->type == TYPE_BOOLEAN ){
			errno = 0;
			return *(int *)value->data;
		} else {
			errno = EINVAL;
			return -1;
		}
	} else {
		errno = ENOENT;
		return -1;
	}
}

GList *config_get_array(GHashTable *config, char *path, int *size){
	struct config_entry *value = (struct config_entry *)g_hash_table_lookup(config, path);
	if( value ){
		if( value->type == TYPE_ARRAY ){
			errno = 0;
			if( size ){
				*size = value->size;
			}
			return (GList *)value->data;
		} else {
			errno = EINVAL;
			return NULL;
		}
	} else {
		errno = ENOENT;
		return NULL;
	}
}

char *config_array_get_string(GList *array, int index){
	struct config_entry *value = (struct config_entry *)g_list_nth_data(array, index);
	if( value ){
		if( value->type == TYPE_STRING ){
			errno = 0;
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

long long config_array_get_int(GList *array, int index){
	struct config_entry *value = (struct config_entry *)g_list_nth_data(array, index);
	if( value ){
		if( value->type == TYPE_INTEGER ){
			errno = 0;
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

double config_array_get_float(GList *array, int index){
	struct config_entry *value = (struct config_entry *)g_list_nth_data(array, index);
	if( value ){
		if( value->type == TYPE_DOUBLE ){
			errno = 0;
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

int config_array_get_boolean(GList *array, int index){
	struct config_entry *value = (struct config_entry *)g_list_nth_data(array, index);
	if( value ){
		if( value->type == TYPE_BOOLEAN ){
			errno = 0;
			return *(int *)value->data;
		} else {
			errno = EINVAL;
			return -1;
		}
	} else {
		errno = ENOENT;
		return -1;
	}
}

GList *config_array_get_array(GList *array, int index, int *size){
	struct config_entry *value = (struct config_entry *)g_list_nth_data(array, index);
	if( value ){
		if( value->type == TYPE_ARRAY ){
			errno = 0;
			if( size ){
				*size = value->size;
			}
			return (GList *)value->data;
		} else {
			errno = EINVAL;
			return NULL;
		}
	} else {
		errno = ENOENT;
		return NULL;
	}
}

GHashTable *config_array_get_object(GList *array, int index){
	struct config_entry *value = (struct config_entry *)g_list_nth_data(array, index);
	if( value ){
		if( value->type == TYPE_OBJECT ){
			errno = 0;
			return (GHashTable *)value->data;
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
	if( config_item ){
		if( config_item->type == TYPE_ARRAY ){
			g_list_free_full((GList *)config_item->data, config_free);
		} else if( config_item->type == TYPE_OBJECT ){
			g_hash_table_destroy((GHashTable *)config_item->data);
		} else {
			free(config_item->data);
		}
		free(config_item);
	}
}

int unload_config(GHashTable *config){
	JsonBuilder *builder = json_builder_new();
	config_unparse(config, builder);
	JsonGenerator *generator = json_generator_new();
	JsonNode *root = json_builder_get_root(builder);
	json_generator_set_root(generator, root);
	json_node_free(root);
	json_generator_set_indent_char(generator, '\t');
	json_generator_set_indent(generator, 1);
	json_generator_set_pretty(generator, TRUE);
	GError *error;
	if( !json_generator_to_file(generator, "twitterbot.conf", &error) ){
		fprintf(stderr, "Error saving configuration file: %s\n", error->message);
		g_error_free(error);
		g_object_unref(builder);
		g_object_unref(generator);
		return 0;
	} else {
		g_object_unref(builder);
		g_object_unref(generator);
		return 1;
	}
}
void config_array_unparse(GList *array, JsonBuilder *builder){
	json_builder_begin_array(builder);
	do {
		struct config_entry *entry = (struct config_entry *)array->data;
		if( !entry ){
			json_builder_add_null_value(builder);
		} else {
			switch( entry->type ){
				case TYPE_ARRAY:
					config_array_unparse((GList *)entry->data, builder);
					break;
				case TYPE_OBJECT:
					config_unparse((GHashTable *)entry->data, builder);
					break;
				case TYPE_INTEGER:
					json_builder_add_int_value(builder, *(long long *)entry->data);
					break;
				case TYPE_BOOLEAN:
					json_builder_add_boolean_value(builder, *(int *)entry->data);
					break;
				case TYPE_DOUBLE:
					json_builder_add_double_value(builder, *(double *)entry->data);
					break;
				case TYPE_STRING:
					json_builder_add_string_value(builder, (char *)entry->data);
					break;
			}
		}
	} while( (array = g_list_next(array)) );
	json_builder_end_array(builder);
}

int hash_table_sort(const void *item1, const void *item2){
	struct hash_table_entry *entry1, *entry2;
	entry1 = (struct hash_table_entry *)item1;
	entry2 = (struct hash_table_entry *)item2;
	return strcmp(entry1->key, entry2->key);
}

inline unsigned int umin(unsigned int first, unsigned int second){
	return first < second ? first : second;
}

void config_unparse(GHashTable *config, JsonBuilder *builder){
	GHashTableIter iterator;
	void *key, *value;
	char *current_key, **old_setting_name, **current_setting_name;
	struct config_entry *current_value;
	struct hash_table_entry *config_array = calloc(g_hash_table_size(config), sizeof(struct hash_table_entry));
	unsigned int i = 0;
	g_hash_table_iter_init(&iterator, config);
	while( g_hash_table_iter_next(&iterator, &key, &value) ){
		config_array[i].key = (char *)key;
		config_array[i++].value = (struct config_entry *)value;
	}
	qsort(config_array, g_hash_table_size(config), sizeof(struct hash_table_entry), hash_table_sort);
	old_setting_name = g_strsplit("", ".", 0);
	current_setting_name = old_setting_name;
	json_builder_begin_object(builder);
	for( i = 0; i < g_hash_table_size(config); ++i ){
		current_key = config_array[i].key;
		current_value = config_array[i].value;
		current_setting_name = g_strsplit(current_key, ".", 0);
		unsigned int j;
		if( !old_setting_name[0] ){
			for( j = 0; j < g_strv_length(current_setting_name) - 1; ++j ){
				json_builder_set_member_name(builder, current_setting_name[j]);
				json_builder_begin_object(builder);
			}
		} else {
			for( j = 0; j < umin(g_strv_length(old_setting_name) - 1, g_strv_length(current_setting_name) - 1); ++j ){
				if( strcmp(old_setting_name[j], current_setting_name[j]) ){
					unsigned int k;
					for( k = j; k < g_strv_length(old_setting_name) - 1; ++k ){
						json_builder_end_object(builder);
					}
					for( k = j; k < g_strv_length(old_setting_name) - 1; ++k ){
						json_builder_set_member_name(builder, current_setting_name[k]);
						json_builder_begin_object(builder);
					}
					break;
				}
			}
			if( g_strv_length(current_setting_name) > g_strv_length(old_setting_name) ){
				for( j = g_strv_length(old_setting_name) - 1; j < g_strv_length(current_setting_name) - 1; ++j ){
					json_builder_set_member_name(builder, current_setting_name[j]);
					json_builder_begin_object(builder);
				}
			}
		}
		json_builder_set_member_name(builder, current_setting_name[j]);
		if( !current_value ){
			json_builder_add_null_value(builder);
		} else {
			switch( current_value->type ){
				case TYPE_ARRAY:
					config_array_unparse((GList *)current_value->data, builder);
					break;
				case TYPE_INTEGER:
					json_builder_add_int_value(builder, *(long long *)current_value->data);
					break;
				case TYPE_BOOLEAN:
					json_builder_add_boolean_value(builder, *(int *)current_value->data);
					break;
				case TYPE_DOUBLE:
					json_builder_add_double_value(builder, *(double *)current_value->data);
					break;
				case TYPE_STRING:
					json_builder_add_string_value(builder, (char *)current_value->data);
					break;
				case TYPE_OBJECT:
					break; /* To make gcc happy */
			}
		}
		g_strfreev(old_setting_name);
		old_setting_name = current_setting_name;
	}
	for( i = g_strv_length(current_setting_name) - 1; i > 0; --i ){
		json_builder_end_object(builder);
	}
	g_strfreev(current_setting_name);
	json_builder_end_object(builder);
}
