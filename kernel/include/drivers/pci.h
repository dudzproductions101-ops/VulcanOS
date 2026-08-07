/* pci.h - minimal PCI enumeration API */

#ifndef VULCAN_DRIVERS_PCI_H
#define VULCAN_DRIVERS_PCI_H

#include "types.h"

void pci_init(void);
const char *pci_get_report(void);
usize pci_get_report_len(void);
u32 pci_config_read32(u8 bus, u8 dev, u8 func, u8 offset);

typedef void (*pci_device_init_fn)(u8 bus, u8 dev, u8 func,
								   u16 vendor, u16 device,
								   u8 class_code, u8 subclass, u8 prog_if);

struct pci_driver {
	const char *name;
	u8 class_code;   /* 0xFF means wildcard */
	u8 subclass;     /* 0xFF wildcard */
	u8 prog_if;      /* 0xFF wildcard */
	u16 vendor;      /* 0xFFFF wildcard */
	u16 device;      /* 0xFFFF wildcard */
	pci_device_init_fn init;
};

bool pci_register_driver(const struct pci_driver *drv);

#endif /* VULCAN_DRIVERS_PCI_H */
