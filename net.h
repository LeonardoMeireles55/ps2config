#ifndef __NET_H__
#define __NET_H__

#include "common.h"

typedef struct {
    uint32_t cmdid;
    union {
        struct {
            char titleID[10];
        } cmd_00;
        struct {
            uint32_t offset;
            uint32_t FuncId;
        } cmd_01;
        struct {
            uint32_t ReplaceDataMask[2];
            uint32_t ReplaceData[2];
            uint32_t OriginalDataMask[2];
            uint32_t OriginalData[2];
        } cmd_08;
        struct {
            uint32_t count;
            struct {
                uint32_t offset;
                uint32_t OriginalData[2];
                uint32_t ReplaceData[2];
            } data[0x20];
        } cmd_09;
        struct {
            uint32_t count;
            struct {
                uint32_t offset;
                uint32_t OriginalData;
                uint32_t ReplaceData;
            } data[0x20];
        } cmd_0A;
        struct {
            uint32_t count;
            struct {
                uint32_t sector;
                uint32_t offset;
                uint32_t size;
                uint32_t ReplaceData[0x100];
                uint32_t OriginalData[0x100];
            } data[0x30];
        } cmd_0B;
        struct {
            uint32_t count;
            uint32_t param[0x40];
        } cmd_12;
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
            uint16_t param[2];
        } twoU16;
        struct {
            uint64_t param;
        } oneU64;
        struct {
            uint32_t param;
        } oneU32;
        struct {
            uint32_t param[2];
        } twoU32;
    };
} NetCommand;

typedef struct {
    uint32_t cmdCount; // not used by config
    NetCommand* commands;
    uint8_t extra_size; // not used by config
    struct {
        uint8_t unk;
        uint8_t disc_count;
        uint8_t disc_index;
        uint8_t resetVM;
    } extra_data;
} NetCfg_t;

int load_NetCfg(const char* filename, NetCfg_t* cfg);
int save_NetCfg(const char* filename, NetCfg_t* cfg);
int NetCfg_to_txt(FILE* file, const NetCfg_t* cfg);
void free_NetCfg(NetCfg_t* cfg);
int NetCfg_scandir_to_txt(char* dirpath, char* logpath);
int netcfg_log(char *filepath, char *logpath);
int netcfg_netcfg(char *filepath, char *dirOut);

#endif
