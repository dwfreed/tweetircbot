int load_config(GHashTable *config_table);
void config_parse(char *path, GHashTable *config, config_setting_t *parent);
char *config_get_string(GHashTable *config, char *path);
int config_get_int(GHashTable *config, char *path);
long long config_get_int64(GHashTable *config, char *path);
double config_get_float(GHashTable *config, char *path);
void *config_get_array(GHashTable *config, char *path, int *element_type, int *size);
void config_free(void *item);
