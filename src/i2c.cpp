#include "i2c.h"
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
#include "i2c.h"

/* i2c_init，打开设备文件 */
int i2c::i2c_init(unsigned int i2c_num) {
    char file_name[0x10];
    sprintf(file_name, "/dev/i2c-%u", i2c_num);
    /* 以读写的方式打开通用设备文件 */
    fd = open(file_name, O_RDWR);
    if (fd < 0) {
        ERROR_LOG("i2c-%u设备文件打开失败!\n", i2c_num);
        return -1;
    }

    /* 设置收不到ACK时的重试次数，默认为1次 */
    if (ioctl(fd, I2C_RETRIES, 1) < 0) {
        close(fd);
        fd = 0;
        ERROR_LOG("设置i2c-%u重试失败!\n", i2c_num);
        return -1;
    }

    // set i2c-1 timeout time, 10ms as unit
    if (ioctl(fd, I2C_TIMEOUT, 1) < 0) {
        close(fd);
        fd = 0;
        ERROR_LOG("设置i2c-2超时失败!\n");
        return -1;
    }
    return 0;
}

/* i2c_read，从指定长度寄存器中读取指定长度数据 */
int i2c::i2c_read(unsigned char slave, unsigned int reg_width, unsigned int reg, unsigned int data_width, unsigned char * buf) {
    int ret;
    struct i2c_rdwr_ioctl_data ssm_msg = {
        0
    };
    ssm_msg.nmsgs = 2;
    ssm_msg.msgs = (struct i2c_msg *)malloc(ssm_msg.nmsgs * sizeof(struct i2c_msg));
    if (ssm_msg.msgs == NULL) {
        ERROR_LOG("内存申请失败!\n");
        return -1;
    }

    unsigned char regs[2] = {
        0
    };
    if (reg_width == 2) {
        regs[0] = (reg >> 8) & 0xff;
        regs[1] = reg & 0xff;
    } else {
        regs[0] = reg & 0xff;
    }
    (ssm_msg.msgs[0]).addr = slave;
    (ssm_msg.msgs[0]).flags = 0;
    //写
    (ssm_msg.msgs[0]).len = reg_width;
    /* reg_width */
    (ssm_msg.msgs[0]).buf = regs;
    //写入需要读出数据的位置

    (ssm_msg.msgs[1]).addr = slave;
    (ssm_msg.msgs[1]).flags = I2C_M_RD;
    //读
    (ssm_msg.msgs[1]).len = data_width;
    /* data_width*/
    (ssm_msg.msgs[1]).buf = buf;
    /* 使用ioctl读出数据 */
    ret = ioctl(fd, I2C_RDWR, &ssm_msg);
    if (ret < 0) {
        ERROR_LOG("读取数据失败，返回:%#x !\n", ret);
        free(ssm_msg.msgs);
        ssm_msg.msgs = NULL;
        return -1;
    }
    free(ssm_msg.msgs);
    ssm_msg.msgs = NULL;
    return 0;
}

/* i2c_read，从8位寄存器里面读取1字节数据 */
int i2c::i2c_read(unsigned char slave, unsigned char reg, unsigned char * buf)
{
    int ret;
    struct i2c_rdwr_ioctl_data ssm_msg = {
        0
    };
    ssm_msg.nmsgs = 2;
    ssm_msg.msgs = (struct i2c_msg *)malloc(ssm_msg.nmsgs * sizeof(struct i2c_msg));
    if (ssm_msg.msgs == NULL) {
        ERROR_LOG("申请内存失败\n");
        return -1;
    }

    unsigned char regs[2] = {
        0
    };
    regs[0] = reg;
    regs[1] = reg;
    (ssm_msg.msgs[0]).addr = slave;
    (ssm_msg.msgs[0]).flags = 0;
    //写
    (ssm_msg.msgs[0]).len = 1;
    /* reg_width*/
    (ssm_msg.msgs[0]).buf = regs;
    //写入需要读出数据的位置

    (ssm_msg.msgs[1]).addr = slave;
    (ssm_msg.msgs[1]).flags = I2C_M_RD;
    //读
    (ssm_msg.msgs[1]).len = 1;
    /* data_width*/
    (ssm_msg.msgs[1]).buf = buf;
    /* 使用ioctl读出数据 */
    ret = ioctl(fd, I2C_RDWR, &ssm_msg);
    if (ret < 0) {
        ERROR_LOG("读取数据失败，返回%#x !\n", ret);
        free(ssm_msg.msgs);
        ssm_msg.msgs = NULL;
        return -1;
    }
    free(ssm_msg.msgs);
    ssm_msg.msgs = NULL;
    return 0;
}

/* i2c_write，向设备中指定的寄存器写入1字节数据*/
int i2c::i2c_write(unsigned char slave, unsigned char reg, unsigned char value)
{
    int ret;
    struct i2c_rdwr_ioctl_data ssm_msg = {
        0
    };
    ssm_msg.nmsgs = 1;
    ssm_msg.msgs = (struct i2c_msg *)malloc(ssm_msg.nmsgs * sizeof(struct i2c_msg));
    if (ssm_msg.msgs == NULL) {
        ERROR_LOG("申请内存失败!\n");
        return -1;
    }
    unsigned char buf[2] = {
        0
    };
    buf[0] = reg;
    //写入的位置
    buf[1] = value;
    //写入的数据
    (ssm_msg.msgs[0]).addr = (unsigned short)slave;
    (ssm_msg.msgs[0]).flags = 0;
    //写
    (ssm_msg.msgs[0]).len = 2;
    //2个字节
    (ssm_msg.msgs[0]).buf = buf;
    /* 使用ioctl函数将消息写入文件中 */
    ret = ioctl(fd, I2C_RDWR, &ssm_msg);
    if (ret < 0) {
        ERROR_LOG("写入错误，返回:%#x, 错误码为:%#x, %s!\n", ret, errno, strerror(errno));
        free(ssm_msg.msgs);
        ssm_msg.msgs = NULL;
        return -1;
    }
    free(ssm_msg.msgs);
    ssm_msg.msgs = NULL;
    return 0;
}


/* ------------- IIC构造函数和析构函数---------------- */
i2c::i2c(unsigned int i2c_num) {
    i2c_init(i2c_num);
}
i2c::i2c(void) {
}
i2c::~i2c(void) {
}


/* --------------IIC提供的公共接口----------------- */
/* 获取i2c的设备文件 */
int i2c::atlas_i2c_get_fd(void)
{
    return fd;
}

/* i2c初始化 */
int i2c::atlas_i2c_init(unsigned int i2c_num) {
    int ret;
    ret = i2c_init(i2c_num);
    if (ret != 0) {
        ERROR_LOG("I2C初始化失败!\n");
        return -1;
    }
    return 0;
}

/* 调用i2c_read()函数 */
int i2c::atlas_i2c_read(unsigned char slave, unsigned int reg_width, unsigned int reg, unsigned int data_width, unsigned char * data) {
    int ret;
    ret = i2c_read(slave, reg_width, reg, data_width, data);
    if (ret != 0) {
        close(fd);
        fd = 0;
        ERROR_LOG("I2C读取%#x设备的%#x寄存器数据失败!\n", slave, reg);
        return -1;
    }
    return 0;
}

/* 调用i2c_read()函数 */
int i2c::atlas_i2c_read(unsigned char slave, unsigned char reg, unsigned char * data)
{
    int ret;
    ret = i2c_read(slave, reg, data);
    if (ret != 0) {
        close(fd);
        fd = 0;
        ERROR_LOG("I2C读取%#x设备的%#x寄存器数据失败!\n", slave, reg);
        return -1;
    }
    return 0;
}

/* 调用i2c_write()函数 */
int i2c::atlas_i2c_write(unsigned char slave, unsigned char reg, unsigned char data)
{
    int ret;
    ret = i2c_write(slave, reg, data);
    if (ret != 0) {
        close(fd);
        fd = 0;
        ERROR_LOG("I2C向%#x设备的%#x寄存器写入%#x数据失败\n", slave, data, reg);
        return -1;
    }
    return 0;
}





