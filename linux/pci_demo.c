#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#define MY_PCI_VENDOR_ID 0x1234
#define MY_PCI_DEVICE_ID 0x5678

struct my_pci_device {
    struct pci_dev* pdev;
    void __iomem* mmio_base; // 内存映射 I/O 基址
    struct cdev cdev;       // 字符设备
    dev_t devt;             // 设备号
};

static int my_pci_open(struct inode* inode, struct file* file) {
    struct my_pci_device* dev = container_of(inode->i_cdev, struct my_pci_device, cdev);
    file->private_data = dev;
    return 0;
}

static int my_pci_close(struct inode* inode, struct file* file) {
    return 0;
}

static ssize_t my_pci_read(struct file* file, char __user* buf, size_t count, loff_t* ppos) {
    struct my_pci_device* dev = file->private_data;
    size_t available = pci_resource_len(dev->pdev, 0) - *ppos;
    size_t read_count = min(count, available);

    if (copy_to_user(buf, dev->mmio_base + *ppos, read_count))
        return -EFAULT;

    *ppos += read_count;
    return read_count;
}

static ssize_t my_pci_write(struct file* file, const char __user* buf, size_t count, loff_t* ppos) {
    struct my_pci_device* dev = file->private_data;
    size_t available = pci_resource_len(dev->pdev, 0) - *ppos;
    size_t write_count = min(count, available);

    if (copy_from_user(dev->mmio_base + *ppos, buf, write_count))
        return -EFAULT;

    *ppos += write_count;
    return write_count;
}

static struct file_operations my_pci_fops = {
    .owner = THIS_MODULE,
    .read = my_pci_read,
    .write = my_pci_write,
    .open = my_pci_open,
    .release = my_pci_close,
};

// 当PCI设备插入时调用
static int my_pci_probe(struct pci_dev* pdev, const struct pci_device_id* id)
{
    struct my_pci_device* my_dev;
    int ret;

    printk(KERN_INFO "My PCI Device (%04x:%04x) plugged in\n",
        pdev->vendor, pdev->device);

    my_dev = devm_kzalloc(&pdev->dev, sizeof(*my_dev), GFP_KERNEL);
    if (!my_dev)
        return -ENOMEM;

    my_dev->pdev = pdev;
    pci_set_drvdata(pdev, my_dev);

    ret = pci_enable_device(pdev);
    if (ret)
        return ret;

    ret = pci_request_region(pdev, 0, "my_pci_mem_region");
    if (ret)
        goto err_pci_request_region;

    my_dev->mmio_base = ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
    if (!my_dev->mmio_base) {
        ret = -EIO;
        goto err_ioremap;
    }

    // 创建字符设备
    ret = alloc_chrdev_region(&my_dev->devt, 0, 1, "my_pci");
    if (ret)
        goto err_alloc_chrdev;

    cdev_init(&my_dev->cdev, &my_pci_fops);
    my_dev->cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_dev->cdev, my_dev->devt, 1);
    if (ret)
        goto err_cdev_add;

    return 0;

err_cdev_add:
    unregister_chrdev_region(my_dev->devt, 1);
err_alloc_chrdev:
    iounmap(my_dev->mmio_base);
err_ioremap:
    pci_release_region(pdev, 0);
err_pci_request_region:
    pci_disable_device(pdev);
    return ret;
}

// 当PCI设备拔出时调用
static void my_pci_remove(struct pci_dev* pdev) {
    struct my_pci_device* my_dev = pci_get_drvdata(pdev);

    cdev_del(&my_dev->cdev);
    unregister_chrdev_region(my_dev->devt, 1);
    iounmap(my_dev->mmio_base);
    pci_release_region(pdev, 0);
    pci_disable_device(pdev);
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
