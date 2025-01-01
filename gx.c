#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "gx.h"
#include "util.h"

extern FILE *gxLog;
extern int verbose;

void es_GxHeader(GxHeader* header) {
    ES_BE(header->hashTitle);
    ES_BE(header->dataOffset);
    ES_BE(header->cmdCount);
}

void es_GxCommand(GxCommand* command) {

    ES_BE(command->cmdId);
    ES_BE(command->align32);
    
    //allow swap and reswap
	uint32_t cmdid = command->cmdId;
	if(cmdid > 0x1000) {
		ES_BE(cmdid);
		if(cmdid > 0x1000) {	
			fprintf(stderr, "cmdid too big\n");
			return;
		}
	}

    switch (cmdid) {
		case 0x00:
        {
			ES_BE(command->cmd_00.EEOffset);
            ES_BE(command->cmd_00.FuncIDOffset);
			break;
        }
        case 0x10:
        {
            ES_BE(command->cmd_type1.DataOffset);
            ES_BE(command->cmd_type1.DataCount);
            break;
        }
        case 0x01: case 0x03: case 0x06: case 0x0B: case 0x0C: case 0x0F:
        case 0x13: case 0x1C: case 0x1E: case 0x21: case 0x24: case 0x28: case 0x2A: case 0x2B:
        {
            ES_BE(command->oneU32.param);
            break;
        }
        case 0x07:
        { 
            ES_BE(command->cmd_07.DataOffset);
            ES_BE_ARRAY(command->cmd_07.ReplaceDataMask);
            ES_BE_ARRAY(command->cmd_07.ReplaceData);
            ES_BE_ARRAY(command->cmd_07.OriginalDataMask);
            ES_BE_ARRAY(command->cmd_07.OriginalData);
            break;
        }
        case 0x08:
        {
            ES_BE(command->cmd_08.DataOffset);
            ES_BE(command->cmd_08.DataCount);
            uint32_t count = command->cmd_08.DataCount;
			if( count > 0x1000 ) {
				ES_LE(count);
				if(count > 0x1000) {
					fprintf(stderr, "cmd_08 count too big\n");
					return;
				}
			}
            for (int i = 0; i < count; i++) {
                ES_BE(command->cmd_08.data[i].offset);
                ES_BE_ARRAY(command->cmd_08.data[i].OriginalData);
                ES_BE_ARRAY(command->cmd_08.data[i].ReplaceData);
            }
            break;
        }
        case 0x09:
        {
            ES_BE(command->cmd_09.DataOffset);
            ES_BE(command->cmd_09.DataCount);
            uint32_t count = command->cmd_09.DataCount;
            if( count > 0x1000 ) {
                ES_LE(count);
                if(count > 0x1000) {
                    fprintf(stderr, "cmd_09 count too big\n");
                    return;
                }
            }
            for (int i = 0; i < count; i++) {
                ES_BE(command->cmd_09.data[i].sector);
                ES_BE(command->cmd_09.data[i].offset);
                ES_BE(command->cmd_09.data[i].size);
                ES_BE(command->cmd_09.data[i].ReplaceDataOffset);
                ES_BE(command->cmd_09.data[i].OriginalDataOffset);
            }
            break;
        }
        case 0x11: case 0x1D: case 0x20:
        {
            ES_BE(command->cmd_type4.DataOffset);
            break;
        }
        case 0x0A:
        {
            ES_BE_ARRAY(command->twoU16.param);
            break;
        }
        case 0x0D: case 0x0E: case 0x22: case 0x23: case 0x25:
        {
            ES_BE_ARRAY(command->twoU32.param);
            break;
        }
        default:
        {
            break;
        }
    }
}

void print_align(FILE *file, uint8_t * data, uint32_t size) {
	if( !show_align ) return;
	
	fprintf(file, "\t\talign : \n");
	
	write_data(file, data, size, 1, 3);
}

int load_GxCfg(const char* filename, GxCfg_t* cfg) {
	
	int ret = -1;
	
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file\n");
        return -1;
    }
    
    memset(cfg, 0, sizeof(GxCfg_t));
    if( fread(&cfg->header, sizeof(GxHeader), 1, file) != 1) {
		perror("Error reading header\n");
		goto end;
	}
	
	es_GxHeader(&cfg->header);
	
    cfg->commands = calloc(0x400000, 1);
	if (!cfg->commands) {
		perror("malloc allocation failed");
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
		
		int32_t cmdId = SWAP_BE(cfg->commands[i].cmdId);
        switch (cmdId) 
		{
			case 0x00:
            {
				if( fread(&cfg->commands[i].cmd_00.EEOffset, sizeof(int32_t), 1, file) != 1 ||
					fread(&cfg->commands[i].cmd_00.align, sizeof(int32_t), 1, file) != 1 ||
					fread(&cfg->commands[i].cmd_00.FuncIDOffset, sizeof(int64_t), 1, file) != 1 ) {
					perror("Error reading cmd_00\n");
					goto end;
				}
				break;
            }
            case 0x07:
            {
                if( fread(&cfg->commands[i].cmd_07.DataOffset, sizeof(int64_t), 1, file) != 1  ) {
					perror("Error reading cmd_type4.DataOffset\n");
					goto end;
				}
                if( fread(cfg->commands[i].cmd_07.align, sizeof(uint8_t), 8, file) != 8  ) {
					perror("Error reading cmd_type4.align\n");
					goto end;
				}
                uint32_t current = ftell(file);
                uint32_t offset = SWAP_BE(cfg->commands[i].cmd_07.DataOffset) - cfg->header.dataOffset + sizeof(GxHeader);
                
                struct stat st;
                if (stat(filename, &st) != 0) {
                    perror("Failed to get file size");
                    goto end;
                }
                size_t file_size = st.st_size;
                if( offset > file_size ) {
                    fprintf(stderr, "DataOffset is bigger than file size, data isn't in the config file\n");
                    goto end;
                }

                fseek(file, offset, SEEK_SET);
               
                if( fread(&cfg->commands[i].cmd_07.ReplaceDataMask, sizeof(uint32_t), 2, file) != 2 ||
                    fread(&cfg->commands[i].cmd_07.ReplaceData, sizeof(uint32_t), 2, file) != 2 ||
                    fread(&cfg->commands[i].cmd_07.OriginalDataMask, sizeof(uint32_t), 2, file) != 2 ||
                    fread(&cfg->commands[i].cmd_07.OriginalData, sizeof(uint32_t), 2, file) != 2) {
                    perror("Error reading cmd_07 data\n");
                    goto end;
                }
                fseek(file, current, SEEK_SET);
                break;
            }
            case 0x08:
            {
                if( fread(&cfg->commands[i].cmd_08.DataOffset, sizeof(int64_t), 1, file) != 1) {
                    perror("Error reading cmd_08.DataOffset\n");
                    goto end;
                }
                if( fread(&cfg->commands[i].cmd_08.DataCount, sizeof(int32_t), 1, file) != 1) {
                    perror("Error reading cmd_08.DataCount\n");
                    goto end;
                }
                if( fread(cfg->commands[i].cmd_08.align, sizeof(uint8_t), 4, file) != 4) {
                    perror("Error reading cmd_08.align\n");
                    goto end;
                }
                uint32_t current = ftell(file);
                uint32_t offset = SWAP_BE(cfg->commands[i].cmd_08.DataOffset) - cfg->header.dataOffset + sizeof(GxHeader);
                
                struct stat st;
                if (stat(filename, &st) != 0) {
                    perror("Failed to get file size");
                    goto end;
                }
                size_t file_size = st.st_size;
                if( offset > file_size ) {
                    fprintf(stderr, "DataOffset is bigger than file size, data isn't in the config file\n");
                    goto end;
                }
                
                fseek(file, offset, SEEK_SET);
                uint32_t count = SWAP_BE(cfg->commands[i].cmd_08.DataCount);
                for(int j = 0; j < count; j++) {
                    if( fread(&cfg->commands[i].cmd_08.data[j].offset, sizeof(uint32_t), 1, file) != 1) {
                        perror("Error reading cmd_08.data[j].offset\n");
                        goto end;
                    }
                    if( fread(&cfg->commands[i].cmd_08.data[j].align, sizeof(uint8_t), 4, file) != 4) {
                        perror("Error reading cmd_08.data[j].align\n");
                        goto end;
                    }
                    if( fread(&cfg->commands[i].cmd_08.data[j].OriginalData, sizeof(uint32_t), 2, file) != 2) {
                        perror("Error reading cmd_08.data[j].OriginalData\n");
                        goto end;
                    }
                    if( fread(&cfg->commands[i].cmd_08.data[j].ReplaceData, sizeof(uint32_t), 2, file) != 2) {
                        perror("Error reading cmd_08.data[j].ReplaceData\n");
                        goto end;
                    }
                }
                fseek(file, current, SEEK_SET);
                break;
            }
            case 0x09:
            {
                if( fread(&cfg->commands[i].cmd_09.DataOffset, sizeof(uint64_t), 1, file) != 1) {
                    perror("Error reading cmd_09.DataOffset\n");
                    goto end;
                }
                if( fread(&cfg->commands[i].cmd_09.DataCount, sizeof(uint32_t), 1, file) != 1) {
                    perror("Error reading cmd_09.DataCount\n");
                    goto end;
                }
                if( fread(cfg->commands[i].cmd_09.align, sizeof(uint8_t), 4, file) != 4) {
                    perror("Error reading cmd_09.align\n");
                    goto end;
                }
                uint32_t current = ftell(file);
                uint32_t offset = SWAP_BE(cfg->commands[i].cmd_09.DataOffset) - cfg->header.dataOffset + sizeof(GxHeader);
                
                struct stat st;
                if (stat(filename, &st) != 0) {
                    perror("Failed to get file size");
                    goto end;
                }
                size_t file_size = st.st_size;
                if( offset > file_size ) {
                    fprintf(stderr, "DataOffset is bigger than file size, data isn't in the config file\n");
                    goto end;
                }
                
                fseek(file, offset, SEEK_SET);
               
                uint32_t count = SWAP_BE(cfg->commands[i].cmd_09.DataCount);
                for(int j = 0; j < count; j++) {
                    if( fread(&cfg->commands[i].cmd_09.data[j].sector, sizeof(uint32_t), 1, file) != 1 ||
                        fread(&cfg->commands[i].cmd_09.data[j].offset, sizeof(uint32_t), 1, file) != 1 ||
                        fread(&cfg->commands[i].cmd_09.data[j].ReplaceDataOffset, sizeof(uint64_t), 1, file) != 1 ||
                        fread(&cfg->commands[i].cmd_09.data[j].OriginalDataOffset, sizeof(uint64_t), 1, file) != 1 ||
                        fread(&cfg->commands[i].cmd_09.data[j].size, sizeof(uint32_t), 1, file) != 1 ||
                        fread(&cfg->commands[i].cmd_09.data[j].align, sizeof(uint8_t), 4, file) != 4 ) {
                        fprintf(stderr, "Error reading cmd_09.data[%d]\n", j);
                        goto end;
                    }

                    uint32_t pos = ftell(file);
                    uint32_t DataSize = SWAP_BE(cfg->commands[i].cmd_09.data[j].size);
                    uint32_t RepDataOff = SWAP_BE(cfg->commands[i].cmd_09.data[j].ReplaceDataOffset) - cfg->header.dataOffset + sizeof(GxHeader);
                    uint32_t OrgDataOff = SWAP_BE(cfg->commands[i].cmd_09.data[j].OriginalDataOffset) - cfg->header.dataOffset + sizeof(GxHeader);
                    
                    fseek(file, RepDataOff, SEEK_SET);
                    if( fread(&cfg->commands[i].cmd_09.data[j].ReplaceData, sizeof(uint8_t), DataSize , file) != DataSize) {
                        perror("Error reading cmd_09.data[j].ReplaceData\n");
                        goto end;
                    }
                    fseek(file, OrgDataOff, SEEK_SET);
                    if( fread(&cfg->commands[i].cmd_09.data[j].OriginalData, sizeof(uint8_t), DataSize, file) != DataSize) {
                        perror("Error reading cmd_09.data[j].OriginalData\n");
                        goto end;
                    }
                    fseek(file, pos, SEEK_SET);
                }
                fseek(file, current, SEEK_SET);
                break;
            }
            case 0x10:
            {
                if( fread(&cfg->commands[i].cmd_type1.DataOffset, sizeof(int64_t), 1, file) != 1) {
					perror("Error reading cmd_type1.DataOffset\n");
					goto end;
				}
                if( fread(&cfg->commands[i].cmd_type1.DataCount, sizeof(int32_t), 1, file) != 1) {
					perror("Error reading cmd_type1.DataCount\n");
					goto end;
				}
                if( fread(cfg->commands[i].cmd_type1.align, sizeof(uint8_t), 4, file) != 4) {
					perror("Error reading cmd_type1.align\n");
					goto end;
				}
                break;
            }
			case 0x01: case 0x03: case 0x06: case 0x0B: case 0x0C: case 0x0F:
            case 0x13: case 0x1C: case 0x1E: case 0x21: case 0x24: case 0x28: case 0x2A: case 0x2B:
                if( fread(&cfg->commands[i].oneU32.param, sizeof(int32_t), 1, file) != 1  ) {
					perror("Error reading oneU32.param\n");
					goto end;
				}
                if( fread(cfg->commands[i].oneU32.align, sizeof(uint8_t), 12, file) != 12 ) {
					perror("Error reading oneU32.align\n");
					return -1;
				}
                break;
            case 0x02: case 0x04: case 0x05: case 0x12: case 0x16: case 0x17: case 0x18: case 0x19:
            case 0x1F: case 0x26: case 0x27: case 0x29:
                if( fread(cfg->commands[i].align, sizeof(uint8_t), 16, file) != 16  ) {
					perror("Error reading align\n");
					goto end;
				}
                break;
            case 0x14: case 0x15: case 0x1A: case 0x1B:
                if( fread(&cfg->commands[i].oneU8.param, sizeof(int8_t), 1, file) != 1  ) {
					perror("Error reading oneU8.param\n");
					goto end;
				}
                if( fread(cfg->commands[i].oneU8.align, sizeof(uint8_t), 15, file) != 15  ) {
					perror("Error reading oneU8.align\n");
					goto end;
				}
                break;
            case 0x11: case 0x1D: case 0x20:
                if( fread(&cfg->commands[i].cmd_type4.DataOffset, sizeof(int64_t), 1, file) != 1  ) {
					perror("Error reading cmd_type4.DataOffset\n");
					goto end;
				}
                if( fread(cfg->commands[i].cmd_type4.align, sizeof(uint8_t), 8, file) != 8  ) {
					perror("Error reading cmd_type4.align\n");
					goto end;
				}
                break;
            case 0x0A:
                if( fread(&cfg->commands[i].twoU16.param, sizeof(int16_t), 2, file) != 2  ) {
					perror("Error reading twoU16.param1\n");
					goto end;
				}
                if( fread(cfg->commands[i].twoU16.align, sizeof(uint8_t), 12, file) != 12  ) {
					perror("Error reading twoU16.align\n");
					goto end;
				}
                break;
            case 0x0D: case 0x0E: case 0x22: case 0x23: case 0x25:
                if( fread(&cfg->commands[i].twoU32.param, sizeof(uint32_t), 2, file) != 2  ) {
					perror("Error reading md_type5.param1\n");
					goto end;
				}
                if( fread(cfg->commands[i].twoU32.align, sizeof(uint8_t), 8, file) != 8  ) {
					perror("Error reading twoU32.align\n");
					goto end;
				}
                break;
            default:
				if( fread(cfg->commands[i].align, sizeof(uint8_t), 16, file) != 16  ) {
					perror("Error reading align\n");
					goto end;
				}
                break;
		}
		es_GxCommand(&cfg->commands[i]);
	}
	ret=0;
end:
    FCLOSE(file);
    return ret;
}

int save_GxCfg(const char* filename, GxCfg_t* cfg) {
	
	int ret=-1;
    
    uint32_t cmdCount = cfg->header.cmdCount;
    if( cmdCount == 0 ) {
        return 0;
    }

    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Error opening file for writing\n");
        return ret;
    }

    // Write header
    es_GxHeader(&cfg->header);
    if (fwrite(&cfg->header, sizeof(GxHeader), 1, file) != 1) {
        perror("Error writing header\n");
        goto end;
    }

    // Write commands
    for (uint32_t i = 0; i < cmdCount; i++) {

        uint32_t cmdId = cfg->commands[i].cmdId;

        es_GxCommand(&cfg->commands[i]);
        if (fwrite(&cfg->commands[i].cmdId, sizeof(uint32_t), 1, file) != 1) {
            perror("Error writing cmdId\n");
            goto end;
        }
        if (fwrite(&cfg->commands[i].align32, sizeof(uint32_t), 1, file) != 1) {
            perror("Error writing align32\n");
            goto end;
        }

        switch (cmdId) {
			case 0x00:
            {
				if( fwrite(&cfg->commands[i].cmd_00.EEOffset, sizeof(int32_t), 1, file) != 1 ||
					fwrite(&cfg->commands[i].cmd_00.align, sizeof(int32_t), 1, file) != 1 ||
					fwrite(&cfg->commands[i].cmd_00.FuncIDOffset, sizeof(int64_t), 1, file) != 1 ) {
					perror("Error writing cmd_00\n");
					goto end;
				}
				break;
            }
            case 0x07: 
            {
                if (fwrite(&cfg->commands[i].cmd_07.DataOffset, sizeof(int64_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].cmd_07.align, sizeof(uint8_t), 8, file) != 8) {
                    perror("Error writing cmd_07 data\n");
                    goto end;
                }
                uint32_t current = ftell(file);
                uint32_t offset = SWAP_BE(cfg->commands[i].cmd_07.DataOffset) - cfg->header.dataOffset + sizeof(GxHeader);
                fseek(file, offset, SEEK_SET);
                if (fwrite(&cfg->commands[i].cmd_07.ReplaceDataMask, sizeof(uint32_t), 2, file) != 2 ||
                    fwrite(&cfg->commands[i].cmd_07.ReplaceData, sizeof(uint32_t), 2, file) != 2 ||
                    fwrite(&cfg->commands[i].cmd_07.OriginalDataMask, sizeof(uint32_t), 2, file) != 2 ||
                    fwrite(&cfg->commands[i].cmd_07.OriginalData, sizeof(uint32_t), 2, file) != 2) {
                    perror("Error writing cmd_07 data\n");
                    goto end;
                }
                fseek(file, current, SEEK_SET);
                break;
            }
            case 0x08:
            {
                if (fwrite(&cfg->commands[i].cmd_08.DataOffset, sizeof(int64_t), 1, file) != 1 ||
                    fwrite(&cfg->commands[i].cmd_08.DataCount, sizeof(int32_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].cmd_08.align, sizeof(uint8_t), 4, file) != 4) {
                    perror("Error writing cmd_08 data\n");
                    goto end;
                }
                uint32_t offset = SWAP_BE(cfg->commands[i].cmd_08.DataOffset) - cfg->header.dataOffset + sizeof(GxHeader);
                uint32_t current = ftell(file);
                fseek(file, offset, SEEK_SET);

                uint32_t count = SWAP_BE(cfg->commands[i].cmd_08.DataCount);
                for (int j = 0; j < count; j++) {
                    if (fwrite(&cfg->commands[i].cmd_08.data[j].offset, sizeof(uint32_t), 1, file) != 1 ||
                        fwrite(cfg->commands[i].cmd_08.data[j].OriginalData, sizeof(uint32_t), 2, file) != 2 ||
                        fwrite(cfg->commands[i].cmd_08.data[j].ReplaceData, sizeof(uint32_t), 2, file) != 2) {
                        perror("Error writing cmd_08 data\n");
                        goto end;
                    }
                }
                fseek(file, current, SEEK_SET);
                break;
            }
            case 0x09:
            {
                if (fwrite(&cfg->commands[i].cmd_09.DataOffset, sizeof(uint64_t), 1, file) != 1 ||
                    fwrite(&cfg->commands[i].cmd_09.DataCount, sizeof(uint32_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].cmd_09.align, sizeof(uint8_t), 4, file) != 4) {
                    perror("Error writing cmd_09 data\n");
                    goto end;
                }
                uint32_t offset = SWAP_BE(cfg->commands[i].cmd_09.DataOffset) - cfg->header.dataOffset + sizeof(GxHeader);
                uint32_t current = ftell(file);
                fseek(file, offset, SEEK_SET);
                uint32_t count = SWAP_BE(cfg->commands[i].cmd_09.DataCount);
                for (int j = 0; j < count; j++) {
                    if (fwrite(&cfg->commands[i].cmd_09.data[j].sector, sizeof(uint32_t), 1, file) != 1 ||
                        fwrite(&cfg->commands[i].cmd_09.data[j].offset, sizeof(uint32_t), 1, file) != 1 ||
                        fwrite(&cfg->commands[i].cmd_09.data[j].size, sizeof(uint32_t), 1, file) != 1 ||
                        fwrite(&cfg->commands[i].cmd_09.data[j].ReplaceDataOffset, sizeof(uint64_t), 1, file) != 1 ||
                        fwrite(&cfg->commands[i].cmd_09.data[j].OriginalDataOffset, sizeof(uint64_t), 1, file) != 1 ||
                        fwrite(cfg->commands[i].cmd_09.data[j].align, sizeof(uint8_t), 4, file) != 4) {
                        perror("Error writing cmd_09 data\n");
                        goto end;
                    }
                    uint32_t pos = ftell(file);
                    uint32_t DataSize = SWAP_BE(cfg->commands[i].cmd_09.data[j].size);
                    uint32_t RepDataOff = SWAP_BE(cfg->commands[i].cmd_09.data[j].ReplaceDataOffset) - cfg->header.dataOffset + sizeof(GxHeader);
                    uint32_t OrgDataOff = SWAP_BE(cfg->commands[i].cmd_09.data[j].OriginalDataOffset) - cfg->header.dataOffset + sizeof(GxHeader);
                    fseek(file, RepDataOff, SEEK_SET);
                    if (fwrite(cfg->commands[i].cmd_09.data[j].ReplaceData, sizeof(uint8_t), DataSize, file) != DataSize) {
                        perror("Error writing cmd_09.data[j].ReplaceData\n");
                        goto end;
                    }
                    fseek(file, OrgDataOff, SEEK_SET);
                    if (fwrite(cfg->commands[i].cmd_09.data[j].OriginalData, sizeof(uint8_t), DataSize, file) != DataSize) {
                        perror("Error writing cmd_09.data[j].OriginalData\n");
                        goto end;
                    }
                    fseek(file, pos, SEEK_SET);
                }
                fseek(file, current, SEEK_SET);
                break;
            }
            case 0x10:
            {
                if (fwrite(&cfg->commands[i].cmd_type1.DataOffset, sizeof(int64_t), 1, file) != 1 ||
                    fwrite(&cfg->commands[i].cmd_type1.DataCount, sizeof(int32_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].cmd_type1.align, sizeof(uint8_t), 4, file) != 4) {
                    perror("Error writing cmd_type1 data\n");
                    goto end;
                }
                break;
            }
            case 0x01: case 0x03: case 0x06: case 0x0B: case 0x0C: case 0x0F:
            case 0x13: case 0x1C: case 0x1E: case 0x21: case 0x24: case 0x28: case 0x2A: case 0x2B:
            {
                if (fwrite(&cfg->commands[i].oneU32.param, sizeof(int32_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].oneU32.align, sizeof(uint8_t), 12, file) != 12) {
                    perror("Error writing oneU32 data\n");
                    goto end;
                }
                break;
            }
            case 0x02: case 0x04: case 0x05: case 0x12: case 0x16: case 0x17: case 0x18: case 0x19:
            case 0x1F: case 0x26: case 0x27: case 0x29:
            {
                if (fwrite(cfg->commands[i].align, sizeof(uint8_t), 16, file) != 16) {
                    perror("Error writing align\n");
                    goto end;
                }
                break;
            }
            case 0x14: case 0x15: case 0x1A: case 0x1B:
            {
                if (fwrite(&cfg->commands[i].oneU8.param, sizeof(int8_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].oneU8.align, sizeof(uint8_t), 15, file) != 15) {
                    perror("Error writing oneU8 data\n");
                    goto end;
                }
                break;
            }
            case 0x11: case 0x1D: case 0x20:
            {
                if (fwrite(&cfg->commands[i].cmd_type4.DataOffset, sizeof(int64_t), 1, file) != 1 ||
                    fwrite(cfg->commands[i].cmd_type4.align, sizeof(uint8_t), 8, file) != 8) {
                    perror("Error writing cmd_type4 data\n");
                    goto end;
                }
                break;
            }
            case 0x0A:
            {
                if (fwrite(&cfg->commands[i].twoU16.param, sizeof(uint16_t), 2, file) != 2 ||
                    fwrite(cfg->commands[i].twoU16.align, sizeof(uint8_t), 12, file) != 12) {
                    perror("Error writing twoU16 data\n");
                    goto end;
                }
                break;
            }
            case 0x0D: case 0x0E: case 0x22: case 0x23: case 0x25:
            {
                if (fwrite(&cfg->commands[i].twoU32.param, sizeof(int32_t), 2, file) != 2 ||
                    fwrite(cfg->commands[i].twoU32.align, sizeof(uint8_t), 8, file) != 8) {
                    perror("Error writing twoU32 data\n");
                    goto end;
                }
                break;
            }
            default:
            {
                if (fwrite(cfg->commands[i].align, sizeof(uint8_t), 16, file) != 16) {
                    perror("Error writing default align\n");
                    goto end;
                }
                break;
            }
        }
    }
	ret=0;

end:

    FCLOSE(file);
    return ret;
}

int GxCfg_to_txt(FILE* file, const GxCfg_t* cfg) {

    // Write header information
    fprintf(file, "\thashTitle: 0x%016llX\n", cfg->header.hashTitle);
    fprintf(file, "\tdataOffset: 0x%08lX\n", cfg->header.dataOffset);
    fprintf(file, "\tcmdCount: 0x%08lX\n", cfg->header.cmdCount);
    fflush(file);
    // Write commands
    for (uint32_t i = 0; i < cfg->header.cmdCount; i++) {
        fprintf(file, "\t#%d cmdId: 0x%08lX\n", i + 1, cfg->commands[i].cmdId);
		print_align(file, (uint8_t *) &cfg->commands[i].align32, sizeof(cfg->commands[i].align32));
        fflush(file);

        switch (cfg->commands[i].cmdId) {
			case 0x00:
            {
				fprintf(file, "\t\tEEOffset : 0x%08llX\n", cfg->commands[i].cmd_00.EEOffset);
                print_align(file, cfg->commands[i].cmd_00.align, sizeof(cfg->commands[i].cmd_00.align));
				fprintf(file, "\t\tFuncIDOffset  : 0x%016llX\n", cfg->commands[i].cmd_00.FuncIDOffset);
				break;
            }
            case 0x07:
            {
                fprintf(file, "\t\tDataOffset : 0x%016llX\n", cfg->commands[i].cmd_07.DataOffset);
                print_align(file, cfg->commands[i].cmd_07.align, sizeof(cfg->commands[i].cmd_07.align));
                fprintf(file, "\t\tReplaceDataMask  : %08X %08X\n", cfg->commands[i].cmd_07.ReplaceDataMask[0], cfg->commands[i].cmd_07.ReplaceDataMask[1]);
                fprintf(file, "\t\tReplaceData      : %08X %08X\n", cfg->commands[i].cmd_07.ReplaceData[0], cfg->commands[i].cmd_07.ReplaceData[1]);
                fprintf(file, "\t\tOriginalDataMask : %08X %08X\n", cfg->commands[i].cmd_07.OriginalDataMask[0], cfg->commands[i].cmd_07.OriginalDataMask[1]);
                fprintf(file, "\t\tOriginalData     : %08X %08X\n", cfg->commands[i].cmd_07.OriginalData[0], cfg->commands[i].cmd_07.OriginalData[1]);
                break;
            }
            case 0x08:
            {
                fprintf(file, "\t\tDataOffset : 0x%016llX\n", cfg->commands[i].cmd_08.DataOffset);
                fprintf(file, "\t\tDataCount  : 0x%08lX\n", cfg->commands[i].cmd_08.DataCount);
                print_align(file, cfg->commands[i].cmd_08.align, sizeof(cfg->commands[i].cmd_08.align));
                
                for (int j = 0; j < cfg->commands[i].cmd_08.DataCount; j++) {
                    fprintf(file, "\t\t#%d offset : 0x%08lX\n", j, cfg->commands[i].cmd_08.data[j].offset);
                    print_align(file, cfg->commands[i].cmd_08.data[j].align, sizeof(cfg->commands[i].cmd_08.data[j].align));
                    fprintf(file, "\t\t#%d OriginalData : %08X %08X\n", j, cfg->commands[i].cmd_08.data[j].OriginalData[0], cfg->commands[i].cmd_08.data[j].OriginalData[1]);
                    fprintf(file, "\t\t#%d ReplaceData  : %08X %08X\n", j, cfg->commands[i].cmd_08.data[j].ReplaceData[0], cfg->commands[i].cmd_08.data[j].ReplaceData[1]);
                }
                break;
            }
            case 0x09:
            {
                fprintf(file, "\t\tDataOffset : 0x%016llX\n", cfg->commands[i].cmd_09.DataOffset);
                fprintf(file, "\t\tDataCount  : 0x%08lX\n", cfg->commands[i].cmd_09.DataCount);
                print_align(file, cfg->commands[i].cmd_09.align, sizeof(cfg->commands[i].cmd_09.align));
                
                for (int j = 0; j < cfg->commands[i].cmd_09.DataCount; j++) {
                    fprintf(file, "\t\t#%d sector : 0x%08lX\n", j, cfg->commands[i].cmd_09.data[j].sector);
                    fprintf(file, "\t\t#%d offset : 0x%08lX\n", j, cfg->commands[i].cmd_09.data[j].offset);
                    fprintf(file, "\t\t#%d size : 0x%08lX\n", j, cfg->commands[i].cmd_09.data[j].size);
                    fprintf(file, "\t\t#%d ReplaceDataOffset  : 0x%016llX\n", j, cfg->commands[i].cmd_09.data[j].ReplaceDataOffset);
                    fprintf(file, "\t\t#%d OriginalDataOffset : 0x%016llX\n", j, cfg->commands[i].cmd_09.data[j].OriginalDataOffset);
                    print_align(file, cfg->commands[i].cmd_09.data[j].align, sizeof(cfg->commands[i].cmd_09.data[j].align));
                    fprintf(file, "\t\t#%d ReplaceData :\n", j);
                    write_data(file, cfg->commands[i].cmd_09.data[j].ReplaceData, cfg->commands[i].cmd_09.data[j].size, 0, 3);
                    fprintf(file, "\t\t#%d OriginalData :\n", j);
                    write_data(file, cfg->commands[i].cmd_09.data[j].OriginalData, cfg->commands[i].cmd_09.data[j].size, 0, 3);
                }
                break;
            }
            case 0x10:
            {
                fprintf(file, "\t\tDataOffset : 0x%016llX\n", cfg->commands[i].cmd_type1.DataOffset);
                fprintf(file, "\t\tDataCount  : 0x%08lX\n", cfg->commands[i].cmd_type1.DataCount);
				print_align(file, cfg->commands[i].cmd_type1.align, sizeof(cfg->commands[i].cmd_type1.align));
                break;
            }
            case 0x01: case 0x03: case 0x06: case 0x0B: case 0x0C: case 0x0F:
            case 0x13: case 0x1C: case 0x1E: case 0x21: case 0x24: case 0x28: case 0x2A: case 0x2B:
            {
                fprintf(file, "\t\tparam : 0x%08lX\n", cfg->commands[i].oneU32.param);
				print_align(file, cfg->commands[i].oneU32.align, sizeof(cfg->commands[i].oneU32.align));
                break;
            }
            case 0x14: case 0x15: case 0x1A: case 0x1B:
            {
                fprintf(file, "\t\tparam : 0x02%X\n", cfg->commands[i].oneU8.param);
				print_align(file, cfg->commands[i].oneU8.align, sizeof(cfg->commands[i].oneU8.align));
                break;
            }
            case 0x11: case 0x1D: case 0x20:
            {
                fprintf(file, "\t\tDataOffset : 0x%016llX\n", cfg->commands[i].cmd_type4.DataOffset);
				print_align(file, cfg->commands[i].cmd_type4.align, sizeof(cfg->commands[i].cmd_type4.align));
                break;
            }
            case 0x0A:
            {
                fprintf(file, "\t\tparam1 : 0x%04X\n", cfg->commands[i].twoU16.param[0]);
                fprintf(file, "\t\tparam2 : 0x%04X\n", cfg->commands[i].twoU16.param[1]);
				print_align(file, cfg->commands[i].twoU16.align, sizeof(cfg->commands[i].twoU16.align));
                break;
            }
            case 0x0D: case 0x0E: case 0x22: case 0x23: case 0x25:
            {
                fprintf(file, "\t\tparam1 : 0x%08lX\n", cfg->commands[i].twoU32.param[0]);
                fprintf(file, "\t\tparam2 : 0x%08lX\n", cfg->commands[i].twoU32.param[1]);
				print_align(file, cfg->commands[i].twoU32.align, sizeof(cfg->commands[i].twoU32.align));
                break;
            }
			case 0x02: case 0x04: case 0x05: case 0x12: case 0x16: case 0x17: case 0x18: case 0x19:
            case 0x1F: case 0x26: case 0x27: case 0x29:
            {
				fprintf(file, "\t\tNone\n");
				print_align(file, cfg->commands[i].align, sizeof(cfg->commands[i].align));
				break;
            }
            default:
            {
                fprintf(file, "\t\tUnknown command id 0x%X\n", cfg->commands[i].cmdId);
				print_align(file, cfg->commands[i].align, sizeof(cfg->commands[i].align));
                break;
            }
        }
        fflush(file);
    }
    fflush(file);
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
        gxLog = fopen(logpath, "w");
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

int gxcfg_gxcfg(char *filepath, char *dirOut) {
    GxCfg_t cfg;
    if (load_GxCfg(filepath, &cfg) != 0) {
        fprintf(stderr, "Failed to load file: %s\n", filepath);
        return -1;
    }

    char outpath[1024];
    snprintf(outpath, sizeof(outpath), "%s/%s", dirOut, custom_basename(filepath));

    if (save_GxCfg(outpath, &cfg) != 0) {
        fprintf(stderr, "Failed to save file: %s\n", outpath);
        free_GxCfg(&cfg);
        return -1;
    }

    free_GxCfg(&cfg);

    unsigned char md5_filepath[MD5_DIGEST_LENGTH];
    unsigned char md5_outpath[MD5_DIGEST_LENGTH];
/*
    if (calculate_md5(filepath, md5_filepath) != 0) {
        fprintf(stderr, "Failed to calculate MD5 for file: %s\n", filepath);
        return -1;
    }
*/
    struct stat st;
    if (stat(outpath, &st) != 0) {
        perror("Failed to get file size");
        return -1;
    }
    size_t outpath_size = st.st_size;

    if( get_md5(filepath, md5_filepath, outpath_size) != 0) {
        fprintf(stderr, "Failed to get MD5 for file: %s\n", filepath);
        return -1;
    }

    if (calculate_md5(outpath, md5_outpath) != 0) {
        fprintf(stderr, "Failed to calculate MD5 for file: %s\n", outpath);
        return -1;
    }

    if (memcmp(md5_filepath, md5_outpath, MD5_DIGEST_LENGTH) != 0) {
        fprintf(stderr, "MD5 mismatch between %s and %s\n", filepath, outpath);
        return -1;
    }

    return 0;
}