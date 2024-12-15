#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "common.h"
#include "gx.h"
#include "net.h"

#define GX_OFFSET 0x0
u32 GxFuncHackIdOffset[] = {0x36B40,
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
                            0x34A70,
                            0x34B48,
                            0x34C20,
                            0x34CF8,
                            0x37714
						};

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

int main() {    
	
	GxCfg_scandir_to_txt("AldoGX", "gxlog.txt");
	
	NetCfg_scandir_to_txt("AldoNET", "netlog.txt");

    return 0;
}
