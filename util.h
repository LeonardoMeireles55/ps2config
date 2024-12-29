#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <openssl/md5.h>

int get_md5(const char *path, unsigned char *md5_result, uint32_t length);
int calculate_md5(const char *path, unsigned char *md5_result);
char* custom_basename(char* path);
int isDirExist(const char *path);
int isFileExist(const char *path);
void write_data(FILE *file, uint8_t *data, uint32_t size, uint8_t check_align, uint8_t indent);

#endif // UTIL_H
