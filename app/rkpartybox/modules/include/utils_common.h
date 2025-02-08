#ifndef _UTILS_COMMON_H_
#define _UTILS_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BIT(x) (1<<x)
#define CSTR(x) (#x)
#define CVAL(x) CSTR(x)

char *cJSON_file_read(const char *jsonfile);

#ifdef __cplusplus
}
#endif
#endif