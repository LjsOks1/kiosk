#include <gst/gst.h>
#include <gst/video/video.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "kiosk.h"
#include "qrcode.h"

gpointer upload_file(gpointer);
gpointer gpi_thread(gpointer);

void pin1_pressed(CustomData* data)
{
	  if(GST_STATE(data->recordbin)!=GST_STATE_NULL ||
			   GST_STATE(data->uploadbin)!=GST_STATE_NULL ||
						GST_STATE(data->qrcodebin)!=GST_STATE_NULL) {
								  return;
			}
		 introbin_set_source(data);
		 play_intro(data);
			recordbin_start(data);	
}

void pin2_pressed(CustomData* data)
{

	    if(GST_STATE(data->recordbin)!=GST_STATE_NULL ||
					   GST_STATE(data->uploadbin)!=GST_STATE_NULL ) {
								  return;
					}
		   if(GST_STATE(data->qrcodebin)==GST_STATE_PLAYING) {
   					hide_qr_code(data);
					}
					else {
        data->selected_config=(data->selected_config+1)%data->logo_count;
        bkgbin_set_source(data);
					}
	
}
static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  CustomData *cdata=(CustomData*)data;
  GMainLoop *loop = (GMainLoop *) (cdata->loop);

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("EOS received on pipeline. Stop recordbin.\n");
//      g_main_loop_quit (loop);
    break;
    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
				}
    case GST_MESSAGE_STATE_CHANGED: {
      GstState olds, news;
      gst_message_parse_state_changed(msg,&olds,&news,NULL);
      char filename[100];
      if(strcmp(GST_OBJECT_NAME(msg->src),"kiosk")==0) {
         g_print("Element %s changed state from %s to %s \n",
               GST_OBJECT_NAME(msg->src),
						   gst_element_state_get_name(olds),
						   gst_element_state_get_name(news));
		       sprintf(filename,"kiosk_%s",gst_element_state_get_name(news));
		       gst_debug_bin_to_dot_file(GST_BIN(cdata->pipeline),
							              GST_DEBUG_GRAPH_SHOW_ALL,filename);
		       if(GST_STATE_TRANSITION(olds,news)==GST_STATE_CHANGE_READY_TO_PAUSED) {
//           GstElement *recordbin;
//           recordbin=gst_bin_get_by_name(pipeline,"recordbin");
//             gst_element_set_state(recordbin,GST_STATE_NULL);
         }
      }
      if(strcmp(GST_OBJECT_NAME(msg->src),"recordbin")==0) {
					    g_print("Element %s changed state from %s to %s \n",
								 GST_OBJECT_NAME(msg->src),
								     gst_element_state_get_name(olds),
								     gst_element_state_get_name(news));
					    sprintf(filename,"kiosk_RECORDER_%s",gst_element_state_get_name(news));
         gst_debug_bin_to_dot_file(GST_BIN(cdata->pipeline),
																					GST_DEBUG_GRAPH_SHOW_ALL,filename);
         if(strcmp(gst_element_state_get_name(news),"PAUSED")==0 &&
									   strcmp(gst_element_state_get_name(olds),"READY")==0 ) {
             gst_element_set_state(GST_ELEMENT(msg->src),GST_STATE_PLAYING);
// 	           g_timeout_add_seconds (cdata->config[cdata->selected_config].duration,
//													        (GSourceFunc)recordbin_stop,cdata);    
             gint64 pos2;
             pos2=gst_element_get_base_time(cdata->recordbin);
             GstClock *clock;
             clock=gst_pipeline_get_clock(GST_PIPELINE(cdata->pipeline));
             GstClockTime clock_time;
             clock_time=gst_clock_get_time(clock);
             gst_object_unref(clock);
             cdata->recordbin_offset=clock_time-pos2;
         }
 				    if(strcmp(gst_element_state_get_name(news),"PLAYING")==0) {
								     record_sign(cdata->displaybin,FALSE);
								     g_print("Recording started with filename:%s\n",cdata->sink_filename);
					    }
					    else {
								    record_sign(cdata->displaybin,TRUE);
					    }
					    if(strcmp(gst_element_state_get_name(news),"NULL")==0) {
								    g_print("Recorderbin stopped, uploading and creating QR code... \n");
												hide_prompter(cdata);
								    show_upload(cdata);
								    GThread **t=&cdata->upload_thread;
							     *t=g_thread_new("upload_thread",
							     upload_file,data);
					    }   
					 } 
	     if(strcmp(GST_OBJECT_NAME(msg->src),"qrcodebin")==0 ||
									     strcmp(GST_OBJECT_NAME(msg->src),"uploadbin")==0 ||
														strcmp(GST_OBJECT_NAME(msg->src),"prompterbin")==0) {
					    g_print("Element %s changed state from %s to %s \n",
		   						 GST_OBJECT_NAME(msg->src),
					   			 gst_element_state_get_name(olds),
								    gst_element_state_get_name(news));
					    sprintf(filename,"kiosk_%s__%s",GST_OBJECT_NAME(msg->src),
									          gst_element_state_get_name(news));
         gst_debug_bin_to_dot_file(GST_BIN(cdata->pipeline),
																					GST_DEBUG_GRAPH_SHOW_ALL,filename);
 								if((strcmp(gst_element_state_get_name(news),"PAUSED")==0) &&
									    (strcmp(gst_element_state_get_name(olds),"READY")==0)) {
									   /* Set offset on src pad. */
									   gint64 pos2;
	 	         pos2=gst_element_get_base_time(cdata->pipeline);
		          GstClock *clock;
		          clock=gst_pipeline_get_clock(GST_PIPELINE(cdata->pipeline));
		          GstClockTime clock_time;
		          clock_time=gst_clock_get_time(clock);
		          gst_object_unref(clock);
		          g_print("Pipeline times: start_time=%ld  base_time=%ld\n clock_time=%ld\n",
								          (long int)pos2,(long int)clock_time);
											 GstPad *src_pad;
           	src_pad=gst_element_get_static_pad(GST_ELEMENT(msg->src),"src");
		          gst_pad_set_offset(src_pad,clock_time-pos2);
		          gst_object_unref(src_pad);
		          gst_element_set_state(GST_ELEMENT(msg->src),GST_STATE_PLAYING);
         } 
						}
      if(strcmp(GST_OBJECT_NAME(msg->src),"introbin")==0) {
            g_print("Element %s changed state from %s to %s \n",
						            GST_OBJECT_NAME(msg->src),
															   gst_element_state_get_name(olds),
						            gst_element_state_get_name(news));
			         if(strcmp(gst_element_state_get_name(news),"PAUSED")==0 &&
												    strcmp(gst_element_state_get_name(olds),"READY")==0 ) {
			             sprintf(filename,"kiosk_introbin_%s",gst_element_state_get_name(news));
		              gst_debug_bin_to_dot_file(GST_BIN(cdata->pipeline),
							              GST_DEBUG_GRAPH_SHOW_ALL,filename);
//																introbin_set_pad_offset(cdata);
                gint64 dur;
                gst_element_query_duration(GST_ELEMENT(msg->src),GST_FORMAT_TIME,&dur);
																g_print("introbin paused with duration:%ld sec\n",dur/GST_SECOND);
																cdata->introbin_duration=dur;
                gst_element_set_state(GST_ELEMENT(msg->src),GST_STATE_PLAYING);
                g_object_set (cdata->pipeline, "message-forward", TRUE, NULL);
            }
 	     }   
      break;
    }
    case GST_MESSAGE_ELEMENT: {
      const GstStructure *s=gst_message_get_structure(msg);
      if(gst_structure_has_name(s,"GstBinForwarded")) {
         GstMessage *forward_msg=NULL;
         gst_structure_get(s,"message",GST_TYPE_MESSAGE,&forward_msg,NULL);
									g_print("GstBinForwarded message from %s \n",
                 GST_OBJECT_NAME(GST_MESSAGE_SRC(forward_msg)));
		       if(GST_MESSAGE_TYPE(forward_msg) == GST_MESSAGE_EOS) {
					        g_print("EOS from element %s\n",
         					        GST_OBJECT_NAME(GST_MESSAGE_SRC(forward_msg)));
						       /* A good time to stop the recordbin */
						       if(strcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(forward_msg)),"recordbin")==0) {
									       gst_element_set_state(cdata->recordbin,GST_STATE_NULL);
                gst_element_unlink(cdata->recordbin,cdata->valve);
                gst_element_unlink(cdata->recordbin,cdata->avalve);
                gst_object_ref(cdata->recordbin);
                gst_bin_remove(GST_BIN(cdata->pipeline),cdata->recordbin);
						       }
						       if(strcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(forward_msg)),"introbin")==0) {
	               GstPad *vsrc,*asrc,*vsink,*asink;
												    if(GST_STATE(cdata->recordbin)==GST_STATE_PLAYING) {
																   show_prompter(cdata);
															 }
																gst_element_set_state(cdata->introbin,GST_STATE_NULL);
																vsrc=gst_element_get_static_pad(cdata->introbin,"vsrc");
																asrc=gst_element_get_static_pad(cdata->introbin,"asrc");
																vsink=gst_pad_get_peer(vsrc);
																asink=gst_pad_get_peer(asrc);
																gst_pad_unlink(vsrc,vsink);
 															gst_pad_unlink(asrc,asink);
                gst_element_release_request_pad(cdata->adder,asink);  
                gst_element_release_request_pad(cdata->mixer,vsink);  
                gst_object_ref(cdata->introbin); //Remove reduce to refcount...
                gst_bin_remove(GST_BIN(cdata->pipeline),cdata->introbin);
 					       }
		       }
		       gst_message_unref(forward_msg);
      }
      break;
    }
    case GST_MESSAGE_APPLICATION: {
							if(gst_message_has_name(msg,"UploadDone")) {
								char link[200];
								strcpy(link,g_thread_join(cdata->upload_thread));          
								g_print("Dropbox URL link:%s\n",link);
								hide_upload(cdata);
								generate_qr_code(link);            
								show_qr_code(cdata); 
							}
							if(gst_message_has_name(msg,"Pin1Pressed")) { 
								g_print("Pin1 pressed\n");
								pin1_pressed(cdata);            
							}
							if(gst_message_has_name(msg,"Pin2Pressed")) {
								g_print("Pin2 pressed\n");        
								pin2_pressed(cdata); 
							}
					  break;
				}
    default:
						break;
  }
  return TRUE;
}

void element_removed_cb(GstBin *bin,
		GstElement *element,
		gpointer user_data)
{
	g_print("Element removed. Signal received.\n");
	gst_element_set_state(element,GST_STATE_NULL);
}

static void prompt()
{ 
	g_print("kiosk> "); 
} 
static void handlecommand( gchar sz ,CustomData *data)
{ 
	switch (sz) {
		case 'q':
			//        gst_element_send_event(data->pipeline,gst_event_new_eos());
			g_main_loop_quit (data->loop);
			break;
		case 'r':
     pin1_pressed(data);
		break;
	case 'x':
     pin2_pressed(data);
			break;
		case 'u':
					show_upload(data);
					break;
		case 'i':
					hide_upload(data);
					break;
		case 'f':
					introbin_set_source(data);
     play_intro(data);
					break;
  case 'p':
		   show_prompter(data);
					break;
		case 'o':
		   hide_prompter(data);
					break;
  case 'h':
					g_print("Implemented commands:\n");
					g_print("   n: Next background\n");
					g_print("   p: Prev background\n");
					g_print("   r: Start record\n");
					g_print("   s: Stop record\n");
					g_print("   q: Quit\n");
					break;
		default:
					g_print( "You typed: %c", sz ); 
					break;
	}  
	prompt(); 
} 

static gboolean keyboard_handler(GIOChannel *source,
		GIOCondition cond,
		CustomData *data)
{
	gchar str;

	if (g_io_channel_read_chars (source, &str, 1, NULL, NULL) != 
			G_IO_STATUS_NORMAL) {
		return TRUE;
	}
	handlecommand(str,data); 
	//  g_free (str); 
	return TRUE;
}

int
main (int   argc,
		char *argv[])
{
	guint bus_watch_id;
	CustomData data;
	GIOChannel *io_stdin;
	struct termios oldterm,newterm;
	int fd_console;
	GstElement *tee,*micsrc,*micqueue,*gesturebin;
 GstCaps *miccaps;
	/* Initialisation */
	gst_init (&argc, &argv);
	memset(&data,0,sizeof(data));
	data.loop = g_main_loop_new (NULL, FALSE);

	load_config(&data);

	/* Create gstreamer pipeline */
	data.pipeline=gst_pipeline_new("kiosk");
 data.mixer=gst_element_factory_make("videomixer","mixer1");
	data.valve=gst_element_factory_make("valve","recvalve");
	data.bkgbin=bkgbin_new(&data);
	data.cambin=cambin_new();
	data.recordbin=recordbin_new();
	//  gesturebin=gesturebin_new();
	gesturebin=gst_element_factory_make("fakesink","gesturebin");
	tee=gst_element_factory_make("tee","recordtee");
	data.displaybin=displaybin_new(&data);
	data.qrcodebin=qrcodebin_new();
	data.uploadbin=uploadbin_new();
	data.introbin=introbin_new(&data);
	data.prompterbin=prompterbin_new(&data);
 data.adder=gst_element_factory_make("adder","adder");
	data.avalve=gst_element_factory_make("valve","avalve");
 micsrc=gst_element_factory_make("alsasrc","micsrc");
 micqueue=gst_element_factory_make("queue","micqueue");
	g_object_set(G_OBJECT(micsrc),"device",DEV_MIC,"buffer-time",400000,NULL);

	if(!data.pipeline || !data.cambin || !data.displaybin || !data.mixer
			|| !tee || !data.valve || !micsrc || !data.adder ) {
		g_printerr("One of the elements couldn't be created. Exiting...");
		return -1;
	}

	/* attach message handler to the pipeline */
	data.bus = gst_pipeline_get_bus (GST_PIPELINE (data.pipeline));
	bus_watch_id = gst_bus_add_watch (data.bus, bus_call, &data);
	//  gst_object_unref (bus);

	g_signal_connect(data.displaybin,"element-removed",
			G_CALLBACK(element_removed_cb),&data);

	g_signal_connect(data.pipeline,"element-removed",
			G_CALLBACK(element_removed_cb),&data);


	/* add elements and link them */
	gst_bin_add_many(GST_BIN(data.pipeline),
	     data.bkgbin,
						data.displaybin,
						data.mixer,
			   data.cambin, 
						tee, 
						data.valve,
						micsrc,
      micqueue,
      data.adder,
						data.avalve,
						gesturebin,
						NULL);
 miccaps=gst_caps_new_simple("audio/x-raw",
                             "format",G_TYPE_STRING,"S16LE",
		                           "rate",G_TYPE_INT,44100,
				                         "layout",G_TYPE_STRING,"interleaved",
				                         "channels",G_TYPE_INT,2,
				                         NULL);

	gst_element_link_many(data.bkgbin,data.mixer,tee,data.displaybin,NULL);
	gst_element_link_pads(data.cambin,"src",data.mixer,NULL);
	gst_element_link_pads(data.cambin,"mon",gesturebin,NULL);
	gst_element_link(tee,data.valve);
	gst_element_link_filtered(micsrc,data.adder,miccaps);
 gst_element_link_many(data.adder,micqueue,data.avalve,NULL);
	g_object_set(G_OBJECT(data.valve),"drop",TRUE,NULL);
	g_object_set(G_OBJECT(data.avalve),"drop",TRUE,NULL);
 gst_caps_unref(miccaps);
	/* Setup keyboard handler callback */
	if ((fd_console = open("/dev/stdin",O_RDONLY ))==-1) {
		g_print("Unable to open /dev/stdin\n") ;
		exit(-1) ;
	}

	tcgetattr(fd_console,&oldterm) ;
	tcgetattr(fd_console,&newterm) ;
	newterm.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(fd_console,TCSANOW,&newterm) ;

	io_stdin=g_io_channel_unix_new(fd_console);
	g_io_channel_set_encoding(io_stdin,NULL,NULL);
	g_io_channel_set_buffered(io_stdin,FALSE);
	g_io_add_watch(io_stdin,G_IO_IN,(GIOFunc)keyboard_handler,&data);

	/* Set the pipeline to "paused" state*/
	g_print ("Pausing pipeline...: %s\n", argv[1]);
	gst_element_set_state (data.pipeline, GST_STATE_PAUSED);
 gst_debug_bin_to_dot_file(GST_BIN(data.pipeline),
																					GST_DEBUG_GRAPH_SHOW_ALL,"kiosk_0");
	
	gst_element_set_state(data.pipeline,GST_STATE_PLAYING);
	GST_OBJECT_FLAG_SET(G_OBJECT(data.pipeline),GST_BIN_FLAG_NO_RESYNC);
	gst_element_get_state(data.pipeline,NULL,NULL,GST_CLOCK_TIME_NONE);

	/* Add the recordbin, should stay in NULL state now... */
//	gst_bin_add(GST_BIN(data.pipeline),data.recordbin);
// gst_element_link_pads(data.valve,"src",data.recordbin,"sink");
// gst_element_link_pads(data.avalve,"src",data.recordbin,"asink");
// gst_element_set_state(data.recordbin,GST_STATE_NULL);
// GST_OBJECT_FLAG_SET(G_OBJECT(data.pipeline),GST_BIN_FLAG_NO_RESYNC);
	record_sign(data.displaybin,TRUE);
/* Add thread for the parallel port GPIs */
 g_thread_new("gpi_thread",gpi_thread,&data);
 g_print ("Running...\n");
 prompt();
 g_main_loop_run (data.loop);

  /* Out of the main loop, clean up nicely */
 g_print ("Returned, stopping pipeline\n");
 gst_element_set_state (data.pipeline, GST_STATE_NULL);
 tcsetattr(fd_console,TCSANOW,&oldterm);
 close(fd_console);
 g_print ("Deleting pipeline\n");
 gst_object_unref (GST_OBJECT (data.pipeline));
 g_source_remove (bus_watch_id);
 g_main_loop_unref (data.loop);

 return 0;
}
