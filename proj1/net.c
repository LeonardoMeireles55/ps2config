#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "net.h"
extern void write_data(FILE *file, uint8_t *data, uint32_t size, uint8_t check_align, uint8_t indent);

int load_NetCfg(const char* filename, NetCfg_t* cfg) {
    int ret=-1;
	
	FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

	cfg->cmdCount = 0;
    uint32_t cmdid;

	cfg->commands = calloc(0x400000, 1);
	if (!cfg->commands) {
		perror("malloc allocation failed");
		goto end;
	}
	
    while (fread(&cmdid, sizeof(uint32_t), 1, file) == 1) {
		cmdid = SWAP_LE(cmdid);
		
		/* doesn't work, size prob not good, I used huge malloc before the loop instead
		cfg->commands = realloc(cfg->commands, (cfg->cmdCount + 1) * sizeof(NetCommand));
        if (!cfg->commands) {
            perror("realloc allocation failed");
            goto end;
        }
		*/

        cfg->commands[cfg->cmdCount].cmdid = cmdid;
		switch (cmdid) {
			case 0x03: case 0x05: case 0x06: case 0x14: case 0x16:
			case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x22:
			case 0x23: case 0x25: case 0x2A: case 0x2B: case 0x2D:
			case 0x35: case 0x3E: case 0x40: case 0x41: case 0x44:
			case 0x45: case 0x46: case 0x47: case 0x49: case 0x4A:
			case 0x50:
			{
				break;
			}
			case 0x02: case 0x04: case 0x07: case 0x0D: case 0x0E: case 0x11:
			case 0x15: case 0x1F: case 0x21: case 0x28: case 0x2C:
			case 0x2E: case 0x2F: case 0x3D: case 0x3F: case 0x42:
			case 0x43: case 0x4D: 
			case 0x17: case 0x1C: case 0x1D: case 0x1E :
			{
                if (fread(&cfg->commands[cfg->cmdCount].oneU32.param, sizeof(uint32_t), 1, file) != 1) {
                    perror("Error reading parameter for cmdid 0x04");
                    goto end;
                }
				break;
			}
            case 0x01:
			{
                if (fread(&cfg->commands[cfg->cmdCount].cmd_01.offset, sizeof(uint32_t), 1, file) != 1 ||
                    fread(&cfg->commands[cfg->cmdCount].cmd_01.hackid, sizeof(uint32_t), 1, file) != 1) {
                    perror("Error reading cmd_01 data");
                    goto end;
                }
				break;
			}
            case 0x0F: case 0x10: case 0x26: case 0x27:
			case 0x29: case 0x48: case 0x4B: case 0x4C:
			{
                if (fread(&cfg->commands[cfg->cmdCount].twoU32.param1, sizeof(uint32_t), 1, file) != 1 ||
                    fread(&cfg->commands[cfg->cmdCount].twoU32.param2, sizeof(uint32_t), 1, file) != 1) {
                    perror("Error reading twoU32 parameters");
                    goto end;
                }
                break;
			}
            case 0x00:
			{
                if (fread(&cfg->commands[cfg->cmdCount].cmd_00.titleID, sizeof(char), 10, file) != 10) {
                    perror("Error reading titleID");
                    goto end;
                }
                break;
			}
            case 0x08:
			{
                if (fread(&cfg->commands[cfg->cmdCount].cmd_08.ReplaceDataMask, sizeof(uint64_t), 1, file) != 1 ||
                    fread(&cfg->commands[cfg->cmdCount].cmd_08.ReplaceData, sizeof(uint64_t), 1, file) != 1 ||
                    fread(&cfg->commands[cfg->cmdCount].cmd_08.OriginalDataMask, sizeof(uint64_t), 1, file) != 1 ||
                    fread(&cfg->commands[cfg->cmdCount].cmd_08.OriginalData, sizeof(uint64_t), 1, file) != 1) {
                    perror("Error reading cmd_08 data");
                    goto end;
                }
                break;
			}
            case 0x09:
			{
                uint32_t count;
                if (fread(&count, sizeof(uint32_t), 1, file) != 1) {
					perror("Error reading cmd_09 count");
                    goto end;
				}
				count = SWAP_LE(count);
				cfg->commands[cfg->cmdCount].cmd_09.count = count;
				
                for (uint32_t i = 0; i < count; i++) {
                    fread(&cfg->commands[cfg->cmdCount].cmd_09.data[i].offset, sizeof(uint32_t), 1, file);
                    fread(&cfg->commands[cfg->cmdCount].cmd_09.data[i].OriginalData, sizeof(uint8_t), 8, file);
                    fread(&cfg->commands[cfg->cmdCount].cmd_09.data[i].ReplaceData, sizeof(uint8_t), 8, file);
                }
                break;
			}
            case 0x0A:
			{
                uint32_t count;
                if(fread(&count, sizeof(uint32_t), 1, file) != 1) {
					perror("Error reading cmd_0A count");
                    goto end;
				}
				count = SWAP_LE(count);
				cfg->commands[cfg->cmdCount].cmd_0A.count = count;
				
                for (uint32_t i = 0; i < count; i++) {
                    if( fread(&cfg->commands[cfg->cmdCount].cmd_0A.data[i].offset, sizeof(uint32_t), 1, file) != 1 ||
						fread(cfg->commands[cfg->cmdCount].cmd_0A.data[i].OriginalData, sizeof(uint8_t), 4, file) != 4 ||
						fread(cfg->commands[cfg->cmdCount].cmd_0A.data[i].ReplaceData, sizeof(uint8_t), 4, file) != 4 ) {
						perror("Error reading cmd_0A data");
						goto end;
					}
                }
                break;
			}
            case 0x0B:
			{
				
                uint32_t count;
                if(fread(&count, sizeof(uint32_t), 1, file) != 1) {
					perror("Error reading cmd_0B count");
                    goto end;
				}
				count = SWAP_LE(count);
				cfg->commands[cfg->cmdCount].cmd_0B.count = count;

				for (uint32_t i = 0; i < count; i++) {
                    
                    if( fread(&cfg->commands[cfg->cmdCount].cmd_0B.data[i].sector, sizeof(uint32_t), 1, file) != 1 ||
						fread(&cfg->commands[cfg->cmdCount].cmd_0B.data[i].offset, sizeof(uint32_t), 1, file) != 1) {
						perror("Error reading cmd_0A offset/sector");
						goto end;
					}
					uint32_t size;
                    if( fread(&size, sizeof(uint32_t), 1, file) != 1) {
						perror("Error reading cmd_0A size");
						goto end;
					}
					size = SWAP_LE(size);

                    cfg->commands[cfg->cmdCount].cmd_0B.data[i].size = size;
					
					/* too much issue with it
					cfg->commands[cfg->cmdCount].cmd_0B.data[i].ReplaceData = malloc(size);
					cfg->commands[cfg->cmdCount].cmd_0B.data[i].OriginalData = malloc(size);
					if (!cfg->commands[cfg->cmdCount].cmd_0B.data[i].ReplaceData || !cfg->commands[cfg->cmdCount].cmd_0B.data[i].OriginalData) {
						perror("Failed to allocate memory for ReplaceData or OriginalData");
						goto end;
					}
					*/
                    if( fread(cfg->commands[cfg->cmdCount].cmd_0B.data[i].ReplaceData, sizeof(uint8_t), size, file) != size) {
						perror("Error reading cmd_0B ReplaceData or OriginalData");
						goto end;
					}	
						
					if( fread(cfg->commands[cfg->cmdCount].cmd_0B.data[i].OriginalData, sizeof(uint8_t), size, file) != size ) {
						perror("Error reading cmd_0B OriginalData");
						goto end;
					}
                }
                break;
			}
            case 0x0C:
			{
                if( fread(&cfg->commands[cfg->cmdCount].twoU16, sizeof(uint16_t), 2, file) != 2) {
					perror("Error reading twoU16");
                    goto end;
				}
                break;
			}
            case 0x12:
			{
                uint32_t count;
                if( fread(&count, sizeof(uint32_t), 1, file) != 1) {
					perror("Error reading oneArrayU32 count");
                    goto end;
				}
				count = SWAP_LE(count);
				cfg->commands[cfg->cmdCount].oneArrayU32.count = count;
                if( fread(cfg->commands[cfg->cmdCount].oneArrayU32.paramArray, sizeof(uint32_t), count, file) != count) {
					perror("Error reading oneArrayU32 data");
                    goto end;
				}
                break;
			}
            case 0x13: case 0x20: case 0x24:
			{
                fread(&cfg->commands[cfg->cmdCount].oneU64.param, sizeof(uint64_t), 1, file);
                break;
			}
            default:
			{
                fprintf(stderr, "Unknown cmdid: 0x%X | %s\n", cmdid, filename);
                break;
			}
        }
        cfg->cmdCount++;
    }
	ret=0;
end:
    fclose(file);
    return ret;
}

int save_NetCfg(const char* filename, const NetCfg_t* cfg) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Error opening file for writing");
        return -1;
    }
	
	for (uint32_t i = 0; i < cfg->cmdCount; i++) {
		
		fwrite(&cfg->commands[i].cmdid, sizeof(uint32_t), 1, file);

        switch (cfg->commands[i].cmdid) {
            case 0x03: case 0x05: case 0x06: case 0x14: case 0x16:
			case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x22:
			case 0x23: case 0x25: case 0x2A: case 0x2B: case 0x2D:
			case 0x35: case 0x3E: case 0x40: case 0x41: case 0x44:
			case 0x45: case 0x46: case 0x47: case 0x49: case 0x4A:
			case 0x50:
			{
				break;
			}
			case 0x02: case 0x04: case 0x07: case 0x0D: case 0x0E: case 0x11:
			case 0x15: case 0x1F: case 0x21: case 0x28: case 0x2C:
			case 0x2E: case 0x2F: case 0x3D: case 0x3F: case 0x42:
			case 0x43: case 0x4D:
			{
                fwrite(&cfg->commands[i].oneU32.param, sizeof(uint32_t), 1, file);
                break;
			}
            case 0x01:
			{
                fwrite(&cfg->commands[i].cmd_01.offset, sizeof(uint32_t), 1, file);
				fwrite(&cfg->commands[i].cmd_01.hackid, sizeof(uint32_t), 1, file);
                break;
			}
			case 0x0F: case 0x10: case 0x26: case 0x27:
			case 0x29: case 0x48: case 0x4B: case 0x4C:
			{
                fwrite(&cfg->commands[i].twoU32.param1, sizeof(uint32_t), 1, file);
				fwrite(&cfg->commands[i].twoU32.param2, sizeof(uint32_t), 1, file);
                break;
			}
            case 0x00:
			{
                fwrite(cfg->commands[i].cmd_00.titleID, sizeof(char), 10, file);
                break;
			}
            case 0x08:
			{
				fwrite(&cfg->commands[i].cmd_08.ReplaceDataMask, sizeof(uint64_t), 1, file);
                fwrite(&cfg->commands[i].cmd_08.ReplaceData, sizeof(uint64_t), 1, file);
                fwrite(&cfg->commands[i].cmd_08.OriginalDataMask, sizeof(uint64_t), 1, file);
                fwrite(&cfg->commands[i].cmd_08.OriginalData, sizeof(uint64_t), 1, file);
                break;
			}
            case 0x09:
			{
                fwrite(&cfg->commands[i].cmd_09.count, sizeof(uint32_t), 1, file);
                for (uint32_t j = 0; j < cfg->commands[i].cmd_09.count; j++) {
                    fwrite(&cfg->commands[i].cmd_09.data[j].offset, sizeof(uint32_t), 1, file);
                    fwrite(cfg->commands[i].cmd_09.data[j].OriginalData, sizeof(uint8_t), 8, file);
                    fwrite(cfg->commands[i].cmd_09.data[j].ReplaceData, sizeof(uint8_t), 8, file);
                }
                break;
			}
            case 0x0A:
			{
                fwrite(&cfg->commands[i].cmd_0A.count, sizeof(uint32_t), 1, file);
                for (uint32_t j = 0; j < cfg->commands[i].cmd_0A.count; j++) {
                    fwrite(&cfg->commands[i].cmd_0A.data[j].offset, sizeof(uint32_t), 1, file);
                    fwrite(cfg->commands[i].cmd_0A.data[j].OriginalData, sizeof(uint8_t), 4, file);
                    fwrite(cfg->commands[i].cmd_0A.data[j].ReplaceData, sizeof(uint8_t), 4, file);
                }
                break;
			}
            case 0x0B:
			{
                fwrite(&cfg->commands[i].cmd_0B.count, sizeof(uint32_t), 1, file);
                for (uint32_t j = 0; i < cfg->commands[i].cmd_0B.count; i++) {
                    fwrite(&cfg->commands[i].cmd_0B.data[j].sector, sizeof(uint32_t), 1, file);
                    fwrite(&cfg->commands[i].cmd_0B.data[j].offset, sizeof(uint32_t), 1, file);
                    fwrite(&cfg->commands[i].cmd_0B.data[j].size, sizeof(uint32_t), 1, file);
                    fwrite(cfg->commands[i].cmd_0B.data[j].ReplaceData, sizeof(uint8_t), cfg->commands[i].cmd_0B.data[i].size, file);
                    fwrite(cfg->commands[i].cmd_0B.data[j].OriginalData, sizeof(uint8_t), cfg->commands[i].cmd_0B.data[i].size, file);
                }
                break;
			}
            case 0x0C:
			{
                fwrite(&cfg->commands[i].twoU16.param1, sizeof(uint16_t), 1, file);
				fwrite(&cfg->commands[i].twoU16.param2, sizeof(uint16_t), 1, file);
                break;
			}
            case 0x12:
			{
                fwrite(&cfg->commands[i].oneArrayU32.count, sizeof(uint32_t), 1, file);
                fwrite(cfg->commands[i].oneArrayU32.paramArray, sizeof(uint32_t), cfg->commands[i].oneArrayU32.count, file);
                break;
			}
            case 0x13: case 0x20: case 0x24:
			{
                fwrite(&cfg->commands[i].oneU64.param, sizeof(uint64_t), 1, file);
                break;
			}
            default:
			{
                fprintf(stderr, "Unknown cmdid: 0x%X\n", cfg->commands[i].cmdid);
                break;
			}
        }
    }

    fclose(file);
    return 0;
}

int NetCfg_to_txt(FILE* file, const NetCfg_t* cfg) {
	
    if (!file || !cfg) {
        return -1;
    }
	
	for (uint32_t i = 0; i < cfg->cmdCount; i++) {
		fprintf(file, "\t#%d cmdid: 0x%08X\n", i, cfg->commands[i].cmdid);
		
		switch (cfg->commands[i].cmdid) {
			case 0x03: case 0x05: case 0x06: case 0x14: case 0x16:
			case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x22:
			case 0x23: case 0x25: case 0x2A: case 0x2B: case 0x2D:
			case 0x35: case 0x3E: case 0x40: case 0x41: case 0x44:
			case 0x45: case 0x46: case 0x47: case 0x49: case 0x4A:
			case 0x50:
			{
				break;
			}
			case 0x02: case 0x04: case 0x07: case 0x0D: case 0x0E: case 0x11:
			case 0x15: case 0x1F: case 0x21: case 0x28: case 0x2C:
			case 0x2E: case 0x2F: case 0x3D: case 0x3F: case 0x42:
			case 0x43: case 0x4D: 
			{
				fprintf(file, "\t\tparam: 0x%08X\n", cfg->commands[i].oneU32.param); 
				break;
			}
			case 0x0F: case 0x10: case 0x26: case 0x27:
			case 0x29: case 0x48: case 0x4B: case 0x4C:
			{
				fprintf(file, "\t\tparam1: 0x%08X\n", cfg->commands[i].twoU32.param1);
				fprintf(file, "\t\tparam2: 0x%08X\n", cfg->commands[i].twoU32.param2);
				break;
			}
			case 0x01:
			{
				fprintf(file, "\t\toffset: 0x%08X\n", cfg->commands[i].cmd_01.offset);
				fprintf(file, "\t\thackid: 0x%08X\n", cfg->commands[i].cmd_01.hackid);
				break;
			}
			case 0x00:
			{
				fprintf(file, "\t\ttitleID: %.10s\n", cfg->commands[i].cmd_00.titleID);
				break;
			}
			case 0x08:
			{
				fprintf(file, "\t\tReplaceDataMask: 0x%016llX\n", cfg->commands[i].cmd_08.ReplaceDataMask);
				fprintf(file, "\t\tReplaceData: 0x%016llX\n", cfg->commands[i].cmd_08.ReplaceData);
				fprintf(file, "\t\tOriginalDataMask: 0x%016llX\n", cfg->commands[i].cmd_08.OriginalDataMask);
				fprintf(file, "\t\tOriginalData: 0x%016llX\n", cfg->commands[i].cmd_08.OriginalData);
				break;
			}
			case 0x09:
			{
				fprintf(file, "\t\tcount: 0x%08X\n", cfg->commands[i].cmd_09.count);
				for (uint32_t j = 0; j < cfg->commands[i].cmd_09.count; j++) {
					fprintf(file, "\t\t\toffset: 0x%08X\n", cfg->commands[i].cmd_09.data[j].offset);
					
					fprintf(file, "\t\t\tOriginalData:\n");
					write_data(file, cfg->commands[i].cmd_09.data[j].OriginalData, 8, 0, 5);
				
					fprintf(file, "\t\t\tReplaceData:\n");
					write_data(file, cfg->commands[i].cmd_09.data[j].ReplaceData, 8, 0, 5);
				}
				break;
			}
			case 0x0A:
			{
				fprintf(file, "\t\tcount: 0x%08X\n", cfg->commands[i].cmd_0A.count);
				for (uint32_t j = 0; j < cfg->commands[i].cmd_0A.count; j++) {
					fprintf(file, "\t\t\toffset: 0x%08X\n", cfg->commands[i].cmd_0A.data[j].offset);
					
					fprintf(file, "\t\t\tOriginalData:\n");
					write_data(file, cfg->commands[i].cmd_0A.data[j].OriginalData, 4, 0, 5);
					
					fprintf(file, "\t\t\tReplaceData:\n");
					write_data(file, cfg->commands[i].cmd_0A.data[j].ReplaceData, 4, 0, 5);
				}
				break;
			}
			case 0x0B:
			{
				fprintf(file, "\t\tcount: 0x%08X\n", cfg->commands[i].cmd_0B.count);
				for (uint32_t j = 0; j < cfg->commands[i].cmd_0B.count; j++) {
					fprintf(file, "\t\t\tsector: 0x%08X\n", cfg->commands[i].cmd_0B.data[j].sector);
					fprintf(file, "\t\t\toffset: 0x%08X\n", cfg->commands[i].cmd_0B.data[j].offset);
					fprintf(file, "\t\t\tsize: 0x%08X\n", cfg->commands[i].cmd_0B.data[j].size);
					
					fprintf(file, "\t\t\tReplaceData:\n");
					write_data(file, cfg->commands[i].cmd_0B.data[j].ReplaceData, cfg->commands[i].cmd_0B.data[j].size, 0, 5);

					fprintf(file, "\t\t\tOriginalData:\n");
					write_data(file, cfg->commands[i].cmd_0B.data[j].OriginalData, cfg->commands[i].cmd_0B.data[j].size, 0, 5);
				}
				break;
			}
			case 0x0C:
			{
				fprintf(file, "\t\tparam1: 0x%04X\n", cfg->commands[i].twoU16.param1);
				fprintf(file, "\t\tparam2: 0x%04X\n", cfg->commands[i].twoU16.param2);
				break;
			}
			case 0x12:
			{
				fprintf(file, "\t\tcount: 0x%08X\n", cfg->commands[i].oneArrayU32.count);
				for (uint32_t j = 0; j < cfg->commands[i].oneArrayU32.count; j++) {
					fprintf(file, "\t\t\tparam[%u]: 0x%08X\n", j, cfg->commands[i].oneArrayU32.paramArray[j]);
				}
				break;
			}
			case 0x13: case 0x20: case 0x24:
			{
				fprintf(file, "\t\tparam: 0x%016llX\n", cfg->commands[i].oneU64.param);
				break;
			}
			default:
			{
				fprintf(file, "Unknown command ID: 0x%08X\n", cfg->commands[i].cmdid);
				break;
			}
		}
	}
	
    return 0;
}

void free_NetCfg(NetCfg_t* cfg) {
	
	/*
	for (uint32_t i = 0; i < cfg->cmdCount; i++) 
	{
		if(cfg->commands[i].cmdid == 0x0B ) {
			for (uint32_t j = 0; j < cfg->commands[i].cmd_0B.count; j++) 
			{
				FREE(cfg->commands[i].cmd_0B.data[j].ReplaceData);
				FREE(cfg->commands[i].cmd_0B.data[j].OriginalData);
			}
		}
	}
	*/
    FREE(cfg->commands);
}

int NetCfg_scandir_to_txt(char* dirpath, char* logpath) {
    DIR* dir = opendir(dirpath);
    if (!dir) {
        perror("Error opening directory");
        return -1;
    }

    FILE* logfile = fopen(logpath, "w");
    if (!logfile) {
        perror("Error opening log file");
        closedir(dir);
        return -1;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
		if(!strcmp(entry->d_name, "..") || !strcmp(entry->d_name, ".")) continue;
		
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);

        NetCfg_t cfg;
        if (load_NetCfg(filepath, &cfg) != 0) {
            fprintf(stderr, "Failed to load file: %s\n", filepath);
            continue;
        }
		
		fprintf(logfile, "\n=== File: %s ===\n", entry->d_name);
        if (NetCfg_to_txt(logfile, &cfg) != 0) {
            fprintf(stderr, "Failed to write data for file: %s\n", filepath);
        }

        free_NetCfg(&cfg);
    }

    fclose(logfile);
    closedir(dir);

    return 0;
}