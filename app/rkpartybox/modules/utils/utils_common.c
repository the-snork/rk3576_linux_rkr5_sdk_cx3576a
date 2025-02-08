#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "slog.h"

char *cJSON_file_read(const char *jsonfile)
{
    FILE *f_json = NULL;
    long json_size;
    char *json_data = NULL;

    ALOGD("open %s\n", jsonfile);
    f_json = fopen(jsonfile, "r");
    if (f_json == NULL) {
        ALOGE("%s open %s  failed\n", __func__, jsonfile);
        return NULL;
    }

    fseek(f_json, 0, SEEK_END);
    json_size = ftell(f_json);
    if (json_size < 0) {
        ALOGE("%s ftell failed, invalid file size\n", __func__);
        fclose(f_json);
        return NULL;
    }

    fseek(f_json, 0, SEEK_SET);
    json_data = (char *) os_malloc(json_size + 1);
    if (json_data == NULL) {
        ALOGE("%s Memory allocation failed\n", __func__);
        fclose(f_json);
        return NULL;
    }

    size_t ret = fread(json_data, 1, json_size, f_json);
    if (ret != json_size) {
        ALOGE("%s File read error, expected %ld, got %zu\n", __func__, json_size, ret);
        os_free(json_data);
        fclose(f_json);
        return NULL;
    }

    json_data[json_size] = '\0';

    fclose(f_json);
    f_json = NULL;

    ALOGD("%s len:%d\n", __func__, json_size);
    return json_data;
}