enum types {
	TYPE_OBJECT,
	TYPE_ARRAY,
	TYPE_INTEGER,
	TYPE_BOOLEAN,
	TYPE_DOUBLE,
	TYPE_STRING
};
struct config_entry {
	enum types type;
	int size;
	void *data;
};
struct hash_table_entry {
	char *key;
	struct config_entry *value;
};
int load_config(GHashTable *config_table);
void config_parse_array(JsonReader *reader, GList **array);
void config_parse(char *path, GHashTable *config, JsonReader *reader);
char *config_get_string(GHashTable *config, char *path);
long long config_get_int(GHashTable *config, char *path);
double config_get_float(GHashTable *config, char *path);
int config_get_boolean(GHashTable *config, char *path);
GList *config_get_array(GHashTable *config, char *path, int *size);
char *config_array_get_string(GList *array, int index);
long long config_array_get_int(GList *array, int index);
double config_array_get_float(GList *array, int index);
int config_array_get_boolean(GList *array, int index);
GList *config_array_get_array(GList *array, int index, int *size);
GHashTable *config_array_get_object(GList *array, int index);
void config_free(void *item);
int unload_config(GHashTable *config_table);
void config_unparse(GHashTable *config, JsonBuilder *builder);
void config_array_unparse(GList *array, JsonBuilder *builder);
