/* pci_debug.c
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
#include "pci_debug.h"

/* Usage */
static void show_usage()
{
	printf("\nUsage: pci_debug -s <device>\n"
		   "  -h            Help (this message)\n"
		   "  -s <device>   Slot/device (as per lspci)\n"
		   "  -b <BAR>      Base address region (BAR) to access, eg. 0 for BAR0\n\n");
}


void display_dev(device_t *dev) {
	printf("bar: %d, domain: %04x, bus: %02x, slot: %02x, func: %01x\n", 
	dev->bar, dev->domain, dev->bus, dev->slot, dev->function);
}




#if 0
int main(int argc, char *argv[])
{
	int opt;
	char *slot = 0;
	int status;
	struct stat statbuf;
	device_t device;
	device_t *dev = &device;
	int i;

	/* Clear the structure fields */
	memset(dev, 0, sizeof(device_t));
#if 0
	while ((opt = getopt(argc, argv, "b:hs:")) != -1)
	{
		switch (opt)
		{
		case 'b':
			/* Defaults to BAR0 if not provided */
			dev->bar = atoi(optarg);
			break;
		case 'h':
			show_usage();
			return -1;
		case 's':
			slot = optarg;
			break;
		default:
			show_usage();
			return -1;
		}
	}
	if (slot == 0)
	{
		show_usage();
		return -1;
	}


	/* ------------------------------------------------------------
	 * Open and map the PCI region
	 * ------------------------------------------------------------
	 */

	/* Extract the PCI parameters from the slot string */
	status = sscanf(slot, "%2x:%2x.%1x",
					&dev->bus, &dev->slot, &dev->function);
	if (status != 3)
	{
		printf("Error parsing slot information!\n");
		show_usage();
		return -1;
	}
#endif
	struct pci_access *pacc;
	struct pci_dev *dev_t;
	unsigned int c;
	pacc = pci_alloc(); // get the pci_access structure
	pci_init(pacc);		// initialize the pci library
	pci_scan_bus(pacc); // get the list of devices

	for (dev_t = pacc->devices;; dev_t = dev_t->next)
	{
		pci_fill_info(dev_t, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS); // fill in header info we need
		if (dev_t->vendor_id == 0x10ee && dev_t->device_id == 0x7024)
		{
			dev->bus = dev_t->bus;
			dev->slot = dev_t->dev;
			dev->function = dev_t->func;
			printf("%04x:%02x:%02x.%d vendor=%04x device=%04x class=%04x irq=%d (pin %d) base0=%lx\n",
				   dev_t->domain, dev_t->bus, dev_t->dev, dev_t->func, dev_t->vendor_id, dev_t->device_id,
				   dev_t->device_class, dev_t->irq, c, (long)dev_t->base_addr[0]);
			break;
		}
	}
	pci_cleanup(pacc); // close

	/* Convert to a sysfs resource filename and open the resource */
	snprintf(dev->filename, 99, "/sys/bus/pci/devices/%04x:%02x:%02x.%1x/resource%d",
			 dev->domain, dev->bus, dev->slot, dev->function, dev->bar);
	dev->fd = open(dev->filename, O_RDWR | O_SYNC);
	if (dev->fd < 0)
	{
		printf("Open failed for file '%s': errno %d, %s\n",
			   dev->filename, errno, strerror(errno));
		return -1;
	}

	/* PCI memory size */
	status = fstat(dev->fd, &statbuf);
	if (status < 0)
	{
		printf("fstat() failed: errno %d, %s\n",
			   errno, strerror(errno));
		return -1;
	}
	dev->size = statbuf.st_size;

	/* Map */
	dev->maddr = (unsigned char *)mmap(
		NULL,
		(size_t)(dev->size),
		PROT_READ | PROT_WRITE,
		MAP_SHARED,
		dev->fd,
		0);
	if (dev->maddr == (unsigned char *)MAP_FAILED)
	{
		//		printf("failed (mmap returned MAP_FAILED)\n");
		printf("BARs that are I/O ports are not supported by this tool\n");
		dev->maddr = 0;
		close(dev->fd);
		return -1;
	}

	/* Device regions smaller than a 4k page in size can be offset
	 * relative to the mapped base address. The offset is
	 * the physical address modulo 4k
	 */
	{
		char configname[100];
		int fd;

		snprintf(configname, 99, "/sys/bus/pci/devices/%04x:%02x:%02x.%1x/config",
				 dev->domain, dev->bus, dev->slot, dev->function);
		fd = open(configname, O_RDWR | O_SYNC);
		if (dev->fd < 0)
		{
			printf("Open failed for file '%s': errno %d, %s\n",
				   configname, errno, strerror(errno));
			return -1;
		}

		status = lseek(fd, 0x10 + 4 * dev->bar, SEEK_SET);
		if (status < 0)
		{
			printf("Error: configuration space lseek failed\n");
			close(fd);
			return -1;
		}
		status = read(fd, &dev->phys, 4);
		if (status < 0)
		{
			printf("Error: configuration space read failed\n");
			close(fd);
			return -1;
		}
		dev->offset = ((dev->phys & 0xFFFFFFF0) % 0x1000);
		dev->addr = dev->maddr + dev->offset;
		close(fd);
	}

	/* ------------------------------------------------------------
	 * Tests
	 * ------------------------------------------------------------
	 */

	//	printf("\n");
	//	printf("PCI debug\n");
	//	printf("---------\n\n");
	//	printf(" - accessing BAR%d\n", dev->bar);
	//	printf(" - region size is %d-bytes\n", dev->size);
	//	printf(" - offset into region is %d-bytes\n", dev->offset);

	/* Display help */
	//	display_help(dev);

	/* Process commands */
	//	parse_command(dev);

	//
	init_mem(dev);

	printf("SM2_Sign verification is running .... \n");
	int times = 100000;
	time_t start, finish;
	double duration;
	start = clock();
	for (i = 0; i < times; i++)
	{
		SM2_Sign(dev);
	}
	finish = clock();
	duration = (finish - start) / CLOCKS_PER_SEC;
	printf("sign %d per second\n", (int)(times / duration));

	//	printf("SM2_Verify verification is running .... \n");
	//
	//	for(i=0; i<4000000; i++) {
	//		SM2_Verify(dev);
	//	}

	/* Cleanly shutdown */
	munmap(dev->maddr, dev->size);
	close(dev->fd);
	return 0;
}

void parse_command(device_t *dev)
{
	char *line;
	int len;
	int status;

	while (1)
	{
		line = readline("PCI> ");
		/* Ctrl-D check */
		if (line == NULL)
		{
			printf("\n");
			continue;
		}
		/* Empty line check */
		len = strlen(line);
		if (len == 0)
		{
			continue;
		}
		/* Process the line */
		status = process_command(dev, line);
		if (status < 0)
		{
			break;
		}

		/* Add it to the history */
		add_history(line);
		free(line);
	}
	return;
}

/*--------------------------------------------------------------------
 * User interface
 *--------------------------------------------------------------------
 */
void display_help(device_t *dev)
{
	printf("\n");
	printf("  ?                         Help\n");
	printf("  d[width] addr len         Display memory starting from addr\n");
	printf("                            [width]\n");
	printf("                              8   - 8-bit access\n");
	printf("                              16  - 16-bit access\n");
	printf("                              32  - 32-bit access (default)\n");
	printf("  c[width] addr val         Change memory at addr to val\n");
	printf("  e                         Print the endian access mode\n");
	printf("  e[mode]                   Change the endian access mode\n");
	printf("                            [mode]\n");
	printf("                              b - big-endian (default)\n");
	printf("                              l - little-endian\n");
	printf("  f[width] addr val len inc  Fill memory\n");
	printf("                              addr - start address\n");
	printf("                              val  - start value\n");
	printf("                              len  - length (in bytes)\n");
	printf("                              inc  - increment (defaults to 1)\n");
	printf("  q                          Quit\n");
	printf("\n  Notes:\n");
	printf("    1. addr, len, and val are interpreted as hex values\n");
	printf("       addresses are always byte based\n");
	printf("\n");
}

int process_command(device_t *dev, char *cmd)
{
	if (cmd[0] == '\0')
	{
		return 0;
	}
	switch (cmd[0])
	{
	case '?':
		display_help(dev);
		break;
	case 'c':
	case 'C':
		return change_mem(dev, cmd);
	case 'd':
	case 'D':
		return display_mem(dev, cmd);
	case 'e':
	case 'E':
		return change_endian(dev, cmd);
	case 'f':
	case 'F':
		return fill_mem(dev, cmd);
	case 'i':
	case 'I':
		return init_mem(dev);
	case 't':
	case 'T':
		return test_mem(dev, cmd);
	case 'v':
	case 'V':
		return verify_mem(dev, cmd);
	case 'q':
	case 'Q':
		return -1;
	default:
		break;
	}
	return 0;
}

int display_mem(device_t *dev, char *cmd)
{
	int width = 32;
	int addr = 0;
	int len = 0;
	int status;
	int i;
	unsigned char d8;
	unsigned short d16;
	unsigned int d32;

	/* d, d8, d16, d32 */
	if (cmd[1] == ' ')
	{
		status = sscanf(cmd, "%*c %x %x", &addr, &len);
		if (status != 2)
		{
			printf("Syntax error (use ? for help)\n");
			/* Don't break out of command processing loop */
			return 0;
		}
	}
	else
	{
		status = sscanf(cmd, "%*c%d %x %x", &width, &addr, &len);
		if (status != 3)
		{
			printf("Syntax error (use ? for help)\n");
			/* Don't break out of command processing loop */
			return 0;
		}
	}
	if (addr > dev->size)
	{
		printf("Error: invalid address (maximum allowed is %.8X\n", dev->size);
		return 0;
	}
	/* Length is in bytes */
	if ((addr + len) > dev->size)
	{
		/* Truncate */
		len = dev->size;
	}
	switch (width)
	{
	case 8:
		for (i = 0; i < len; i++)
		{
			if ((i % 16) == 0)
			{
				printf("\n%.8X: ", addr + i);
			}
			d8 = read_8(dev, addr + i);
			printf("%.2X ", d8);
		}
		printf("\n");
		break;
	case 16:
		for (i = 0; i < len; i += 2)
		{
			if ((i % 16) == 0)
			{
				printf("\n%.8X: ", addr + i);
			}
			if (big_endian == 0)
			{
				d16 = read_le16(dev, addr + i);
			}
			else
			{
				d16 = read_be16(dev, addr + i);
			}
			printf("%.4X ", d16);
		}
		printf("\n");
		break;
	case 32:
		for (i = 0; i < len; i += 4)
		{
			if ((i % 16) == 0)
			{
				printf("\n%.8X: ", addr + i);
			}
			if (big_endian == 0)
			{
				d32 = read_le32(dev, addr + i);
			}
			else
			{
				d32 = read_be32(dev, addr + i);
			}
			printf("%.8X ", d32);
		}
		printf("\n");
		break;
	default:
		printf("Syntax error (use ? for help)\n");
		/* Don't break out of command processing loop */
		break;
	}
	printf("\n");
	return 0;
}

int change_mem(device_t *dev, char *cmd)
{
	int width = 32;
	int addr = 0;
	int status;
	unsigned char d8;
	unsigned short d16;
	unsigned int d32;

	/* c, c8, c16, c32 */
	if (cmd[1] == ' ')
	{
		status = sscanf(cmd, "%*c %x %x", &addr, &d32);
		if (status != 2)
		{
			printf("Syntax error (use ? for help)\n");
			/* Don't break out of command processing loop */
			return 0;
		}
	}
	else
	{
		status = sscanf(cmd, "%*c%d %x %x", &width, &addr, &d32);
		if (status != 3)
		{
			printf("Syntax error (use ? for help)\n");
			/* Don't break out of command processing loop */
			return 0;
		}
	}
	if (addr > dev->size)
	{
		printf("Error: invalid address (maximum allowed is %.8X\n", dev->size);
		return 0;
	}
	switch (width)
	{
	case 8:
		d8 = (unsigned char)d32;
		write_8(dev, addr, d8);
		break;
	case 16:
		d16 = (unsigned short)d32;
		if (big_endian == 0)
		{
			write_le16(dev, addr, d16);
		}
		else
		{
			write_be16(dev, addr, d16);
		}
		break;
	case 32:
		if (big_endian == 0)
		{
			write_le32(dev, addr, d32);
		}
		else
		{
			write_be32(dev, addr, d32);
		}
		break;
	default:
		printf("Syntax error (use ? for help)\n");
		/* Don't break out of command processing loop */
		break;
	}
	return 0;
}

int fill_mem(device_t *dev, char *cmd)
{
	int width = 32;
	int addr = 0;
	int len = 0;
	int inc = 0;
	int status;
	int i;
	unsigned char d8;
	unsigned short d16;
	unsigned int d32;

	/* c, c8, c16, c32 */
	if (cmd[1] == ' ')
	{
		status = sscanf(cmd, "%*c %x %x %x %x", &addr, &d32, &len, &inc);
		if ((status != 3) && (status != 4))
		{
			printf("Syntax error (use ? for help)\n");
			/* Don't break out of command processing loop */
			return 0;
		}
		if (status == 3)
		{
			inc = 1;
		}
	}
	else
	{
		status = sscanf(cmd, "%*c%d %x %x %x %x", &width, &addr, &d32, &len, &inc);
		if ((status != 3) && (status != 4))
		{
			printf("Syntax error (use ? for help)\n");
			/* Don't break out of command processing loop */
			return 0;
		}
		if (status == 4)
		{
			inc = 1;
		}
	}
	if (addr > dev->size)
	{
		printf("Error: invalid address (maximum allowed is %.8X\n", dev->size);
		return 0;
	}
	/* Length is in bytes */
	if ((addr + len) > dev->size)
	{
		/* Truncate */
		len = dev->size;
	}
	switch (width)
	{
	case 8:
		for (i = 0; i < len; i++)
		{
			d8 = (unsigned char)(d32 + i * inc);
			write_8(dev, addr + i, d8);
		}
		break;
	case 16:
		for (i = 0; i < len / 2; i++)
		{
			d16 = (unsigned short)(d32 + i * inc);
			if (big_endian == 0)
			{
				write_le16(dev, addr + 2 * i, d16);
			}
			else
			{
				write_be16(dev, addr + 2 * i, d16);
			}
		}
		break;
	case 32:
		for (i = 0; i < len / 4; i++)
		{
			if (big_endian == 0)
			{
				write_le32(dev, addr + 4 * i, d32 + i * inc);
			}
			else
			{
				write_be32(dev, addr + 4 * i, d32 + i * inc);
			}
		}
		break;
	default:
		printf("Syntax error (use ? for help)\n");
		/* Don't break out of command processing loop */
		break;
	}
	return 0;
}

int change_endian(device_t *dev, char *cmd)
{
	char endian = 0;
	int status;

	/* e, el, eb */
	status = sscanf(cmd, "%*c%c", &endian);
	if (status < 0)
	{
		/* Display the current setting */
		if (big_endian == 0)
		{
			printf("Endian mode: little-endian\n");
		}
		else
		{
			printf("Endian mode: big-endian\n");
		}
		return 0;
	}
	else if (status == 1)
	{
		switch (endian)
		{
		case 'b':
			big_endian = 1;
			break;
		case 'l':
			big_endian = 0;
			break;
		default:
			printf("Syntax error (use ? for help)\n");
			/* Don't break out of command processing loop */
			break;
		}
	}
	else
	{
		printf("Syntax error (use ? for help)\n");
		/* Don't break out of command processing loop */
	}
	return 0;
}
#endif
// ***************************************************************************************
// Memory Init
// ***************************************************************************************
#if 0
int init_mem(device_t *dev)
{
	int addr = 0;
	int status = 0;
	int i;
	unsigned int d32;
	int base_address = 0x00000;

	unsigned int SM2_in_code[512] = {
		0x01000000, 0x01000100, 0x13000000, 0x0D0E1216, 0x0E3C2B40, 0x13000000, 0x0D1A2226, 0x0E3C2B40,
		0x13000000, 0x0D0E1A1E, 0x0E382B80, 0x13000000, 0x02383803, 0x02393903, 0x0F000038, 0x103C2C00,
		0x13000000, 0x113C383C, 0x13000000, 0x09000006, 0x09000108, 0x12003F3E, 0x0C36023E, 0x02373637,
		0x02363737, 0x02373736, 0x023C3C36, 0x023D3D37, 0x023C3C2A, 0x023D3D2A, 0x12003C3C, 0x12003D3D,
		0x13000000, 0x02363C03, 0x02373636, 0x02373637, 0x02363604, 0x05373736, 0x05373705, 0x02363D03,
		0x02363636, 0x12003737, 0x12003636, 0x07373736, 0x12003737, 0x13000000, 0x01000001, 0x01000101,
		0x13000000, 0x0900000A, 0x0900010C, 0x02362D01, 0x12013736, 0x0C360036, 0x02373637, 0x02372A37,
		0x13000000, 0x02312D2F, 0x02313101, 0x0531312E, 0x12012C31, 0x02313130, 0x02313101, 0x12013131,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x01000000, 0x01000100, 0x13000000, 0x0D0E1216, 0x0E3C2B40, 0x13000000, 0x0D1A2226, 0x0E3C2B40,
		0x13000000, 0x0D0E1A1E, 0x0E382B80, 0x13000000, 0x02383803, 0x02393903, 0x0F000038, 0x103C2C00,
		0x13000000, 0x113C383C, 0x13000000, 0x09000006, 0x09000108, 0x12003F3E, 0x0C36023E, 0x02373637,
		0x02363737, 0x02373736, 0x023C3C36, 0x023D3D37, 0x023C3C2A, 0x023D3D2A, 0x12003C3C, 0x12003D3D,
		0x13000000, 0x02363C03, 0x02373636, 0x02373637, 0x02363604, 0x05373736, 0x05373705, 0x02363D03,
		0x02363636, 0x12003737, 0x12003636, 0x07373736, 0x12003737, 0x13000000, 0x01000001, 0x01000101,
		0x13000000, 0x0900000A, 0x0900010C, 0x02362D01, 0x12013736, 0x0C360036, 0x02373637, 0x02372A37,
		0x13000000, 0x053C303C, 0x022F2F3C, 0x022F2F01, 0x0731312F, 0x02313137, 0x02313101, 0x12013C3C,
		0x12013131, 0x13000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

	unsigned int SM2_ex_code[256] = {
		0x07010000, 0x0F000001, 0x0C000000, 0x19000000, 0x0B010001, 0x0D010000, 0x19000000, 0x012A3B00,
		0x1C010200, 0x012B0100, 0x022B0200, 0x15000000, 0x17000000, 0x16000000, 0x18000000, 0x15060000,
		0x17030000, 0x16000000, 0x18000000, 0x04383C00, 0x04393D00, 0x043A3E00, 0x043B3F00, 0x15110000,
		0x16000000, 0x15130000, 0x16000000, 0x05013C00, 0x05023D00, 0x1B000000, 0x0F000037, 0x0C000000,
		0x19000000, 0x1C030400, 0x012B0300, 0x022B0400, 0x15000000, 0x17000000, 0x16000000, 0x18000000,
		0x15060000, 0x17030000, 0x16000000, 0x18000000, 0x04383C00, 0x04393D00, 0x043A3E00, 0x043B3F00,
		0x0B030101, 0x022D0300, 0x02310000, 0x022F0100, 0x02300200, 0x15110000, 0x16000000, 0x15130000,
		0x172E0000, 0x18000000, 0x17310000, 0x16000000, 0x18000000, 0x033C3C00, 0x17390000, 0x18000000,
		0x06033C00, 0x0C030000, 0x19000000, 0x0F000003, 0x0C000000, 0x19000000, 0x06043100, 0x1B000000,
		0x0C030000, 0x19000000, 0x0C040000, 0x19000000, 0x0D030000, 0x19000000, 0x0D040000, 0x19000000,
		0x0F050304, 0x0C050000, 0x19000000, 0x0806053C, 0x0807053D, 0x022C0600, 0x012C0700, 0x02380000,
		0x02390100, 0x01380000, 0x01390100, 0x012B0400, 0x15000000, 0x17000000, 0x16000000, 0x18000000,
		0x170C0000, 0x150C0000, 0x16000000, 0x15090000, 0x16000000, 0x15110000, 0x16000000, 0x18000000,
		0x04383C00, 0x04393D00, 0x043A3E00, 0x043B3F00, 0x15110000, 0x16000000, 0x012A3B00, 0x15130000,
		0x16000000, 0x05053C00, 0x0F050502, 0x12050503, 0x0C050000, 0x1A000000, 0x1B000000, 0x07030000,
		0x0F000300, 0x0C000000, 0x19000000, 0x0803003C, 0x0804003D, 0x022C0300, 0x012C0400, 0x02380100,
		0x02390200, 0x01380100, 0x01390200, 0x012B0000, 0x15000000, 0x17000000, 0x16000000, 0x18000000,
		0x170C0000, 0x150C0000, 0x16000000, 0x15090000, 0x16000000, 0x18000000, 0x03383C00, 0x03393D00,
		0x033A3E00, 0x033B3F00, 0x17110000, 0x1E3C3800, 0x1E3D3900, 0x1E3E3A00, 0x1E3F3B00, 0x012A3B00,
		0x18000000, 0x022A3B00, 0x15130000, 0x17130000, 0x16000000, 0x18000000, 0x17210000, 0x18000000,
		0x06033700, 0x0C030000, 0x1A000000, 0x05033C00, 0x05043D00, 0x06053C00, 0x06063D00, 0x1B000000,
		0x08030039, 0x0804003A, 0x022C0300, 0x012C0400, 0x02380100, 0x02390200, 0x01380100, 0x01390200,
		0x013C0100, 0x013D0200, 0x15000000, 0x17000000, 0x16000000, 0x18000000, 0x15210000, 0x16000000,
		0x05033700, 0x0C030000, 0x1A000000, 0x170C0000, 0x150C0000, 0x16000000, 0x18000000, 0x03383C00,
		0x03393D00, 0x033A3E00, 0x033B3F00, 0x17110000, 0x18000000, 0x022A3B00, 0x17130000, 0x18000000,
		0x06033C00, 0x06043D00, 0x1B000000, 0x023C0300, 0x023D0400, 0x013C0500, 0x013D0600, 0x15000000,
		0x17000000, 0x16000000, 0x18000000, 0x15210000, 0x17210000, 0x16000000, 0x18000000, 0x06073700,
		0x0C070000, 0x1A000000, 0x05073700, 0x0C070000, 0x1A000000, 0x08093F01, 0x09090938, 0x080A3F03,
		0x090A0A38, 0x012D0000, 0x012E0200, 0x012F0900, 0x01300A00, 0x152E0000, 0x16000000, 0x15390000,
		0x16000000, 0x032C3100, 0x01380500, 0x01390600, 0x02380300, 0x02390400, 0x15000000, 0x16000000,
		0x150C0000, 0x170C0000, 0x16000000, 0x18000000, 0x03383C00, 0x03393D00, 0x033A3E00, 0x033B3F00,
		0x17110000, 0x18000000, 0x022A3B00, 0x17130000, 0x18000000, 0x06073C00, 0x06083D00, 0x1B000000};

	unsigned int SM2_Data[512] = {
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00008020,
		0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF,
		0xFFFFFFFC, 0x00000001, 0xFFFFFFFE, 0x00000000, 0xFFFFFFFF, 0x00000001, 0x00000000, 0x00000001,
		0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x7203DF6B, 0x21C6052B, 0x53BBF409, 0x39D54123,
		0x6F39132F, 0x82E4C7BC, 0x2B0068D3, 0xB08941D4, 0xDF1E8D34, 0xFC8319A5, 0x327F9E88, 0x72350975,
		0x00000004, 0x00000000, 0x00000000, 0x00000002, 0x37F08253, 0x78E7EB52, 0xB1102FDB, 0x18AAFB74,
		0xEB5E412B, 0x22B3D3B6, 0x20FC84C3, 0xAFFE0D43, 0xD4412542, 0xC5342A7D, 0xAD5D36EE, 0x873FB0DD,
		0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0xFFFFFFFC, 0x00000000, 0x00000004,
		0x00000040, 0x00000020, 0x00000010, 0x00000010, 0x0000002F, 0xFFFFFFF0, 0x00000020, 0x00000030,
		0xFFFFFFF2, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFF3, 0x0000000C, 0xFFFFFFFF, 0xFFFFFFF3,
		0x903F8622, 0xE8838B21, 0x49E60541, 0x7A9470F1, 0xC73CDE6B, 0xA6D4DEAE, 0x4348C18C, 0xAF037508,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000600, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00155555, 0x55555555, 0x55555555, 0x2AAAAAAA, 0xAAAAAAAA,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000052A, 0xAAAAAAAA, 0xAAAAAAAA,
		0x55555555, 0x55555000, 0x00000555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555554,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x60000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x15555555, 0x55555555, 0x2AAAAAAA, 0xAAAAAAAA,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x50440924, 0xA8A88809,
		0xAA952A24, 0xA4890142, 0xA08A4A55, 0x4AA01152, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA,
		0x4459E97D, 0x8704EC17, 0x5A87B666, 0xB0930F0C, 0xF9E607B9, 0x729B013F, 0x84CA2643, 0xD0600A7A,
		0x8F359753, 0x075CD6F6, 0x3533EC19, 0xB8A923E3, 0x07D795E3, 0x34CA57EA, 0x04D53964, 0xF0B43775,
		0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0xFFFFFFFC, 0x00000000, 0x00000004,
		0xFFFFFFF2, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFF3, 0x0000000C, 0xFFFFFFFF, 0xFFFFFFF3,
		0xC09E1FE1, 0x4AA8A4FB, 0x34381262, 0xF203B2C1, 0x70407E79, 0x6437FECD, 0x2CCF8082, 0xEB60C348,
		0x2C25A5C8, 0xF788C9E7, 0x700E80E2, 0x244EC4A3, 0x3D73AF83, 0xF83B8DDD, 0xFF5933B4, 0x883E3F21,
		0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0xFFFFFFFC, 0x00000000, 0x00000004,
		0xFFFFFFF2, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFF3, 0x0000000C, 0xFFFFFFFF, 0xFFFFFFF3,
		0x4471A7C5, 0x4CCA7D66, 0x5D96C92F, 0x533CCF43, 0xDF48A731, 0x187B7E95, 0xD3BA1388, 0xE6836771,
		0xEC7A0511, 0x12D090E9, 0x18910872, 0x6609F928, 0xE91943C5, 0xE1FC3968, 0x04256EDE, 0x81EB8C59,
		0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0xFFFFFFFC, 0x00000000, 0x00000004,
		0xFFFFFFF2, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFF3, 0x0000000C, 0xFFFFFFFF, 0xFFFFFFF3,
		0x27994104, 0x5A2D3159, 0xA003DA59, 0x466D30D4, 0xAFDC4C3D, 0x45534DDA, 0xEE7D586A, 0x2A452D43,
		0x8D5B3D1B, 0x30599D1C, 0x7FFE78F5, 0x0158B97D, 0xE0684BDC, 0x88473877, 0xC1C1B801, 0xB6628CC5,
		0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0xFFFFFFFC, 0x00000000, 0x00000004,
		0xFFFFFFF2, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFF3, 0x0000000C, 0xFFFFFFFF, 0xFFFFFFF3,
		0x7006BDA7, 0x8C0FBC93, 0xE82F5B00, 0x1FDF3AD4, 0xE0976032, 0x9BDD6C35, 0x5B1393CE, 0x25D46365,
		0xBCD42201, 0x5A332E19, 0x464D8B1A, 0x54DCD086, 0x2FF9294D, 0x92373ABA, 0x75D73E79, 0xAFF2F249,
		0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0xFFFFFFFC, 0x00000000, 0x00000004,
		0xFFFFFFF2, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFF3, 0x0000000C, 0xFFFFFFFF, 0xFFFFFFF3,
		0x54910AD4, 0xFF79F940, 0x2D59190A, 0x1DC49448, 0x9527D593, 0xF5AEAE60, 0xECE64A90, 0x80AF78E7,
		0xE8242565, 0x095C3079, 0xB6C3B762, 0x41B7B231, 0x6C653C2C, 0x20ECA6F5, 0x9B3BF422, 0x8F4F85B9,
		0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0xFFFFFFFC, 0x00000000, 0x00000004,
		0xFFFFFFF2, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFF3, 0x0000000C, 0xFFFFFFFF, 0xFFFFFFF3,
		0xC5DB0BE9, 0x0D2E255F, 0xCC849156, 0x8CE4163B, 0x82B475E7, 0x16B51C1E, 0xC95379BA, 0x017C3B63,
		0x103AAF68, 0x6F39DEDB, 0x99816AA0, 0x36995785, 0xEBCDEAD9, 0xAA98A39F, 0xA90A9A4A, 0x458BFE12,
		0x00000004, 0x00000000, 0x00000000, 0x00000000, 0x00000003, 0xFFFFFFFC, 0x00000000, 0x00000004,
		0xFFFFFFF2, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFF3, 0x0000000C, 0xFFFFFFFF, 0xFFFFFFF3,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001,
		0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555,
		0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555,
		0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555,
		0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555,
		0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555,
		0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555,
		0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80000000, 0x00000000, 0x00000000, 0x00000000,
		0xffffffff, 0xffffffff, 0xfC000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x03ffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xfff00000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000fffff, 0xffffffff,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};

	printf("Memory initialization!\n");

	// Soft Reset
	d32 = 0x55550000;
	addr = base_address + 0x300 * 4; // @0x300

	if (big_endian == 0)
	{
		write_le32(dev, addr, d32);
	}
	else
	{
		write_be32(dev, addr, d32);
	}

	// SM2_in_code
	for (i = 0; i < 512; i++)
	{
		d32 = SM2_in_code[i];
		addr = base_address + 0x000 * 4 + 4 * i;

		if (big_endian == 0)
		{
			write_le32(dev, addr, d32);
		}
		else
		{
			write_be32(dev, addr, d32);
		}

		// readback
		if (big_endian == 0)
		{
			d32 = read_le32(dev, addr);
		}
		else
		{
			d32 = read_be32(dev, addr);
		}

		// Check
		if (d32 != SM2_in_code[i])
			printf("SM2_in_code write error: index is %.8X, Expected data is %.8X, Actual data is %.8X\n", i, SM2_in_code[i], d32);
	}

	printf("\n");

	// SM2_ex_code
	for (i = 0; i < 256; i++)
	{
		d32 = SM2_ex_code[i];
		addr = base_address + 0x0200 * 4 + 4 * i;

		if (big_endian == 0)
		{
			write_le32(dev, addr, d32);
		}
		else
		{
			write_be32(dev, addr, d32);
		}

		// readback
		if (big_endian == 0)
		{
			d32 = read_le32(dev, addr);
		}
		else
		{
			d32 = read_be32(dev, addr);
		}

		// Check
		if (d32 != SM2_ex_code[i])
			printf("SM2_ex_code write error: index is %.8X, Expected data is %.8X, Actual data is %.8X\n", i, SM2_ex_code[i], d32);
	}

	// SM2 command Registers
	d32 = 0x00000010;
	addr = base_address + 0x300 * 4; // @0x300

	if (big_endian == 0)
	{
		write_le32(dev, addr, d32);
	}
	else
	{
		write_be32(dev, addr, d32);
	}

	// sleep
	sleep(2);
	printf("sleep 2s!\n");

	// SM2_Data
	for (i = 0; i < 512; i++)
	{
		d32 = SM2_Data[i];
		addr = base_address + 0x000 * 4 + 4 * i;

		if (big_endian == 0)
		{
			write_le32(dev, addr, d32);
		}
		else
		{
			write_be32(dev, addr, d32);
		}
	}

	// SM2 command Registers
	d32 = 0x00000020;
	addr = base_address + 0x300 * 4; // @0x300

	if (big_endian == 0)
	{
		write_le32(dev, addr, d32);
	}
	else
	{
		write_be32(dev, addr, d32);
	}

	// sleep
	sleep(2);
	printf("sleep 2s!\n");

	return 0;
}

// ***********************************************************************
// test_mem
// ***********************************************************************
int test_mem(device_t *dev, char *cmd)
{
	int status;
	int i;

	int loopcnt = 1;
	int addr = 0;
	int base_address = 0x00000;
	unsigned int wd32;
	unsigned int rd32;

	/* c, c8, c16, c32 */
	if (cmd[1] == ' ')
	{
		status = sscanf(cmd, "%*c %x %d", &base_address, &loopcnt);
		if (status != 2)
		{
			printf("Syntax error (use ? for help)\n");
			/* Don't break out of command processing loop */
			return 0;
		}
	}
	else
	{
		printf("Syntax error (use ? for help)\n");
		/* Don't break out of command processing loop */
		return 0;
	}

	while (loopcnt >= 1)
	{

		// SM2 message buffer
		for (i = 0; i < 512; i++)
		{
			wd32 = rand();
			addr = base_address + 0x000 * 4 + 4 * i;

			if (big_endian == 0)
			{
				write_le32(dev, addr, wd32);
			}
			else
			{
				write_be32(dev, addr, wd32);
			}

			// readback
			if (big_endian == 0)
			{
				rd32 = read_le32(dev, addr);
			}
			else
			{
				rd32 = read_be32(dev, addr);
			}

			// Check
			if (rd32 != wd32)
				printf("SM2 data buffer R/W error: index is %.8X, Expected data is %.8X, Actual data is %.8X\n", i, wd32, rd32);
		}

		// SM2 message buffer
		for (i = 0; i < 256; i++)
		{
			wd32 = rand();
			addr = base_address + 0x200 * 4 + 4 * i;

			if (big_endian == 0)
			{
				write_le32(dev, addr, wd32);
			}
			else
			{
				write_be32(dev, addr, wd32);
			}

			// readback
			if (big_endian == 0)
			{
				rd32 = read_le32(dev, addr);
			}
			else
			{
				rd32 = read_be32(dev, addr);
			}

			// Check
			if (rd32 != wd32)
				printf("SM2 Instruction buffer R/W: index is %.8X, Expected data is %.8X, Actual data is %.8X\n", i, wd32, rd32);
		}

		// SM3 message buffer
		for (i = 0; i < 256; i++)
		{
			wd32 = rand();
			addr = base_address + 0x400 * 4 + 4 * i;

			if (big_endian == 0)
			{
				write_le32(dev, addr, wd32);
			}
			else
			{
				write_be32(dev, addr, wd32);
			}

			// readback
			if (big_endian == 0)
			{
				rd32 = read_le32(dev, addr);
			}
			else
			{
				rd32 = read_be32(dev, addr);
			}

			// Check
			if (rd32 != wd32)
				printf("SM3 data buffer R/W error: index is %.8X, Expected data is %.8X, Actual data is %.8X\n", i, wd32, rd32);
		}

		/*
		// SM3 Hash buffer
		for (i = 0; i < 8; i++) {
			wd32 	= rand();
			addr 	= base_address + 0x600*4 + 4*i		;
			
			if (big_endian == 0) {
				write_le32(dev, addr, wd32);
			} else {
				write_be32(dev, addr, wd32);
			}
		
		// readback
			if (big_endian == 0) {
				rd32 = read_le32(dev, addr);
			} else {
				rd32 = read_be32(dev, addr);
			}
			
		// Check
		if(rd32!=wd32)
			printf("SM3 Hash R/W error: index is %.8X, Expected data is %.8X, Actual data is %.8X\n", i, wd32,rd32);
		}
*/

		loopcnt = loopcnt - 1;
	}

	return 0;
}

// ***********************************************************************
// Verification
// ***********************************************************************
int verify_mem(device_t *dev, char *cmd)
{
	int addr = 0;
	int status = 0;
	int i;
	unsigned int wd32;
	unsigned int rd32;

	int err = 0;
	unsigned int msg[128];
	unsigned int rslt[128];

	int base_address = 0x00000;

	if (cmd[1] == ' ')
	{
		status = sscanf(cmd, "%*c %x", &base_address);
		if (status != 1)
		{
			printf("Syntax error (use ? for help)\n");
			/* Don't break out of command processing loop */
			return 0;
		}
	}
	else
	{
		printf("Syntax error (use ? for help)\n");
		/* Don't break out of command processing loop */
		return 0;
	}

	printf("Function Verification ....\n");

	//
	{
		printf("SM3 Hash verification is running .... \n");

		msg[0] = 0x6626437D;
		msg[1] = 0x8870B3E5;
		msg[2] = 0xF4184985;
		msg[3] = 0xF14CE201;
		msg[4] = 0x7F8B2D2D;
		msg[5] = 0x1E2E6BA3;
		msg[6] = 0xE94212DA;
		msg[7] = 0xA0B61DBE;
		msg[8] = 0x31D79A11;
		msg[9] = 0x8C33CD18;
		msg[10] = 0xE3DED5B2;
		msg[11] = 0x4FBD2892;
		msg[12] = 0xFEF95394;
		msg[13] = 0xECA251D1;
		msg[14] = 0x6C4D39D2;
		msg[15] = 0x151450DF;
		msg[16] = 0x80000000;
		msg[17] = 0x00000000;
		msg[18] = 0x00000000;
		msg[19] = 0x00000000;
		msg[20] = 0x00000000;
		msg[21] = 0x00000000;
		msg[22] = 0x00000000;
		msg[23] = 0x00000000;
		msg[24] = 0x00000000;
		msg[25] = 0x00000000;
		msg[26] = 0x00000000;
		msg[27] = 0x00000000;
		msg[28] = 0x00000000;
		msg[29] = 0x00000000;
		msg[30] = 0x00000000;
		msg[31] = 0x00000200;

		rslt[0] = 0xA487F065;
		rslt[1] = 0xFD524E95;
		rslt[2] = 0x38327A44;
		rslt[3] = 0x564928B7;
		rslt[4] = 0x6723B43A;
		rslt[5] = 0x7942CE93;
		rslt[6] = 0x9EFD265D;
		rslt[7] = 0x438ED406;

		// Message
		for (i = 0; i < 32; i++)
		{
			wd32 = msg[i];
			addr = base_address + 0x400 * 4 + 4 * i;
			write_le32(dev, addr, wd32);
		}

		// Command
		wd32 = 0x00000209;
		addr = base_address + 0x700 * 4;
		write_le32(dev, addr, wd32);

		// Check Busy Bit
		sleep(1);

		rd32 = read_le32(dev, addr);
		if (rd32 & 0x00000001)
		{
			printf("SM3: the busy bit is set %.8x", rd32);
		}

		// Hash
		err = 0;
		for (i = 0; i < 8; i++)
		{
			addr = base_address + 0x600 * 4 + 4 * i;
			rd32 = read_le32(dev, addr);

			if (rd32 != rslt[i])
			{
				err = 1;
				printf("SM3 result error: index is %.8X, Expected data is %.8X, Actual data is %.8X\n", i, rslt[i], rd32);
			}
		}

		if (err == 0)
			printf("SM3 works successfully! \n");
	}

	// ***********************************************************************
	//
	// ***********************************************************************
	{
		printf("SM3 Hash verification is running .... \n");

		msg[0] = 0x6626437D;
		msg[1] = 0x8870B3E5;
		msg[2] = 0xF4184985;
		msg[3] = 0xF14CE201;
		msg[4] = 0x7F8B2D2D;
		msg[5] = 0x1E2E6BA3;
		msg[6] = 0xE94212DA;
		msg[7] = 0xA0B61DBE;
		msg[8] = 0x31D79A11;
		msg[9] = 0x8C33CD18;
		msg[10] = 0xE3DED5B2;
		msg[11] = 0x4FBD2892;
		msg[12] = 0xFEF95394;
		msg[13] = 0xECA251D1;
		msg[14] = 0x6C4D39D2;
		msg[15] = 0x151450DF;
		msg[16] = 0x80000000;
		msg[17] = 0x00000000;
		msg[18] = 0x00000000;
		msg[19] = 0x00000000;
		msg[20] = 0x00000000;
		msg[21] = 0x00000000;
		msg[22] = 0x00000000;
		msg[23] = 0x00000000;
		msg[24] = 0x00000000;
		msg[25] = 0x00000000;
		msg[26] = 0x00000000;
		msg[27] = 0x00000000;
		msg[28] = 0x00000000;
		msg[29] = 0x00000000;
		msg[30] = 0x00000000;
		msg[31] = 0x00000200;

		rslt[0] = 0xA487F065;
		rslt[1] = 0xFD524E95;
		rslt[2] = 0x38327A44;
		rslt[3] = 0x564928B7;
		rslt[4] = 0x6723B43A;
		rslt[5] = 0x7942CE93;
		rslt[6] = 0x9EFD265D;
		rslt[7] = 0x438ED406;

		// Message
		for (i = 0; i < 32; i++)
		{
			wd32 = msg[i];
			addr = base_address + 0x400 * 4 + 4 * i;
			write_le32(dev, addr, wd32);
		}

		// Command
		wd32 = 0x00000209;
		addr = base_address + 0x700 * 4;
		write_le32(dev, addr, wd32);

		// Check Busy Bit
		sleep(1);

		rd32 = read_le32(dev, addr);
		if (rd32 & 0x00000001)
		{
			printf("SM3: the busy bit is set %.8x", rd32);
		}

		// Hash
		err = 0;
		for (i = 0; i < 8; i++)
		{
			addr = base_address + 0x600 * 4 + 4 * i;
			rd32 = read_le32(dev, addr);

			if (rd32 != rslt[i])
			{
				err = 1;
				printf("SM3 result error: index is %.8X, Expected data is %.8X, Actual data is %.8X\n", i, rslt[i], rd32);
			}
		}

		if (err == 0)
			printf("SM3 works successfully! \n");
	}

	return 0;
}

// ***********************************************************************
// SM2_Sign Verification
// ***********************************************************************
int SM2_Sign(device_t *dev)
{
	int addr = 0;
	int i;
	unsigned int wd32;
	unsigned int rd32;

	int err = 0;
	unsigned int msg[128];
	unsigned int rslt[128];
	unsigned int rdata[128];

	int base_address = 0x00000;

	//
	// printf("SM2_Sign verification is running .... \n");

	msg[0] = 0x12345678;
	msg[1] = 0x12345678;
	msg[2] = 0x12345678;
	msg[3] = 0x12345678;
	msg[4] = 0x12345678;
	msg[5] = 0x12345678;
	msg[6] = 0x12345678;
	msg[7] = 0x12345678;
	msg[8] = 0x3ea27606;
	msg[9] = 0xca83bffa;
	msg[10] = 0x3b946b0c;
	msg[11] = 0xdb8ff076;
	msg[12] = 0xe4dd3d1a;
	msg[13] = 0xaf2bd6e2;
	msg[14] = 0x7290cefd;
	msg[15] = 0xc4365cf6;
	msg[16] = 0xfd93ea51;
	msg[17] = 0x6080a881;
	msg[18] = 0x1b3a16ff;
	msg[19] = 0x5f465ff7;
	msg[20] = 0x2a1c94b6;
	msg[21] = 0xa55f6fa5;
	msg[22] = 0xcb7bd2b2;
	msg[23] = 0x501023c6;

	rslt[0] = 0xFEC0CC3D;
	rslt[1] = 0xCF0FF0F4;
	rslt[2] = 0x151E84FF;
	rslt[3] = 0x352E5AE9;
	rslt[4] = 0x7E85877A;
	rslt[5] = 0x41F811E9;
	rslt[6] = 0xAA0D9EE9;
	rslt[7] = 0x0F96BA4E;
	rslt[8] = 0x30306453;
	rslt[9] = 0x3A131EB5;
	rslt[10] = 0x5A98979C;
	rslt[11] = 0xFEA4C6E8;
	rslt[12] = 0x2B373791;
	rslt[13] = 0x0134695C;
	rslt[14] = 0x3AD16748;
	rslt[15] = 0x26356802;

	// Message
	//		for (i = 0; i < 24; i++) {
	//			wd32 	= msg[i]	;
	//			addr 	= base_address + 0x000*4 + 4*i	;
	//			write_le32(dev, addr, wd32);
	//		}

	// Message
	base_address = 0x00000;
	addr = base_address + 0x000 * 4;
	memcpy(dev->addr + addr, msg, sizeof(unsigned int) * 24);
	msync((void *)(dev->addr + addr), sizeof(unsigned int) * 24, MS_SYNC | MS_INVALIDATE);

	// Command
	wd32 = 0x00001e01;
	addr = base_address + 0x300 * 4;
	write_le32(dev, addr, wd32);

	// Message
	base_address = 0x4000;
	addr = base_address + 0x000 * 4;
	memcpy(dev->addr + addr, msg, sizeof(unsigned int) * 24);
	msync((void *)(dev->addr + addr), sizeof(unsigned int) * 24, MS_SYNC | MS_INVALIDATE);

	// Command
	wd32 = 0x00001e01;
	addr = base_address + 0x300 * 4;
	write_le32(dev, addr, wd32);

	// Check Busy Bit
	base_address = 0x0000;
	addr = base_address + 0x301 * 4;
	rd32 = read_le32(dev, addr);

	while ((rd32 & 0x00000001) == 0x1)
	{
		rd32 = read_le32(dev, addr);
		// printf("waiting ebusy...\n");
	}

	// result
	addr = base_address + 0x000 * 4 + 24 * 4;
	memcpy(rdata, dev->addr + addr, sizeof(unsigned int) * 16);

	err = 0;
	for (i = 0; i < 16; i++)
	{
		// printf("rdata: %.8x\n", rdata[i]);
		if (rdata[i] != rslt[i])
		{
			err = 1;
			printf("SM2_Sign result error: index is %.8X, Expected data is %.8X, Actual data is %.8X\n", i, rslt[i], rdata[i]);
		}
	}

	// // Check Busy Bit
	// base_address  	= 0x4000;
	// addr 			= base_address + 0x301*4 			;
	// rd32 			= read_le32(dev, addr);

	// while((rd32&0x00000001)==0x1){
	// 	rd32 = read_le32(dev, addr);
	// }

	// // result
	// addr 	= base_address + 0x000*4 + 24*4 ;
	// memcpy(rdata, dev->addr + addr, sizeof(unsigned int) * 16);

	// for (i = 0; i < 16; i++) {
	// 	if(rdata[i]!=rslt[i]){
	// 	err = 1;
	// 	printf("SM2_Sign result error: index is %.8X, Expected data is %.8X, Actual data is %.8X\n", i, rslt[i],rdata[i]);
	// 	}
	// }

	if (err != 0)
	{
		printf("Fatal Error: SM2_Sign\n");
		exit(1);
	}

	return 0;
}

// ***********************************************************************
// SM2_Verify
// ***********************************************************************
int SM2_Verify(device_t *dev)
{
	int addr = 0;
	int i;
	unsigned int wd32;
	unsigned int rd32;
	unsigned int msg[128];

	int base_address = 0x00000;

	//

	msg[0] = 0xaeec7b42;
	msg[1] = 0xb9b67ee4;
	msg[2] = 0x106a5695;
	msg[3] = 0x1bfdd0da;
	msg[4] = 0x8d1038d3;
	msg[5] = 0xef5b308b;
	msg[6] = 0x1354ce6f;
	msg[7] = 0x43caf93a;
	msg[8] = 0x1a37a2c4;
	msg[9] = 0x5bfd14a4;
	msg[10] = 0x438410e3;
	msg[11] = 0x48ae543f;
	msg[12] = 0x60b047b8;
	msg[13] = 0x7f75c8bd;
	msg[14] = 0xabc4bf77;
	msg[15] = 0xcabb953a;

	msg[16] = 0xfd93ea51;
	msg[17] = 0x6080a881;
	msg[18] = 0x1b3a16ff;
	msg[19] = 0x5f465ff7;
	msg[20] = 0x2a1c94b6;
	msg[21] = 0xa55f6fa5;
	msg[22] = 0xcb7bd2b2;
	msg[23] = 0x501023c6;

	msg[24] = 0xFEC0CC3D;
	msg[25] = 0xCF0FF0F4;
	msg[26] = 0x151E84FF;
	msg[27] = 0x352E5AE9;
	msg[28] = 0x7E85877A;
	msg[29] = 0x41F811E9;
	msg[30] = 0xAA0D9EE9;
	msg[31] = 0x0F96BA4E;
	msg[32] = 0x30306453;
	msg[33] = 0x3A131EB5;
	msg[34] = 0x5A98979C;
	msg[35] = 0xFEA4C6E8;
	msg[36] = 0x2B373791;
	msg[37] = 0x0134695C;
	msg[38] = 0x3AD16748;
	msg[39] = 0x26356802;

	// Message
	//		for (i = 0; i < 40; i++) {
	//			wd32 	= msg[i]	;
	//			addr 	= base_address + 0x000*4 + 4*i	;
	//			write_le32(dev, addr, wd32);
	//		}

	addr = base_address + 0x000 * 4;

	memcpy(dev->addr + addr, msg, sizeof(unsigned int) * 40);

	msync((void *)(dev->addr + addr), sizeof(unsigned int) * 40, MS_SYNC | MS_INVALIDATE);

	// Command
	wd32 = 0x00004801;
	addr = base_address + 0x300 * 4;
	write_le32(dev, addr, wd32);

	// Check Busy Bit
	addr = base_address + 0x301 * 4;
	rd32 = read_le32(dev, addr);

	while ((rd32 & 0x00000001) == 0x1)
	{
		rd32 = read_le32(dev, addr);
	}

	// Hash
	if ((rd32 & 0x00000002) == 0x2)
	{
		printf("Fatal Error: SM2_Verify\n");
		exit(1);
	}

	return 0;
}
#endif

/* ----------------------------------------------------------------
 * Raw pointer read/write access
 * ----------------------------------------------------------------
 */
static void write_8(device_t *dev, unsigned int addr, unsigned char data)
{
	*(volatile unsigned char *)(dev->addr + addr) = data;
	msync((void *)(dev->addr + addr), 1, MS_SYNC | MS_INVALIDATE);
}

static unsigned char read_8(device_t *dev, unsigned int addr)
{
	return *(volatile unsigned char *)(dev->addr + addr);
}

static void write_le16(device_t *dev, unsigned int addr, unsigned short int data)
{
	if (__BYTE_ORDER != __LITTLE_ENDIAN)
	{
		data = bswap_16(data);
	}
	*(volatile unsigned short int *)(dev->addr + addr) = data;
	msync((void *)(dev->addr + addr), 2, MS_SYNC | MS_INVALIDATE);
}

static unsigned short int read_le16(device_t *dev, unsigned int addr)
{
	unsigned int data = *(volatile unsigned short int *)(dev->addr + addr);
	if (__BYTE_ORDER != __LITTLE_ENDIAN)
	{
		data = bswap_16(data);
	}
	return data;
}

static void write_be16(device_t *dev, unsigned int addr, unsigned short int data)
{
	if (__BYTE_ORDER == __LITTLE_ENDIAN)
	{
		data = bswap_16(data);
	}
	*(volatile unsigned short int *)(dev->addr + addr) = data;
	msync((void *)(dev->addr + addr), 2, MS_SYNC | MS_INVALIDATE);
}

static unsigned short intread_be16(device_t *dev, unsigned int addr)
{
	unsigned int data = *(volatile unsigned short int *)(dev->addr + addr);
	if (__BYTE_ORDER == __LITTLE_ENDIAN)
	{
		data = bswap_16(data);
	}
	return data;
}

void write_le32(device_t *dev, unsigned int addr, unsigned int data)
{
	if (__BYTE_ORDER != __LITTLE_ENDIAN)
	{
		data = bswap_32(data);
	}
	*(volatile unsigned int *)(dev->addr + addr) = data;
	msync((void *)(dev->addr + addr), 4, MS_SYNC | MS_INVALIDATE);
}

unsigned int read_le32(device_t *dev, unsigned int addr)
{
	unsigned int data = *(volatile unsigned int *)(dev->addr + addr);
	if (__BYTE_ORDER != __LITTLE_ENDIAN)
	{
		data = bswap_32(data);
	}
	return data;
}

static void write_be32(device_t *dev, unsigned int addr, unsigned int data)
{
	if (__BYTE_ORDER == __LITTLE_ENDIAN)
	{
		data = bswap_32(data);
	}
	*(volatile unsigned int *)(dev->addr + addr) = data;
	msync((void *)(dev->addr + addr), 4, MS_SYNC | MS_INVALIDATE);
}

static unsigned int read_be32(device_t *dev, unsigned int addr)
{
	unsigned int data = *(volatile unsigned int *)(dev->addr + addr);
	if (__BYTE_ORDER == __LITTLE_ENDIAN)
	{
		data = bswap_32(data);
	}
	return data;
}
