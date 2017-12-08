 /****************************************************************
 * Constants
 ****************************************************************/

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define AXI_GPIO_BASE  896  /*AXI GPIO controller ,/sys/class/gpio/gpiochip897*/
#define RS485_RX_EN  0  /* AXI GPIO 0*/
#define RS485_TX_EN  1  /* AXI GPIO 1*/
#define nTEC_ON      2  /* AXI GPIO 2*/
#define LASER_ON     3  /* AXI GPIO 3*/
#define GPRS_ON_OFF  4  /* AXI GPIO 4*/
#define LED0         5  /* AXI GPIO 5*/
#define LED1         6  /* AXI GPIO 6*/
#define LED2         7  /* AXI GPIO 7*/

#define AD_START     9  /* AXI GPIO 9*/

int gpio_export(unsigned int gpio);
int gpio_unexport(unsigned int gpio);
int gpio_set_dir(unsigned int gpio, unsigned int out_flag);
int gpio_set_value(unsigned int gpio, unsigned int value);
int gpio_get_value(unsigned int gpio, unsigned int *value);
int gpio_set_edge(unsigned int gpio, char *edge);
int gpio_fd_open(unsigned int gpio);
int gpio_fd_close(int fd);
void GPIO_Write(unsigned int gpio_n,unsigned int volt_H_L);
unsigned int GPIO_Read(unsigned int gpio_n);
