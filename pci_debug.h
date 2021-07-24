/*
 * @Author: your name
 * @Date: 2021-07-23 21:03:01
 * @LastEditTime: 2021-07-24 18:50:50
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /HSM2_PCIE/pci_debug.h
 */
/* pci_debug.h
 *
 * 6/21/2010 D. W. Hawkins
 *
 * PCI debug registers interface.
 *
 * This tool provides a debug interface for reading and writing
 * to PCI registers via the device base address registers (BARs).
 * The tool uses the PCI resource nodes automatically created
 * by recently Linux kernels.
 *
 * The readline library is used for the command line interface
 * so that up-arrow command recall works. Command-line history
 * is not implemented. Use -lreadline -lcurses when building.
 *
 * ----------------------------------------------------------------
 */

#ifndef _PCI_DEBUG_
#define _PCI_DEBUG_

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
#include <readline/readline.h>
#include <readline/history.h>

/* PCI device */
typedef struct
{
	/* Base address region */
	unsigned int bar;

	/* Slot info */
	unsigned int domain;
	unsigned int bus;
	unsigned int slot;
	unsigned int function;

	/* Resource filename */
	char filename[100];

	/* File descriptor of the resource */
	int fd;

	/* Memory mapped resource */
	unsigned char *maddr;
	unsigned int size;
	unsigned int offset;

	/* PCI physical address */
	unsigned int phys;

	/* Address to pass to read/write (includes offset) */
	unsigned char *addr;
} device_t;

void display_dev(device_t *dev);

// void display_help(device_t *dev);
// void parse_command(device_t *dev);
// int process_command(device_t *dev, char *cmd);
// int change_mem(device_t *dev, char *cmd);
// int fill_mem(device_t *dev, char *cmd);
// int display_mem(device_t *dev, char *cmd);
// int change_endian(device_t *dev, char *cmd);
// int init_mem(device_t *dev);
// int test_mem(device_t *dev, char *cmd);
// int verify_mem(device_t *dev, char *cmd);

/* Endian read/write mode */
static int big_endian = 0;

/* Low-level access functions */
static void write_8(device_t *dev, unsigned int addr, unsigned char data);
static unsigned char read_8(device_t *dev, unsigned int addr);
static void write_le16(device_t *dev, unsigned int addr, unsigned short int data);
static unsigned short int read_le16(device_t *dev, unsigned int addr);
static void write_be16(device_t *dev, unsigned int addr, unsigned short int data);
static unsigned short int read_be16(device_t *dev, unsigned int addr);
void write_le32(device_t *dev, unsigned int addr, unsigned int data);
unsigned int read_le32(device_t *dev, unsigned int addr);
static void write_be32(device_t *dev, unsigned int addr, unsigned int data);
static unsigned int read_be32(device_t *dev, unsigned int addr);

/* Usage */
static void show_usage();


#endif