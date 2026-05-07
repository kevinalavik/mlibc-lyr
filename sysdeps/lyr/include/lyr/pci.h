#ifndef _LYR_PCI_H
#define _LYR_PCI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define LYR_PCI_MAP_PATH "/dev/devices/pci.map"
#define LYR_PCI_MAP_ABI_VERSION 1

typedef struct __attribute__((packed)) lyr_pcimap {
	uint16_t abi_version;
	uint16_t entry_size;

	uint16_t vendor_id;
	uint16_t device_id;

	uint8_t bus;
	uint8_t slot;
	uint8_t function;

	uint8_t class_code;
	uint8_t subclass;
	uint8_t prog_if;
	uint8_t revision;

	uint8_t bound;
	uint8_t reserved[5];

	char device_name[32];
	char driver_name[32];
} lyr_pcimap_t;

#ifdef __cplusplus
}
#endif

#endif // _LYR_PCI_H
