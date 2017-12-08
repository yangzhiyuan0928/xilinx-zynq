/*****************************************************************************
 *       Laser current signal generator controlling subrotines
 *
 *****************************************************************************
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>

#include "laserCNTRL.h"

#define DEVICE_CLK_PERIOD 20 //ns in 50MHz 
#define DEVICE_CLK    50000000.0   // in Hz 
#define VALUE_2EXP32  4294967296.0
#define DA_FULL       2500.0  //in mv
#define DA_FULLCODE   65536   //16-bit DA 

static char gDevicePath_L[L_MAX_PATH_SIZE];
static char gNodeName_L[L_MAX_NAME_SIZE];

static int line_from_file(char* filename, char* linebuf);
static int line_to_file(char* filename, char* linebuf);

	
/****************************************************************
 *
 * read and write laserCNTRL sysfs file
 *
 ****************************************************************/
int laserCNTRL_read_param(struct laserCNTRLcommand *param)
{
	char filename[L_MAX_PATH_SIZE];
	char read_value[L_MAX_VALUE_SIZE];
	unsigned long value;

	memset(filename, 0, sizeof(filename) );

	sprintf(filename, "%s/%s", gDevicePath_L, param->name);

	if (line_from_file(filename,read_value) == RET_SUCCESS)
	{
		sscanf(read_value,"%lu",&value);
    	param->value = param->convphy_from_laserCNTRL(value);
	}
	else
	{
		printf("\n***Error: reading file %s\n",filename);
		param->value = 0;
		return RET_FILE_READ_FAILED;
	}

	return RET_SUCCESS;
}

int laserCNTRL_write_param(struct laserCNTRLcommand *param)
{
	char filename[L_MAX_PATH_SIZE];
	char write_value[L_MAX_VALUE_SIZE];
	unsigned long  value;

	memset(filename, 0, sizeof(filename) );

	sprintf(filename, "%s/%s", gDevicePath_L, param->name );
	value = param->convphy_to_laserCNTRL(param->value);
	sprintf(write_value,"%lu",value);
	if (line_to_file(filename,write_value) != RET_SUCCESS)
	{
		printf("\n***Error: reading file %s\n",filename);
		param->value = 0;
		return RET_FILE_READ_FAILED;
	}

	return RET_SUCCESS;
}


int laserCNTRL_get_node(const char * deviceName)
{
	struct dirent **namelist;
	char file[L_MAX_PATH_SIZE];
	char name[L_MAX_NAME_SIZE];
	int i,n;
	int flag = 0;

	n = scandir(SYS_PATH_LASER, &namelist, 0, alphasort);
	if (n < 0)
		return RET_ERR_DEV_NOT_FOUND;

	for (i=0; i < n; i++)
	{
		sprintf(file, "%s/%s/devname", SYS_PATH_LASER, namelist[i]->d_name);
		if ((line_from_file(file,name) == 0) && (strcmp(name,deviceName) ==  0))
		{
			flag =1;
			strcpy(gNodeName_L, namelist[i]->d_name);
			sprintf(gDevicePath_L, "%s/%s", SYS_PATH_LASER, gNodeName_L);
			break;
		}
	}

	if(flag == 0) return RET_ERR_DEV_NOT_FOUND;
	return RET_SUCCESS;
}


/*******************************************
 *
 *  laserCNTRL registers setting subrouting
 *
 *******************************************/
unsigned long OUTEN2_dev(double value)
{
  return (unsigned long)value;
}

double dev2_OUTEN2(unsigned long value)
{
  return (double)value;
}

unsigned long Freq2Phaseinc(double freq)
{
unsigned long value;

    value = ceil((freq *VALUE_2EXP32)/DEVICE_CLK);
    return value;
}
	
double Phaseinc2Freq(unsigned long phaseinc)
{
     return(((double)phaseinc * DEVICE_CLK)/VALUE_2EXP32);
}

unsigned long Angle2Phaseinit(double angle)
{
unsigned long  value;
    value = ceil((angle *VALUE_2EXP32)/360.0);
    return value;
}

double Phaseinit2Angle(unsigned long phaseinit)
{
     return(((double)phaseinit * 360.0)/VALUE_2EXP32);
}

unsigned long Voltag2Amp(double voltage)
{
unsigned long value;
    value = ceil((voltage *DA_FULLCODE)/DA_FULL);
    return value;
}

double Amp2Voltag(unsigned long amp)
{
   return((double)amp*DA_FULL/DA_FULLCODE);
}


unsigned long Time2Dly(double time_l)
{
unsigned long value;
    value = ceil(time_l*1000/DEVICE_CLK_PERIOD);//from us to ns
    return value;
}

double Dly2Time(unsigned long dly)
{
    return((float)dly*DEVICE_CLK_PERIOD/1000);
} 

static int line_from_file(char* filename, char* linebuf)
{
    char* s;
    int i;
    FILE* fp = fopen(filename, "r");
    if (!fp) return RET_CANNOT_OPEN_FILE;
    s = fgets(linebuf, L_MAX_VALUE_SIZE, fp);
    fclose(fp);
    if (!s) return RET_FILE_READ_FAILED;

    for (i=0; (*s)&&(i<L_MAX_VALUE_SIZE); i++) {
        if (*s == '\n') *s = 0;
        s++;
    }
    return RET_SUCCESS;
}

static int line_to_file(char* filename, char* linebuf)
{
    int fd;
    fd = open(filename, O_WRONLY);
    if (fd < 0) return RET_CANNOT_OPEN_FILE;
    write(fd, linebuf, strlen(linebuf));
    close(fd);
    return RET_SUCCESS;
}




