#include <string.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "kiosk.h"

/*
static void on_new_decoded_pad(GstElement *decbin, 
                               GstPad *srcpad,
                               gpointer data)
{  
   gboolean result;
   GstPad *sinkpad;
   GstCaps *new_pad_caps;
   char* name;

   name=gst_pad_get_name(srcpad);
   new_pad_caps=gst_pad_query_caps(srcpad,NULL);
   g_print("Caps:%s\n",gst_caps_to_string(new_pad_caps));

//   if(strncmp(name,"video",5)==0) {
       sinkpad=gst_element_get_static_pad(data,"sink");
       result=gst_pad_link(srcpad,sinkpad);
       if(!result) {
          g_printerr("Couldn't link background decodebin element. Background will be missing...\n");
       }
//   }
}
*/

gboolean eos_callback(GstPad *pad,
                      GstObject *parent,
                      GstEvent *event)
{
    GstEvent *seek_event;
    GstElement *bkgdec;
    GValue v=G_VALUE_INIT;
    GstPad *srcpad;
    GstIterator *srcpads;
    gboolean result;

    g_print("Decodebin received EOS. Someone should handle that...\n");
    
    bkgdec=gst_pad_get_parent_element(pad);
    if(bkgdec->numsrcpads>0) {
        srcpads=gst_element_iterate_src_pads(bkgdec);
        gst_iterator_next(srcpads,&v);
        srcpad=GST_PAD(g_value_get_object(&v));
        seek_event=gst_event_new_seek ( 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                         GST_SEEK_TYPE_SET, 0,
                         GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);     
        result=gst_pad_send_event(srcpad,seek_event);
        if(result==TRUE) {
            g_print("seek event sent OK.\n");
        } else {
           g_print("seek sent FAILED.\n");
        }
        g_value_reset(&v);
        g_value_unset(&v);
        gst_iterator_free(srcpads);
        return TRUE;
    }
    return gst_pad_event_default(pad,parent,event);
}

/* Creates simple bin that plays a background JPEG image or sequence 
*  with 30fps. Should be used as the first input of the video mixer. 
*  Scaling should be set when linked to the mixer element! 
*  bin elements : multifilesrc ! jpegdec ! videoscale ! queue
*  src ghost pad is added as an output to the bin.*/
GstElement* bkgbin_new(CustomData *data)
{
   GstElement *bkgbin,*bkgsrc,*bkgdec,*bkgscale,*bkgqueue,*bkgfreeze;
   GstCaps *freeze_caps,*scale_caps;
   GstPad *pad,*dec_pad_sink;
   //Create bin, elements, caps and link everything.
   bkgbin=gst_bin_new("bkgbin");
   bkgsrc=gst_element_factory_make("multifilesrc","bkgsrc");
   bkgdec=gst_element_factory_make("jpegdec","bkgdec");
   bkgfreeze=gst_element_factory_make("imagefreeze","bkgfreeze");
   bkgscale=gst_element_factory_make("videoscale","bkgscale");
   bkgqueue=gst_element_factory_make("queue","bkgqueue");
   gst_bin_add_many(GST_BIN(bkgbin),bkgsrc,bkgdec,bkgscale,bkgqueue,bkgfreeze,NULL);
   freeze_caps=gst_caps_new_simple("video/x-raw",
                                "framerate",GST_TYPE_FRACTION,FRAMES_PER_SEC,1,
                                NULL);
   scale_caps=gst_caps_new_simple("video/x-raw",
//                                "format",G_TYPE_STRING,"YUV",
//                                "alpha",G_TYPE_INT,0,
//                                "framerate",GST_TYPE_FRACTION,FRAMES_PER_SEC,1,
                                "width",G_TYPE_INT,CAMERA_RES_WIDTH,
                                "height",G_TYPE_INT,CAMERA_RES_HEIGHT,
                                NULL);
//   gst_element_link(bkgsrc,bkgdec);
   gst_element_link_many(bkgsrc,bkgdec,NULL);
   /* decodebin's src pad is a sometimes pad - it gets created dynamically */
//   g_signal_connect(bkgdec,"pad-added",G_CALLBACK(on_new_decoded_pad),bkgscale);
   gst_element_link(bkgdec,bkgscale);
   gst_element_link_filtered(bkgscale,bkgfreeze,scale_caps);
   gst_element_link_filtered(bkgfreeze,bkgqueue,freeze_caps);
//   gst_element_link_filtered(bkgscale,bkgqueue,scale_caps);
   gst_caps_unref(scale_caps);
   gst_caps_unref(freeze_caps);
   //Create the ghost src pad for the bin.
   pad=gst_element_get_static_pad(bkgqueue,"src");
   gst_element_add_pad(bkgbin,gst_ghost_pad_new("src",pad));
   gst_object_unref(pad);
   
   /* set initial parameters */
   g_object_set(G_OBJECT(bkgsrc),"location",data->config[data->selected_config].background,
          "loop",TRUE,NULL);
//   g_object_set(G_OBJECT(bkgqueue),"leaky",2,NULL);
   
   /* set eos handler function */
   dec_pad_sink=gst_element_get_static_pad(bkgdec,"sink");
//   gst_pad_set_event_function(dec_pad_sink,eos_callback);

   return bkgbin;
}



/* Sets the source jpeg file location for the background bin. */
int bkgbin_set_source(CustomData *data)
{
   GstElement *bkgsrc;

   bkgsrc=gst_bin_get_by_name(GST_BIN(data->bkgbin),"bkgsrc");
   gst_element_set_state(data->bkgbin,GST_STATE_NULL);
   g_object_set(G_OBJECT(bkgsrc),"location",
			        data->config[data->selected_config].background,NULL);
   gst_element_sync_state_with_parent(data->bkgbin);
			gst_object_unref(bkgsrc);
//   g_object_set(G_OBJECT(bkgsrc),"start-index",start_index,NULL);
//   g_object_set(G_OBJECT(bkgsrc),"stop-index",stop_index,NULL);
//   g_object_set(G_OBJECT(bkgsrc),"loop",loop,NULL);
   return 0;
}
