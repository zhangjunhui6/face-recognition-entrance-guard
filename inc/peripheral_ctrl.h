#ifndef PERIPHERAL_CTRL_H
#define PERIPHERAL_CTRL_H

#include "i2c.h"
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

#define MLX90614_IIC_ADDR   0x5A
#define MLX90614_TA 		0x06
#define MLX90614_TOBJ1 		0x07


class PeripheralCtrl {
    public:

    static PeripheralCtrl& GetInstance() {
        static PeripheralCtrl instance;
        return instance;
    }

    float get_object_temp();
    float get_ambient_temp();

    void Init();

    int set_buzzer(unsigned int high_or_low);

    int set_light(unsigned int r, unsigned int g, unsigned int b);

    private:
    gpio io_ctrl;
    i2c i2c2_ctrl;

    private:
    float get_temp(unsigned char reg);

};
#endif
