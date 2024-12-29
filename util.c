#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <openssl/evp.h>
#include <openssl/md5.h>

int get_md5(const char *path, unsigned char *md5_result, uint32_t length) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("Error opening file for MD5 calculation");
        return -1;
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        perror("Error creating MD5 context");
        fclose(file);
        return -1;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_md5(), NULL) != 1) {
        perror("Error initializing MD5 context");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return -1;
    }

    unsigned char buffer[1024];
    size_t bytes_read;
    uint32_t total_read = 0;
    while (total_read < length && (bytes_read = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        if (total_read + bytes_read > length) {
            bytes_read = length - total_read;
        }
        if (EVP_DigestUpdate(mdctx, buffer, bytes_read) != 1) {
            perror("Error updating MD5 context");
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            return -1;
        }
        total_read += bytes_read;
    }

    if (EVP_DigestFinal_ex(mdctx, md5_result, NULL) != 1) {
        perror("Error finalizing MD5 context");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return -1;
    }

    EVP_MD_CTX_free(mdctx);
    fclose(file);
    return 0;
}

int calculate_md5(const char *path, unsigned char *md5_result) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("Error opening file for MD5 calculation");
        return -1;
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        perror("Error creating MD5 context");
        fclose(file);
        return -1;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_md5(), NULL) != 1) {
        perror("Error initializing MD5 context");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return -1;
    }

    unsigned char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) != 0) {
        if (EVP_DigestUpdate(mdctx, buffer, bytes_read) != 1) {
            perror("Error updating MD5 context");
            EVP_MD_CTX_free(mdctx);
            fclose(file);
            return -1;
        }
    }

    if (EVP_DigestFinal_ex(mdctx, md5_result, NULL) != 1) {
        perror("Error finalizing MD5 context");
        EVP_MD_CTX_free(mdctx);
        fclose(file);
        return -1;
    }

    EVP_MD_CTX_free(mdctx);
    fclose(file);
    return 0;
}

char* custom_basename(char* path) {
    char *base = strrchr(path, '/');
    return base ? base + 1 : path;
}

void write_data(FILE *file, uint8_t *data, uint32_t size, uint8_t check_align, uint8_t indent) {
	uint8_t flag=0;
	for(int j=0; j < indent; j++) fprintf(file, "\t");
	
	for(int i=0; i < size; i++) {
		fprintf(file, "%02X", data[i]);
		if( data[i] != 0) flag=1;
		if( size <= i+1) break;
		if( (i+1)%16 == 0 ) {
			fprintf(file, "\n");
			for(int j=0; j < indent; j++) fprintf(file, "\t");
		} else 
		if( (i+1)%4 == 0 ) fprintf(file, " ");
	}
	if( flag && check_align ) fprintf(file, "  WARNING!");
	fprintf(file, "\n");
}

int isFileExist(const char *path) {
	struct stat buffer;
	FILE *file = fopen(path, "r");
	if (file) {
		fclose(file);
		return 0;
	}
	return 1;
}

int isDirExist(const char *path) {
	DIR* dir = opendir(path);
	if (dir) {
		closedir(dir);
		return 0;
	}
	return 1;
}
