#ifndef Z7NAU_BRAM_H
#define Z7NAU_BRAM_H

struct z7nau_bram_dev{
        u8    cap;
        dev_t  devno;
        struct cdev cdev;
        struct device  dev;
 	 void   __iomem *base;
       const char *bram_name;
};

struct class  * z7nau_bram_class;

#define MODULE_NAME  "z7nau_bram"

#define Z7NAU_BRAM_MINOR_START       0
#define Z7NAU_BRAM_MINOR_COUNT       2

#define CAP_R_W   0
#define CAP_WRITE 1
#define CAP_READ  2

#endif
