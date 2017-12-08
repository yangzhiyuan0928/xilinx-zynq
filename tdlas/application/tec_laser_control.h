#ifndef TEC_LASER_H_
#define TEC_LASER_H_

#define MAX_CMD_NAME_SIZE 100
#define MAX_UNIT_NAME_SIZE 50

struct xadc_command
{
	const enum XADC_Param parameter_id;
	const char cmd_name[MAX_CMD_NAME_SIZE];
    double     xadc_result;
	const char unit[MAX_UNIT_NAME_SIZE];
	double (* const convphy_fn)(double);
};

/*******function declare*************************/
/*Laser PD value to laser power*/
double Volt_to_Lpower(double Volt);
//TEC cooling current limitation
double Volt_to_Iteclimitc(double Volt);
//TEC hrating current limitation
double Volt_to_Iteclimith(double Volt);
//TEC voltage limitation
double Volt_to_Vteclimit(double Volt );
//TEC current,A
double Volt_to_Itec(double Volt );
//laser current limitatiom,mA
double Volt_to_Ilaserlimit(double Volt);
//laser current, mA
double Volt_to_Ilaser(double Volt);
//laser temperature
double Volt_to_LTemp(double Volt);
//LTM4644 temperature
double Temp_Ltm4644(double Volt);
//turn on laser current driver
void Laser_ION(void);
//turn off laser current driver
void Laser_IOFF(void);
/*TEC controller on*/
void Laser_TON(void);
/*TEC controller off*/
void Laser_TOFF(void);
/*TEC status query*/
unsigned int TContr_status(void);
/*laser current dirver status query*/
unsigned int IContr_status(void);
/*laser temperature setting to voltage*/
double Temp_to_Volt(double temperature);


double Ilaser_to_Volt(double Current);

#endif //TEC_LASER_H_
