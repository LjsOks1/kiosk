#include <gst/gst.h>
#include <gst/video/video.h>
#include "kiosk.h"

/* Creates the gesture  bin.
*  bin: queue ! videoscale ! xvimagesink */
GstElement* gesturebin_new()
{
   GstElement *gesturebin,*gesturequeue,*gesturescale,*facedetect,*skindetect,
            *handdetect,*gesturesink,*gestureconv1,*gestureconv2;
   GstCaps *gest_caps,*out_caps;
   GstPad *pad;
   gesturebin=gst_bin_new("gesturebin");
   gesturequeue=gst_element_factory_make("queue","gesturequeue");
   gesturescale=gst_element_factory_make("videoscale","gesturescale");
   gestureconv1=gst_element_factory_make("videoconvert","gestureconv1");
   skindetect=gst_element_factory_make("edgedetect","skindetect");
   handdetect=gst_element_factory_make("handdetect","handdetect");
   facedetect=gst_element_factory_make("facedetect","facedetect");
   gestureconv2=gst_element_factory_make("videoconvert","gestureconv2");
//   gesturescale=gst_element_factory_make("videoscale","gesturescale");
   gesturesink=gst_element_factory_make("xvimagesink","gesturesink");
   gst_bin_add_many(GST_BIN(gesturebin),gesturequeue,facedetect,gestureconv1,
             handdetect,gestureconv2,gesturescale,gesturesink,skindetect,NULL);
   gest_caps=gst_caps_new_simple("video/x-raw",
                                "width",G_TYPE_INT,320,
                                "height",G_TYPE_INT,240,
                                NULL);
   out_caps=gst_caps_new_simple("video/x-raw",
                                "format",G_TYPE_STRING,"RGB",
                                NULL);
//   gst_element_link_filtered(displayqueue,displayconvert,out_caps);
   gst_element_link_filtered(gesturescale,gestureconv1,gest_caps);
   gst_element_link_many(gestureconv1,handdetect,gestureconv2,
             gesturesink,NULL);
//   gst_element_link_many(gesturequeue,gesturescale,NULL);
//   gst_element_link_filtered(gesturescale,gesturesink,display_caps);
   gst_caps_unref(gest_caps);
   gst_caps_unref(out_caps);
   pad=gst_element_get_static_pad(gesturescale,"sink");
   gst_element_add_pad(gesturebin,gst_ghost_pad_new("sink",pad));
   gst_object_unref(pad);

   /* Set initial parameters */
   g_object_set(G_OBJECT(gesturesink),"display",":0.0","qos",FALSE,NULL);
   g_object_set(G_OBJECT(gesturesink),"sync",FALSE,NULL);
   g_object_set(G_OBJECT(gesturequeue),"leaky",1,NULL);
   g_object_set(G_OBJECT(skindetect),"method",0,NULL);
 
   return gesturebin;
}

