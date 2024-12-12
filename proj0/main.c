#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <dirent.h>

typedef uint8_t		u8;
typedef uint16_t 	u16;
typedef uint32_t	u32;
typedef uint64_t 	u64;
typedef int8_t		s8;
typedef int16_t 	s16;
typedef int32_t		s32;
typedef int64_t 	s64;

#define IS_BIG_ENDIAN 			(!*(unsigned char *)&(uint16_t){1})
#define IS_LITTLE_ENDIAN		(*(unsigned char *)&(uint16_t){1})

#define ENDIAN_SWAP_16(x)		(((x) & 0x00FF) << 8 | ((x) & 0xFF00) >> 8)
#define ENDIAN_SWAP_32(x)		(((x) & 0x000000FF) << 24 | ((x) & 0x0000FF00) << 8 | \
								 ((x) & 0x00FF0000) >>  8 | ((x) & 0xFF000000) >> 24  )
#define ENDIAN_SWAP_64(x)		(((x) & 0x00000000000000FFULL) << 56 | ((x) & 0x000000000000FF00ULL) << 40 | \
								 ((x) & 0x0000000000FF0000ULL) << 24 | ((x) & 0x00000000FF000000ULL) <<  8 | \
								 ((x) & 0x000000FF00000000ULL) >>  8 | ((x) & 0x0000FF0000000000ULL) >> 24 | \
								 ((x) & 0x00FF000000000000ULL) >> 40 | ((x) & 0xFF00000000000000ULL) >> 56 )
#define ENDIAN_SWAP(x)			(sizeof(x) == 2 ? ENDIAN_SWAP_16(x) : (sizeof(x) == 4 ? ENDIAN_SWAP_32(x) : ENDIAN_SWAP_64(x)))

// note for me...
// SWAP to/from BigEndian & LittleEndian
// if the file is in LittleEndian (like IRD) use SWAP_LE
// if it's BigEndian (like ISO) use SWAP_BE
#define SWAP_BE(x)				(IS_BIG_ENDIAN    ? x : ENDIAN_SWAP(x))
#define SWAP_LE(x)				(IS_LITTLE_ENDIAN ? x : ENDIAN_SWAP(x))

#define NET_GX 0
#define NET_SOFT 1
#define GX_SOFT 2
#define GX_NET 3
#define SOFT_NET 4
#define SOFT_GX 5

u8 verbose = 0;


u8 is_dir(char *path)
{
	struct stat path_stat;
    stat(path, &path_stat);
	if (stat(path, &path_stat) != 0) return 0;
    return S_ISDIR(path_stat.st_mode);
}

char *GetExtension(char *path)
{
	if( path == NULL ) return NULL;
	
	int n = strlen(path);
    int m = n;

    while(m > 0 && path[m] != '.' && path[m] != '/') m--;
    
	if(path[m] != '.') return NULL;
	
    if(path[m] == '.') return &path[m];

    return NULL;
}

static void print_help()
{
	printf( "\nUsage of ps2 config converter:\n"
			"    Format:\n"
			"        ps2config-convert.exe [option] <mode> <input> <output>\n"
			"    Description :\n"
			"        Convert PS2 CONFIG from and to NET/GX/SOFT.\n"
			"    Option:\n"
			"        -h, --help                      This help text.\n"
			"        -v, --verbose                   Make the operation more talkative.\n"
			"    mode:\n"
			"        -0, --NET_GX         From NET to GX.\n"
			"        -1, --NET_SOFT       From NET to SOFT.\n"
			"        -2, --GX_SOFT        From GX to SOFT.\n"
			"        -3, --GX_NET         From GX to NET.\n"
			"        -4, --SOFT_NET       From SOFT to NET.\n"
			"        -5, --SOFT_GX        From SOFT to GX.\n"
			"    Note:\n"
			"        input/output supports file and directory.\n"
			);
}

#define col_gx   0
#define col_soft 1
#define col_net  2

s32 emuID[0x51][3] = {
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
    {    -1,    -1, 0x0A},
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
    {    -1,  0x2C, 0x42},
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
        if (*g != -1 && emuID[i][col_gx] == *g) {
            *s = emuID[i][col_soft];
            *n = emuID[i][col_net];
            return;
        } else 
		if (*s != -1 && emuID[i][col_soft] == *s) {
            *g = emuID[i][col_gx];
            *n = emuID[i][col_net];
            return;
        } else 
		if (*n != -1 && emuID[i][col_net] == *n) {
            *g = emuID[i][col_gx];
            *s = emuID[i][col_soft];
            return;
        }
    }
}

/* ugly
void getid(s32 *gx, s32 *soft, s32 *net)
{
	if( *gx != -1 ) {
		if( 0x00 <= *gx && *gx < 0x09) {
			*net = *gx + 1;
		} else
		if( 0x09 <=  *gx && *gx < 0x17 ) {
			*net = *gx + 2;
		} else
		if( 0x17 <=  *gx && *gx < 0x1F ) {
			*net = *gx + 3;
		} else 
		if( 0x1F <=  *gx && *gx < 0x2C ) {
			*net = *gx + 4;
		} else
		if( *gx == 0x2C ) { // custom cmd by Kozarovv
			*net = 0x42;
		}
		
		if( 0x00 <= *gx && *gx < 0x17 ) {
			*soft = *gx;
		} else
		if( 0x17 <= *gx && *gx <= 0x1A ) {
			*soft = *gx + 1 ;
		} else
		if( 0x1C <= *gx && *gx < 0x1F ) {
			*soft = *gx;
		} else
		if( 0x1F <= *gx && *gx < 0x2B ) {
			*soft = *gx + 1;
		}
	} else
	if( *soft != -1 ) {
		
		if( 0x00 <= *soft && *soft < 0x09 ) {
			*net = *soft + 1;
		} else
		if( 0x09 <= *soft && *soft < 0x1C ) {
			*net = *soft + 2;
		} else
		if( 0x1C <= *soft && *soft < 0x2C ) {
			*net = *soft + 3;
		}
		
		if( 0x00 <= *soft && *soft < 0x17 ) {
			*gx = *soft;
		} else
		if( 0x17 <  *soft && *soft < 0x1C ) {
			*gx = *soft - 1;
		} else
		if( 0x1C <= *soft && *soft < 0x1F ) {
			*gx = *soft;
		} else
		if( 0x1F <  *soft && *soft < 0x2C ) {
			*gx = *soft - 1;
		}
		
	} else 
	if( *net != -1 ) {
		if( 0x00 < *net && *net < 0x0A ) {
			*soft = *net - 1;
		} else
		if( 0x0A < *net && *net < 0x1E ) {
			*soft = *net - 2;
		} else
		if( 0x1E < *net && *net < 0x2F ) {
			*soft = *net - 3;
		}
		
		if( 0x00 < *net && *net < 0x0A ) {
			*gx = *net - 1;
		} else
		if( 0x0A < *net && *net < 0x19) {
			*gx = *net - 2;
		} else
		if( 0x19 < *net && *net < 0x22) {
			*gx = *net - 3;
		} else
		if( 0x22 < *net && *net < 0x30 ) {
			*gx = *net - 4;
		} else 
		if( *net == 0x42 ) {
			*gx = 0x2C;
		}
	}
}
*/

void convert(u8 type, char *input, char *output)
{
	
	FILE *in = fopen(input, "rb");
	if( in == NULL ) {
		printf("Error: cannot open %s", input);
		exit(0);
	}
	
	FILE *ou = fopen(output, "wb");
	if( ou == NULL ) {
		printf("Error: cannot create %s", output);
		fclose(in);
		exit(0);
	}
	
	u32 cmdID;
	
	u8 data[1024]={0};
	
	while(fread(&cmdID, sizeof(u32), 1, in) == 1) {
		
		cmdID = SWAP_LE(cmdID);
		memset(data, 0, 1024);
		
		// CONVERT ID
		
		s32 newID=-1;
		u32 dataSize = 0;
		
		s32 netID = -1;
		s32 softID = -1;
		s32 gxID = -1;
		
		if( type == NET_GX || NET_SOFT ) { // READ NET
			netID = cmdID;
		} else 
		if( type == SOFT_GX || SOFT_NET ) { // READ SOFT
			softID = cmdID;
		} else 
		if( type == GX_NET || GX_SOFT ) { // READ GX
			gxID = cmdID;
		} 
		
		getid(&gxID, &softID, &netID);

		switch(netID)
		{
			// no param
			case 0x03 :
			case 0x05 : //next id
			case 0x06 :
			case 0x14 :
			case 0x16 : //next id
			case 0x18 : //next id
			case 0x19 :
			case 0x1A :
			case 0x1B :
			case 0x22 :
			case 0x23 :
			case 0x25 : //next id
			case 0x2A :
			case 0x2B :
			case 0x2D : //next id
			case 0x35 :
			case 0x3E :
			case 0x40 :
			case 0x41 :
			case 0x44 :
			case 0x45 :
			case 0x46 :
			case 0x47 :
			case 0x49 :
			case 0x4A :
			case 0x50 :
			{
				dataSize=0;
				break;
			}
			// u32
			case 0x04 :
			case 0x07 :
			case 0x0C :
			case 0x0D :
			case 0x0E :
			case 0x11 :
			case 0x15 :
			case 0x17 :
			case 0x1C :
			case 0x1D :
			case 0x1E :
			case 0x1F :
			case 0x21 :
			case 0x28 :
			case 0x2C :
			case 0x2E :
			case 0x2F :
			case 0x3D :
			case 0x3F :
			case 0x42 :
			case 0x43 :
			case 0x4D :
			{
				dataSize = 4;
				if( fread(data, sizeof(u8), dataSize, in) != dataSize) {
					printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
					return;
				}
				
				break;
			}
			// u32 x 2
			case 0x01 :
			case 0x02 :
			case 0x0F :
			case 0x10 :
			case 0x13 :			
			case 0x20 :
			case 0x24 :
			case 0x26 :
			case 0x27 :
			case 0x29 :
			case 0x48 :
			case 0x4B :
			case 0x4C :
			{
				dataSize = 8;
				if( fread(data, sizeof(u8), dataSize, in) != dataSize) {
					printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
					return;
				}
				
				
				break;
			}
			// TITLE
			case 0x00 :
			{
				dataSize = 10;
				if( fread(data, sizeof(u8), dataSize, in) != dataSize) {
					printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
					return;
				}
				
				
				break;
			}
			case 0x08 :
			{
				dataSize = 32;
				if( fread(data, sizeof(u8), dataSize, in) != dataSize) {
					printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
					return;
				}
				
				break;
			}
			case 0x09 :
			{
				u8 patchSize = 4*5;
				dataSize = 4;
				if( fread(data, sizeof(u8), dataSize, in) != dataSize) {
					printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
					return;
				}
				
				u32 nb = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
				nb=ENDIAN_SWAP(nb);
				
				if( fread(data+dataSize, sizeof(u8),  nb*patchSize, in) != nb*patchSize) {
					printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
					return;
				}
				
				dataSize += nb*patchSize;
				
				break;
			}
			case 0x0A :
			{
				u8 patchSize = 4*3;
				dataSize = 4;
				if( fread(data, sizeof(u8), dataSize, in) != dataSize) {
					printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
					return;
				}
				
				u32 nb = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
				nb=ENDIAN_SWAP(nb);
				
				if( fread(data+dataSize, sizeof(u8),  nb*patchSize, in) != nb*patchSize) {
					printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
					return;
				}
				
				dataSize += nb*patchSize;
				
				break;
			}
			case 0x12 :
			{
				u8 patchSize = 4;
				dataSize = 4;
				if( fread(data, sizeof(u8), dataSize, in) != dataSize) {
					printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
					return;
				}
				
				u32 nb = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
				nb=ENDIAN_SWAP(nb);
				
				if( fread(data+dataSize, sizeof(u8),  nb*patchSize, in) != nb*patchSize) {
					printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
					return;
				}
				
				dataSize += nb*patchSize;
				
				break;
			}
			case 0x0B :
			{
				dataSize = 4;
				if( fread(data, sizeof(u8), dataSize, in) != dataSize) {
					printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
					return;
				}
				
				u32 nb = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
				nb=ENDIAN_SWAP(nb);
				
				int i;
				for( i=0; i<nb; i++) {
					
					if( fread(data+dataSize, sizeof(u8), 8, in) != 8) {
						printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
						return;
					}
					dataSize += 8;
					
					if( fread(data+dataSize, sizeof(u8), 4, in) != 4) {
						printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
						return;
					}
					
					u32 patchSize = (data[dataSize] << 24) | (data[dataSize+1] << 16) | (data[dataSize+2] << 8) | data[dataSize+3];
					patchSize=ENDIAN_SWAP(patchSize);
					
					dataSize+=4;
					
					if( fread(data+dataSize, sizeof(u8), patchSize*2, in) != patchSize*2) {
						printf("Error: failed to read param for cmd: %X. Path: %s", netID, input);
						return;
					}
					dataSize+=patchSize*2;
				}
				
				break;
			}
			default :
				break;
		}
		
		
		if( type == GX_NET || type == SOFT_NET ) {			
			if( netID != - 1 ) {
				if( fwrite(&netID, sizeof(s32), 1, ou) != 1) {
					printf("Error: failed to write netID : %X. Path: %s", netID, input);
					return;
				}
				if( fwrite(data, 1, dataSize, ou) != dataSize) {
					printf("Error: failed to write netID data : %X. Path: %s", netID, input);
					return;
				}
			}
		} else
		if( type == NET_GX || type == SOFT_GX ) {			
			if( gxID != - 1 ) {
				if( fwrite(&gxID, sizeof(s32), 1, ou) != 1) {
					printf("Error: failed to write gxID : %X. Path: %s", gxID, input);
					return;
				}
				if( fwrite(data, 1, dataSize, ou) != dataSize) {
					printf("Error: failed to write gxID data : %X. Path: %s", gxID, input);
					return;
				}
			}
		} else
		if( type == NET_SOFT || type == GX_SOFT ) {			
			if( softID != - 1 ) {
				if( fwrite(&softID, sizeof(s32), 1, ou) != 1) {
					printf("Error: failed to write softID : %X. Path: %s", softID, input);
					return;
				}
				if( fwrite(data, 1, dataSize, ou) != dataSize) {
					printf("Error: failed to write softID data : %X. Path: %s", softID, input);
					return;
				}
			}
		}
	}
	
	fclose(in);
	fclose(ou);
	
}


void convert_loop(u8 type, char *input, char *output)
{
	char in[255]={0};
	char ou[255]={0};
	
	if( input == NULL ) {
		switch(type)
		{
			case GX_NET :
			case GX_SOFT :
			{
				strcpy(in, "GX");
				break;
			}
			case NET_GX :
			case NET_SOFT :
			{
				strcpy(in, "NET");
				break;
			}
			case SOFT_NET :
			case SOFT_GX :
			{
				strcpy(in, "SOFT");
				break;
			}
			default :
			{
				printf("Error: type unknown\n");
				exit(0);
			}
		}
		if( !is_dir(in) ) {
			printf("Error: cannot find input\n");
			return;
		}
	} else {
		strcpy(in, input);
	}
	
	if( output == NULL ) {
		switch(type)
		{
			case GX_NET :
			case SOFT_NET :
			{
				strcpy(ou, "NET");
				break;
			}
			case NET_GX :
			case SOFT_GX :
			{
				strcpy(ou, "GX");
				break;
			}
			case NET_SOFT :
			case GX_SOFT :
			{
				strcpy(ou, "SOFT");
				break;
			}
			default :
			{
				printf("Error: type unknown\n");
				exit(0);
				break;
			}
		}
		if( !is_dir(ou) ) {	
			if( mkdir(ou, 0777) !=0) {
				printf("Error: failed to create directory '%s'", ou);
				return;
			}
		}
	} else {
		strcpy(ou, output);
	}
	
	DIR *d;
	struct dirent *dir;

	int l = strlen(in);
	int i;

	for(i=0;i<l;i++){
		if(in[i]=='\\') in[i]='/';
	}

	if( is_dir(in) ) {
		d = opendir(in);
		if(d==NULL) return;

		while ((dir = readdir(d))) {
			if(!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) continue;

			char temp[512];
			sprintf(temp, "%s/%s", in, dir->d_name);

			if(is_dir(temp)) convert_loop(type, temp, ou);
			else {

				char *ext = GetExtension(temp);
				if( strcasecmp(ext, ".CONFIG") == 0) {
					char temp2[512];
					sprintf(temp2, "%s/%s", ou, dir->d_name);
					convert(type, temp, temp2);
				}
			}
		}
		closedir(d);
	} else {
		char *ext = GetExtension(in);
		if( strcasecmp(ext, ".CONFIG") == 0) {
			convert(type, in, ou);
		}
	}

}

void print_tab()
{
	int g,s,n;
	int i;
	char g_str[10], s_str[10], n_str[10];
	
	for(i=0;i<0x50;i++)
	{
		g=-1;
		s=-1;
		n=i;
		
		getid(&g,&s,&n);
		printf("%s | %s | %s\n", (g == -1) ? " N/A" : (sprintf(g_str, "0x%02X", g), g_str), (s == -1) ? " N/A" : (sprintf(s_str, "0x%02X", s), s_str), (n == -1) ? " N/A" : (sprintf(n_str, "0x%02X", n), n_str));
	}
	
	printf("\n\n");
	
	for(i=0;i<0x50;i++)
	{
		g=-1;
		s=i;
		n=-1;
		
		getid(&g,&s,&n);
		printf("%s | %s | %s\n", (g == -1) ? " N/A" : (sprintf(g_str, "0x%02X", g), g_str), (s == -1) ? " N/A" : (sprintf(s_str, "0x%02X", s), s_str), (n == -1) ? " N/A" : (sprintf(n_str, "0x%02X", n), n_str));
	}
	
	
	printf("\n\n");
	
	for(i=0;i<0x50;i++)
	{
		g=i;
		s=-1;
		n=-1;
		
		getid(&g,&s,&n);
		printf("%s | %s | %s\n", (g == -1) ? " N/A" : (sprintf(g_str, "0x%02X", g), g_str), (s == -1) ? " N/A" : (sprintf(s_str, "0x%02X", s), s_str), (n == -1) ? " N/A" : (sprintf(n_str, "0x%02X", n), n_str));
	}
}


int main(int argc, char *argv[])
{	
	if(argc==1) print_help();	
	
	verbose=0;
	s8 type = -1;
	int i;
	u32 a = 1;
    
    for(i=1; i<argc; i++) {
		if( !strcmp(argv[i], "-0") || !strcmp(argv[i], "-NET_GX") ) {
            type = NET_GX ;
            a++;
        } else
		if( !strcmp(argv[i], "-1") || !strcmp(argv[i], "-NET_SOFT") ) {
            type = NET_SOFT ;
            a++;
        } else
		if( !strcmp(argv[i], "-2") || !strcmp(argv[i], "-GX_SOFT") ) {
            type = GX_SOFT ;
            a++;
        } else
		if( !strcmp(argv[i], "-3") || !strcmp(argv[i], "-GX_NET") ) {
            type = GX_NET ;
            a++;
        } else
		if( !strcmp(argv[i], "-4") || !strcmp(argv[i], "-SOFT_NET") ) {
            type = SOFT_NET ;
            a++;
        } else
		if( !strcmp(argv[i], "-5") || !strcmp(argv[i], "-SOFT_GX") ) {
            type = SOFT_GX ;
            a++;
        } else
        if( !strcmp(argv[i], "-v") ||  !strcmp(argv[i], "--verbose") ) {
            verbose=1;
            a++;
        } else
        if( !strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            print_help();
            return 1;
        }
    }
	
	if( type == -1) {
		printf("Error: you must define the mode\n");
		return 1;
	}
	
	if( argc + 1 == argc) {
		convert_loop(type, argv[a], argv[a+1]);
	} else 
	if( argc == argc) {
		convert_loop(type, argv[a], NULL);
	} else {
		convert_loop(type, NULL, NULL);
	}
	return 0;
}