#ifndef Z7_NAU_SYSTEM_H
#define Z7_NAU_SYSTEM_H

#include <stdbool.h>


struct  task_parameter { 
        int    task_no; 
        int    da_samplefreq;
        int    da_wavelength;
        int    ad_samplefreq;
        int    ad_avarage;
	int    avarage_delay; 
        int    ad_length;
        int    switch_interval;
        int    turns;
        int    turns_interval;
        int    io_delay;  
        int    trig_mode;
	int    lmk_pll1_n;
	int    lmk_pll2_r;
	int    lmk_pll2_n_pre;
	int    lmk_pll2_n;
	int    lmk_DIVIDE0_1;
	int    lmk_DIVIDE2_3;
     };


struct info_savedfiles{
       short  turns_saved;
       short  switchs_saved;//in last turn
       short  turns_upload;
       short  switchs_upload;
};


#define  KEY_CLOSE   0
#define  KEY_OPEN    1

#define  FILENAME_LEN  40

#define  AXI_CLK  100000000   //in Hz
#define  mSEC2CYCLE(x) (x * AXI_CLK / 1000)  
#define  SEC2CYCLE(x)  (x * AXI_CLK)

#define  LED_ALWAYS_ON  0xffffffff  
#define  LED_ALWAYS_OFF 0x00000000
#define  SWITCHTABLE_COLMAX  120
#define  SWITCHTABLE_COLNUM  2

#define  SWTABLE_WORD1  0xc000
#define  SWTABLE_WORD2  0x8000
#define  SWTABLE_WORD3  0x4000
#define  SWTABLE_WORD4  0x0000

/*********************************
*  net communication command     *
**********************************/

#define  CMD_UPLOAD_PARAMETER   1
#define  CMD_UPLOAD_DAWAVE      2
#define  CMD_UPLOAD_SWTABLE     3
#define  CMD_UPLOAD_DATAFILE    4
#define  CMD_GET_PARAMETER      5
#define  CMD_GET_DAWAVE         6
#define  CMD_GET_SWTABLE        7
#define  CMD_SELFCHECK          8
#define  CMD_FILECHECK          9
#define  CMD_REQUEST_FILE       10
#define  CMD_BYTEORDERCHECK     11
#define  CMD_DELETE_FILE        12


int net_task();
int datasample_task();
void lmk_init();
int upload_datafile(int sockfd_dt,struct info_savedfiles *info_f,short *buffer,char *filename);
int get_parameter(int sockfd_dt);
int upload_parameter(int sockfd_dt);
int get_dawave(int sockfd_dt, short *buffer);
int upload_dawave(int sockfd_dt, short *buffer);
int get_switchtable(int sockfd_dt);
int upload_switchtable(int sockfd_dt);
int  z7nau_selfcheck(int sockfd_dt);
int check_savedfiles(struct info_savedfiles *info_f,int *file_num);
int delete_file(struct info_savedfiles *info_savef);
void ad_sigal_handler(int signum);

#endif












