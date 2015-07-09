#include <stdio.h>
#include <time.h>
#include <string.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "kiosk.h"

/* Creates the record bin.
*   
*  bin: queue ! x264enc ! mp4mux name=mux ! filesink 
        alsasrc ! voaacenc ! mux. */

GstElement* recordbin_new()
{
   GstElement *recordbin,*vqueue,*vscale,*vconv,*venc,*mux, *sink,
              *aqueue1, *audioconv,*aenc,*aqueue2;

   GstPad *pad;
   GstCaps *out_caps,*record_caps;
   recordbin=gst_bin_new("recordbin");
   vqueue=gst_element_factory_make("queue","vqueue");
   vscale=gst_element_factory_make("videoscale","vscale");
   vconv=gst_element_factory_make("videoconvert","recvconv");
   venc=gst_element_factory_make("x264enc","venc");
   mux=gst_element_factory_make("mp4mux","mux");
   sink=gst_element_factory_make("filesink","recordersink");
   aenc=gst_element_factory_make("voaacenc","aenc");
   aqueue1=gst_element_factory_make("queue","aqueue1");
   audioconv=gst_element_factory_make("audioconvert","audioconv");
   aqueue2=gst_element_factory_make("queue","aqueue2");
   gst_bin_add_many(GST_BIN(recordbin),vqueue,vscale,vconv,venc,mux,sink,
             aqueue1,audioconv,aenc,aqueue2,NULL);

   record_caps=gst_caps_new_simple("video/x-raw",
                                "width",G_TYPE_INT,REC_RES_WIDTH,
                                "height",G_TYPE_INT,REC_RES_HEIGHT,
                                NULL);
   out_caps=gst_caps_new_simple("video/x-raw",
                                "format",G_TYPE_STRING,"I420",
                                "width",G_TYPE_INT,REC_RES_WIDTH,
                                "height",G_TYPE_INT,REC_RES_HEIGHT,
                                NULL);

   gst_element_link(vqueue,vscale);
   gst_element_link_filtered(vscale,vconv,record_caps);
   gst_element_link_filtered(vconv,venc,out_caps);
   gst_element_link_many(venc,mux,sink,NULL);
   gst_element_link_many(aqueue1,audioconv,aenc,aqueue2,mux,NULL);
   gst_caps_unref(record_caps);
   gst_caps_unref(out_caps);

   /* Setup video ghost pad for recorderbin */
   pad=gst_element_get_static_pad(vqueue,"sink");
   gst_element_add_pad(recordbin,gst_ghost_pad_new("sink",pad));
   gst_object_unref(pad);

   pad=gst_element_get_static_pad(aqueue1,"sink");
   gst_element_add_pad(recordbin,gst_ghost_pad_new("asink",pad));
   gst_object_unref(pad);

  /* Set initial parameters */
   g_object_set(G_OBJECT(venc),"cabac",FALSE,"ref",2,"me",2,"subme",6,NULL);
   g_object_set(G_OBJECT(sink),"location","test.mp4","qos",FALSE,NULL);
   g_object_set(G_OBJECT(vqueue),"leaky",1,NULL);
   return recordbin;
}

void 
recordbin_start(gpointer user_data)
{
   CustomData *d=user_data;
   GstElement *filesink;
   time_t rawtime;
   struct tm *timeinfo;
   char media_filename[100];

   filesink=gst_bin_get_by_name(GST_BIN(d->recordbin),"recordersink");
  
   time(&rawtime);
   timeinfo=localtime(&rawtime);
   sprintf(media_filename,"record_%02i%02i%02i_%02i%02i%02i.mp4",
         timeinfo->tm_year-100,timeinfo->tm_mon+1,timeinfo->tm_mday,
         timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
   g_object_set(G_OBJECT(filesink),"location",media_filename,NULL);
   strcpy(d->sink_filename,media_filename);
 
   /* Lets add the recordbin to the pipeline. */
   gst_bin_add_many(GST_BIN(d->pipeline),d->recordbin,NULL);
   gst_element_link_pads(d->valve,NULL,d->recordbin,"sink");
   gst_element_link_pads(d->avalve,NULL,d->recordbin,"asink");
   g_object_set(G_OBJECT(d->valve),"drop",FALSE,NULL);
   g_object_set(G_OBJECT(d->avalve),"drop",FALSE,NULL);

   gst_element_set_state(d->recordbin,GST_STATE_PAUSED);
   gst_object_unref(filesink);
   /* This should stop the recording in 30 seconds. 
      Comment out if you don't want that. Now in main.c */
   g_timeout_add_seconds (d->config[d->selected_config].duration,(GSourceFunc)recordbin_stop,d);    
}


/* This function should return FALSE otherwise the timer that triggered 
* the callback triggers it again!!!!! */
gboolean 
recordbin_stop(CustomData *d)
{
   g_object_set(G_OBJECT(d->valve),"drop",TRUE,NULL);
   g_object_set(G_OBJECT(d->avalve),"drop",TRUE,NULL);

/* push EOS to the recordbin */
   GstPad *rec_sink;
   rec_sink=gst_element_get_static_pad(d->recordbin,"sink");
   g_print("Sending EOS event..., first set pipeline to get recordbin eos message \n");
   g_object_set(G_OBJECT(d->pipeline),"message-forward",TRUE,NULL);
   if(!gst_pad_send_event(rec_sink,gst_event_new_eos())) {
      g_print("Couldn't end EOS event to recordbin.\n");
   }
   gst_object_unref(rec_sink);

   GstPad *arec_sink;
   arec_sink=gst_element_get_static_pad(d->recordbin,"asink");
   gst_pad_send_event(arec_sink,gst_event_new_eos());
   gst_object_unref(arec_sink);

   return FALSE;
}
