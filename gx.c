#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "gx.h"
extern FILE *gxLog;
extern void write_data(FILE *file, uint8_t *data, uint32_t size, uint8_t check_align, uint8_t indent);

void es_GxHeader(GxHeader* header) {
    header->hashTitle = ENDIAN_SWAP(header->hashTitle);
    header->dataOffset = ENDIAN_SWAP(header->dataOffset);
    header->cmdCount = ENDIAN_SWAP(header->cmdCount);
}

void es_GxCommand(GxCommand* command) {
    command->cmdId = ENDIAN_SWAP(command->cmdId);
    command->align32 = ENDIAN_SWAP(command->align32);

    switch (command->cmdId) {
		case 0x00:
			command->data.cmd_type0.EEOffset = ENDIAN_SWAP(command->data.cmd_type0.EEOffset);
            command->data.cmd_type0.FuncIDOffset = ENDIAN_SWAP(command->data.cmd_type0.FuncIDOffset);
			break;
        case 0x08: case 0x09: case 0x10:
            command->data.cmd_type1.cmdDataOffset = ENDIAN_SWAP(command->data.cmd_type1.cmdDataOffset);
            command->data.cmd_type1.cmdDataCount = ENDIAN_SWAP(command->data.cmd_type1.cmdDataCount);
            break;
        case 0x01: case 0x03: case 0x06: case 0x0B: case 0x0C: case 0x0F:
        case 0x13: case 0x1C: case 0x1E: case 0x21: case 0x24: case 0x28: case 0x2A: case 0x2B:
            command->data.cmd_type2.param = ENDIAN_SWAP(command->data.cmd_type2.param);
            break;
        case 0x14: case 0x15: case 0x1A: case 0x1B:
            command->data.cmd_type3.param = ENDIAN_SWAP(command->data.cmd_type3.param);
            break;
        case 0x07: case 0x11: case 0x1D: case 0x20:
            command->data.cmd_type4.cmdDataOffset = ENDIAN_SWAP(command->data.cmd_type4.cmdDataOffset);
            break;
        case 0x0A:
            command->data.cmd_type5.param1 = ENDIAN_SWAP(command->data.cmd_type5.param1);
            command->data.cmd_type5.param2 = ENDIAN_SWAP(command->data.cmd_type5.param2);
            break;
        case 0x0D: case 0x0E: case 0x22: case 0x23: case 0x25:
            command->data.cmd_type6.param1 = ENDIAN_SWAP(command->data.cmd_type6.param1);
            command->data.cmd_type6.param2 = ENDIAN_SWAP(command->data.cmd_type6.param2);
            break;
        default:
            break;
    }
}

int load_GxCfg(const char* filename, GxCfg_t* cfg) {
	
	int ret = -1;
	
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file\n");
        return -1;
    }

    if( fread(&cfg->header, sizeof(GxHeader), 1, file) != 1) {
		perror("Error reading header\n");
		goto end;
	}
	
	es_GxHeader(&cfg->header);
	
    cfg->commands = (GxCommand*)malloc(cfg->header.cmdCount * sizeof(GxCommand));
    if (!cfg->commands) {
        perror("Memory allocation error\n");
        goto end;
    }

    for (uint32_t i = 0; i < cfg->header.cmdCount; i++) {
        if( fread(&cfg->commands[i].cmdId, sizeof(uint32_t), 1, file) != 1) {
			perror("Error reading cmdId\n");
			goto end;
		}
        if( fread(&cfg->commands[i].align32, sizeof(uint32_t), 1, file) != 1) {
			perror("Error reading align32\n");
			goto end;
		}
		
		int32_t cmdId = ENDIAN_SWAP(cfg->commands[i].cmdId);
		
        switch (cmdId) 
		{
			case 0x00:
				if( fread(&cfg->commands[i].data.cmd_type0.EEOffset, sizeof(int32_t), 1, file) != 1 ||
					fread(&cfg->commands[i].data.cmd_type0.align, sizeof(int32_t), 1, file) != 1 ||
					fread(&cfg->commands[i].data.cmd_type0.FuncIDOffset, sizeof(int64_t), 1, file) != 1 ) {
					perror("Error reading cmd_type0\n");
					goto end;
				}
				break;
            case 0x08: case 0x09: case 0x10:
                if( fread(&cfg->commands[i].data.cmd_type1.cmdDataOffset, sizeof(int64_t), 1, file) != 1) {
					perror("Error reading cmd_type1.cmdDataOffset\n");
					goto end;
				}
                if( fread(&cfg->commands[i].data.cmd_type1.cmdDataCount, sizeof(int32_t), 1, file) != 1) {
					perror("Error reading cmd_type1.cmdDataCount\n");
					goto end;
				}
                if( fread(cfg->commands[i].data.cmd_type1.align, sizeof(uint8_t), 4, file) != 4) {
					perror("Error reading cmd_type1.align\n");
					goto end;
				}
                break;
				
			case 0x01: case 0x03: case 0x06: case 0x0B: case 0x0C: case 0x0F:
            case 0x13: case 0x1C: case 0x1E: case 0x21: case 0x24: case 0x28: case 0x2A: case 0x2B:
                if( fread(&cfg->commands[i].data.cmd_type2.param, sizeof(int32_t), 1, file) != 1  ) {
					perror("Error reading cmd_type2.param\n");
					goto end;
				}
                if( fread(cfg->commands[i].data.cmd_type2.align, sizeof(uint8_t), 12, file) != 12 ) {
					perror("Error reading cmd_type2.align\n");
					return -1;
				}
                break;
            case 0x02: case 0x04: case 0x05: case 0x12: case 0x16: case 0x17: case 0x18: case 0x19:
            case 0x1F: case 0x26: case 0x27: case 0x29:
                if( fread(cfg->commands[i].data.align, sizeof(uint8_t), 16, file) != 16  ) {
					perror("Error reading data.align\n");
					goto end;
				}
                break;
            case 0x14: case 0x15: case 0x1A: case 0x1B:
                if( fread(&cfg->commands[i].data.cmd_type3.param, sizeof(int8_t), 1, file) != 1  ) {
					perror("Error reading cmd_type3.param\n");
					goto end;
				}
                if( fread(cfg->commands[i].data.cmd_type3.align, sizeof(uint8_t), 15, file) != 15  ) {
					perror("Error reading cmd_type3.align\n");
					goto end;
				}
                break;
            case 0x07: case 0x11: case 0x1D: case 0x20:
                if( fread(&cfg->commands[i].data.cmd_type4.cmdDataOffset, sizeof(int64_t), 1, file) != 1  ) {
					perror("Error reading cmd_type4.cmdDataOffset\n");
					goto end;
				}
                if( fread(cfg->commands[i].data.cmd_type4.align, sizeof(uint8_t), 8, file) != 8  ) {
					perror("Error reading cmd_type4.align\n");
					goto end;
				}
                break;
            case 0x0A:
                if( fread(&cfg->commands[i].data.cmd_type5.param1, sizeof(int16_t), 1, file) != 1  ) {
					perror("Error reading cmd_type5.param1\n");
					goto end;
				}
                if( fread(&cfg->commands[i].data.cmd_type5.param2, sizeof(int16_t), 1, file) != 1  ) {
					perror("Error reading cmd_type5.param2\n");
					goto end;
				}
                if( fread(cfg->commands[i].data.cmd_type5.align, sizeof(uint8_t), 12, file) != 12  ) {
					perror("Error reading cmd_type5.align\n");
					goto end;
				}
                break;
            case 0x0D: case 0x0E: case 0x22: case 0x23: case 0x25:
                if( fread(&cfg->commands[i].data.cmd_type6.param1, sizeof(int32_t), 1, file) != 1  ) {
					perror("Error reading md_type5.param1\n");
					goto end;
				}
                if( fread(&cfg->commands[i].data.cmd_type6.param2, sizeof(int32_t), 1, file) != 1  ) {
					perror("Error reading cmd_type6.param2\n");
					goto end;
				}
                if( fread(cfg->commands[i].data.cmd_type6.align, sizeof(uint8_t), 8, file) != 8  ) {
					perror("Error reading cmd_type6.align\n");
					goto end;
				}
                break;
            default:
				if( fread(cfg->commands[i].data.align, sizeof(uint8_t), 16, file) != 16  ) {
					perror("Error reading data.align\n");
					goto end;
				}
                break;
		}
		es_GxCommand(&cfg->commands[i]);
	}
	
	ret=0;
	
end:

    fclose(file);
    return ret;
}

int save_GxCfg(const char* filename, const GxCfg_t* cfg) {
	
	int ret=-1;
	
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Error opening file for writing\n");
        return ret;
    }

    // Write header
    if (fwrite(&cfg->header, sizeof(GxHeader), 1, file) != 1) {
        perror("Error writing header\n");
        goto end;
    }

    // Write commands
    for (uint32_t i = 0; i < cfg->header.cmdCount; i++) {
        if (fwrite(&cfg->commands[i].cmdId, sizeof(uint32_t), 1, file) != 1) {
            perror("Error writing cmdId\n");
            goto end;
        }
        if (fwrite(&cfg->commands[i].align32, sizeof(uint32_t), 1, file) != 1) {
            perror("Error writing align32\n");
            goto end;
        }

        switch (cfg->commands[i].cmdId) {
			case 0x00:
				if( fwrite(&cfg->commands[i].data.cmd_type0.EEOffset, sizeof(int32_t), 1, file) != 1 ||
					fwrite(&cfg->commands[i].data.cmd_type0.align, sizeof(int32_t), 1, file) != 1 ||
					fwrite(&cfg->commands[i].data.cmd_type0.FuncIDOffset, sizeof(int64_t), 1, file) != 1 ) {
					perror("Error writing cmd_type0\n");
					goto end;
				}
				break;
				
            case 0x08: case 0x09: case 0x10:
                if (fwrite(&cfg->commands[i].data.cmd_type1.cmdDataOffset, sizeof(int64_t), 1, file) != 1 ||
                    fwrite(&cfg->commands[i].data.cmd_type1.cmdDataCount, sizeof(int32_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].data.cmd_type1.align, sizeof(uint8_t), 4, file) != 4) {
                    perror("Error writing cmd_type1 data\n");
                    goto end;
                }
                break;
            case 0x01: case 0x03: case 0x06: case 0x0B: case 0x0C: case 0x0F:
            case 0x13: case 0x1C: case 0x1E: case 0x21: case 0x24: case 0x28: case 0x2A: case 0x2B:
                if (fwrite(&cfg->commands[i].data.cmd_type2.param, sizeof(int32_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].data.cmd_type2.align, sizeof(uint8_t), 12, file) != 12) {
                    perror("Error writing cmd_type2 data\n");
                    goto end;
                }
                break;
            case 0x02: case 0x04: case 0x05: case 0x12: case 0x16: case 0x17: case 0x18: case 0x19:
            case 0x1F: case 0x26: case 0x27: case 0x29:
                if (fwrite(cfg->commands[i].data.align, sizeof(uint8_t), 16, file) != 16) {
                    perror("Error writing data.align\n");
                    goto end;
                }
                break;
            case 0x14: case 0x15: case 0x1A: case 0x1B:
                if (fwrite(&cfg->commands[i].data.cmd_type3.param, sizeof(int8_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].data.cmd_type3.align, sizeof(uint8_t), 15, file) != 15) {
                    perror("Error writing cmd_type3 data\n");
                    goto end;
                }
                break;
            case 0x07: case 0x11: case 0x1D: case 0x20:
                if (fwrite(&cfg->commands[i].data.cmd_type4.cmdDataOffset, sizeof(int64_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].data.cmd_type4.align, sizeof(uint8_t), 8, file) != 8) {
                    perror("Error writing cmd_type4 data\n");
                    goto end;
                }
                break;
            case 0x0A:
                if (fwrite(&cfg->commands[i].data.cmd_type5.param1, sizeof(int16_t), 1, file) != 1 ||
                    fwrite(&cfg->commands[i].data.cmd_type5.param2, sizeof(int16_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].data.cmd_type5.align, sizeof(uint8_t), 12, file) != 12) {
                    perror("Error writing cmd_type5 data\n");
                    goto end;
                }
                break;
            case 0x0D: case 0x0E: case 0x22: case 0x23: case 0x25:
                if (fwrite(&cfg->commands[i].data.cmd_type6.param1, sizeof(int32_t), 1, file) != 1 ||
                    fwrite(&cfg->commands[i].data.cmd_type6.param2, sizeof(int32_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].data.cmd_type6.align, sizeof(uint8_t), 8, file) != 8) {
                    perror("Error writing cmd_type6 data\n");
                    goto end;
                }
                break;
            default:
                if (fwrite(cfg->commands[i].data.align, sizeof(uint8_t), 16, file) != 16) {
                    perror("Error writing default data.align\n");
                    goto end;
                }
                break;
        }
    }
	
	ret=0;
	
end:
    fclose(file);
    return ret;
}

void print_align(FILE *file, uint8_t * data, uint32_t size) {
	if( !show_align ) return;
	
	fprintf(file, "\t\talign : \n");
	
	write_data(file, data, size, 1, 3);
}

int GxCfg_to_txt(FILE* file, const GxCfg_t* cfg) {

    // Write header information
    fprintf(file, "GxHeader:\n");
    fprintf(file, "\thashTitle: 0x%016llX\n", cfg->header.hashTitle);
    fprintf(file, "\tdataOffset: 0x%08lX\n", cfg->header.dataOffset);
    fprintf(file, "\tcmdCount: 0x%08lX\n", cfg->header.cmdCount);

    // Write commands
    for (uint32_t i = 0; i < cfg->header.cmdCount; i++) {
        fprintf(file, "\t#%d cmdId: 0x%08lX\n", i + 1, cfg->commands[i].cmdId);
		print_align(file, (uint8_t *) &cfg->commands[i].align32, sizeof(cfg->commands[i].align32));

        switch (cfg->commands[i].cmdId) {
			case 0x00:
				fprintf(file, "\t\tEEOffset : 0x%08llX\n", cfg->commands[i].data.cmd_type0.EEOffset);
                print_align(file, cfg->commands[i].data.cmd_type0.align, sizeof(cfg->commands[i].data.cmd_type0.align));
				fprintf(file, "\t\tFuncIDOffset  : 0x%016llX\n", cfg->commands[i].data.cmd_type0.FuncIDOffset);
				break;
            case 0x08: case 0x09: case 0x10:
                fprintf(file, "\t\tcmdDataOffset : 0x%016llX\n", cfg->commands[i].data.cmd_type1.cmdDataOffset);
                fprintf(file, "\t\tcmdDataCount  : 0x%08lX\n", cfg->commands[i].data.cmd_type1.cmdDataCount);
				print_align(file, cfg->commands[i].data.cmd_type1.align, sizeof(cfg->commands[i].data.cmd_type1.align));
                break;

            case 0x01: case 0x03: case 0x06: case 0x0B: case 0x0C: case 0x0F:
            case 0x13: case 0x1C: case 0x1E: case 0x21: case 0x24: case 0x28: case 0x2A: case 0x2B:
                fprintf(file, "\t\tparam : 0x%08lX\n", cfg->commands[i].data.cmd_type2.param);
				print_align(file, cfg->commands[i].data.cmd_type2.align, sizeof(cfg->commands[i].data.cmd_type2.align));
                break;

            case 0x14: case 0x15: case 0x1A: case 0x1B:
                fprintf(file, "\t\tparam : 0x02%X\n", cfg->commands[i].data.cmd_type3.param);
				print_align(file, cfg->commands[i].data.cmd_type3.align, sizeof(cfg->commands[i].data.cmd_type3.align));
                break;
            case 0x07: case 0x11: case 0x1D: case 0x20:
                fprintf(file, "\t\tcmdDataOffset : 0x%016llX\n", cfg->commands[i].data.cmd_type4.cmdDataOffset);
				print_align(file, cfg->commands[i].data.cmd_type4.align, sizeof(cfg->commands[i].data.cmd_type4.align));
                break;
            case 0x0A:
                fprintf(file, "\t\tparam1 : 0x%04X\n", cfg->commands[i].data.cmd_type5.param1);
                fprintf(file, "\t\tparam2 : 0x%04X\n", cfg->commands[i].data.cmd_type5.param2);
				print_align(file, cfg->commands[i].data.cmd_type5.align, sizeof(cfg->commands[i].data.cmd_type5.align));
                break;
            case 0x0D: case 0x0E: case 0x22: case 0x23: case 0x25:
                fprintf(file, "\t\tparam1 : 0x%08lX\n", cfg->commands[i].data.cmd_type6.param1);
                fprintf(file, "\t\tparam2 : 0x%08lX\n", cfg->commands[i].data.cmd_type6.param2);
				print_align(file, cfg->commands[i].data.cmd_type6.align, sizeof(cfg->commands[i].data.cmd_type6.align));
                break;
			case 0x02: case 0x04: case 0x05: case 0x12: case 0x16: case 0x17: case 0x18: case 0x19:
            case 0x1F: case 0x26: case 0x27: case 0x29:
				fprintf(file, "\t\tNone\n");
				print_align(file, cfg->commands[i].data.align, sizeof(cfg->commands[i].data.align));
				break;
            default:
                fprintf(file, "\t\tUnknown command id 0x%X\n", cfg->commands[i].cmdId);
				print_align(file, cfg->commands[i].data.align, sizeof(cfg->commands[i].data.align));
				
                break;
        }
    }

    return 0;
}

void free_GxCfg(GxCfg_t* cfg) {
    FREE(cfg->commands);
}

int GxCfg_scandir_to_txt(char* dirpath, char* logpath) {
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

        GxCfg_t cfg;
        if (load_GxCfg(filepath, &cfg) != 0) {
            fprintf(stderr, "Failed to load file: %s\n", filepath);
            continue;
        }

		fprintf(logfile, "\n=== File: %s ===\n", entry->d_name);
        if (GxCfg_to_txt(logfile, &cfg) != 0) {
            fprintf(stderr, "Failed to write data for file: %s\n", filepath);
        }

        free_GxCfg(&cfg);
    }

    fclose(logfile);
    closedir(dir);

    return 0;
}

int gxcfg_log(char *filename, char *logpath)
{
    if (!gxLog) {
        gxLog = fopen("gxlog.txt", "w");
        if (!gxLog) {
            perror("Error opening log file");
            return -1;
        }
    }
    GxCfg_t cfg;
    if (load_GxCfg(filename, &cfg) != 0) {
        fprintf(stderr, "Failed to load file: %s\n", filename);
        return -1;
    }
    
    fprintf(gxLog, "\n=== File: %s ===\n", filename);
    if (GxCfg_to_txt(gxLog, &cfg) != 0) {
        fprintf(stderr, "Failed to write data for file: %s\n", filename);
    }

    free_GxCfg(&cfg);
    
    return 0;
}