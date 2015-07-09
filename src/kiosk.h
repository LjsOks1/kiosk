

#define CAMERA_RES_WIDTH  800 
#define CAMERA_RES_HEIGHT  448
#define DISPLAY_RES_WIDTH  1280
#define DISPLAY_RES_HEIGHT 720
#define REC_RES_WIDTH      640
#define REC_RES_HEIGHT     360
#define FRAMES_PER_SEC     30

#define DEV_CAMERA    "/dev/video0"
#define DEV_MIC       "plughw:1,0"
#define DEV_SPEAKER   "hw:1,0"

typedef struct _ConfigItem {
  char background[255];
  char intro[255];
  char prompter[255];
  int duration;
} ConfigItem;

typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *displaybin;
  GMainLoop *loop;
  GstElement *recordbin;
  GstElement *qrcodebin;
  GstElement *uploadbin;
  GstElement *valve;
  GstElement *introbin;
  GstElement *avalve;
  GstElement *adder;
  GstElement *mixer;
  GstElement *cambin;
  GstElement *bkgbin;
  GstElement *prompterbin;
  GstBus *bus;
  GThread *upload_thread;
  char sink_filename[255]; //the filename the last recording was saved as. (no path)
  GstClockTime introbin_offset;
  GstClockTime introbin_duration;
  GstClockTime recordbin_offset;


/* Config stuff */
  int logo_count;          //Total number of logos in the config file
  ConfigItem config[10];    //Array to store logo configurations
  int selected_config;     //Active logo
 
} CustomData;

/* Functions defined in bkgbin.c */
GstElement* bkgbin_new(CustomData*);
int bkgbin_set_source(CustomData*);

/* Functions defined in cambin.c */
GstElement* cambin_new();

/* Functions defined in displaybin.c */
GstElement* displaybin_new(CustomData*);
GstElement* qrcodebin_new();
GstElement* uploadbin_new();
void record_sign(GstElement*,gboolean);
int show_qr_code(gpointer);
int hide_qr_code(gpointer);
int show_upload(gpointer);
int hide_upload(gpointer);

/* Functions defined in recordbin.c */
GstElement* recordbin_new();
void recordbin_start(gpointer);
gboolean recordbin_stop(CustomData*);


/* Functions defined in gesturebin.c */
GstElement *gesturebin_new();

/* Functions defined in qrcode.c */

/* Functions defined in introbin.c */
GstElement* introbin_new(CustomData*);
int play_intro(gpointer);
int introbin_set_source(CustomData*);

/* Functions defined in prompterbin.c */
GstElement* prompterbin_new(CustomData*);
int show_prompter(gpointer);
int hide_prompter(gpointer);

/* unction defined in parseconfig.c */
int load_config(CustomData*);

