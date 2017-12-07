#ifndef AD_REG_H
#define AD_REG_H

#define AD9266_SPI_WORD_LENGTH  3
#define AD9266_SPI_MODE  0
#define AD9266_SPI_LSB  0
#define AD9266_SPI_BITS  8
#define AD9266_SPI_MAXSPEED 10000000
#define AD9266_REG_NUM  15


char ad_reg_val[AD9266_REG_NUM][3] = {
     {0x00,0x00,0x18},  //SPI port configuration register
     {0x00,0x08,0x00},  //Modes: full power down
     {0x00,0x09,0x00},  //Clock: Disable internal duty cycle stabilizer
     {0x00,0x0b,0x00},  //Clock Divide: 0x00 1分频
     {0x00,0x0d,0x00},  //Test Mode: single
     {0x00,0x10,0x00},  //Offset adjust: 
     {0x00,0x14,0x01},  //Output Mode: 3.3V CMOS twos complement
     {0x00,0x15,0x22},  //Output adjust: DCO/data driver strength: 1 stripe
     {0x00,0x16,0x00},  //Output phase: DCO no delay
     {0x00,0x17,0xc0},  //Output Delay: 0.56ns
     {0x00,0x19,0x5a},  //User_Patt1_LSB
     {0x00,0x1a,0x5a},  //User_Patt1_MSB
     {0x00,0x1b,0xa5},  //User_Patt2_LSB
     {0x00,0x1c,0xa5},  //User_Patt2_MSB
     {0x00,0xff,0x01}   //OR/MODE select: OR
};


#endif /*AD_REG_H*/
