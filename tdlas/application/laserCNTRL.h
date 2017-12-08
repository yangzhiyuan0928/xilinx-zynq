#ifndef LASER_CNTRL_H_
#define LASER_CNTRL_H_

#define L_MAX_PATH_SIZE	200
#define L_MAX_NAME_SIZE	50
#define L_MAX_VALUE_SIZE  100
#define L_MAX_UNIT_NAME_SIZE 50

#define SYS_PATH_LASER	"/sys/class/laserCNTRL"
#define DEVICE_L_NAME 	"laserCNTRL.0"

enum EErrorCode
{
	RET_SUCCESS,
	RET_ERR_DEV_NOT_FOUND,
	RET_CANNOT_OPEN_FILE,
	RET_FILE_READ_FAILED,
	RET_FILE_WRITE_FAILED
};

enum LASERCNTRL_Param
{
	EParamLfreq_sq1x,
        EParamLphase_sq1x,
	EParamLfreq_sq2x,
	EParamLphase_sq2x,
	EParamLfreq_sin,
	EParamLphase_sin,
	EParamLamp_sin,
	EParamLfreq_saw,	
	EParamLamp_saw,
	EParamLdc1,	
	EParamLdc2,
	EParamLdelay_saw,	
	EParamLouten,
	EParamLMax
};

struct laserCNTRLcommand
{
	const char name[L_MAX_NAME_SIZE];
    double     value;
	const char unit[L_MAX_UNIT_NAME_SIZE];
	unsigned long (* const convphy_to_laserCNTRL)(double value);
	double (* const convphy_from_laserCNTRL)(unsigned long value);
};

struct laserCNTRL_IVfunction
{
       double (* const convlaser_I2V)(double value);
};

unsigned long Freq2Phaseinc(double freq);
double Phaseinc2Freq(unsigned long phaseinc);
unsigned long Angle2Phaseinit(double angle);
double Phaseinit2Angle(unsigned long phaseinit);
unsigned long Voltag2Amp(double voltage);
double Amp2Voltag(unsigned long);
unsigned long Time2Dly(double time_l);
double Dly2Time(unsigned long dly);
unsigned long OUTEN2_dev(double value);
double dev2_OUTEN2(unsigned long value);

int laserCNTRL_read_param(struct laserCNTRLcommand *param);
int laserCNTRL_get_node(const char * deviceName);
int laserCNTRL_write_param(struct laserCNTRLcommand *param);
#endif //LASER_CNTRL_H_
