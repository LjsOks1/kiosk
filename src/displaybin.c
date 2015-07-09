#include <gst/gst.h>
#include <gst/video/video.h>
#include "kiosk.h"

GstElement* qrcodebin_new()
{
   GstElement *qrcodebin, *src,*dec,*scale,*box,*valve;
   GstCaps *scale_caps,*fs_caps;
   GstPad *pad;

   scale_caps=gst_caps_new_simple("video/x-raw",
                                "width",G_TYPE_INT,360,
                                "height",G_TYPE_INT,360,
                                NULL);
   fs_caps=gst_caps_new_simple("image/png",
                                "framerate",GST_TYPE_FRACTION,1,1,
                                NULL);


   qrcodebin=gst_bin_new("qrcodebin");
   src=gst_element_factory_make("multifilesrc",NULL);
   dec=gst_element_factory_make("pngdec",NULL);
   scale=gst_element_factory_make("videoscale",NULL);
   box=gst_element_factory_make("videobox",NULL);
   valve=gst_element_factory_make("valve","qrvalve");
   gst_bin_add_many(GST_BIN(qrcodebin),src,valve,dec,scale,box,NULL);
   gst_element_link_many(src,dec,scale,NULL);
   gst_element_link_filtered(scale,box,scale_caps);
   gst_element_link(box,valve);
   gst_caps_unref(scale_caps);

   pad=gst_element_get_static_pad(valve,"src");
   gst_element_add_pad(qrcodebin,gst_ghost_pad_new("src",pad));
   gst_object_unref(pad);


   g_object_set(G_OBJECT(src),"location","lastqr.png",
                                   "loop",TRUE,
                                   "start-index",2,
                                   "stop-index",12,
                                   "caps",fs_caps,
                                   NULL);   
   gst_caps_unref(fs_caps);
   g_object_set(G_OBJECT(box),"border-alpha",0.0,
                                   "top",-44,
                                   "left",-220,
                                   NULL);
   g_object_set(G_OBJECT(valve),"drop",FALSE,NULL);
   return qrcodebin;
}

int show_qr_code(gpointer app) {

   CustomData *d=(CustomData*)app;
   
   GstElement *dskmixer=gst_bin_get_by_name(GST_BIN(d->displaybin),"dskmixer");
   GstElement *qr_valve=gst_bin_get_by_name(GST_BIN(d->qrcodebin),"qrvalve");
   gst_bin_add_many(GST_BIN(d->displaybin),d->qrcodebin,NULL);
   g_object_set(qr_valve,"drop",FALSE,NULL);
   gst_element_link(d->qrcodebin,dskmixer);
   gst_element_set_state(d->qrcodebin,GST_STATE_PLAYING);
   g_object_unref(dskmixer);
   g_object_unref(qr_valve);
   return 0;
}

int hide_qr_code(gpointer app) {

   CustomData *d=(CustomData*)app;
   GstElement *mixer=gst_bin_get_by_name(GST_BIN(d->displaybin),"dskmixer");
   GstElement *qr_valve=gst_bin_get_by_name(GST_BIN(d->qrcodebin),"qrvalve");

   g_object_set(qr_valve,"drop",TRUE,NULL);
   GstPad *qrcode_src=gst_element_get_static_pad(d->qrcodebin,"src");
   GstPad *peer=gst_pad_get_peer(qrcode_src);
   if(!gst_pad_send_event(peer,gst_event_new_eos())) {
     g_print("couldn't send eos event to displaymixer");
   }
   gst_pad_unlink(qrcode_src,peer);
   gst_element_release_request_pad(mixer,peer);  
   gst_object_unref(peer);
   gst_object_unref(qrcode_src);
   gst_object_unref(mixer);
   gst_object_unref(qr_valve);
   gst_object_ref(d->qrcodebin); //Remove reduce to refcount...
   gst_bin_remove(GST_BIN(d->displaybin),d->qrcodebin);
   return 0;
}

GstElement* uploadbin_new()
{
   GstElement *uploadbin, *src,*dec,*scale,*box,*valve;
   GstCaps *scale_caps,*fs_caps;
   GstPad *pad;

   scale_caps=gst_caps_new_simple("video/x-raw",
                                "width",G_TYPE_INT,200,
                                "height",G_TYPE_INT,200,
                                NULL);
   fs_caps=gst_caps_new_simple("image/png",
                                "framerate",GST_TYPE_FRACTION,12,1,
                                NULL);


   uploadbin=gst_bin_new("uploadbin");
   src=gst_element_factory_make("multifilesrc",NULL);
   dec=gst_element_factory_make("pngdec",NULL);
   scale=gst_element_factory_make("videoscale",NULL);
   box=gst_element_factory_make("videobox",NULL);
   valve=gst_element_factory_make("valve","uploadvalve");
   gst_bin_add_many(GST_BIN(uploadbin),src,valve,dec,scale,box,NULL);
   gst_element_link_many(src,dec,scale,NULL);
   gst_element_link_filtered(scale,box,scale_caps);
   gst_element_link(box,valve);
   gst_caps_unref(scale_caps);

   pad=gst_element_get_static_pad(valve,"src");
   gst_element_add_pad(uploadbin,gst_ghost_pad_new("src",pad));
   gst_object_unref(pad);

   g_object_set(G_OBJECT(src),"location",
          "media/uploading/Frame %d (100ms) (replace).png",
                                   "loop",TRUE,
                                   "start-index",2,
                                   "stop-index",12,
                                   "caps",fs_caps,
                                   NULL);   
   g_object_set(G_OBJECT(box),"border-alpha",0.0,
                                   "top",-124,
                                   "left",-300,
                                   NULL);
 
   gst_caps_unref(fs_caps);
   g_object_set(G_OBJECT(valve),"drop",FALSE,NULL);
   return uploadbin;
}

int show_upload(gpointer app) {

   CustomData *d=(CustomData*)app;
   
   GstElement *dskmixer=gst_bin_get_by_name(GST_BIN(d->displaybin),"dskmixer");
   GstElement *upload_valve=gst_bin_get_by_name(GST_BIN(d->uploadbin),"uploadvalve");
   gst_bin_add_many(GST_BIN(d->displaybin),d->uploadbin,NULL);
   g_object_set(upload_valve,"drop",FALSE,NULL);
   gst_element_link(d->uploadbin,dskmixer);
   gst_element_set_state(d->uploadbin,GST_STATE_PAUSED);
   g_object_unref(dskmixer);
   g_object_unref(upload_valve);
   return 0;
}

int hide_upload(gpointer app) {

   CustomData *d=(CustomData*)app;
   GstElement *mixer=gst_bin_get_by_name(GST_BIN(d->displaybin),"dskmixer");
   GstElement *upload_valve=gst_bin_get_by_name(GST_BIN(d->uploadbin),"uploadvalve");

   g_object_set(upload_valve,"drop",TRUE,NULL);
   GstPad *upload_src=gst_element_get_static_pad(d->uploadbin,"src");
   GstPad *peer=gst_pad_get_peer(upload_src);
   if(!gst_pad_send_event(peer,gst_event_new_eos())) {
     g_print("couldn't send eos event to displaymixer");
   }
   gst_pad_unlink(upload_src,peer);
   gst_element_release_request_pad(mixer,peer);  
   gst_object_unref(peer);
   gst_object_unref(upload_src);
   gst_object_unref(mixer);
   gst_object_unref(upload_valve);
   gst_object_ref(d->uploadbin); //Remove reduce to refcount...
   gst_bin_remove(GST_BIN(d->displaybin),d->uploadbin);
   return 0;
}
 
/* Callback to set textoverlay text as required. */
GstPadProbeReturn
timeoverlay_cb(GstPad *pad,
               GstPadProbeInfo *info,
	       CustomData *data)
{
   GstElement *to=gst_pad_get_parent_element(pad);
   if(data->introbin !=NULL & GST_STATE(data->introbin)==GST_STATE_PLAYING) {
      char sec[100];
      GstClockTime intro_start,pipeline_time;
      intro_start=gst_element_get_base_time(data->introbin);
      pipeline_time=GST_BUFFER_TIMESTAMP(info->data);
      sprintf(sec,"00:%02lld",
         (data->introbin_duration-pipeline_time+data->introbin_offset)/GST_SECOND);
      g_object_set(to,"text",sec,"silent",FALSE,"color",0xffffffff,NULL);
   } 
   else {
      if(data->recordbin !=NULL & GST_STATE(data->recordbin)==GST_STATE_PLAYING) {
         char sec[100];
         GstClockTime record_start,pipeline_time;
         record_start=gst_element_get_base_time(data->recordbin);
         pipeline_time=GST_BUFFER_TIMESTAMP(info->data);
         sprintf(sec,"00:%02lld",
            (data->config[data->selected_config].duration*GST_SECOND-
												      pipeline_time+data->recordbin_offset)/GST_SECOND);
         g_object_set(to,"text",sec,"silent",FALSE,"color",0xffff0000,NULL);
      } 
      else {
         g_object_set(to,"silent",TRUE,NULL);
      }
   }   
   gst_object_unref(to);
   return GST_PAD_PROBE_OK;
}
 

/* Creates the display bin.
*  Scales the image to the display resolution and sends it to 
*  xvimagesink. 
*  bin: queue ! videomixer !videoconvert ! videoscale ! xvimagesink */
GstElement* displaybin_new(CustomData *data)
{
   GstElement *displaybin,*displayqueue,*dskmixer,*displayconvert, *timeoverlay,
              *displayscale, *displaysink;
   GstCaps *display_caps,*out_caps;
   GstPad *pad;
   displaybin=gst_bin_new("displaybin");
   displayqueue=gst_element_factory_make("queue","displayqueue");
   dskmixer=gst_element_factory_make("videomixer","dskmixer");
   displayconvert=gst_element_factory_make("videoconvert","displayconvert");
   timeoverlay=gst_element_factory_make("textoverlay","displaytimeoverlay");
   g_object_set(timeoverlay,"font-desc","Liquid Crystal 30",
       "halignment",0,"valignment",1,NULL);
   displayscale=gst_element_factory_make("videoscale","displayscale");
   displaysink=gst_element_factory_make("fbdevsink","displaysink");
   gst_bin_add_many(GST_BIN(displaybin),displayqueue,dskmixer,displayconvert,
             timeoverlay,displayscale,displaysink,NULL);
   display_caps=gst_caps_new_simple("video/x-raw",
                                "width",G_TYPE_INT,DISPLAY_RES_WIDTH,
                                "height",G_TYPE_INT,DISPLAY_RES_HEIGHT,
                                NULL);
   out_caps=gst_caps_new_simple("video/x-raw",
                                "format",G_TYPE_STRING,"AYUV",
                                "width",G_TYPE_INT,CAMERA_RES_WIDTH,
                                "height",G_TYPE_INT,CAMERA_RES_HEIGHT,
                                NULL);
//   gst_element_link_filtered(displayqueue,displayconvert,out_caps);
   gst_element_link_many(displayqueue,dskmixer,displayconvert,timeoverlay,
            displayscale,NULL);
//   gst_element_link(displayconvert,displayscale);
   gst_element_link_filtered(displayscale,displaysink,display_caps);
   gst_caps_unref(display_caps);
   gst_caps_unref(out_caps);
   pad=gst_element_get_static_pad(displayqueue,"sink");
   gst_element_add_pad(displaybin,gst_ghost_pad_new("sink",pad));
   gst_object_unref(pad);
//   gst_element_link(qrcodebin,dskmixer);
   /* Set initial parameters */
   g_object_set(G_OBJECT(displaysink),"qos",FALSE,NULL); // for fbdevsink
//   g_object_set(G_OBJECT(displaysink),"display",":0","qos",FALSE,NULL);
   g_object_set(G_OBJECT(displaysink),"sync",FALSE,NULL);
   g_object_set(G_OBJECT(displayqueue),"leaky",1,NULL);

   /* Let's add the callback to update the textoverlay element. */
   GstPad *to_sink=gst_element_get_static_pad(timeoverlay,"video_sink");
   gst_pad_add_probe(to_sink,GST_PAD_PROBE_TYPE_BUFFER,timeoverlay_cb,data,NULL);
  // The bottom right on-air sign... 
   GstElement *brdsksrc,*brdskdec,*brdskscale,*brdskbox;
   GstCaps *brscale_caps,*brmfs_caps;
   brscale_caps=gst_caps_new_simple("video/x-raw",
                                "width",G_TYPE_INT,92,
                                "height",G_TYPE_INT,48,
                                NULL);
   brmfs_caps=gst_caps_new_simple("image/x-tga",
                                "framerate",GST_TYPE_FRACTION,2,1,
                                NULL);


   brdsksrc=gst_element_factory_make("multifilesrc","brdsksrc");
   brdskdec=gst_element_factory_make("avdec_targa","brdskdec");
   brdskscale=gst_element_factory_make("videoscale","brdskscale");
   brdskbox=gst_element_factory_make("videobox","brdskbox");
//   brdskvalve=gst_element_factory_make("valve","brdskvalve");
//   brdskqueue=gst_element_factory_make("queue","brdskqueue");
   gst_bin_add_many(GST_BIN(displaybin),brdsksrc,brdskdec,brdskscale,brdskbox,NULL);
   gst_element_link_many(brdsksrc,brdskdec,brdskscale,NULL);
   gst_element_link_filtered(brdskscale,brdskbox,brscale_caps);
   gst_element_link_many(brdskbox,dskmixer,NULL);
   gst_caps_unref(brscale_caps);
   g_object_set(G_OBJECT(brdsksrc),"location","media/on-air/on-air_%d.tga",
                                   "loop",TRUE,
                                   "start-index",1,
                                   "stop-index",1,
                                   "caps",brmfs_caps,
                                   NULL);   
   g_object_set(G_OBJECT(brdskbox),"border-alpha",0.0,
                                   "top",-328,
                                   "left",-675,
                                   NULL);

   gst_caps_unref(brmfs_caps);
  
 
   // End of on-air sign 


/* End of DSK elements. */

   return displaybin;
}

void record_sign(GstElement *displaybin,gboolean drop)
{
   GstElement *brdsksrc;

   brdsksrc=gst_bin_get_by_name(GST_BIN(displaybin),"brdsksrc");
   gst_element_set_state(brdsksrc,GST_STATE_NULL);
   if(drop)
      g_object_set(brdsksrc,"location","media/on-air/on-air_0.tga",NULL);
   else
      g_object_set(brdsksrc,"location","media/on-air/on-air_1.tga",NULL);
   gst_element_set_state(brdsksrc,GST_STATE_PLAYING);
   gst_object_unref(brdsksrc);
}


