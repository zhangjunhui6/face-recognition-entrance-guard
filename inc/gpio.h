#ifndef GPIO_H
#define GPIO_H
#include "i2c.h"

//ascend310 pin: GPIO0 GPIO1
#define ASCEND310_GPIO_0_DIR          "/sys/class/gpio/gpio504/direction"
#define ASCEND310_GPIO_1_DIR          "/sys/class/gpio/gpio444/direction"
#define ASCEND310_GPIO_0_VAL          "/sys/class/gpio/gpio504/value"
#define ASCEND310_GPIO_1_VAL          "/sys/class/gpio/gpio444/value"
/* PCA6416 */
#define PCA6416_SLAVE_ADDR                 0x20
#define PCA6416_GPIO_CFG_REG               0x07
#define PCA6416_GPIO_PORARITY_REG          0x05
#define PCA6416_GPIO_OUT_REG               0x03
#define PCA6416_GPIO_IN_REG                0x01
//GPIO MASK
#define GPIO3_MASK                         0x10
#define GPIO4_MASK                         0x20
#define GPIO5_MASK                         0x40
#define GPIO6_MASK                         0x80
#define GPIO7_MASK                         0x08

#define INFO_LOG(fmt, args...) fprintf(stdout, "[INFO]  " fmt "\n", ##args)
#define WARN_LOG(fmt, args...) fprintf(stdout, "[WARN]  " fmt "\n", ##args)
#define ERROR_LOG(fmt, args...) fprintf(stdout, "[ERROR]  " fmt "\n", ##args)


class gpio {
    public:
    gpio(void);
    ~gpio(void);

    int gpio_set_direction(int pin, int direction);   // pin 0,1 3,4,5,6,7  dir=0 -> input   dir=1 ->ouput
    int gpio_set_value(int pin, int val);       // pin 0,1 3,4,5,6,7  val=0 -> low level   val=1 ->high level
    int gpio_get_value(int pin, int * val);      // pin 0,1 3,4,5,6,7  val=0 -> low level   val=1 ->high level

    private:
    int PCA6416_gpio_set_direction(int pin, int dir);
    int PCA6416_gpio_set_value(int pin, int val);
    int PCA6416_gpio_get_value(int pin, int * val);
    int ASCEND310_gpio_set_direction(int pin, int dir);
    int ASCEND310_gpio_set_value(int pin, int val);
    int ASCEND310_gpio_get_value(int pin, int * val);

    private:
    i2c i2c1_ctrl;
};

#endif
