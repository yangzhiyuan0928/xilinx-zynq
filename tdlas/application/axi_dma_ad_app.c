
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "AXI_gpio.h"
#include "axi_dma_ad.h"

#define CHAN1X_DEVNAME  "/dev/channel1x"
#define CHAN2X_DEVNAME  "/dev/channel2x"

#define DMA_BUF_NUM  32

/* Set up circular buffer */

ElemType buf1[DMA_BUF_NUM];
ElemType buf2[DMA_BUF_NUM];


/* Thread capture ADC samples and put it to Circular buffer */
int captureADCSamples(void *ptr)
{
    int fd = -1, read_len;
	int socketfd,status;
    struct thread_param_t  *th_p = ptr;
    unsigned int capture_num =0;;
    struct sockaddr_in s_addr;
    
    //Open XADC dev node	
    fd = open(th_p->devfile_name, O_RDONLY);
    if(fd < 0){
    printf("captureThread :: dev open error \n");
    goto EXIT0;	
    }


    //sockets for CMD and data transmition
    socketfd=socket(AF_INET,SOCK_STREAM,0);
   	if(socketfd<=0){
         printf("socket  failed,sockfd=%d\n",socketfd);
         return -ENODEV;

    }

   //connection for CHAN
   //set server IP address and port
   memset(&s_addr, 0, sizeof(s_addr));     
   s_addr.sin_port=htons(th_p->port_n);
   s_addr.sin_family=AF_INET;
   s_addr.sin_addr.s_addr= inet_addr(th_p->server_ip);

   //connect to the socket
   status=connect(socketfd,(struct sockaddr * )&s_addr,sizeof(s_addr));
	 if(status){
      printf("command thread: connect socket failed, sockfd=%d\n",socketfd);
      goto EXIT1; 
   }

	if (ioctl(fd, AXI_ADC_DMA_CONFIG)) {
            printf("ADC DMA CONFIG failed: %s\n", strerror(errno));
	}
  if (ioctl(fd, AXI_ADC_DMA_START) ) {
	    printf("ADC DMA START  failed: %s\n", strerror(errno));
	}
  if (ioctl(fd, AXI_ADC_ADSTART) ) {
	    printf("ADC START  failed: %s\n", strerror(errno));
	}

    th_p->flag_ad = true;

  while ( th_p->capture_start ) {
	read_len = read(fd, th_p->buf_p[capture_num].buf, FIFO_SIZE) ;
    read_len = writen(socketfd,th_p->buf_p[capture_num].buf, FIFO_SIZE);
    printf("send: %d \n",capture_num);
    capture_num ++;
    if(capture_num >= DMA_BUF_NUM) 
       capture_num = 0;
  }

  if (ioctl(fd, AXI_ADC_DMA_STOP)) {
	    printf("ADC DMA STOP failed: %s\n", strerror(errno));
	}
  if (ioctl(fd, AXI_ADC_ADSTOP) ) {
	    printf("ADC STOP failed: %s\n", strerror(errno));
	}

    printf("axi_adc_app :: Data Capture Thread Terminated \n");
    close(socketfd);
    close(fd);
    return 0;
EXIT1:
	close(fd);
EXIT0:
	return -1;
}

int dma_ad_start(struct thread_param_t *thread_parameter){

    int rc;
    //disable ad_start
    GPIO_Write(AD_START,0);

    if(thread_parameter[0].chan_st){
       thread_parameter[0].devfile_name = CHAN1X_DEVNAME;;
       thread_parameter[0].buf_p = buf1;
       thread_parameter[0].capture_start =true;
       thread_parameter[0].flag_ad = false;
       rc = pthread_create(&thread_parameter[0].t_datacapture, NULL, captureADCSamples,&thread_parameter[0] );
       if (rc != 0) {
	        perror("pthread_create :: chan1x error \n");
	         goto EXIT0;
       }
       while (!thread_parameter[0].flag_ad); 
    }
    
    if(thread_parameter[1].chan_st){
       thread_parameter[1].devfile_name = CHAN2X_DEVNAME;
       thread_parameter[1].buf_p = buf2;
       thread_parameter[1].capture_start =true;
       thread_parameter[1].flag_ad = false;    	    
       rc = pthread_create(&thread_parameter[1].t_datacapture, NULL, captureADCSamples, &thread_parameter[1]);
       if (rc != 0) {
	         perror("pthread_create :: chan2x error \n");
	         goto EXIT1;
       }

       while (!thread_parameter[1].flag_ad );
    }
    
    GPIO_Write(AD_START,1);
    return 0;
EXIT1:
	  pthread_cancel(thread_parameter[0].t_datacapture);
EXIT0:
	return -1;    
       
}

int dma_ad_stop(struct thread_param_t *thread_parameter){

    if(thread_parameter[0].chan_st){
       thread_parameter[0].capture_start = false;	
      pthread_join(thread_parameter[0].t_datacapture, NULL);
    }
    
    if(thread_parameter[1].chan_st){
      thread_parameter[1].capture_start = false;
      pthread_join(thread_parameter[1].t_datacapture, NULL);        	
    }
 
    //disable ad_start
    GPIO_Write(AD_START,0);

    printf("Exiting application :v1 \n");

    return 0;

}
