#include <gst/gst.h>
#include <gst/video/video.h>
#include "kiosk.h"

/* Creates the camera live source. Outputs RGBA with 30fps.  
*  Should be used as the second input of the video mixer. 
*  Scaling should be set when linked to the mixer element! 
*  bin elements : v4l2src ! videoflip ! alpha ! queue 
*  src ghost pad is added as an output to the bin.*/
GstElement* cambin_new()
{
   GstElement *cambin,*v4l2src,*videoflip,*alpha,*queue,*camtee,*teequeue;
   GstCaps *cam_caps,*alpha_caps;
   GstPad *pad;
   cambin=gst_bin_new("cambin");
   v4l2src=gst_element_factory_make("v4l2src","camv4l2src");
   camtee=gst_element_factory_make("tee","camtee");
   teequeue=gst_element_factory_make("queue","teequeue");
   videoflip=gst_element_factory_make("videoflip","camvideoflip");
   alpha=gst_element_factory_make("alpha","camalpha");
   queue=gst_element_factory_make("queue","camqueue");
   gst_bin_add_many(GST_BIN(cambin),v4l2src,videoflip,alpha,queue,
         camtee,teequeue,NULL);
   cam_caps=gst_caps_new_simple("video/x-raw",
                                "format",G_TYPE_STRING,"YUY2",
                                "width",G_TYPE_INT,CAMERA_RES_WIDTH,
                                "height",G_TYPE_INT,CAMERA_RES_HEIGHT,
                                "framerate",GST_TYPE_FRACTION,FRAMES_PER_SEC,1,
                                NULL);
   alpha_caps=gst_caps_new_simple("video/x-raw",
                                "format",G_TYPE_STRING,"AYUV",
                                "width",G_TYPE_INT,CAMERA_RES_WIDTH,
                                "height",G_TYPE_INT,CAMERA_RES_HEIGHT,
                                "framerate",GST_TYPE_FRACTION,FRAMES_PER_SEC,1,
                                NULL);
   
   gst_element_link_filtered(v4l2src,camtee,cam_caps);
   gst_caps_unref(cam_caps);
   gst_element_link_many(camtee,videoflip,alpha,NULL);
   gst_element_link_filtered(alpha,queue,alpha_caps);
   gst_caps_unref(alpha_caps);  
   gst_element_link(camtee,teequeue);
   
   pad=gst_element_get_static_pad(queue,"src");
   gst_element_add_pad(cambin,gst_ghost_pad_new("src",pad));
   gst_object_unref(pad);

   pad=gst_element_get_static_pad(teequeue,"src");
   gst_element_add_pad(cambin,gst_ghost_pad_new("mon",pad));
   gst_object_unref(pad);
   /* set initial parameters */
   g_object_set(G_OBJECT(alpha),"method",3,
//                        "alpha",0.0,
                        "angle",60.0,
                        "noise-level",1.0,
                        "target-r",136,
                        "target-g",177,
                        "target-b",133,
                        "black-sensitivity",100,
                        "white-sensitivity",110,
                        "prefer-passthrough",TRUE,
                        NULL);
   g_object_set(G_OBJECT(videoflip),"method",4,NULL); 
   g_object_set(G_OBJECT(teequeue),"leaky",1,NULL);
   return cambin;
}

