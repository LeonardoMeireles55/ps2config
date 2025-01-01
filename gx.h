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
			uint32_t EEOffset;
			uint8_t align[4];
			int64_t FuncIDOffset;
		} cmd_00;
		
        struct {
            uint64_t DataOffset;
            uint8_t align[8];

            uint32_t ReplaceDataMask[2];
            uint32_t ReplaceData[2];
            uint32_t OriginalDataMask[2];
            uint32_t OriginalData[2];
        } cmd_07;

        struct {
            uint64_t DataOffset;
            uint32_t DataCount;
            uint8_t align[4];
            struct {
                uint32_t offset;
                uint8_t align[4];
                uint32_t OriginalData[2];
                uint32_t ReplaceData[2];
            } data[];
        } cmd_08;
        
        struct {
            uint64_t DataOffset;
            uint32_t DataCount;
            uint8_t align[4];
            struct {
                uint32_t sector;
                uint32_t offset;
                uint64_t ReplaceDataOffset;
                uint64_t OriginalDataOffset;
                uint8_t align[4];
                
                uint8_t ReplaceData[0x400];
                uint8_t OriginalData[0x400];
                uint32_t size;
            } data[];
        } cmd_09;

        struct {
            int64_t DataOffset;
            int32_t DataCount;
            uint8_t align[4];
        } cmd_type1;

        struct {
            int32_t param;
            uint8_t align[12];
        } oneU32;

        struct {
            int8_t param;
            uint8_t align[15];
        } oneU8;

        struct {
            int64_t DataOffset;
            uint8_t align[8];
        } cmd_type4;

        struct {
            int16_t param[2];
            uint8_t align[12];
        } twoU16;

        struct {
            int32_t param[2];
            uint8_t align[8];
        } twoU32;

        uint8_t align[16];
    };
} GxCommand;

typedef struct {
    GxHeader header;
    GxCommand* commands;
} GxCfg_t;


void es_GxHeader(GxHeader* header);
void es_GxCommand(GxCommand* command);
int load_GxCfg(const char* filename, GxCfg_t* cfg);
int save_GxCfg(const char* filename, GxCfg_t* cfg);
int GxCfg_to_txt(FILE* file, const GxCfg_t* cfg);
void free_GxCfg(GxCfg_t* cfg);
int GxCfg_scandir_to_txt(char* dirpath, char* logpath);
int gxcfg_log(char *filename, char *logpath);
int gxcfg_gxcfg(char *filename, char *dirOut);

#endif