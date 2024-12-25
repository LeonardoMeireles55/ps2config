#ifndef __GX_H__
#define __GX_H__

#include "common.h"

typedef struct {
    uint64_t hashTitle;
    uint32_t dataOffset;
    uint32_t cmdCount;
} GxHeader;

typedef struct {
    uint32_t cmdId;
    uint32_t align32;

    union {
		struct {
			int32_t EEOffset;
			uint8_t align[4];
			int64_t FuncIDOffset;
		} cmd_type0;
		
        struct {
            int64_t cmdDataOffset;
            int32_t cmdDataCount;
            uint8_t align[4];
        } cmd_type1;

        struct {
            int32_t param;
            uint8_t align[12];
        } cmd_type2;

        struct {
            int8_t param;
            uint8_t align[15];
        } cmd_type3;

        struct {
            int64_t cmdDataOffset;
            uint8_t align[8];
        } cmd_type4;

        struct {
            int16_t param1;
            int16_t param2;
            uint8_t align[12];
        } cmd_type5;

        struct {
            int32_t param1;
            int32_t param2;
            uint8_t align[8];
        } cmd_type6;

        uint8_t align[16];
    } data;
} GxCommand;

typedef struct {
    GxHeader header;
    GxCommand* commands;
} GxCfg_t;


void es_GxHeader(GxHeader* header);
void es_GxCommand(GxCommand* command);
int load_GxCfg(const char* filename, GxCfg_t* cfg);
int save_GxCfg(const char* filename, const GxCfg_t* cfg);
int GxCfg_to_txt(FILE* file, const GxCfg_t* cfg);
void free_GxCfg(GxCfg_t* cfg);
int GxCfg_scandir_to_txt(char* dirpath, char* logpath);
int gxcfg_log(char *filename, char *logpath);

#endif