
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include <sys/io.h>             // LPT port basic I/O access
#include <signal.h>
#include <syslog.h>
#include <string.h>
#define BASEPORT 0x378          // LPT1
#include <gst/gst.h>
#include "kiosk.h"

void signal_handler(int sig);
int user_signal = 0;

gpointer gpi_thread(gpointer p)
    {
    int  b, c1, c2, pin1, pin2;
//*************     LPT Port hardware I/O Access Enable     *************
    if (ioperm(BASEPORT, 3, 1)==-1) {
//        perror("ioperm");
        return NULL;
    }
//***********************************************************************
    b=0;
    c1=c2=0;
    pin1=pin2=0;
    b=((inb(BASEPORT+1)&120))>>3; //default: 15 DEC
    c1=(b&8)>>3;        //10-es pin
    c2=(b&4)>>2;        //12-es pin
    if(c1==0) pin1=1 ;
    if(c2==0) pin2=1 ;
    syslog(LOG_INFO,"Initial GPI state is : %d %d \n", pin1, pin2); 
// /var/log/user.log-ba logol
    CustomData *cdata=(CustomData*)p;
    do {
        b=((inb(BASEPORT+1)&120))>>3; //default: 15 DEC
        c1=(b&8)>>3;    //10-es pin
        c2=(b&4)>>2;    //12-es pin

    if((pin1==0)&&(c1==0)){     //Pin1 lenyom.
     gst_bus_post(cdata->bus,gst_message_new_application(
        GST_OBJECT_CAST(cdata->pipeline),
        gst_structure_new_empty("Pin1Pressed")));
//        printf("A");
        pin1=1;
        syslog(LOG_INFO,"Sending 'A' GPI triggered. \n"); // /var/log/user.log-ba logol
        }

    if((pin1==1)&&(c1==1)){     //Pin1 felenged. (LPT port default: b==15)
//        printf("a");
        pin1=0;
        syslog(LOG_INFO,"Sending 'a' GPI triggered. \n");
        }

    if((pin2==0)&&(c2==0)){     //Pin2 lenyom.
      gst_bus_post(cdata->bus,gst_message_new_application(
        GST_OBJECT_CAST(cdata->pipeline),
        gst_structure_new_empty("Pin2Pressed")));
//       printf("B");
        pin2=1;
        syslog(LOG_INFO,"Sending 'B' GPI triggered. \n"); // /var/log/user.log-ba logol
        }

    if((pin2==1)&&(c2==1)){     //Pin2 felenged. (LPT port default: b==15)
//        printf("b");
        pin2=0;
        syslog(LOG_INFO,"Sending 'b' GPI triggered. \n");
        }

        fflush(stdout);

    usleep(100000);  // waits 100 ms

    } while (1) ;

    closelog();

    if(ioperm(BASEPORT, 3, 0)) { // LPT port hardware I/O access denied
        perror("ioperm");
    }
   return NULL;
}


