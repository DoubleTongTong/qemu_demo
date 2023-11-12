#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/interrupt.h>

#define MY_PCI_VENDOR_ID 0x1234
#define MY_PCI_DEVICE_ID 0x5678

// 该结构体保存我们设备特定的信息
struct my_pci_device {
    struct pci_dev* pdev;
    void __iomem* mmio_base; // 内存映射 I/O 基址
    unsigned int irq;        // 中断号
};

// 当PCI设备插入时调用
static int my_pci_probe(struct pci_dev* pdev, const struct pci_device_id* id)
{
    struct my_pci_device* my_dev;
    int ret;

    printk(KERN_INFO "My PCI Device (%04x:%04x) plugged in\n",
        pdev->vendor, pdev->device);

    // 分配设备结构体内存
    my_dev = kzalloc(sizeof(*my_dev), GFP_KERNEL);
    if (!my_dev) {
        dev_err(&pdev->dev, "Unable to allocate memory for the PCI device\n");
        return -ENOMEM;
    }

    // 保存设备指针
    my_dev->pdev = pdev;
    pci_set_drvdata(pdev, my_dev);

    // 启用PCI设备
    ret = pci_enable_device(pdev);
    if (ret) {
        dev_err(&pdev->dev, "Unable to enable PCI device\n");
        goto err_enable_device;
    }

    // 请求PCI设备的BAR0区域
    ret = pci_request_region(pdev, 0, "my_pci_mem_region");
    if (ret) {
        dev_err(&pdev->dev, "Unable to request PCI I/O memory region\n");
        goto err_request_region;
    }

    // 内存映射PCI设备的BAR0区域
    my_dev->mmio_base = ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
    if (!my_dev->mmio_base) {
        dev_err(&pdev->dev, "Unable to remap PCI I/O memory\n");
        goto err_ioremap;
    }

    // 读取并保存中断号
    my_dev->irq = pdev->irq;

    // 在这里可以进行更多的初始化工作，例如设置中断处理函数等

    return 0;

err_ioremap:
    pci_release_region(pdev, 0);
err_request_region:
    pci_disable_device(pdev);
err_enable_device:
    kfree(my_dev);
    return ret;
}

// 当PCI设备拔出时调用
static void my_pci_remove(struct pci_dev* pdev)
{
    struct my_pci_device* my_dev = pci_get_drvdata(pdev);

    // 清除内存映射
    if (my_dev->mmio_base) {
        iounmap(my_dev->mmio_base);
    }

    // 释放BAR0区域
    pci_release_region(pdev, 0);

    // 禁用PCI设备
    pci_disable_device(pdev);

    // 释放设备结构体内存
    kfree(my_dev);
}

// 定义PCI设备ID表
static const struct pci_device_id my_pci_id_table[] = {
    { PCI_DEVICE(MY_PCI_VENDOR_ID, MY_PCI_DEVICE_ID) },
    { 0, } // 列表终结符
};
MODULE_DEVICE_TABLE(pci, my_pci_id_table);

// 定义PCI驱动结构体
static struct pci_driver my_pci_driver = {
    .name = "my_pci_driver",
    .id_table = my_pci_id_table,
    .probe = my_pci_probe,
    .remove = my_pci_remove,
};

// 驱动初始化函数
static int __init my_pci_init(void)
{
    printk(KERN_INFO "My PCI Driver loaded\n");
    return pci_register_driver(&my_pci_driver);
}

// 驱动退出函数
static void __exit my_pci_exit(void)
{
    pci_unregister_driver(&my_pci_driver);
}

module_init(my_pci_init);
module_exit(my_pci_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("PCI driver for a custom QEMU simulated device");
