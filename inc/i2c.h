#ifndef I2C_H
#define I2C_H

#include <sys/types.h>

/* I2C Device*/
#define I2C_RETRIES	0x0701	/* number of times a device address  */
#define I2C_TIMEOUT	0x0702	/* set timeout - call with int 		*/

/* I2C_MSG flags */
#define I2C_M_TEN	0x10	/* we have a ten bit chip address	*/
#define I2C_M_RD	0x01
#define I2C_M_STOP  0x8000
#define I2C_M_NOSTART	0x4000
#define I2C_M_REV_DIR_ADDR	0x2000
#define I2C_M_IGNORE_NAK	0x1000
#define I2C_M_NO_RD_ACK		0x0800
#define I2C_M_RECV_LEN  0x0400

/* this is for i2c-dev.c	*/
#define I2C_SLAVE	0x0703	    /* 设置从设备地址 */
#define I2C_SLAVE_FORCE	0x0706	/* 强制设置从地址 */
#define I2C_TENBIT	0x0704	    /* 选择地址位长：0 for 7 bit addrs, != 0 for 10 bit */
#define I2C_FUNCS	0x0705	    /* Get the adapter functionality */
#define I2C_RDWR	0x0707	    /* Combined R/W transfer (one stop only)*/

#define INFO_LOG(fmt, args...) fprintf(stdout, "[INFO]  " fmt "\n", ##args)
#define WARN_LOG(fmt, args...) fprintf(stdout, "[WARN]  " fmt "\n", ##args)
#define ERROR_LOG(fmt, args...) fprintf(stdout, "[ERROR]  " fmt "\n", ##args)


class i2c {

    public:
    i2c(unsigned int i2c_num);
    i2c(void);
    ~i2c(void);

    int atlas_i2c_get_fd(void);
    int atlas_i2c_init(unsigned int i2c_num);
    int atlas_i2c_read(unsigned char slave, unsigned int reg_width, unsigned int reg, unsigned int data_width, unsigned char * data);
    int atlas_i2c_read(unsigned char slave, unsigned char reg, unsigned char * data);
    int atlas_i2c_write(unsigned char slave, unsigned char reg, unsigned char data);

    private:
    int fd;
    struct i2c_msg {
        unsigned short addr;    /* 从设备地址 */
        unsigned short flags;   /* 读写方向 */
        unsigned short len;     /* 消息长度，单位为字节 */
        unsigned char * buf;     /* 消息数据 */
    };
    struct i2c_rdwr_ioctl_data {
        struct i2c_msg * msgs;   /* i2c_msg[] 指针 */
        int nmsgs;              /* i2c_msg 数量 */
    };

    private:
    int i2c_init(unsigned int i2c_num);
    int i2c_read(unsigned char slave, unsigned int reg_width, unsigned int reg, unsigned int data_width, unsigned char * buf);
    int i2c_read(unsigned char slave, unsigned char reg, unsigned char * buf);
    int i2c_write(unsigned char slave, unsigned char reg, unsigned char value);

};
#endif
