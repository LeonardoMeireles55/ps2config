#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <stdint.h>

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
#define ENDIAN_SWAP_64_32(x)	((x & 0xFFFFFFFF00000000) >> 32) | ((x & 0x00000000FFFFFFFF) << 32)
#define ENDIAN_SWAP(x)			(sizeof(x) == 2 ? ENDIAN_SWAP_16(x) : (sizeof(x) == 4 ? ENDIAN_SWAP_32(x) : ENDIAN_SWAP_64(x)))
	
#define ARRAY_SIZE(val) (sizeof(val) / sizeof((val)[0]))

// note for myself...
// SWAP to/from BigEndian & LittleEndian
// if the file is in LittleEndian (like IRD) use SWAP_LE
// if it's BigEndian (like ISO) use SWAP_BE
#define SWAP_BE(x)				(IS_BIG_ENDIAN    ? x : ENDIAN_SWAP(x))
#define SWAP_LE(x)				(IS_LITTLE_ENDIAN ? x : ENDIAN_SWAP(x))

#define ES_LE_ARRAY(val) \
	do \
	{  if( IS_BIG_ENDIAN) {\
			for(size_t index = 0; index < ARRAY_SIZE(val); index++) \
			{ \
				val[index] = ENDIAN_SWAP(val[index]); \
			} \
		} \
	} while(0)

#define ES_BE_ARRAY(val) \
	do \
	{  if( IS_LITTLE_ENDIAN) {\
			for(size_t index = 0; index < ARRAY_SIZE(val); index++) \
			{ \
				val[index] = ENDIAN_SWAP(val[index]); \
			} \
		} \
	} while(0)

#define ES_BE(x) \
	do { \
		if (IS_LITTLE_ENDIAN) { \
			x = ENDIAN_SWAP(x); \
		} \
	} while (0)

#define ES_LE(x) \
	do { \
		if (IS_BIG_ENDIAN) { \
			x = ENDIAN_SWAP(x); \
		} \
	} while (0)

#define FREE(x)					if(x!=NULL) {free(x);x=NULL;}
#define FCLOSE(x) 				if(x!=NULL) {fclose(x);x=NULL;}

// show align data in logs
#define show_align 1

// MODE
#define NET_GX 1
#define NET_SOFT 2
#define GX_SOFT 3
#define GX_NET 4
#define SOFT_NET 5
#define SOFT_GX 6
#define NET_TXT 7
#define GX_TXT 8
#define SOFT_TXT 9

// to test functions load/save
#define NET_NET 100
#define GX_GX 101
#define SOFT_SOFT 102

#define GX_DATA_OFFSET 0x00341178

#endif