#include <gst/gst.h>
#include <gst/video/video.h>
#include "kiosk.h"

 
/* Callback to do scrolling */
GstPadProbeReturn
scroll_cb(GstPad *pad,
          GstPadProbeInfo *info,
          gpointer d)
{
  CustomData *data=(CustomData*)d;
  if((data->prompterbin !=NULL) &
     (data->recordbin !=NULL) & 
     (GST_STATE(data->prompterbin)==GST_STATE_PLAYING) &
     (GST_STATE(data->recordbin) == GST_STATE_PLAYING)) {

      gint width, height,top,bottom,window;
      const GstStructure *str;
      GstClockTime pipeline_time;
      GstClockTime scroll_start,scroll_stop;
      GstCaps *buf_caps;
      buf_caps=gst_pad_get_current_caps(pad);
      str = gst_caps_get_structure (buf_caps, 0);
      if( gst_structure_get_int (str, "width", &width) && 
          gst_structure_get_int (str, "height", &height) ) {

         window=width*CAMERA_RES_HEIGHT/CAMERA_RES_WIDTH;
         if(window>height)
            window=height;
         GstClock *clock;
         clock=gst_pipeline_get_clock(GST_PIPELINE(data->pipeline));
         pipeline_time=gst_clock_get_time(clock);
         scroll_start=data->recordbin_offset+data->introbin_duration;
         scroll_stop=data->recordbin_offset+
             data->config[data->selected_config].duration*GST_SECOND-3*GST_SECOND;
         if(pipeline_time>scroll_start) {
            top=((pipeline_time-scroll_start)*(height-window))/(scroll_stop-scroll_start);
            bottom=height-(top+window);
         }
         else {
            top=0;
            bottom=height-window;
         }
         GstElement *to=gst_pad_get_parent_element(pad);
         g_object_set(to,"top",top,"bottom",bottom,NULL);
         gst_object_unref(to);
	 gst_object_unref(clock);
     }
     else {
        g_print("Cannot get caps for prompter image\n");
     }
     gst_caps_unref(buf_caps);
  }   
  return GST_PAD_PROBE_OK;
}
 

GstElement* prompterbin_new(CustomData *data)
{
   GstElement *prompterbin, *src,*dec,*freeze,*scale,*box,*valve;
   GstCaps *scale_caps;
   GstPad *pad;

   scale_caps=gst_caps_new_simple("video/x-raw",
                                "width",G_TYPE_INT,CAMERA_RES_WIDTH,
                                "height",G_TYPE_INT,CAMERA_RES_HEIGHT,
				"framerate",GST_TYPE_FRACTION,30,1,
                                NULL);
   prompterbin=gst_bin_new("prompterbin");
   src=gst_element_factory_make("filesrc","promptersrc");
   dec=gst_element_factory_make("pngdec",NULL);
   freeze=gst_element_factory_make("imagefreeze","prompterfreeze");
   scale=gst_element_factory_make("videoscale",NULL);
   box=gst_element_factory_make("videobox","prompterbox");
   valve=gst_element_factory_make("valve","promptervalve");
   gst_bin_add_many(GST_BIN(prompterbin),src,valve,dec,scale,box,freeze,NULL);
   gst_element_link_many(src,dec,freeze,box,scale,NULL);
   gst_element_link_filtered(scale,valve,scale_caps);
   gst_caps_unref(scale_caps);

   pad=gst_element_get_static_pad(valve,"src");
   gst_element_add_pad(prompterbin,gst_ghost_pad_new("src",pad));
   gst_object_unref(pad);

   g_object_set(G_OBJECT(src),"location",data->config[data->selected_config].prompter,
                               NULL);   
   g_object_set(G_OBJECT(box),"border-alpha",0.0,
//                               "top",0,
//                               "bottom",500,
                               NULL);
   g_object_set(G_OBJECT(valve),"drop",FALSE,NULL);
   
   /* Let's add the callback to achieve scrolling */
   GstPad *to_sink=gst_element_get_static_pad(box,"sink");
   gst_pad_add_probe(to_sink,GST_PAD_PROBE_TYPE_BUFFER,scroll_cb,data,NULL);
   
   return prompterbin;
}

int show_prompter(gpointer app) {

   CustomData *d=(CustomData*)app;
   gboolean result;
   
   GstElement *dskmixer=gst_bin_get_by_name(GST_BIN(d->displaybin),"dskmixer");
   GstElement *prompter_valve=gst_bin_get_by_name(GST_BIN(d->prompterbin),"promptervalve");
   GstElement *src=gst_bin_get_by_name(GST_BIN(d->prompterbin),"promptersrc");
   GstElement *box=gst_bin_get_by_name(GST_BIN(d->prompterbin),"prompterbox");
   gst_bin_add_many(GST_BIN(d->displaybin),d->prompterbin,NULL);
   result=gst_element_link(d->prompterbin,dskmixer);
   if(result ==FALSE) {
       g_print("Couldn't link prompter to display.(%i)  Bad... \n",result);
   } 
   g_object_set(src,"location",d->config[d->selected_config].prompter,NULL);
   g_object_set(box,"top",0,"bottom",0,NULL);
   g_object_set(prompter_valve,"drop",FALSE,NULL);
   gst_element_set_state(d->prompterbin,GST_STATE_PAUSED);
   g_object_unref(dskmixer);
   g_object_unref(prompter_valve);
   g_object_unref(src);
   gst_object_unref(box);
   return 0;
}

int hide_prompter(gpointer app) {

   CustomData *d=(CustomData*)app;
   GstElement *mixer=gst_bin_get_by_name(GST_BIN(d->displaybin),"dskmixer");
   GstElement *prompter_valve=gst_bin_get_by_name(GST_BIN(d->prompterbin),"promptervalve");

   g_object_set(prompter_valve,"drop",TRUE,NULL);
   GstPad *prompter_src=gst_element_get_static_pad(d->prompterbin,"src");
   GstPad *peer=gst_pad_get_peer(prompter_src);
   if(peer!=NULL) {
       g_print("Disconnecting prompterbin from %s.\n",GST_PAD_NAME(peer));
       if(!gst_pad_send_event(peer,gst_event_new_eos())) {
          g_print("couldn't send eos event to displaymixer from prompter");
       }
       gst_pad_unlink(prompter_src,peer);
       gst_element_release_request_pad(mixer,peer);  
       gst_object_unref(peer);
   }
   else {
      g_print("Prompter was not linked! Bad...\n");
   }
   gst_object_unref(prompter_src);
   gst_object_unref(mixer);
   gst_object_unref(prompter_valve);
   gst_object_ref(d->prompterbin); //Remove reduce to refcount...
   gst_bin_remove(GST_BIN(d->displaybin),d->prompterbin);
   return 0;
}
 
