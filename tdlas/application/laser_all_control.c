

/*****************************************************************************
 *       Laser and TEC monitor, control subrotines
 *
 *****************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>

//
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

//�߳�
#include <pthread.h>            //
#include <semaphore.h>

//timer
#include  <stdint.h>      //definition of uint64_t
#include  <sys/timerfd.h>

#include "xadc_core_if.h"
#include "tec_laser_control.h"
#include "spi_ad5620.h"
#include "AXI_gpio.h"
#include "axi_dma_ad.h"
#include "laserCNTRL.h"

#define VCC_INT_CMD			"xadc_vccint"
#define VCC_AUX_CMD			"xadc_vccaux"
#define VCC_BRAM_CMD		"xadc_vccbram"
#define VCC_TEMP_CMD		"xadc_temp"
#define VCC_EXT_VPN_CMD		"TEMP_LTM4644"
#define VCC_EXT_CH2_CMD		"LASER_I"
#define VCC_EXT_CH3_CMD		"TEC_VTEC"
#define VCC_EXT_CH4_CMD		"TEC_VLIM"
#define VCC_EXT_CH5_CMD		"TEC_ITEC"
#define VCC_EXT_CH7_CMD		"P_MON"
#define VCC_EXT_CH9_CMD		"L_ILM"
#define VCC_EXT_CH12_CMD	"TEC_ILIMH"
#define VCC_EXT_CH14_CMD	"TEC_ILIMC"
#define VCC_EXT_CH16_CMD	"T_MON"

#define XADC_RATIO        2.5  //all inputs are attenuated by 2/5 before into XADC
#define ITEC_SENSOR_R     20.0   //mOHM, TEC current sensor resistor
#define ITEC_SENSOR_C     500.0  //25*ITEC_SENSOR_R,mOHM
#define LASER_VI_TRANSFER (1000.0/LASER_I_AMP/LASER_I_SENSOR_R) // 1000/12.4/LASER_I_SENSOR_R,transfer function of laser current control, mA/V
#define LASER_I_AMP       12.4 // laser current detector amplification
#define LASER_I_SENSOR_R  1.0   //OHM, V_L=I_L * R, laser current sensor resistor
#define T_TEC_HIGH        40.0  //in Degree Celsius
#define T_TEC_LOW         15.0  //in Degree Celsius
#define TEC_VREF          2500.0 //in mV

#define L_I_LIM           130.0  // in mA, identical with hardware setting on the board

#define LTM4644_VG0       1200.0  // mV,the band gap voltage of 1.2V extrapolated to absolute zero or -273 centidegree.
#define LTM4644_COFF      2.0     // mV/K, dVD/dT

#define SERVER_IP         "223.3.50.20"

#define HEAD_TAG(x)  ((x >>16) & 0xffff)
#define HEAD_CMD(x)  (x & 0xff)
#define HEAD_LEN(x)  ((x >>8) & 0xff)
#define CMD_HEAD     0xa5a5

#define RLEN_RANG_T_I (4+3*sizeof(double))
#define RLEN_TEC_MON  ((EParamMax - EParamVAux2)*sizeof(double) + 4)
#define S_HEAD_CREAT(CMD_HEAD,RLEN_TEC_MON,TEC_MONITOR_CMD) ((CMD_HEAD & 0xffff) <<16) | ((RLEN_TEC_MON & 0xff) << 8) | ( TEC_MONITOR_CMD & 0xff)


#define CHAN_EN(x,chan_no)   (x & (1<<(chan_no-1)))

//SYS control command list for TCP
#define SOC_POWER_CMD           0x11
#define TEC_MON_START_CMD       0x12
#define TEC_MON_STOP_CMD        0x13
#define TEC_ON_CMD              0x14
#define TEC_OFF_CMD             0x15
#define LASER_ION_CMD           0x16
#define LASER_IOFF_CMD          0x17
#define TEC_TSET_CMD            0x20
#define LASER_ISET_CMD          0x21
#define AD_START_CMD            0x18
#define AD_STOP_CMD             0x19
#define RANGE_T_I_CMD           0x1A  
#define LASER_IWAVE_CMD         0x30
#define TEC_MONITOR_DAT_CMD     0x31


struct thread_param_c{
		int             socket;
		pthread_mutex_t *pmutex;
		bool            stop;
};


static struct laserCNTRLcommand laser_cmd_list[EParamLMax] = {
	[EParamLfreq_sq1x]={"FREQSQ1X",0,"Hz",Freq2Phaseinc,Phaseinc2Freq},
        [EParamLphase_sq1x]={"PHASESQ1X",0,"degree",Angle2Phaseinit,Phaseinit2Angle},
	[EParamLfreq_sq2x]={"FREQSQ2X",0,"Hz",Freq2Phaseinc,Phaseinc2Freq},
	[EParamLphase_sq2x]={"PHASESQ2X",0,"degree",Angle2Phaseinit,Phaseinit2Angle},
	[EParamLfreq_sin]={"FREQSIN",0,"Hz",Freq2Phaseinc,Phaseinc2Freq},
	[EParamLphase_sin]={"PHASESIN",0,"degree",Angle2Phaseinit,Phaseinit2Angle},
	[EParamLamp_sin]={"AMPSIN",0,"mv",Voltag2Amp,Amp2Voltag},
	[EParamLfreq_saw]={"FREQSAW",0,"Hz",Freq2Phaseinc,Phaseinc2Freq},
	[EParamLamp_saw]={"AMPSAW",0,"mv",Voltag2Amp,Amp2Voltag},
	[EParamLdc1]={"DC1",0,"mv",Voltag2Amp,Amp2Voltag},
	[EParamLdc2]={"DC2",0,"mv",Voltag2Amp,Amp2Voltag},
	[EParamLdelay_saw]={"DLYSAW",0,"us",Time2Dly,Dly2Time},
	[EParamLouten]     ={"OUTEN",0,"",OUTEN2_dev,dev2_OUTEN2},
};

//default laser current settings
static double laserCNTRL_setvalue[EParamLMax]={
	[EParamLfreq_sq1x] = 10000,
        [EParamLphase_sq1x]= 0,
	[EParamLfreq_sq2x] = 20000,
	[EParamLphase_sq2x]= 90,
	[EParamLfreq_sin]  = 10000,
	[EParamLphase_sin] = 0,
	[EParamLamp_sin]   = 200,
	[EParamLfreq_saw]  = 50,
	[EParamLamp_saw]   = 1500,
	[EParamLdc1]       = 200,
	[EParamLdc2]       = 400,
	[EParamLdelay_saw] = 1000,
	[EParamLouten]     = 1
};


//laser current settings to voltage unit
static struct laserCNTRL_IVfunction laser_IV[EParamLMax]={
	[EParamLfreq_sq1x] ={ NULL},
        [EParamLphase_sq1x]= {NULL},
	[EParamLfreq_sq2x] = {NULL},
	[EParamLphase_sq2x]= {NULL},
	[EParamLfreq_sin]  = {NULL},
	[EParamLphase_sin] = {NULL},
	[EParamLamp_sin]   = {Ilaser_to_Volt},
	[EParamLfreq_saw]  = {NULL},
	[EParamLamp_saw]   = {Ilaser_to_Volt},
	[EParamLdc1]       = {Ilaser_to_Volt},
	[EParamLdc2]       = {Ilaser_to_Volt},
	[EParamLdelay_saw] = {NULL},
	[EParamLouten]     = {NULL}
        
};


struct xadc_command command_list[EParamMax] = {
				{EParamVccInt, 	VCC_INT_CMD,0, "mV",NULL},
				{EParamVccAux, 	VCC_AUX_CMD,0,"mV",NULL},
				{EParamVccBRam, VCC_BRAM_CMD,0, "mV",NULL},
				{EParamTemp, 	VCC_TEMP_CMD,0, "Degree Celsius",NULL},
				{EParamVAux0, 	VCC_EXT_VPN_CMD,0, "Degree Celsius",Temp_Ltm4644},
				{EParamVAux2, 	VCC_EXT_CH2_CMD,0, "mA",Volt_to_Ilaser},				
				{EParamVAux3, 	VCC_EXT_CH3_CMD,0, "mV",NULL},				
				{EParamVAux4, 	VCC_EXT_CH4_CMD,0, "mV",Volt_to_Vteclimit},
				{EParamVAux5, 	VCC_EXT_CH5_CMD,0, "A",Volt_to_Itec},				
				{EParamVAux7, 	VCC_EXT_CH7_CMD,0, "mW",Volt_to_Lpower},					
				{EParamVAux9, 	VCC_EXT_CH9_CMD,0, "mA",Volt_to_Ilaserlimit},
				{EParamVAux12, 	VCC_EXT_CH12_CMD,0, "A",Volt_to_Iteclimith},				
				{EParamVAux14, 	VCC_EXT_CH14_CMD,0, "A",Volt_to_Iteclimitc},					
				{EParamVAux16, 	VCC_EXT_CH16_CMD,0, "Degree Celsius",Volt_to_LTemp}	
};

void handle_sys_cntrl(void );
void handle_tec_monitor(void *arg);

int main(void)
{
	int status;

  pthread_t thread_sys_cntrl;
 
// creat thread for CMD and data
  status = pthread_create(&thread_sys_cntrl,NULL,handle_sys_cntrl,NULL);
	if(status)
	  {
	    printf("creat thread failes");
      goto EXIT0;
	  }

 // soc xadc initialization
 	if(xadc_core_init(EXADC_INIT_READ_ONLY) != 0)
	{
		perror("Couldn't Start the XADC Core\nExiting\n");
		goto EXIT1;
	}

 while(1){};
 
 
 	xadc_core_deinit();
	return 0;
	
EXIT1:
 	xadc_core_deinit();
  pthread_cancel(thread_sys_cntrl);

EXIT0:
  return  status;
}


/*******************************************
*     thread for tcp network
********************************************/
void handle_sys_cntrl(void ){
	int i,status,rec_n,send_len,*p_buf;
	unsigned int s_head;
	char s_buff[100];
	int sockfd;
	double  value,*result;
	struct sockaddr_in s_addr;
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
	struct thread_param_c th_param; 
    struct thread_param_t thread_parameter[2];
    pthread_t  thread_tec_monitor;

  
//sockets for CMD and data transmition
   sockfd=socket(AF_INET,SOCK_STREAM,0);
   if(sockfd<=0){
     printf("socket  failed,sockfd=%d\n",sockfd);
     goto EXIT1;
      
  }

//connection for CMD
   //set server IP address and port
   memset(&s_addr, 0, sizeof(s_addr));     
   s_addr.sin_port=htons(60100);
   s_addr.sin_family=AF_INET;
   s_addr.sin_addr.s_addr= inet_addr(SERVER_IP);

  //connect to the socket and server
  status=connect(sockfd,(struct sockaddr * )&s_addr,sizeof(s_addr));
	if(status){
     printf("command thread: connect socket failed, sockfd=%d\n",sockfd);
    goto EXIT0; 
  }

  while(1){
  //head detecting, only copy head
  rec_n = recv(sockfd,s_buff, sizeof(unsigned int),0);
  s_head = *(unsigned int *)s_buff;
  if(HEAD_TAG(s_head)!= CMD_HEAD){
     //deal with invalid cmd packet	
	 continue;
  }
  if(!HEAD_LEN(s_head)){
  //len=0, peer received package error
  continue;
  }	

  switch(HEAD_CMD(s_head)){	
  	case SOC_POWER_CMD:
  		break;
        case RANGE_T_I_CMD:
          p_buf =(int*) s_buff;
          *p_buf = S_HEAD_CREAT(CMD_HEAD,RLEN_RANG_T_I,RANGE_T_I_CMD);
          p_buf++;

          result =(double *)p_buf;//8 bytes alignment???
          *result =T_TEC_HIGH;
          result++;
          *result =T_TEC_LOW;
          result++;
          *result =L_I_LIM;
          break;
  	case TEC_MON_START_CMD:
  		// creat thread for CMD and data
  		th_param.socket = sockfd;
  		th_param.pmutex = &mtx;
  		th_param.stop = false;
        status = pthread_create(&thread_tec_monitor,NULL,handle_tec_monitor,&th_param);
	    if(status)
	    {
	       printf("creat thread failes");
        }
  		break;
   	case TEC_MON_STOP_CMD:
  		th_param.stop = true;
        pthread_join(thread_tec_monitor, NULL);
   		break;

  	case TEC_ON_CMD:
  		Laser_TON();
  		break;

  	case TEC_OFF_CMD:
	    Laser_TOFF();
  		break;  		

  	case LASER_ION_CMD:
      Laser_ION();
   	  break;

  	case LASER_IOFF_CMD:
      Laser_IOFF(); 	
	  break;

  	case TEC_TSET_CMD:
          rec_n = read(sockfd,s_buff,HEAD_LEN(s_head)-sizeof(unsigned int));
 	  value = Temp_to_Volt(*(double *)s_buff);// in degree
	  Laser_temp_set(value);
 	  break;

  	case LASER_ISET_CMD:
          rec_n = read(sockfd,&s_buff,HEAD_LEN(s_head)-sizeof(unsigned int));
          result =(double*) s_buff; 
          /***********************************************************
           *          laserCNTRL sysfs interface
           ***********************************************************/

          if (laserCNTRL_get_node(DEVICE_L_NAME)!= RET_SUCCESS)
          {
	     perror(DEVICE_L_NAME " Device Not Found");
	     goto EXIT0;
          }

          for (i=0; i < EParamLMax; i++){
            if(laser_IV[i].convlaser_I2V != NULL)
              laser_cmd_list[i].value = laser_IV[i].convlaser_I2V(*result);
            else
              laser_cmd_list[i].value = *result;
            result++;
      	    laserCNTRL_write_param(&laser_cmd_list[i]);
            printf("%s: %.2f %s\n",laser_cmd_list[i].name,laser_cmd_list[i].value, laser_cmd_list[i].unit);
          }

 	  break;  
       case AD_START_CMD:
         rec_n = read(sockfd,s_buff,HEAD_LEN(s_head)-sizeof(unsigned int));
         thread_parameter[0].chan_st = false;
         if( CHAN_EN(*s_buff,1)){

           thread_parameter[0].chan_st = true;
           thread_parameter[0].port_n = 60200;
           thread_parameter[0].server_ip = SERVER_IP;
         }
         thread_parameter[1].chan_st = false;
         if(CHAN_EN(*s_buff,2)){

           thread_parameter[1].chan_st = true;
           thread_parameter[1].port_n = 60300;
           thread_parameter[1].server_ip = SERVER_IP;
          }
          dma_ad_start(thread_parameter);
 	  	  break;
       case AD_STOP_CMD:
         dma_ad_stop(thread_parameter);
 	  	  break;
//  	case LASER_IWAVE:
//  		break;  			  	
  	default:
  		//invalid cmd packet
  		break;
  		
  }
// s_head = ntohl(s_head);
  if (HEAD_CMD(s_head) == RANGE_T_I_CMD)
      send_len = RLEN_RANG_T_I;
   else
      send_len = 4;  //sizeof(s_head);//only CMD returned
  pthread_mutex_lock(&mtx);
  status =writen(sockfd,s_buff,send_len);
  pthread_mutex_unlock(&mtx);
  
 }
 
 
EXIT0:
  close(sockfd);
EXIT1:
	return;
}


void handle_tec_monitor(void *arg){
	  struct thread_param_c *t_param1 = arg;
	  int sockfd = t_param1->socket;
	  char s_buff[100];
	  pthread_mutex_t *p_mutex = t_param1->pmutex;
	  int  clockfd;
	  int status,i;
	  struct itimerspec  ts;
	  uint64_t  timer_value;
	  int *s_head;
      double *pbuf;
	  
	clockfd = timerfd_create(CLOCK_REALTIME,0);
//	if (clockfd==-1)
		//errEXIT("failed to open timer file!");
	ts.it_interval.tv_sec = 10;	//1 second period
	ts.it_interval.tv_nsec = 0;		
	ts.it_value.tv_sec = 10;	    //first expiration
	ts.it_value.tv_nsec = 0;	
	timerfd_settime(clockfd,0,&ts,NULL);//arm the timer
	
  while(!t_param1->stop){
    status =read(clockfd,&timer_value,sizeof( uint64_t));//wait for 1 second
    s_head =(int*) s_buff;
    *s_head = S_HEAD_CREAT(CMD_HEAD,RLEN_TEC_MON,TEC_MONITOR_DAT_CMD);
    s_head++;

    pbuf =(double *)s_head;//8 bytes alignment???
    //update TEC setup information
    
	  for (i=EParamVAux2; i < EParamMax; i++){
		  command_list[i].xadc_result = XADC_RATIO * xadc_touch(command_list[i].parameter_id);
		  if (command_list[i].convphy_fn !=NULL)
		      command_list[i].xadc_result = command_list[i].convphy_fn( command_list[i].xadc_result);
		  printf("%s: %.2f %s\n",command_list[i].cmd_name,command_list[i].xadc_result, command_list[i].unit);
      *pbuf = command_list[i].xadc_result;
      pbuf++;
	  }     	
  
    pthread_mutex_lock(p_mutex);
    status =writen(sockfd,s_buff, RLEN_TEC_MON);
    pthread_mutex_unlock(p_mutex);    
  	
  }  		
	memset(&ts, 0, sizeof(struct itimerspec));
	timerfd_settime(clockfd,0,&ts,NULL);//disarm the timer
	close(clockfd);				
	
}

ssize_t writen(int fd, void *buffer, size_t n)
{
  ssize_t numWritten; /* # of bytes written by last write() */
  size_t totWritten; /* Total # of bytes written so far */
  char *buf;
   
   
  buf = buffer; /* No pointer arithmetic on "void *" */
  for (totWritten = 0; totWritten < n; ) { 
    numWritten = write(fd, buf, n - totWritten);
    if (numWritten <= 0) {
     if (numWritten == -1 && errno == EINTR)
      continue; /* Interrupted --> restart write() */
     else
      return -1; /* Some other error */
    }
    totWritten += numWritten;
    buf += numWritten;
  }
  return totWritten; /* Must be 'n' bytes if we get here */
}

ssize_t readn(int fd, void *buffer, size_t n)
{
  ssize_t numRead; /* # of bytes fetched by last read() */
  size_t totRead; /* Total # of bytes read so far */
  char *buf;
  
  buf = buffer; /* No pointer arithmetic on "void *" */
  for (totRead = 0; totRead < n; ) {
    numRead = read(fd, buf, n - totRead);
    if (numRead == 0) /* EOF */
    return totRead; /* May be 0 if this is first read() */
    if (numRead == -1) {
      if (errno == EINTR)
          continue; /* Interrupted --> restart read() */
      else
         return -1; /* Some other error */
    }
    totRead += numRead;
    buf += numRead;
  }
  return totRead; /* Must be 'n' bytes if we get here */
}


/*************************************************
*  functions for ADC voltage to physical units
*
**************************************************/

/*Laser PD value to laser power*/
double Volt_to_Lpower(double Volt)
{
	return 0;
}

//TEC cooling current limitation
double Volt_to_Iteclimitc(double Volt)
{
	double Current;
	Current=(Volt - TEC_VREF/2)/ITEC_SENSOR_C;
	return Current;
}

//TEC hrating current limitation
double Volt_to_Iteclimith(double Volt)
{
	double Current;
	Current=(TEC_VREF/2 - Volt)/ITEC_SENSOR_C;
	return Current;
}

//TEC voltage limitation
double Volt_to_Vteclimit(double Volt )
{
	double tec_volt;
	tec_volt=Volt*5.0;
	return tec_volt;
}

//TEC current,A
double Volt_to_Itec(double Volt )
{
	double Current;
	Current=(Volt-TEC_VREF/2)/ITEC_SENSOR_C;
	return Current;
}

//laser current limitatiom,mA
double Volt_to_Ilaserlimit(double Volt)
{
	double Current;
	Current=Volt*LASER_VI_TRANSFER/1000.0;
	return Current;
}

//laser current, mA
double Volt_to_Ilaser(double Volt)
{
	double Current;
	Current=Volt*LASER_VI_TRANSFER/1000.0;
	return Current;
}

//laser temperature
double Volt_to_LTemp(double Volt)
{
	double Temp_set;
	Temp_set=Volt*(T_TEC_HIGH-T_TEC_LOW)/TEC_VREF+T_TEC_LOW;
	return Temp_set;
}

//LTM4644 temperature
double Temp_Ltm4644(double Volt)
{
	double Temp_set;
	Temp_set = (LTM4644_VG0 - Volt)/LTM4644_COFF - 273;
	return Temp_set;	
}


//laser current->voltage, mV
double Ilaser_to_Volt(double Current)
{
	double Volt;
	Volt= Current*1000.0/LASER_VI_TRANSFER;
	return Volt;
}




//*****************************************************************************
//----------------------system status and control functions-------------------//
//*****************************************************************************
//turn on laser current driver
void Laser_ION(void)
{
   GPIO_Write(LASER_ON,1);
}
//turn off laser current driver
void Laser_IOFF(void)
{
   GPIO_Write(LASER_ON,0);
}

/*TEC controller on*/
void Laser_TON(void)
{
   GPIO_Write(nTEC_ON,0);
}

/*TEC controller off*/
void Laser_TOFF(void)
{
   GPIO_Write(nTEC_ON,1);	 
}

/*TEC status query*/
unsigned int TContr_status(void)
{
    return GPIO_Read(nTEC_ON);
}

/*laser current dirver status query*/
unsigned int IContr_status(void)
{
    return GPIO_Read(LASER_ON);
}

/*laser temperature setting to voltage*/
double Temp_to_Volt(double temperature)
{
	double Temp_set_volt;
	Temp_set_volt=TEC_VREF * (temperature - T_TEC_LOW)/(T_TEC_HIGH-T_TEC_LOW);
	return Temp_set_volt;
}






