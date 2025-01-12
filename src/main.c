#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "common.h"
#include "util.h"
#include "gx.h"
#include "net.h"

FILE *netLog=NULL;
FILE *gxLog=NULL;
int verbose=0;

s64 GxFuncFuncIdOffset[] = {0x36B40,
							0x35FB0,
							0x34068,
							0x34144,
							0x33F98,
							0x36CF8,
							0x34224,
							0x37850,
							0x33DFC,
							0x36C04,
							0x36EF0,
							0x34354,
							0x34424,
							0x34520,
							0x345FC,
							0x365F0,
							0x36510,
							0x36430,
							0x34DD0,
							0x366C4,
							0x34EDC,
							0x3795C,
							0x3521C,
							0x347D0,
							0x35300,
							0x36E28,
							0x37614,
							0x35434,
							0x354F8,
							0x355BC,
							0x35680,
							0x35744,
							0x35808,
							0x358CC,
							0x35990,
							0x35A54,
							0x35B18,
							0x35BDC,
							0x35CA0,
							0x35D64,
							0x35E28,
							0x35EEC,
							0x35158,
							0x34994,
							0x36FC8,
							0x3607C,
						 0xFFFFFFFF,
							0x34A70,
							0x34B48,
							0x34C20,
							0x34CF8,
							0x37714
						};

#define col_gx   0
#define col_soft 1
#define col_net  2
s32 emuID[0x51][3] = {
//       GX   SOFT   NET
	{    -1,    -1, 0x00},
	{  0x00,  0x00, 0x01},
	{  0x01,  0x01, 0x02},
	{  0x02,  0x02, 0x03},
	{  0x03,  0x03, 0x04},
	{  0x04,  0x04, 0x05},
	{  0x05,  0x05, 0x06},
	{  0x06,  0x06, 0x07},
	{  0x07,  0x07, 0x08},
	{  0x08,  0x08, 0x09},
	{  0x2C,    -1, 0x0A},
	{  0x09,  0x09, 0x0B},
	{  0x0A,  0x0A, 0x0C},
	{  0x0B,  0x0B, 0x0D},
	{  0x0C,  0x0C, 0x0E},
	{  0x0D,  0x0D, 0x0F},
	{  0x0E,  0x0E, 0x10},
	{  0x0F,  0x0F, 0x11},
	{  0x10,  0x10, 0x12},
	{  0x11,  0x11, 0x13},
	{  0x12,  0x12, 0x14},
	{  0x13,  0x13, 0x15},
	{  0x14,  0x14, 0x16},
	{  0x15,  0x15, 0x17},
	{  0x16,  0x16, 0x18},
	{    -1,  0x17, 0x19},
	{  0x17,  0x18, 0x1A},
	{  0x18,  0x19, 0x1B},
	{  0x19,  0x1A, 0x1C},
	{  0x1A,  0x1B, 0x1D},
	{  0x1B,    -1, 0x1E},
	{  0x1C,  0x1C, 0x1F},
	{  0x1D,  0x1D, 0x20},
	{  0x1E,  0x1E, 0x21},
	{    -1,  0x1F, 0x22},
	{  0x1F,  0x20, 0x23},
	{  0x20,  0x21, 0x24},
	{  0x21,  0x22, 0x25},
	{  0x22,  0x23, 0x26},
	{  0x23,  0x24, 0x27},
	{  0x24,  0x25, 0x28},
	{  0x25,  0x26, 0x29},
	{  0x26,  0x27, 0x2A},
	{  0x27,  0x28, 0x2B},
	{  0x28,  0x29, 0x2C},
	{  0x29,  0x2A, 0x2D},
	{  0x2A,  0x2B, 0x2E},
	{  0x2B,    -1, 0x2F},
	{    -1,    -1, 0x30},
	{    -1,    -1, 0x31},
	{    -1,    -1, 0x32},
	{    -1,    -1, 0x33},
	{    -1,    -1, 0x34},
	{    -1,    -1, 0x35},
	{    -1,    -1, 0x36},
	{    -1,    -1, 0x37},
	{    -1,    -1, 0x38},
	{    -1,    -1, 0x39},
	{    -1,    -1, 0x3A},
	{    -1,    -1, 0x3B},
	{    -1,    -1, 0x3C},
	{    -1,    -1, 0x3D},
	{    -1,    -1, 0x3E},
	{    -1,    -1, 0x3F},
	{    -1,    -1, 0x40},
	{    -1,    -1, 0x41},
	{  0x2C,    -1, 0x42},
	{    -1,    -1, 0x43},
	{    -1,    -1, 0x44},
	{    -1,    -1, 0x45},
	{    -1,    -1, 0x46},
	{    -1,    -1, 0x47},
	{    -1,    -1, 0x48},
	{    -1,    -1, 0x49},
	{    -1,    -1, 0x4A},
	{    -1,    -1, 0x4B},
	{    -1,    -1, 0x4C},
	{    -1,    -1, 0x4D},
	{    -1,    -1, 0x4E},
	{    -1,    -1, 0x4F},
	{    -1,    -1, 0x50}
};

void getid(s32 *g, s32 *s, s32 *n) {
	for (int i = 0; i < 0x51; i++) {
		if (g != NULL && *g != -1 && emuID[i][col_gx] == *g) {
			if (s != NULL) *s = emuID[i][col_soft];
			if (n != NULL) *n = emuID[i][col_net];
			return;
		} else if (s != NULL && *s != -1 && emuID[i][col_soft] == *s) {
			if (g != NULL) *g = emuID[i][col_gx];
			if (n != NULL) *n = emuID[i][col_net];
			return;
		} else if (n != NULL && *n != -1 && emuID[i][col_net] == *n) {
			if (g != NULL) *g = emuID[i][col_gx];
			if (s != NULL) *s = emuID[i][col_soft];
			return;
		}
	}
}

// type 1: SLES-12345
// type 2: SLES_123.45
int check_TitleID_sanity(const char *title, int type) {
	if (type == 1) {
		if (strlen(title) != 10) {
			return -1;
		}

		for (int i = 0; i < 4; i++) {
			if (title[i] < 'A' || title[i] > 'Z') {
				return -1;
			}
		}
		if (title[4] != '-') {
			return -1;
		}
		for (int i = 5; i < 10; i++) {
			if (title[i] < '0' || title[i] > '9') {
				return -1;
			}
		}
	} else if (type == 2) {
		/* ignore, so we can keep extension 
		if (strlen(title) != 11) {
			return -1;
		}
		*/

		for (int i = 0; i < 4; i++) {
			if (title[i] < 'A' || title[i] > 'Z') {
				return -1;
			}
		}
		if (title[4] != '_') {
			return -1;
		}
		if (title[8] != '.') {
			return -1;
		}
		for (int i = 5; i < 8; i++) {
			if (title[i] < '0' || title[i] > '9') {
				return -1;
			}
		}
		for (int i = 9; i < 11; i++) {
			if (title[i] < '0' || title[i] > '9') {
				return -1;
			}
		}
	} else {
		return -1;
	}

	return 0;
}

uint64_t getTitleHash(const char* title) {

	if( check_TitleID_sanity(title, 2) != 0) {
		if(title [0] == 0) return 0;
		fprintf(stderr, "getTitleHash, Invalid TitleID: %s\n", title);
		return 0;
	}

	uint64_t decimal_id = 0;
	for (int i = 5; i <= 10; i++) {
		if (i != 8) {
			decimal_id = decimal_id * 10 + (title[i] - '0');
		}
	}

	uint8_t temp[5];
	temp[0] = ((title[0] >> 4) & 7) | ((decimal_id << 3) & 0xF8);
	temp[1] = ((title[1] >> 3) & 0xF) | ((title[0] << 4) & 0xF0);
	temp[2] = ((title[2] >> 2) & 0x1F) | ((title[1] << 5) & 0xE0);
	temp[3] = ((title[3] >> 1) & 0x3F) | ((title[2] << 6) & 0xC0);
	temp[4] = ((decimal_id >> 10) & 0x7F) | ((title[3] << 7) & 0x80);

	uint8_t temp10 = (decimal_id >> 2) & 0xF8;
	for (int i = 0; i < 5; i++) {
		temp[i] ^= temp10;
	}

	uint64_t result = 0;
	for (int i = 0; i < 5; i++) {
		result |= ((uint64_t)temp[i] << (i * 8));
	}

	return result;
}

// from SLES-12345 to SLES_123.45
int changeTitleIDFormat(const char *input_id, char *output_id) {
    
	if( check_TitleID_sanity(input_id, 1) != 0) {
		return -1;
	}

    strncpy(output_id, input_id, 4);
    output_id[4] = '_';
    strncpy(output_id + 5, input_id + 5, 3);
    output_id[8] = '.';
    strncpy(output_id + 9, input_id + 8, 2);
    output_id[11] = '\0';

    return 0;
}

int convert_NetToGx(const NetCfg_t* netCfg, GxCfg_t* gxCfg) {
	
	int ret=-1;

	if (!netCfg || !gxCfg) {
		perror("convert_NetToGx invalid input"); 
		goto end;
	}

	memset(&gxCfg->header, 0, sizeof(gxCfg->header));
	gxCfg->header.dataOffset = GX_DATA_OFFSET;
	gxCfg->header.cmdCount=0;
	gxCfg->header.hashTitle=0;

	gxCfg->commands = calloc(0x100, sizeof(GxCommand));
	if (!gxCfg->commands) {
		perror("Error allocating memory for GX commands");
		goto end;
	}

	for (uint32_t i = 0; i<netCfg->cmdCount; i++) {
		NetCommand* netCmd = &netCfg->commands[i];
		GxCommand* gxCmd = &gxCfg->commands[gxCfg->header.cmdCount];

		s32 netID = netCmd->cmdid;
		s32 gxID = -1;
		getid(&gxID, NULL, &netID);

		if( netID == 0x00) {
			char TitleID[11]={0};
			if( changeTitleIDFormat(netCmd->cmd_00.titleID, TitleID) == 0) {
				gxCfg->header.hashTitle = getTitleHash(TitleID);
			}
			continue;
		}
		if( netID == 0x3D) continue; // rev

		if(gxID == -1) {
			//fprintf(stderr, "Unsupported NET command ID: %X\n", netID);
			continue;
		}

		gxCmd->cmdId = gxID;

		switch (gxID) {
			case 0x00: // net 0x01
			{
				if( 0x34 <= netCmd->cmd_01.FuncId || netCmd->cmd_01.FuncId == 0x2E) {
					//fprintf(stderr, "Unsupported NET command ID: 0x%02X | FunctionID: 0x%08lX\n", netID, netCmd->cmd_01.FuncId);
					continue;
				}
				gxCmd->cmd_00.EEOffset = netCmd->cmd_01.offset;
				gxCmd->cmd_00.FuncIDOffset = GxFuncFuncIdOffset[netCmd->cmd_01.FuncId];
				break;
			}
			//u32
			case 0x01: case 0x03: case 0x06: case 0x0B: case 0x0C: case 0x0F:
            case 0x13: case 0x1C: case 0x1E: case 0x21: case 0x24: case 0x28: case 0x2A: case 0x2B:
			{
				gxCmd->oneU32.param = netCmd->oneU32.param;
				break;
			}
			case 0x07:
			{
				memcpy(gxCmd->cmd_07.OriginalDataMask, netCmd->cmd_08.OriginalDataMask, sizeof(netCmd->cmd_08.OriginalDataMask));
				memcpy(gxCmd->cmd_07.OriginalData, netCmd->cmd_08.OriginalData, sizeof(netCmd->cmd_08.OriginalData));
				memcpy(gxCmd->cmd_07.ReplaceDataMask, netCmd->cmd_08.ReplaceDataMask, sizeof(netCmd->cmd_08.ReplaceDataMask));
				memcpy(gxCmd->cmd_07.ReplaceData, netCmd->cmd_08.ReplaceData, sizeof(netCmd->cmd_08.ReplaceData));
				break;
			}
			case 0x08:
			{
				u8 cmdExist = 0;
				for (uint32_t j = 0; j < gxCfg->header.cmdCount; j++) {
					if(gxCfg->commands[j].cmdId == 0x08) {
						gxCmd = &gxCfg->commands[j];
						cmdExist=1;
					}
				}
				for(uint32_t j=0; j<netCmd->cmd_09.count; j++) {
					uint32_t k = gxCmd->cmd_08.DataCount + j;
					gxCmd->cmd_08.data[k].offset = netCmd->cmd_09.data[j].offset;
					memcpy(gxCmd->cmd_08.data[k].OriginalData, netCmd->cmd_09.data[j].OriginalData, sizeof(netCmd->cmd_09.data[j].OriginalData));
					memcpy(gxCmd->cmd_08.data[k].ReplaceData, netCmd->cmd_09.data[j].ReplaceData, sizeof(netCmd->cmd_09.data[j].ReplaceData));
				}
				gxCmd->cmd_08.DataCount += netCmd->cmd_09.count;
				if(cmdExist) continue;
				break;
			}
			case 0x09:
			{
				gxCmd->cmd_09.DataCount = netCmd->cmd_0B.count;
				for(uint32_t j=0; j<gxCmd->cmd_09.DataCount; j++) {
					gxCmd->cmd_09.data[j].offset = netCmd->cmd_0B.data[j].offset;
					gxCmd->cmd_09.data[j].sector = netCmd->cmd_0B.data[j].sector;
					gxCmd->cmd_09.data[j].size = netCmd->cmd_0B.data[j].size;

					memcpy(gxCmd->cmd_09.data[j].ReplaceData, netCmd->cmd_0B.data[j].ReplaceData, sizeof(netCmd->cmd_0B.data[j].ReplaceData));
					memcpy(gxCmd->cmd_09.data[j].OriginalData, netCmd->cmd_0B.data[j].OriginalData, sizeof(netCmd->cmd_0B.data[j].OriginalData));
				}
				break;
			}
			case 0x2C:
			{
				u8 cmdExist = 0;
				for (uint32_t j = 0; j<gxCfg->header.cmdCount; j++) {
					if(gxCfg->commands[j].cmdId == 0x2C) {
						gxCmd = &gxCfg->commands[j];
						cmdExist=1;
					}
				}
				if(netID == 0x42) {
					if( 0x100000 < netCmd->cmd_42.offset) continue;
					//original data always 0 before 0x100000
					for(uint32_t j=0; j<netCmd->cmd_42.count; j++) {
						uint32_t k = gxCmd->cmd_2C.DataCount + j;
						gxCmd->cmd_2C.data[k].offset = netCmd->cmd_42.offset + j*4;
						gxCmd->cmd_2C.data[k].ReplaceData = netCmd->cmd_42.param[j];
					}
					gxCmd->cmd_2C.DataCount += netCmd->cmd_42.count;
				} else
				if(netID == 0x0A) {
					for(uint32_t j=0; j<netCmd->cmd_0A.count; j++) {
						uint32_t k = gxCmd->cmd_2C.DataCount + j;
						gxCmd->cmd_2C.data[k].offset = netCmd->cmd_0A.data[j].offset;
						gxCmd->cmd_2C.data[k].ReplaceData = netCmd->cmd_0A.data[j].ReplaceData;
						gxCmd->cmd_2C.data[k].OriginalData = netCmd->cmd_0A.data[j].OriginalData;
					}
					gxCmd->cmd_2C.DataCount += netCmd->cmd_0A.count;
				}
				if(cmdExist) continue;
				break;
			}
			case 0x10:
			{
				gxCmd->cmd_10.DataCount = netCmd->cmd_12.count;
				memcpy(gxCmd->cmd_10.param, netCmd->cmd_12.param, sizeof(netCmd->cmd_12.param));
				break;
			}
			// u8 net 16 17 1D 1E
            case 0x14: case 0x15: case 0x1A: case 0x1B:
			{
                gxCmd->oneU32.param = netCmd->oneU32.param;
				break;
			}
			// u64 net 13 20 24
			case 0x11: case 0x1D: case 0x20:
			{
                gxCmd->oneU64.param = ((uint64_t) gxCmd->twoU32.param[0] << 32) | gxCmd->twoU32.param[1];
				break;
			}
			// 2 u16 net 0C
            case 0x0A:
			{
				memcpy(gxCmd->twoU16.param, netCmd->twoU16.param, sizeof(netCmd->twoU16.param));
				break;
			}
			// 2 u32 
            case 0x0D: case 0x0E: case 0x22: case 0x23: case 0x25:
			{
				memcpy(gxCmd->twoU32.param, netCmd->twoU32.param, sizeof(netCmd->twoU32.param));
				break;
			}
			case 0x02: case 0x04: case 0x05: case 0x12: case 0x16: case 0x17: case 0x18: case 0x19:
            case 0x1F: case 0x26: case 0x27: case 0x29:
			{
				break;
			}
            default:
			{
				fprintf(stderr, "Unhandled GX command ID: 0x%02X\n", gxID);
				continue;
			}
		}
		gxCfg->header.cmdCount++;
	}

	u64 virt_DataOffset = GX_DATA_OFFSET + 0x18*gxCfg->header.cmdCount;
	for (uint32_t i = 0; i<gxCfg->header.cmdCount; i++) {
		GxCommand* gxCmd = &gxCfg->commands[i];
		switch(gxCmd->cmdId)
		{
			case 0x07:
			{
				gxCmd->cmd_07.DataOffset = virt_DataOffset;
				virt_DataOffset += 0x20;
				break;
			}
			case 0x08:
			{
				gxCmd->cmd_08.DataOffset = virt_DataOffset;
				virt_DataOffset += 0x18*gxCmd->cmd_08.DataCount;
				break;
			}
			case 0x09:
			{
				gxCmd->cmd_09.DataOffset = virt_DataOffset;
				virt_DataOffset += 0x20*gxCmd->cmd_09.DataCount;
				for(uint32_t j=0; j<gxCmd->cmd_09.DataCount; j++) {
					gxCmd->cmd_09.data[j].ReplaceDataOffset = virt_DataOffset;
					virt_DataOffset += gxCmd->cmd_09.data[j].size;
					gxCmd->cmd_09.data[j].OriginalDataOffset = virt_DataOffset;
					virt_DataOffset += gxCmd->cmd_09.data[j].size;
				}
				break;
			}
			case 0x10:
			{
				gxCmd->cmd_10.DataOffset = virt_DataOffset;
				virt_DataOffset += gxCmd->cmd_10.DataCount * 4;
				break;
			}
			case 0x2C:
			{
				gxCmd->cmd_2C.DataOffset = virt_DataOffset;
				virt_DataOffset += gxCmd->cmd_2C.DataCount * 0x18;
				break;
			}
			default:
			{
				break;
			}
		}
	}
	ret=0;
end:
	return ret;
}

int convert(char *fileIn, char *dirOut, uint8_t type)
{
	int ret=-1;
	GxCfg_t gxcfg;
	NetCfg_t netcfg;

	switch(type)
	{
		case NET_GX:
		{
			if(load_NetCfg(fileIn, &netcfg)) {
				fprintf(stderr, "Failed to load_NetCfg %s\n", fileIn);
				goto end;
			}
			if(convert_NetToGx(&netcfg, &gxcfg)) {
				fprintf(stderr, "Failed to convert_NetToGx %s\n", fileIn);
				goto end;
			}
			/*
			if( cfg.header.cmdCount == 0) {
				fprintf(stderr, "No commands found %s\n", fileIn);
				goto end;
			}
			*/
			if( gxcfg.header.hashTitle == 0) {
				gxcfg.header.hashTitle = getTitleHash(custom_basename(fileIn));
				if(gxcfg.header.hashTitle == 0) {
					fprintf(stderr, "Failed to get TitleID %s | %s\n", fileIn, custom_basename(fileIn));
					goto end;
				}
			}
			
			char outFilePath[1024];
			snprintf(outFilePath, sizeof(outFilePath), "%s/%s", dirOut, custom_basename(fileIn));
			char *dot = strrchr(outFilePath, '.');
			if (dot) *dot = '\0';
			if(save_GxCfg(outFilePath, &gxcfg))  {
				fprintf(stderr, "Failed to save_GxCfg %s\n", dirOut);
				goto end;
			}
			break;
		}
		default:
			printf("Unsupported type %X : %s\n", type, fileIn);
			break;
	}
	ret=0;
end:
	free_NetCfg(&netcfg);
	free_GxCfg(&gxcfg);
	return ret;
}

void scan_task(const char *in, void (*func)(), const char *arg1, uint8_t arg2) {
	struct dirent *entry;
    DIR *dp = opendir(in);

    if (dp == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dp))) {

		if(!strcmp(entry->d_name, "..") || !strcmp(entry->d_name, ".")) continue;

		char filepath[1024];
		snprintf(filepath, sizeof(filepath), "%s/%s", in, entry->d_name);

		if(arg1 != NULL && arg2 != 0) {
			func(filepath, arg1, arg2);
		} else
		if(arg1 != NULL) {
			func(filepath, arg1);
		} else 
		if(arg2 != 0) {
			func(filepath, arg2);	
		} else {
			func(filepath);
		}
		
    }

    closedir(dp);
}

static void print_help() {
	printf( "\nUsage of ps2config-cmd v0.4\n"
			"    Format\n"
			"        ps2config-cmd.exe [option] <mode> <input> <output>\n"
			"    Description\n"
			"        Convert PS2 CONFIG from and to NET/GX/SOFT.\n"
			"    Option\n"
			"        -h, --help           Show this help text.\n"
			"        -v, --verbose        Make the operations more talkative.\n"
			"    Mode\n"
			"        -1, --NET_GX         Convert from NET to GX.\n"
			"        -2, --NET_SOFT       Convert from NET to SOFT.\n"
			"        -3, --GX_SOFT        Convert from GX to SOFT.\n"
			"        -4, --GX_NET         Convert from GX to NET.\n"
			"        -5, --SOFT_NET       Convert from SOFT to NET.\n"
			"        -6, --SOFT_GX        Convert from SOFT to GX.\n"
			"        -7, --NET_TXT        Extract data from NET to TXT.\n"
			"        -8, --GX_TXT         Extract data from GX to TXT.\n"
			"        -9, --SOFT_TXT       Extract data from SOFT to TXT.\n"
			 );
}

int main(int argc, char *argv[]) {
	
	if (argc < 4) {
		print_help();
		return 1;
	}

	int mode = -1;
	char *input = NULL;
	char *output = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			print_help();
			return 0;
		} else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
			verbose = 1;
		} else if (strcmp(argv[i], "-1") == 0 || strcmp(argv[i], "--NET_GX") == 0) {
			mode = NET_GX;
		} else if (strcmp(argv[i], "-2") == 0 || strcmp(argv[i], "--NET_SOFT") == 0) {
			mode = NET_SOFT;
		} else if (strcmp(argv[i], "-3") == 0 || strcmp(argv[i], "--GX_SOFT") == 0) {
			mode = GX_SOFT;
		} else if (strcmp(argv[i], "-4") == 0 || strcmp(argv[i], "--GX_NET") == 0) {
			mode = GX_NET;
		} else if (strcmp(argv[i], "-5") == 0 || strcmp(argv[i], "--SOFT_NET") == 0) {
			mode = SOFT_NET;
		} else if (strcmp(argv[i], "-6") == 0 || strcmp(argv[i], "--SOFT_GX") == 0) {
			mode = SOFT_GX;
		} else if (strcmp(argv[i], "-7") == 0 || strcmp(argv[i], "--NET_TXT") == 0) {
			mode = NET_TXT;
		} else if (strcmp(argv[i], "-8") == 0 || strcmp(argv[i], "--GX_TXT") == 0) {
			mode = GX_TXT;
		} else if (strcmp(argv[i], "-9") == 0 || strcmp(argv[i], "--SOFT_TXT") == 0) {
			mode = SOFT_TXT;
		} else if (strcmp(argv[i], "-100") == 0 || strcmp(argv[i], "--NET_NET") == 0) {
			mode = NET_NET;
		} else if (strcmp(argv[i], "-101") == 0 || strcmp(argv[i], "--GX_GX") == 0) {
			mode = GX_GX;
		} else if (strcmp(argv[i], "-102") == 0 || strcmp(argv[i], "--SOFT_SOFT") == 0) {
			mode = SOFT_SOFT;
		} else if (input == NULL) {
			input = argv[i];
		} else if (output == NULL) {
			output = argv[i];
		}
	}
	

	if (mode == -1 || input == NULL || output == NULL) {
		fprintf(stderr, "Invalid arguments\nType ps2config-cmd.exe -h or --help for help\n");
		return 1;
	}

	if( isDirExist(input) != 0 ) {
		fprintf(stderr, "Input directory not found: %s\n", input);
		return 1;
	}
	if( mode != NET_TXT && mode != GX_TXT && mode != SOFT_TXT) {
		if( isDirExist(output) != 0 ) {
			printf("Output directory not found...\nCreating output directory: %s\n", output);
			if (mkdir(output, 0777) != 0) {
				perror("mkdir");
				return 1;
			}
		}
	}

	if (verbose) {
		printf("Mode: %d\n", mode);
		printf("Input: %s\n", input);
		printf("Output: %s\n", output);
	}

	switch(mode)
	{
		case NET_SOFT:
		case GX_SOFT: case GX_NET:
		case SOFT_NET: case SOFT_GX:
		{
			fprintf(stderr, "NET_SOFT, GX_SOFT, GX_NET, SOFT_NET, SOFT_GX not implemented.\n");
			break;
		}
		case NET_GX:
		{
			scan_task(input, (void (*)())convert, output, mode); 
			break;
		}
		case NET_NET:
		{
			scan_task(input, (void (*)())netcfg_netcfg, output, 0); 
			break;
		}
		case GX_GX: 
		{
			scan_task(input, (void (*)())gxcfg_gxcfg, output, 0); 
			break;
		}
		case SOFT_SOFT:
		{
			fprintf(stderr, "SOFT_SOFT not implemented.\n");
			break;
		}
		case NET_TXT:
		{
			scan_task(input, (void (*)())netcfg_log, output, 0); 
			break;
		}
		case GX_TXT:
		{
			scan_task(input, (void (*)())gxcfg_log, output, 0); 
			break;
		}
		case SOFT_TXT:
		{
			fprintf(stderr, "SOFT_TXT not implemented.\n");
			break;
		}
		default:
		{
			fprintf(stderr, "Invalid mode: %d\n", mode);
			return 1;
		}
	}

	FCLOSE(gxLog);
	FCLOSE(netLog);

	return 0;
}
