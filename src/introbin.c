#include <string.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "kiosk.h"

gboolean introbin_set_pad_offset(CustomData *data)
{
  gint64 pos2;
  pos2=gst_element_get_base_time(data->pipeline);
  GstClock *clock;
  clock=gst_pipeline_get_clock(GST_PIPELINE(data->pipeline));
  GstClockTime clock_time;
  clock_time=gst_clock_get_time(clock);
  gst_object_unref(clock);
  g_print("Pipeline times: base_time=%lld\n clock_time=%lld",
		            pos2,clock_time);
  GstElement *dec=gst_bin_get_by_name(GST_BIN(data->introbin),"introdec");
  GstPad *src_pad1,*src_pad2;
  src_pad1=gst_element_get_static_pad(GST_ELEMENT(dec),"src_0");
  gst_pad_set_offset(src_pad1,clock_time-pos2);
  gst_object_unref(src_pad1);
  src_pad2=gst_element_get_static_pad(GST_ELEMENT(dec),"src_1");
  gst_pad_set_offset(src_pad2,clock_time-pos2);
  gst_object_unref(src_pad2);

  return TRUE;
}
static void on_new_decoded_pad(GstElement *introdec, 
                               GstPad *srcpad,
                               gpointer data)
{  
   GstPadLinkReturn result;
   GstPad *sinkpad;
   GstCaps *new_pad_caps;

   CustomData *cdata=(CustomData*)data;
   GstElement *introbin=cdata->introbin;

   new_pad_caps=gst_pad_query_caps(srcpad,NULL);
   g_print("Caps:%s\n",gst_caps_to_string(new_pad_caps));

   /* Setup src pad offset, sync with pipeline. */
   gint64 pos2;
   pos2=gst_element_get_base_time(cdata->pipeline);
   GstClock *clock;
   clock=gst_pipeline_get_clock(GST_PIPELINE(cdata->pipeline));
   GstClockTime clock_time;
   clock_time=gst_clock_get_time(clock);
   gst_object_unref(clock);
//   g_print("Pipeline times: base_time=%lld\n clock_time=%lld\n",
//		            pos2,clock_time);
   gst_pad_set_offset(srcpad,clock_time-pos2);
   cdata->introbin_offset=clock_time-pos2;

   if(strncmp(gst_caps_to_string(new_pad_caps),"video",5)==0) {
       GstElement *vqueue;
       vqueue=gst_bin_get_by_name(GST_BIN(introbin),"introscale");
       sinkpad=gst_element_get_static_pad(vqueue,"sink");
       result=gst_pad_link(srcpad,sinkpad);
       if(result!=GST_PAD_LINK_OK) {
          g_printerr("Couldn't link introbin decodebin video pad...\n");
       }
       gst_object_unref(vqueue);
   }
   if(strncmp(gst_caps_to_string(new_pad_caps),"audio",5)==0) {
       GstElement *arate;
       arate=gst_bin_get_by_name(GST_BIN(introbin),"introaudiorate");
       sinkpad=gst_element_get_static_pad(arate,"sink");
       result=gst_pad_link(srcpad,sinkpad);
       if(result!=GST_PAD_LINK_OK) {
          GstCaps *peer_caps;
	  peer_caps=gst_pad_query_caps(sinkpad,NULL);
	  g_print("SinkCaps:%s\n",gst_caps_to_string(peer_caps));
          g_printerr("Couldn't link introbin decodebin audio pad...\n");
	  gst_caps_unref(peer_caps);
       }
       gst_object_unref(arate);
   }
}

/*
gboolean intro_eos_callback(GstPad *pad,
                      GstObject *parent,
                      GstEvent *event)
{
    GstEvent *seek_event;
    GstElement *bkgdec;
    GValue v=G_VALUE_INIT;
    GstPad *srcpad;
    GstIterator *srcpads;
    gboolean result;

    g_print("Introbin received EOS. ...\n");
    
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
*/
int play_intro(gpointer app) 
{

   CustomData *d=(CustomData*)app;
   
   GstElement *mixer=gst_bin_get_by_name(GST_BIN(d->pipeline),"mixer1");
   GstElement *vvalve=gst_bin_get_by_name(GST_BIN(d->introbin),"introvvalve");
   GstElement *avalve=gst_bin_get_by_name(GST_BIN(d->introbin),"introavalve");
   gst_bin_add_many(GST_BIN(d->pipeline),d->introbin,NULL);
   gst_element_link_pads(d->introbin,"vsrc",mixer,NULL);
   gst_element_link_pads(d->introbin,"asrc",d->adder,NULL);
   g_object_set(G_OBJECT(vvalve),"drop",FALSE,NULL);
   g_object_set(G_OBJECT(avalve),"drop",FALSE,NULL);
   gst_element_set_state(d->introbin,GST_STATE_PAUSED);
   g_object_unref(mixer);
   g_object_unref(vvalve);
   g_object_unref(avalve);
   return 0;
}

GstPadProbeReturn
introbin_pad_block_cb(GstPad *pad,
                      GstPadProbeInfo *info,
		                    gpointer user_data)
{
   CustomData *data=(CustomData*)user_data;  
//   g_print("Introbin pad callback triggered. Type:%d\n",GST_PAD_PROBE_INFO_TYPE(info));
   if(GST_PAD_PROBE_INFO_TYPE(info)|GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
      GstEvent *event=GST_PAD_PROBE_INFO_EVENT(info);
//      g_print("Introbin blocking pad received %s event...",GST_EVENT_TYPE_NAME(event));
      if(strcmp(GST_EVENT_TYPE_NAME(event),"stream-start")==0) {
//         g_print("Dropped.\n");
         return GST_PAD_PROBE_DROP;
      }
      if(strcmp(GST_EVENT_TYPE_NAME(event),"eos")==0) {
         char *pad_name=gst_pad_get_name(pad);
         g_print("Introbin EOS received on pad:%s.\n",pad_name);
	 GstElement *valve;
	 if(strcmp(pad_name,"vsrc")==0) 
   	    valve=gst_bin_get_by_name(data->introbin,"introvvalve");
	 else 
	    valve=gst_bin_get_by_name(data->introbin,"introavalve");

	 g_object_set(G_OBJECT(valve),"drop",TRUE,NULL);
	 g_print("%s DROP set TRUE.\n",gst_element_get_name(valve));
	 g_object_unref(valve);
//	 GstPad *peer=gst_pad_get_peer(pad);
//         GstElement *mixer=gst_bin_get_by_name(data->pipeline,"mixer1");
//         gst_pad_unlink(pad,peer);
//         if(!gst_pad_send_event(peer,gst_event_new_eos())) {
//            g_print("couldn't send eos event to displaymixer");
//         }   
//         gst_element_release_request_pad(mixer,peer);  
//         gst_object_unref(peer);
//         gst_object_unref(mixer);
//         gst_object_ref(data->introbin); //Remove reduce to refcount...
//         gst_bin_remove(GST_BIN(data->pipeline),data->introbin);
           return GST_PAD_PROBE_PASS;
      }
      if(strcmp(GST_EVENT_TYPE_NAME(event),"segment")==0) {
         g_print("Introbin segment received.\n");
         return GST_PAD_PROBE_PASS;
      }
//      g_print("Passed.\n");
      return GST_PAD_PROBE_PASS;

   }
   return GST_PAD_PROBE_PASS;
}


/* Creates simple bin that plays a file with decodebin.
*  Should be used as the last input of the video mixer. 
*  Scaling should be set when linked to the mixer element! 
*  bin elements : filesrc ! decodebin ! videoscale ! queue
                                  decodebin.audio ' aqueue
*  src ghost pads are added as outputs to the bin.*/
GstElement* introbin_new(CustomData *data)
{
   GstElement *introbin,*introsrc,*introdec,*introscale,*introvqueue,
           *introatee,*introaqueue1,*introaqueue2,*introaudiosink,*introaudiorate,
	   *introvvalve,*introavalve;
   GstCaps *scale_caps,*audio_caps;
   GstPad *vpad,*apad;
   //Create bin, elements, caps and link everything.
   introbin=gst_bin_new("introbin");
   introsrc=gst_element_factory_make("filesrc","introsrc");
   introdec=gst_element_factory_make("decodebin","introdec");
   introscale=gst_element_factory_make("videoscale","introscale");
   introvqueue=gst_element_factory_make("queue","introvqueue");
   introvvalve=gst_element_factory_make("valve","introvvalve");
   introaqueue1=gst_element_factory_make("queue","introaqueue1");
   introaqueue2=gst_element_factory_make("queue","introaqueue2");
   introavalve=gst_element_factory_make("valve","introavalve");
   introatee=gst_element_factory_make("tee","introatee");
   introaudiosink=gst_element_factory_make("alsasink","introaudiosink");
   introaudiorate=gst_element_factory_make("audioresample","introaudiorate");
   g_object_set(G_OBJECT(introaudiosink),"device",DEV_SPEAKER,"sync",TRUE,"qos",FALSE,NULL);
   gst_bin_add_many(GST_BIN(introbin),introsrc,introdec,introscale,introvqueue,
       introatee,introaqueue1,introaqueue2,introaudiosink,introaudiorate,
       introvvalve,introavalve,NULL);
   scale_caps=gst_caps_new_simple("video/x-raw",
//                                "format",G_TYPE_STRING,"YUV",
//                                "alpha",G_TYPE_INT,0,
//                                "framerate",GST_TYPE_FRACTION,FRAMES_PER_SEC,1,
                                "width",G_TYPE_INT,CAMERA_RES_WIDTH,
                                "height",G_TYPE_INT,CAMERA_RES_HEIGHT,
                                NULL);
   audio_caps=gst_caps_new_simple("audio/x-raw",
                                  "format",G_TYPE_STRING,"S16LE",
				  "rate",G_TYPE_INT,44100,
				  "layout",G_TYPE_STRING,"interleaved",
				  "channels",G_TYPE_INT,2,
				  NULL);
   gst_element_link_many(introsrc,introdec,NULL);
   /* decodebin's src pad is a sometimes pad - it gets created dynamically */
   g_signal_connect(introdec,"pad-added",G_CALLBACK(on_new_decoded_pad),data);
   gst_element_link_filtered(introscale,introvqueue,scale_caps);
   gst_element_link(introvqueue,introvvalve);
   gst_element_link_many(introatee,introaqueue1,introaudiosink,NULL);
   gst_element_link_filtered(introaudiorate,introatee,audio_caps);
   gst_element_link_many(introatee,introaqueue2,introavalve,NULL);
   gst_caps_unref(scale_caps);
//   gst_element_link(introaqueue,introafakesink);
    //Create the ghost src pad for the bin.
   vpad=gst_element_get_static_pad(introvvalve,"src");
   gst_element_add_pad(introbin,gst_ghost_pad_new("vsrc",vpad));
   GstPad *vgpad=gst_element_get_static_pad(introbin,"vsrc");
   gst_pad_add_probe (vgpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
         (GstPadProbeCallback) introbin_pad_block_cb,data, NULL);
   gst_object_unref(vpad);
   gst_object_unref(vgpad);
   
   apad=gst_element_get_static_pad(introavalve,"src");
   gst_element_add_pad(introbin,gst_ghost_pad_new("asrc",apad));
   GstPad *agpad=gst_element_get_static_pad(introbin,"asrc");
   gst_pad_add_probe (agpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
         (GstPadProbeCallback) introbin_pad_block_cb,data, NULL);
   gst_object_unref(apad);
   gst_object_unref(agpad);
   /* set initial parameters */
   g_object_set(G_OBJECT(introsrc),"location",data->config[data->selected_config].intro,NULL);
//   g_object_set(G_OBJECT(introafakesink),"sync",TRUE,NULL);
//   g_object_set(G_OBJECT(bkgqueue),"leaky",2,NULL);
   
   /* set eos handler function */
//   dec_pad_sink=gst_element_get_static_pad(bkgdec,"sink");
//   gst_pad_set_event_function(dec_pad_sink,eos_callback);

   return introbin;
}



/* Sets the source jpeg file location for the background bin. */
int introbin_set_source(CustomData *data)
{
   GstElement *introsrc;

   introsrc=gst_bin_get_by_name(GST_BIN(data->introbin),"introsrc");
   gst_element_set_state(data->introbin,GST_STATE_NULL);
   g_object_set(G_OBJECT(introsrc),"location",
			       data->config[data->selected_config].intro,NULL);
   gst_element_sync_state_with_parent(data->introbin);
   gst_object_unref(introsrc);
   return 0;
}
