#include "libHSM2.h"

void sign_test()
{
    device_t *dev;
    open_device(&dev);
    SM2_Init(dev, BASE_ADDR1);
    U32 rand[8] = {0x12345678, 0x12345678, 0x12345678, 0x12345678,
                   0x12345678, 0x12345678, 0x12345678, 0x12345678};
    U32 pri_key[8] = {0x3ea27606, 0xca83bffa, 0x3b946b0c, 0xdb8ff076,
                      0xe4dd3d1a, 0xaf2bd6e2, 0x7290cefd, 0xc4365cf6};
    U32 hash[8] = {0xfd93ea51, 0x6080a881, 0x1b3a16ff, 0x5f465ff7,
                   0x2a1c94b6, 0xa55f6fa5, 0xcb7bd2b2, 0x501023c6};
    U32 sign[16];
    int flag = SM2_Sign(dev, BASE_ADDR0, rand, pri_key, hash, sign);
    printf("sign result: ");
    for (int i = 0; i < 16; i++)
    {
        printf("0x%.8x ", sign[i]);
    }
    printf("\n");
    close_device(dev);
}

int main(void)
{

    sign_test();
    return 0;
}

int open_device(device_t **dev)
{
    *dev = (device_t *)malloc(sizeof(device_t));
    memset(*dev, 0, sizeof(device_t));

    struct pci_access *pacc;
    struct pci_dev *dev_t;

    pacc = pci_alloc(); // get the pci_access structure
    pci_init(pacc);     // initialize the pci library
    pci_scan_bus(pacc); // get the list of devices

    for (dev_t = pacc->devices;; dev_t = dev_t->next)
    {
        pci_fill_info(dev_t, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS); // fill in header info we need
        if (dev_t->vendor_id == 0x10ee && dev_t->device_id == 0x7024)
        {
            (*dev)->bus = dev_t->bus;
            (*dev)->slot = dev_t->dev;
            (*dev)->function = dev_t->func;
            printf("%04x:%02x:%02x.%d vendor=%04x device=%04x class=%04x irq=%d base0=%lx\n",
                   dev_t->domain, dev_t->bus, dev_t->dev, dev_t->func, dev_t->vendor_id, dev_t->device_id,
                   dev_t->device_class, dev_t->irq, (long)dev_t->base_addr[0]);
            break;
        }
    }
    pci_cleanup(pacc); // close

    // Convert to a sysfs resource filename and open the resource
    snprintf((*dev)->filename, 99, "/sys/bus/pci/devices/%04x:%02x:%02x.%1x/resource%d",
             (*dev)->domain, (*dev)->bus, (*dev)->slot, (*dev)->function, (*dev)->bar);
    // snprintf((*dev)->filename, 99, "/sys/bus/pci/devices/%04x:%02x:%02x.%1x/resource%d",
    //          0x0000, 0x07, 0x00, 0x00, 0x0);
    printf("fd: %s\n", (*dev)->filename);

    (*dev)->fd = open((*dev)->filename, O_RDWR | O_SYNC);
    if ((*dev)->fd < 0)
    {
        printf("Open failed for file '%s': errno %d, %s\n",
               (*dev)->filename, errno, strerror(errno));
        return -1;
    }

    // PCI memory size
    struct stat statbuf;
    int status = fstat((*dev)->fd, &statbuf);
    if (status < 0)
    {
        printf("fstat() failed: errno %d, %s\n", errno, strerror(errno));
        return -1;
    }
    (*dev)->size = statbuf.st_size;

    // Map
    (*dev)->maddr = (U8 *)mmap(
        NULL, (size_t)((*dev)->size), PROT_READ | PROT_WRITE, MAP_SHARED, (*dev)->fd, 0);
    if ((*dev)->maddr == (U8 *)MAP_FAILED)
    {
        //		printf("failed (mmap returned MAP_FAILED)\n");
        printf("BARs that are I/O ports are not supported by this tool\n");
        (*dev)->maddr = 0;
        close((*dev)->fd);
        return -1;
    }

    /* Device regions smaller than a 4k page in size can be offset
	 * relative to the mapped base address. The offset is
	 * the physical address modulo 4k
	 */

    char configname[100];
    int fd;

    // snprintf(configname, 99, "/sys/bus/pci/devices/%04x:%02x:%02x.%1x/config",
    //          (*dev)->domain, (*dev)->bus, (*dev)->slot, (*dev)->function);
    snprintf(configname, 99, "/sys/bus/pci/devices/%04x:%02x:%02x.%1x/config",
             0x0000, 0x07, 0x00, 0x00);
    fd = open(configname, O_RDWR | O_SYNC);
    if ((*dev)->fd < 0)
    {
        printf("Open failed for file '%s': errno %d, %s\n",
               configname, errno, strerror(errno));
        return -1;
    }

    status = lseek(fd, 0x10 + 4 * (*dev)->bar, SEEK_SET);
    if (status < 0)
    {
        printf("Error: configuration space lseek failed\n");
        close(fd);
        return -1;
    }
    status = read(fd, &((*dev)->phys), 4);
    if (status < 0)
    {
        printf("Error: configuration space read failed\n");
        close(fd);
        return -1;
    }
    (*dev)->offset = (((*dev)->phys & 0xFFFFFFF0) % 0x1000);
    (*dev)->addr = (*dev)->maddr + (*dev)->offset;
    close(fd);
    printf("device opened!\n");
    return 0;
}

void close_device(device_t *dev)
{
    munmap(dev->maddr, dev->size);
    close(dev->fd);
    free(dev);
}

void SM2_Init(device_t *dev, U32 base_addr)
{
    /**
     * @description: init HSM2
     * @param: 
     *          dev - pcie device
     * @return: none
     */

    U32 addr, d32;

    U32 SM2_in_code[512] = {
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

    U32 SM2_ex_code[256] = {
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

    U32 SM2_Data[512] = {
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

    // Soft reset command
    d32 = CMD_SOFTRST;
    addr = base_addr + CMD_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, &d32, sizeof(U32) * 1);
    msync((void *)(dev->addr + addr), sizeof(U32) * 1, MS_SYNC | MS_INVALIDATE);

    // SM2_in_code
    addr = base_addr + DATA_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, SM2_in_code, sizeof(U32) * 512);
    msync((void *)(dev->addr + addr), sizeof(U32) * 512, MS_SYNC | MS_INVALIDATE);

    // SM2_ex_code
    addr = base_addr + PARAM_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, SM2_in_code, sizeof(U32) * 256);
    msync((void *)(dev->addr + addr), sizeof(U32) * 256, MS_SYNC | MS_INVALIDATE);

    // Init command 1
    d32 = CMD_INIT1;
    addr = base_addr + CMD_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, &d32, sizeof(U32) * 1);
    msync((void *)(dev->addr + addr), sizeof(U32) * 1, MS_SYNC | MS_INVALIDATE);
    // sleep(2);

    // Wait for EBUSY signal
    addr = base_addr + STATE_ADDR * sizeof(U32);
    d32 = read_le32(dev, addr);
    while (d32 & 1)
    {
        d32 = read_le32(dev, addr);
    }

    // SM2_Data
    addr = base_addr + DATA_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, SM2_Data, sizeof(U32) * 512);
    msync((void *)(dev->addr + addr), sizeof(U32) * 512, MS_SYNC | MS_INVALIDATE);

    // Init command 2
    d32 = CMD_INIT2;
    addr = base_addr + CMD_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, &d32, sizeof(U32) * 1);
    msync((void *)(dev->addr + addr), sizeof(U32) * 1, MS_SYNC | MS_INVALIDATE);

    // Wait for EBUSY signal
    addr = base_addr + STATE_ADDR * sizeof(U32);
    d32 = read_le32(dev, addr);
    while (d32 & 1)
    {
        d32 = read_le32(dev, addr);
    }
    printf("Initialization finished!\n");
}

int SM2_GenKey(device_t *dev, U32 base_addr, U32 *rand, U32 *pri_key, U32 *pub_key)
{
    /**
     * @description: generate key pair
     * @param: 
     *          dev - pcie device
     *          rand - random number sequence, 32 * 8
     *          pri_key - private key, 32 * 8
     *          pub_key - public key, 32 * 16 
     * @return: int
     *          0 - success
     *          1 - random number false 
     */

    U32 addr, d32;
    // write random number sequence
    addr = base_addr + DATA_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, rand, sizeof(U32) * 8);
    msync((void *)(dev->addr + addr), sizeof(U32) * 8, MS_SYNC | MS_INVALIDATE);

    // write start command
    d32 = CMD_GENKEY;
    addr = base_addr + CMD_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, &d32, sizeof(U32) * 1);
    msync((void *)(dev->addr + addr), sizeof(U32) * 1, MS_SYNC | MS_INVALIDATE);

    // Wait for EBUSY signal
    addr = base_addr + STATE_ADDR * sizeof(U32);
    d32 = read_le32(dev, addr);
    while (d32 & 1)
    {
        d32 = read_le32(dev, addr);
    }
    int check = d32 & 2;

    // read private key
    addr = base_addr + DATA_ADDR * sizeof(U32);
    memcpy(pri_key, dev->addr + addr, sizeof(U32) * 8);

    // read public key
    addr += sizeof(U32) * 8;
    memcpy(pub_key, dev->addr + addr, sizeof(U32) * 16);

    return check;
}

int SM2_Sign(device_t *dev, U32 base_addr, U32 *rand, U32 *pri_key, U32 *hash, U32 *sign)
{
    /**
     * @description: signature
     * @param: 
     *          dev - pcie device
     *          rand - random number sequence, 32 * 8
     *          pri_key - private key, 32 * 8
     *          hash - hash value, 32 * 8
     *          sign - sign result(r, s), 32 * 16 
     * @return: int
     *          0 - success
     *          1 - random number false or
     *              r = 0 mod n or
     *              r + k = 0 mod n
     */
    U32 addr, d32;
    // write rand, pri_key, hash
    addr = base_addr + DATA_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, rand, sizeof(U32) * 8);                      // random number sequence
    memcpy(dev->addr + addr + sizeof(U32) * 8, pri_key, sizeof(U32) * 8); // private key
    memcpy(dev->addr + addr + sizeof(U32) * 16, hash, sizeof(U32) * 8);   // hash
    msync((void *)(dev->addr + addr), sizeof(U32) * 24, MS_SYNC | MS_INVALIDATE);

    // write start command
    d32 = CMD_SIGN;
    addr = base_addr + CMD_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, &d32, sizeof(U32) * 1);
    msync((void *)(dev->addr + addr), sizeof(U32) * 1, MS_SYNC | MS_INVALIDATE);

    // Wait for EBUSY signal
    addr = base_addr + STATE_ADDR * sizeof(U32);
    d32 = read_le32(dev, addr);
    while (d32 & 1)
    {
        d32 = read_le32(dev, addr);
    }
    int check = d32 & 2;

    // read sign result
    addr = base_addr + DATA_ADDR * sizeof(U32) + sizeof(U32) * 24;
    memcpy(sign, dev->addr + addr, sizeof(U32) * 16);

    return check;
}

int SM2_Verify(device_t *dev, U32 base_addr, U32 *pub_key, U32 *hash, U32 *sign)
{

    U32 addr, d32;
    // write pub_key, hash, sign
    addr = base_addr + DATA_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, pub_key, sizeof(U32) * 16);                 // public key
    memcpy(dev->addr + addr + sizeof(U32) * 16, hash, sizeof(U32) * 8);  // hash value
    memcpy(dev->addr + addr + sizeof(U32) * 24, sign, sizeof(U32) * 16); // signature result
    msync((void *)(dev->addr + addr), sizeof(U32) * 40, MS_SYNC | MS_INVALIDATE);

    // write start command
    d32 = CMD_VERIFY;
    addr = base_addr + CMD_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, &d32, sizeof(U32) * 1);
    msync((void *)(dev->addr + addr), sizeof(U32) * 1, MS_SYNC | MS_INVALIDATE);

    // Wait for EBUSY signal
    addr = base_addr + STATE_ADDR * sizeof(U32);
    d32 = read_le32(dev, addr);
    while (d32 & 1)
    {
        d32 = read_le32(dev, addr);
    }
    int check = d32 & 2;

    return check;
}

int SM2_Encrypt(device_t *dev, U32 base_addr, U32 *rand, U32 *pub_key, U32 *C1, U32 *S)
{
    U32 addr, d32;
    // write rand, pub_key
    addr = base_addr + DATA_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, rand, sizeof(U32) * 8);                       // random number sequence
    memcpy(dev->addr + addr + sizeof(U32) * 8, pub_key, sizeof(U32) * 16); // public key
    msync((void *)(dev->addr + addr), sizeof(U32) * 24, MS_SYNC | MS_INVALIDATE);

    // write start command
    d32 = CMD_ENCRYPT;
    addr = base_addr + CMD_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, &d32, sizeof(U32) * 1);
    msync((void *)(dev->addr + addr), sizeof(U32) * 1, MS_SYNC | MS_INVALIDATE);

    // Wait for EBUSY signal
    addr = base_addr + STATE_ADDR * sizeof(U32);
    d32 = read_le32(dev, addr);
    while (d32 & 1)
    {
        d32 = read_le32(dev, addr);
    }
    int check = d32 & 2;

    // read sign result
    addr = base_addr + DATA_ADDR * sizeof(U32);
    memcpy(C1, dev->addr + addr + sizeof(U32) * 24, sizeof(U32) * 16);
    memcpy(S, dev->addr + addr + sizeof(U32) * 40, sizeof(U32) * 16);

    return check;
}

int SM2_Decrypt(device_t *dev, U32 base_addr, U32 *pri_key, U32 *C1, U32 *S)
{
    U32 addr, d32;
    // write pri_key, C1
    addr = base_addr + DATA_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, rand, sizeof(U32) * 8);                  // private key
    memcpy(dev->addr + addr + sizeof(U32) * 8, C1, sizeof(U32) * 16); // C1
    msync((void *)(dev->addr + addr), sizeof(U32) * 24, MS_SYNC | MS_INVALIDATE);

    // write start command
    d32 = CMD_DECRYPT;
    addr = base_addr + CMD_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, &d32, sizeof(U32) * 1);
    msync((void *)(dev->addr + addr), sizeof(U32) * 1, MS_SYNC | MS_INVALIDATE);

    // Wait for EBUSY signal
    addr = base_addr + STATE_ADDR * sizeof(U32);
    d32 = read_le32(dev, addr);
    while (d32 & 1)
    {
        d32 = read_le32(dev, addr);
    }
    int check = d32 & 2;

    // read sign result
    addr = base_addr + DATA_ADDR * sizeof(U32);
    memcpy(S, dev->addr + addr + sizeof(U32) * 24, sizeof(U32) * 16);

    return check;
}

int SM2_KeyExchange(device_t *dev, U32 base_addr, U32 *self_r, U32 *self_Rx, U32 *self_d,
                    U32 *other_R, U32 *other_P, U32 *UV)
{
    U32 addr, d32;
    // write self_r, self_Rx, self_d, other_R, other_P
    addr = base_addr + DATA_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, self_r, sizeof(U32) * 8);
    memcpy(dev->addr + addr + sizeof(U32) * 8, self_Rx, sizeof(U32) * 8);
    memcpy(dev->addr + addr + sizeof(U32) * 16, self_d, sizeof(U32) * 8);
    memcpy(dev->addr + addr + sizeof(U32) * 24, other_R, sizeof(U32) * 16);
    memcpy(dev->addr + addr + sizeof(U32) * 40, other_P, sizeof(U32) * 16);
    msync((void *)(dev->addr + addr), sizeof(U32) * 56, MS_SYNC | MS_INVALIDATE);

    // write start command
    d32 = CMD_KEYX;
    addr = base_addr + CMD_ADDR * sizeof(U32);
    memcpy(dev->addr + addr, &d32, sizeof(U32) * 1);
    msync((void *)(dev->addr + addr), sizeof(U32) * 1, MS_SYNC | MS_INVALIDATE);

    // Wait for EBUSY signal
    addr = base_addr + STATE_ADDR * sizeof(U32);
    d32 = read_le32(dev, addr);
    while (d32 & 1)
    {
        d32 = read_le32(dev, addr);
    }
    int check = d32 & 2;

    // read result
    addr = base_addr + DATA_ADDR * sizeof(U32);
    memcpy(UV, dev->addr + addr + sizeof(U32) * 56, sizeof(U32) * 16);

    return check;
}

/* ----------------------------------------------------------------
 * Raw pointer read/write access
 * 
 * copy from "PCI debug registers interface * 6/21/2010 D. W. Hawkins"
 *
 * ----------------------------------------------------------------
 */
static void write_8(device_t *dev, U32 addr, U8 data)
{
    *(volatile U8 *)(dev->addr + addr) = data;
    msync((void *)(dev->addr + addr), 1, MS_SYNC | MS_INVALIDATE);
}

static U8 read_8(device_t *dev, U32 addr)
{
    return *(volatile U8 *)(dev->addr + addr);
}

static void write_le16(device_t *dev, U32 addr, U16 data)
{
    if (__BYTE_ORDER != __LITTLE_ENDIAN)
    {
        data = bswap_16(data);
    }
    *(volatile U16 *)(dev->addr + addr) = data;
    msync((void *)(dev->addr + addr), 2, MS_SYNC | MS_INVALIDATE);
}

static U16 read_le16(device_t *dev, U32 addr)
{
    U32 data = *(volatile U16 *)(dev->addr + addr);
    if (__BYTE_ORDER != __LITTLE_ENDIAN)
    {
        data = bswap_16(data);
    }
    return data;
}

static void write_be16(device_t *dev, U32 addr, U16 data)
{
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
        data = bswap_16(data);
    }
    *(volatile U16 *)(dev->addr + addr) = data;
    msync((void *)(dev->addr + addr), 2, MS_SYNC | MS_INVALIDATE);
}

static U16 read_be16(device_t *dev, U32 addr)
{
    U32 data = *(volatile U16 *)(dev->addr + addr);
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
        data = bswap_16(data);
    }
    return data;
}

static void write_le32(device_t *dev, U32 addr, U32 data)
{
    if (__BYTE_ORDER != __LITTLE_ENDIAN)
    {
        data = bswap_32(data);
    }
    *(volatile U32 *)(dev->addr + addr) = data;
    msync((void *)(dev->addr + addr), 4, MS_SYNC | MS_INVALIDATE);
}

static U32 read_le32(device_t *dev, U32 addr)
{
    U32 data = *(volatile U32 *)(dev->addr + addr);
    if (__BYTE_ORDER != __LITTLE_ENDIAN)
    {
        data = bswap_32(data);
    }
    return data;
}

static void write_be32(device_t *dev, U32 addr, U32 data)
{
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
        data = bswap_32(data);
    }
    *(volatile U32 *)(dev->addr + addr) = data;
    msync((void *)(dev->addr + addr), 4, MS_SYNC | MS_INVALIDATE);
}

static U32 read_be32(device_t *dev, U32 addr)
{
    U32 data = *(volatile U32 *)(dev->addr + addr);
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
        data = bswap_32(data);
    }
    return data;
}
