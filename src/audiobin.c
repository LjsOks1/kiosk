#include <string.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "kiosk.h"

/* Creates simple bin that handles audio.*/
GstElement* audiobin_new()
{
   GstElement *audiobin,*metee,*adder,*monqueue,*recqueue,*micsrc,*micqueue,*mequeue;
   GstPad *pad,*dec_pad_sink;
			GstCaps *me_caps;
   //Create bin, elements, caps and link everything.
   audiobin=gst_bin_new("audiobin");
   metee=gst_element_factory_make("tee","metee");
   adder=gst_element_factory_make("adder","adder");
   monqueue=gst_element_factory_make("queue","monqueue");
   recqueue=gst_element_factory_make("queue","recqueue");
			micqueue=gst_element_factory_make("queue","micqueue");
			micsrc=gst_element_factory_make("alsasrc","micsrc");
			mequeue=gst_element_factory_make("queue","mequeue");
   gst_bin_add_many(GST_BIN(audiobin),
			    metee,
							adder,
							monqueue,
							recqueue,
//       micqueue,
       mequeue,
							micsrc,
							NULL);
   me_caps=gst_caps_new_simple("audio/x-raw",
			                             "format",G_TYPE_STRING,"S16LE",
                                "channel",G_TYPE_INT,2,
																																"rate",G_TYPE_INT,44100,
																																"layout",G_TYPE_STRING,"interleaved",
                                NULL);
 
   gst_element_link(mequeue,metee);
   gst_element_link(metee,monqueue);
   gst_element_link_many(metee,recqueue,adder,NULL);
			gst_element_link_many(micsrc,adder,NULL);
   
 	 pad=gst_element_get_static_pad(mequeue,"sink");
   gst_element_add_pad(audiobin,gst_ghost_pad_new("mesink",pad));
   gst_object_unref(pad);

   pad=gst_element_get_static_pad(adder,"src");
   gst_element_add_pad(audiobin,gst_ghost_pad_new("recsrc",pad));
   gst_object_unref(pad);
 		
			pad=gst_element_get_static_pad(monqueue,"src");
   gst_element_add_pad(audiobin,gst_ghost_pad_new("monsrc",pad));
   gst_object_unref(pad);
   
			/* set initial parameters */
    g_object_set(G_OBJECT(micsrc),"device","plughw:2,0","buffer-time",400000,NULL);
//    g_object_set(G_OBJECT(mequeue),"leaky",2,NULL);
//				g_object_set(G_OBJECT(recqueue),"leaky",1,NULL);
   gst_caps_unref(me_caps);  
   return audiobin;
}



