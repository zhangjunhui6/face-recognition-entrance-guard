#include "gpio.h"
#include <memory>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <cerrno>
#include <cstring>

gpio::gpio(void)
{
    i2c1_ctrl.atlas_i2c_init(1);
}

gpio::~gpio(void)
{

}


/*通过写PCA6416_GPIO_CFG_REG寄存器实现,dir为0则将对应位置1，否则将对应位置0*/
int gpio::PCA6416_gpio_set_direction(int pin, int dir)
{
    unsigned char slave;
    unsigned char reg;
    unsigned char data;
    int ret;

    if ((pin != 3) && (pin != 4) && (pin != 5) && (pin != 6) && (pin != 7)) {
        ERROR_LOG("PCA6416 引脚错误 ,引脚号必须为:3,4,5,6,7之一\n");
        return -1;
    }

    // 设置GPIO属性为输出output
    slave = PCA6416_SLAVE_ADDR;
    reg = PCA6416_GPIO_CFG_REG;
    data = 0;
    ret = i2c1_ctrl.atlas_i2c_read(slave, reg, &data);
    //需要先获取原来的数据
    if (dir == 0)
    {
        if (pin == 3)
            data |= GPIO3_MASK;
        else if (pin == 4)
            data |= GPIO4_MASK;
        else if (pin == 5)
            data |= GPIO5_MASK;
        else if (pin == 6)
            data |= GPIO6_MASK;
        else if (pin == 7)
            data |= GPIO7_MASK;
    }
    else
    {
        if (pin == 3)
            data &= ~GPIO3_MASK;
        else if (pin == 4)
            data &= ~GPIO4_MASK;
        else if (pin == 5)
            data &= ~GPIO5_MASK;
        else if (pin == 6)
            data &= ~GPIO6_MASK;
        else if (pin == 7)
            data &= ~GPIO7_MASK;
    }

    ret = i2c1_ctrl.atlas_i2c_write(slave, reg, data);
    return 0;
}

/*通过写PCA6416_GPIO_OUT_REG寄存器实现，val为0则将对应位置0，否则置1*/
int gpio::PCA6416_gpio_set_value(int pin, int val)
{
    unsigned char slave;
    unsigned char reg;
    unsigned char data;
    int ret;

    if ((pin != 3) && (pin != 4) && (pin != 5) && (pin != 6) && (pin != 7)) {
        ERROR_LOG("PCA6416 引脚错误 ,引脚号必须为:3,4,5,6,7之一\n");
        return -1;
    }

    // 设置 GPIO 输出电平
    slave = PCA6416_SLAVE_ADDR;
    reg = PCA6416_GPIO_OUT_REG;
    data = 0;
    ret = i2c1_ctrl.atlas_i2c_read(slave, reg, &data);

    if (val == 0)
    {
        if (pin == 3)
            data &= ~GPIO3_MASK;
        else if (pin == 4)
            data &= ~GPIO4_MASK;
        else if (pin == 5)
            data &= ~GPIO5_MASK;
        else if (pin == 6)
            data &= ~GPIO6_MASK;
        else if (pin == 7)
            data &= ~GPIO7_MASK;
    }
    else {
        if (pin == 3)
            data |= GPIO3_MASK;
        else if (pin == 4)
            data |= GPIO4_MASK;
        else if (pin == 5)
            data |= GPIO5_MASK;
        else if (pin == 6)
            data |= GPIO6_MASK;
        else if (pin == 7)
            data |= GPIO7_MASK;
    }
    ret = i2c1_ctrl.atlas_i2c_write(slave, reg, data);
    return 0;
}

/*通过读PCA6416_GPIO_IN_REG寄存器实现，将读到的数据与MASK相与得到val*/
int gpio::PCA6416_gpio_get_value(int pin, int * val)
{
    unsigned char slave;
    unsigned char reg;
    unsigned char data;
    int ret;

    if ((pin != 3) && (pin != 4) && (pin != 5) && (pin != 6) && (pin != 7)) {
        ERROR_LOG("PCA6416引脚错误 ,引脚号必须为:3,4,5,6,7之一\n");
        return -1;
    }

    // 获取 GPIO 输入电平
    slave = PCA6416_SLAVE_ADDR;
    reg = PCA6416_GPIO_IN_REG;
    data = 0;
    ret = i2c1_ctrl.atlas_i2c_read(slave, reg, &data);

    if (pin == 3)
        data &= GPIO3_MASK;
    else if (pin == 4)
        data &= GPIO4_MASK;
    else if (pin == 5)
        data &= GPIO5_MASK;
    else if (pin == 6)
        data &= GPIO6_MASK;
    else if (pin == 7)
        data &= GPIO7_MASK;

    if (data > 0)
        *val = 1;
    else
        *val = 0;

    return 0;
}


/*通过向ASCEND310_GPIO_0/1_DIR文件写入"in" "out"实现方向控制*/
int gpio::ASCEND310_gpio_set_direction(int pin, int dir)
{
    int fd_direction;

    if (pin == 0)
        fd_direction = open(ASCEND310_GPIO_0_DIR, O_WRONLY);
    else if (pin == 1)
        fd_direction = open(ASCEND310_GPIO_1_DIR, O_WRONLY);
    else
    {
        ERROR_LOG("ASCEND310 引脚错误 ,引脚号必须为:0,1之一\n");
        return -1;
    }

    if (-1 == fd_direction)
    {
        ERROR_LOG("打开 gpio 方向文件失败，引脚号为:%d", pin);
        return (-1);
    }

    if (dir == 0)
    {
        if (-1 == write(fd_direction, "in", sizeof("in")))
        {
            ERROR_LOG("gpio写方向文件失败,引脚号为:%d", pin);
            close(fd_direction);
            return (-1);
        }
    }
    else {
        if (-1 == write(fd_direction, "out", sizeof("out")))
        {
            ERROR_LOG("gpio写方向文件失败,引脚号为:%d", pin);
            close(fd_direction);
            return (-1);
        }
    }

    close(fd_direction);
    return 0;
}

/*通过向ASCEND310_GPIO_0/1_VAL文件写入"0"或"1"实现*/
int gpio::ASCEND310_gpio_set_value(int pin, int val)
{
    int fd_gpio_value;
    unsigned char value;

    if (pin == 0)
        fd_gpio_value = open(ASCEND310_GPIO_0_VAL, O_WRONLY);
    else if (pin == 1)
        fd_gpio_value = open(ASCEND310_GPIO_1_VAL, O_WRONLY);
    else {
        ERROR_LOG("ASCEND310 引脚错误 ,引脚号必须为:0,1之一\n");
        return -1;
    }

    if (-1 == fd_gpio_value)
    {
        ERROR_LOG("打开 gpio 数值文件失败，引脚号为:%d", pin);
        return (-1);
    }

    if (val == 0)
    {
        value = '0';
        if (-1 == write(fd_gpio_value, &value, sizeof(value)))
        {
            ERROR_LOG("gpio写操作失败，引脚号为:%d", pin);
            close(fd_gpio_value);
            return (1);
        }
    } else {
        value = '1';
        if (-1 == write(fd_gpio_value, &value, sizeof(value)))
        {
            ERROR_LOG("gpio写操作失败，引脚号为:%d", pin);
            close(fd_gpio_value);
            return (-1);
        }
    }

    close(fd_gpio_value);
    return 0;
}

/*通过读取ASCEND310_GPIO_0/1_VAL文件实现*/
int gpio::ASCEND310_gpio_get_value(int pin, int * val)
{
    int fd_gpio_value;
    char value_str[3];

    if (pin == 0)
        fd_gpio_value = open(ASCEND310_GPIO_0_VAL, O_RDONLY);
    else if (pin == 1)
        fd_gpio_value = open(ASCEND310_GPIO_1_VAL, O_RDONLY);
    else {
        ERROR_LOG("ASCEND310 引脚错误 ,引脚号必须为:0,1之一\n");
        return -1;
    }

    if (-1 == fd_gpio_value)
    {
        ERROR_LOG("打开 gpio 数值文件错误，引脚号为:%d", pin);
        return (-1);
    }

    if (-1 == read(fd_gpio_value, value_str, 3))
    {
        ERROR_LOG("读取gpio数值失败,引脚号为:%d", pin);
        return -1;

    }
    *val = atoi(value_str);
    close(fd_gpio_value);
    return 0;
}


int gpio::gpio_set_direction(int pin, int direction)
{
    if ((pin == 0) || (pin == 1))
        return ASCEND310_gpio_set_direction(pin, direction);
    else
        return PCA6416_gpio_set_direction(pin, direction);
}

int gpio::gpio_set_value(int pin, int val)
{
    if ((pin == 0) || (pin == 1))
        return ASCEND310_gpio_set_value(pin, val);
    else
        return PCA6416_gpio_set_value(pin, val);

}

int gpio::gpio_get_value(int pin, int * val)
{

    if ((pin == 0) || (pin == 1))
        return ASCEND310_gpio_get_value(pin, val);
    else
        return PCA6416_gpio_get_value(pin, val);
}
