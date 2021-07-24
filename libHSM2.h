/*
 * @Author: your name
 * @Date: 2021-07-21 20:55:24
 * @LastEditTime: 2021-07-24 21:02:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /sm2_sign/libHSM2.h
 */

#ifndef _LIB_HSM2_
#define _LIB_HSM2_

#include "lib/pci.h"

#include <stdio.h>
#include <errno.h> // errno
#include <string.h>
#include <fcntl.h> // open()
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/mman.h> // #define MAP_FAILED
#include <sys/types.h>
#include <sys/stat.h> // fstat()
#include <stdlib.h>
#include <unistd.h>
#include <byteswap.h>
#include <time.h>
/* Readline support */
// #include <readline/readline.h>
// #include <readline/history.h>


#define BASE_ADDR0 0x00000000
#define BASE_ADDR1 0x00004000

#define DATA_ADDR 0x00000000
#define PARAM_ADDR 0x00000200
#define CMD_ADDR 0x00000300
#define STATE_ADDR 0x00000301

#define CMD_SOFTRST 0x55550000
#define CMD_INIT1 0x00000010
#define CMD_INIT2 0x00000020

#define CMD_GENKEY 0x00000001
#define CMD_SIGN 0x00001e01
#define CMD_VERIFY 0x00004801
#define CMD_ENCRYPT 0x00007701
#define CMD_DECRYPT 0x0000a801
#define CMD_KEYX 0x0000cb01


typedef unsigned int U32;
typedef unsigned short int U16;
typedef unsigned char U8;

/* PCI device */
typedef struct
{
	/* Base address region */
	U32 bar;

	/* Slot info */
	U32 domain;
	U32 bus;
	U32 slot;
	U32 function;

	/* Resource filename */
	char filename[100];

	/* File descriptor of the resource */
	int fd;

	/* Memory mapped resource */
	U8 *maddr;
	U32 size;
	U32 offset;

	/* PCI physical address */
	U32 phys;

	/* Address to pass to read/write (includes offset) */
	U8 *addr;
} device_t;


int open_device(device_t **dev);
void close_device(device_t *dev);
void SM2_Init(device_t *dev, U32 base_addr);

int SM2_GenKey(device_t *dev, U32 base_addr, U32 *rand, U32 *pri_key, U32 *pub_key);
int SM2_Sign(device_t *dev, U32 base_addr, U32 *rand, U32 *pri_key, U32 *hash, U32 *sign);
int SM2_Verify(device_t *dev, U32 base_addr, U32 *pub_key, U32 *hash, U32 *sign);
int SM2_Encrypt(device_t *dev, U32 base_addr, U32 *rand, U32 *pub_key, U32 *C1, U32 *S);
int SM2_Decrypt(device_t *dev, U32 base_addr, U32 *pri_key, U32 *C1, U32 *S);
int SM2_KeyExchange(device_t *dev, U32 base_addr, U32 *self_r, U32 *self_Rx, U32 *self_d, U32 *other_R, U32 *other_P, U32 *UV);


/* Low-level access functions */
static void write_8(device_t *dev, U32 addr, U8 data);
static U8 read_8(device_t *dev, U32 addr);
static void write_le16(device_t *dev, U32 addr, U16 data);
static U16 read_le16(device_t *dev, U32 addr);
static void write_be16(device_t *dev, U32 addr, U16 data);
static U16 read_be16(device_t *dev, U32 addr);
static void write_le32(device_t *dev, U32 addr, U32 data);
static U32 read_le32(device_t *dev, U32 addr);
static void write_be32(device_t *dev, U32 addr, U32 data);
static U32 read_be32(device_t *dev, U32 addr);


#endif