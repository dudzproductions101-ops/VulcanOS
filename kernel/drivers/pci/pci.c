

#include "drivers/pci.h"
#include "arch/x86_64/cpu.h"
#include "mm/allocator.h"
#include "printk.h"


static inline usize append_str(char *buf, usize buf_size, usize off, const char *s)
{
    while (*s && off + 1 < buf_size) {
        buf[off++] = *s++;
    }
    return off;
}

static inline usize append_hex_u8(char *buf, usize buf_size, usize off, u8 v)
{
    const char *hex = "0123456789abcdef";
    if (off + 2 + 1 >= buf_size) return off; 
    buf[off++] = hex[(v >> 4) & 0xF];
    buf[off++] = hex[v & 0xF];
    return off;
}

static inline usize append_hex_u16(char *buf, usize buf_size, usize off, u16 v)
{
    const char *hex = "0123456789abcdef";
    for (int i = 3; i >= 0; i--) {
        if (off + 1 >= buf_size) return off;
        buf[off++] = hex[(v >> (i * 4)) & 0xF];
    }
    return off;
}


static char *report_buf = NULL;
static usize report_len = 0;


#define MAX_PCI_DRIVERS 16
static const struct pci_driver *registered_pci_drivers[MAX_PCI_DRIVERS];
static usize registered_pci_driver_count = 0;

bool pci_register_driver(const struct pci_driver *drv)
{
    if (registered_pci_driver_count >= MAX_PCI_DRIVERS) return false;
    registered_pci_drivers[registered_pci_driver_count++] = drv;
    printk_level(LOG_INFO, "pci: registered driver %s\n", drv->name);
    return true;
}

static inline void outl(u16 port, u32 val)
{
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline u32 inl(u16 port)
{
    u32 ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

u32 pci_config_read32(u8 bus, u8 dev, u8 func, u8 offset)
{
    u32 addr = (u32)(0x80000000u | ((u32)bus << 16) | ((u32)dev << 11) | ((u32)func << 8) | (offset & 0xFC));
    outl(0xCF8, addr);
    return inl(0xCFC);
}

void pci_init(void)
{
    
    const usize buf_size = 16384;
    report_buf = kmalloc(buf_size);
    if (!report_buf) {
        return;
    }
    usize off = 0;

    for (u8 bus = 0; bus < 8; bus++) {
        for (u8 dev = 0; dev < 32; dev++) {
            for (u8 func = 0; func < 8; func++) {
                u32 val = pci_config_read32(bus, dev, func, 0x00);
                u16 vendor = (u16)(val & 0xFFFF);
                if (vendor == 0xFFFF) {
                    if (func == 0) break; 
                    continue;
                }
                u16 device = (u16)((val >> 16) & 0xFFFF);

                u32 class_reg = pci_config_read32(bus, dev, func, 0x08);
                u8 prog_if = (class_reg >> 8) & 0xFF;
                u8 subclass = (class_reg >> 16) & 0xFF;
                u8 class_code = (class_reg >> 24) & 0xFF;

                
                off = append_str(report_buf, buf_size, off, "pci ");
                
                off = append_hex_u8(report_buf, buf_size, off, bus);
                off = append_str(report_buf, buf_size, off, ":");
                
                off = append_hex_u8(report_buf, buf_size, off, dev);
                off = append_str(report_buf, buf_size, off, ".");
                
                if (off + 2 < buf_size) {
                    report_buf[off++] = '0' + (func % 10);
                }
                off = append_str(report_buf, buf_size, off, " vendor=0x");
                off = append_hex_u16(report_buf, buf_size, off, vendor);
                off = append_str(report_buf, buf_size, off, " device=0x");
                off = append_hex_u16(report_buf, buf_size, off, device);
                off = append_str(report_buf, buf_size, off, " class=0x");
                off = append_hex_u8(report_buf, buf_size, off, class_code);
                off = append_str(report_buf, buf_size, off, " subclass=0x");
                off = append_hex_u8(report_buf, buf_size, off, subclass);
                off = append_str(report_buf, buf_size, off, " progif=0x");
                off = append_hex_u8(report_buf, buf_size, off, prog_if);
                if (off + 1 < buf_size) report_buf[off++] = '\n';
                if (off + 128 >= buf_size) {
                    
                    report_len = off;
                }

                
                for (usize di = 0; di < registered_pci_driver_count; di++) {
                    const struct pci_driver *pd = registered_pci_drivers[di];
                    bool match = true;
                    if (pd->class_code != 0xFF && pd->class_code != class_code) match = false;
                    if (pd->subclass != 0xFF && pd->subclass != subclass) match = false;
                    if (pd->prog_if != 0xFF && pd->prog_if != prog_if) match = false;
                    if (pd->vendor != 0xFFFF && pd->vendor != vendor) match = false;
                    if (pd->device != 0xFFFF && pd->device != device) match = false;
                    if (match && pd->init) {
                        printk_level(LOG_INFO, "pci: binding driver %s to %02x:%02x.%u\n", pd->name, bus, dev, func);
                        pd->init(bus, dev, func, vendor, device, class_code, subclass, prog_if);
                    }
                }
            }
        }
    }
    report_len = off;
}

const char *pci_get_report(void)
{
    return report_buf;
}

usize pci_get_report_len(void)
{
    return report_len;
}
