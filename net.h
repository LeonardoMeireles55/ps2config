#ifndef __NET_H__
#define __NET_H__

#include "common.h"

typedef struct {
    uint32_t cmdid;
    union {
        struct {} empty;
        struct {
            uint32_t param;
        } oneU32;
        struct {
            uint32_t param1;
            uint32_t param2;
        } twoU32;
        struct {
            uint32_t offset;
            uint32_t hackid;
        } cmd_01;
        struct {
            char titleID[10];
        } cmd_00;
        struct {
            uint64_t ReplaceDataMask;
            uint64_t ReplaceData;
            uint64_t OriginalDataMask;
            uint64_t OriginalData;
        } cmd_08;
        struct {
            uint32_t count;
            struct {
                uint32_t offset;
                uint8_t OriginalData[8];
                uint8_t ReplaceData[8];
            } data[];
        } cmd_09;
        struct {
            uint32_t count;
            struct {
                uint32_t offset;
                uint8_t OriginalData[4];
                uint8_t ReplaceData[4];
            } data[];
        } cmd_0A;
        struct {
            uint32_t count;
            struct {
                uint32_t sector;
                uint32_t offset;
                uint32_t size;
                uint8_t ReplaceData[0x400];
                uint8_t OriginalData[0x400];
            } data[];
        } cmd_0B;
        struct {
            uint32_t count;
            uint32_t offset;
            uint32_t param[0x400];
        } cmd_42;
        struct {
            uint32_t offset;
            uint32_t Redirect;
            uint32_t data[0x10];
        } cmd_4B;
        struct {
            uint16_t param1;
            uint16_t param2;
        } twoU16;
        struct {
            uint32_t count;
            uint32_t paramArray[];
        } oneArrayU32;
        struct {
            uint64_t param;
        } oneU64;
    };
} NetCommand;

typedef struct {
    uint32_t cmdCount;
    NetCommand* commands;
} NetCfg_t;

int load_NetCfg(const char* filename, NetCfg_t* cfg);
int save_NetCfg(const char* filename, const NetCfg_t* cfg);
int NetCfg_to_txt(FILE* file, const NetCfg_t* cfg);
void free_NetCfg(NetCfg_t* cfg);
int NetCfg_scandir_to_txt(char* dirpath, char* logpath);
int netcfg_log(char *filepath, char *logpath);

#endif
