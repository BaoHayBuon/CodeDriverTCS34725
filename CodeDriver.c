#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define DRIVER_NAME "tcs34725_driver"
#define CLASS_NAME "tcs34725"
#define DEVICE_NAME "tcs34725"

// Địa chỉ của sensor TCS34725
#define TCS34725_ADDR 0x29

// Các thanh ghi cần thiết của TCS34725
#define TCS34725_REG_ENABLE        0x00
#define TCS34725_REG_CONTROL       0x01
#define TCS34725_REG_CDATAL        0x14
#define TCS34725_REG_CDATAH        0x15
#define TCS34725_REG_RDATAL        0x16
#define TCS34725_REG_RDATAH        0x17
#define TCS34725_REG_GDATAL        0x18
#define TCS34725_REG_GDATAH        0x19
#define TCS34725_REG_BDATAL        0x1A
#define TCS34725_REG_BDATAH        0x1B
#define TCS34725_REG_ID            0x12
#define TCS34725_REG_PON           0x01  // Power ON
#define TCS34725_REG_AEN           0x02  // Enable RGBC (Red, Green, Blue, Clear) sensor

// IOCTL commands
#define TCS34725_IOCTL_MAGIC 't'
#define TCS34725_IOCTL_READ_COLOR _IOR(TCS34725_IOCTL_MAGIC, 1, int)

static struct i2c_client *tcs34725_client;
static struct class* tcs34725_class = NULL;
static struct device* tcs34725_device = NULL;
static int major_number;

// Hàm đọc 2 byte từ sensor và kết hợp thành giá trị 16-bit
static int tcs34725_read_word(struct i2c_client *client, u8 reg)
{
    u8 buf[2];
    if (i2c_smbus_read_i2c_block_data(client, reg, sizeof(buf), buf) < 0) {
        printk(KERN_ERR "Failed to read data from sensor\n");
        return -EIO;
    }
    return (buf[0] << 8) | buf[1];
}

// Hàm enable
static int tcs34725_enable(struct i2c_client *client)
{
    int ret;   
    ret = i2c_smbus_write_byte_data(client, TCS34725_REG_ENABLE, TCS34725_REG_PON | TCS34725_REG_AEN);
    if (ret < 0) {
        printk(KERN_ERR "Failed to enable sensor\n");
        return ret;
    }
    return 0;
}

// Hàm đọc giá trị màu (Clear, Red, Green, Blue)
static long tcs34725_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int color_data;
    switch (cmd) {
        case TCS34725_IOCTL_READ_COLOR:
            color_data = tcs34725_read_word(tcs34725_client, TCS34725_REG_CDATAL);
            color_data = (color_data << 8) | tcs34725_read_word(tcs34725_client, TCS34725_REG_CDATAH);
            break;
        default:
            return -EINVAL;
    }
    if (copy_to_user((int __user *)arg, &color_data, sizeof(color_data))) {
        return -EFAULT;
    }
    return 0;
}

// Hàm open
static int tcs34725_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "TCS34725 device opened\n");
    return 0;
}

// Hàm release
static int tcs34725_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "TCS34725 device closed\n");
    return 0;
}

static struct file_operations fops = {
    .open = tcs34725_open,
    .unlocked_ioctl = tcs34725_ioctl,
    .release = tcs34725_release,
};

// Hàm probe
static int tcs34725_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    tcs34725_client = client;
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "Failed to register a major number\n");
        return major_number;
    }
    tcs34725_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(tcs34725_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to register device class\n");
        return PTR_ERR(tcs34725_class);
    }
    tcs34725_device = device_create(tcs34725_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(tcs34725_device)) {
        class_destroy(tcs34725_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to create the device\n");
        return PTR_ERR(tcs34725_device);
    }
    ret = tcs34725_enable(client);
    if (ret < 0) {
        return ret;
    }
    printk(KERN_INFO "TCS34725 driver installed\n");
    return 0;
}

// Hàm remove
static void tcs34725_remove(struct i2c_client *client)
{
    device_destroy(tcs34725_class, MKDEV(major_number, 0));
    class_unregister(tcs34725_class);
    class_destroy(tcs34725_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "TCS34725 driver removed\n");
}

// Cấu trúc của driver I2C
static const struct of_device_id tcs34725_of_match[] = {
    { .compatible = "ams,tcs34725", },
    { },
};
MODULE_DEVICE_TABLE(of, tcs34725_of_match);

static struct i2c_driver tcs34725_driver = {
    .driver = {
        .name   = DRIVER_NAME,
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(tcs34725_of_match),
    },
    .probe      = tcs34725_probe,
    .remove     = tcs34725_remove,
};

// Hàm init driver
static int __init tcs34725_init(void)
{
    printk(KERN_INFO "Initializing TCS34725 driver\n");
    return i2c_add_driver(&tcs34725_driver);
}

// Hàm exit driver
static void __exit tcs34725_exit(void)
{
    printk(KERN_INFO "Exiting TCS34725 driver\n");
    i2c_del_driver(&tcs34725_driver);
}

module_init(tcs34725_init);
module_exit(tcs34725_exit);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("TCS34725 I2C Driver with IOCTL Interface");
MODULE_LICENSE("GPL");
