#include "peripheral_ctrl.h"
#include "face_recognition_params.h"
#include "presenter_channels.h"
#include "resource_load.h"
using namespace std;
using namespace ascend::presenter;
using namespace ascend::presenter::facial_recognition;

void PeripheralCtrl::Init() {
    /* i2c-2总线初始化 */
    i2c2_ctrl.atlas_i2c_init(2);
    /* 蜂鸣器模块初始化 */
    io_ctrl.gpio_set_direction(1, 1);
    io_ctrl.gpio_set_value(1, 1);
    /* RGB灯初始化 */
    io_ctrl.gpio_set_direction(5, 1);
    io_ctrl.gpio_set_direction(6, 1);
    io_ctrl.gpio_set_direction(7, 1);
    io_ctrl.gpio_set_value(5, 1);
    io_ctrl.gpio_set_value(6, 0);
    io_ctrl.gpio_set_value(7, 0);
}

/*------------------蜂鸣器模块----------------------*/
int PeripheralCtrl::set_buzzer(unsigned int high_or_low) {
    return io_ctrl.gpio_set_value(1, high_or_low);
}

/*------------------三色LED灯模块-------------------*/
int PeripheralCtrl::set_light(unsigned int r, unsigned int g, unsigned int b) {
    return io_ctrl.gpio_set_value(5, r) || io_ctrl.gpio_set_value(6, g) || io_ctrl.gpio_set_value(7, b);
}

/*------------------体温检测模块--------------------*/
/* 环境温度 */
float PeripheralCtrl::get_ambient_temp() {
    return get_temp(MLX90614_TA);
}
/* 检测到的目标温度 */
float PeripheralCtrl::get_object_temp() {
    return get_temp(MLX90614_TOBJ1);
}
/* 读取指定寄存器的值 */
float PeripheralCtrl::get_temp(unsigned char reg) {
    unsigned char buf[2];
    int ret = i2c2_ctrl.atlas_i2c_read(MLX90614_IIC_ADDR, 1, reg, 2, buf);
    if (ret != 0) {
        ERROR_LOG("读取体温失败！\n");
        return -1;
    }

    unsigned int data = buf[0] | (buf[1] << 8);
    float temp = data * 0.02 - 273.15;

    return temp;
}
