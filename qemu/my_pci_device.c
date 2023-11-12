#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_device.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "hw/pci/msi.h"
#include "hw/pci/msix.h"

#define TYPE_MY_PCI_DEVICE "my-pci-device" // 定义我们设备的类型名称。
#define MY_PCI_DEVICE(obj) OBJECT_CHECK(MyPCIDeviceState, (obj), TYPE_MY_PCI_DEVICE) // 宏，用于从Object转换为我们的设备实例。

// 我们自定义PCI设备的状态结构体。
typedef struct {
    PCIDevice parent_obj; // 继承自PCIDevice，必须作为第一个字段。
    MemoryRegion mem; // 用于表示设备的内存区域。
} MyPCIDeviceState;

// 实现设备的实现化函数。
static void my_pci_device_realize(PCIDevice *pci_dev, Error **errp)
{
    MyPCIDeviceState *d = MY_PCI_DEVICE(pci_dev);

    uint8_t *pci_conf = pci_dev->config; // PCI配置空间的指针。
    pci_conf[PCI_INTERRUPT_PIN] = 1; // 设置中断引脚为INT A。

    // 初始化100MB的VRAM内存区域。
    memory_region_init_ram(&d->mem, OBJECT(pci_dev), "my-pci-device-mem", 0x100000, errp);
    // 将VRAM内存区域注册到PCI设备的BAR0。
    pci_register_bar(pci_dev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &d->mem);
}

// 设备类初始化函数。
static void my_pci_device_class_init(ObjectClass *klass, void *data)
{
    PCIDeviceClass *pc = PCI_DEVICE_CLASS(klass);

    pc->realize = my_pci_device_realize; // 设置realize回调函数。
    pc->vendor_id = 0x1234; // 设定PCI设备的供应商ID。
    pc->device_id = 0x5678; // 设定PCI设备的设备ID。
    pc->class_id = PCI_CLASS_OTHERS; // 设定PCI设备的类别。
}

// 定义TypeInfo结构体，用于注册设备类型。
static const TypeInfo my_pci_device_info = {
    .name = TYPE_MY_PCI_DEVICE, // 类型名称。
    .parent = TYPE_PCI_DEVICE, // 父类型。
    .instance_size = sizeof(MyPCIDeviceState), // 实例大小。
    .class_init = my_pci_device_class_init, // 类初始化函数。
    .interfaces = (InterfaceInfo[]) {
        { .type = INTERFACE_CONVENTIONAL_PCI_DEVICE },
        { }, /* end of list */
    },
};

// 注册设备类型的函数。
static void my_pci_device_register_types(void)
{
    type_register_static(&my_pci_device_info); // 注册设备类型。
}

type_init(my_pci_device_register_types) // 初始化类型系统时注册设备。
