#include <gst/gst.h>
#include <libconfig.h>
#include "kiosk.h"
#include <stdio.h>
#include <stdlib.h>

int load_config(CustomData *d) 
{
   config_t cfg, *cf;
//   CustomData *d=*data;
    const config_setting_t *logos,*logo;
    int count, n;
    cf = &cfg;
    config_init(cf);
    if (!config_read_file(cf, "kiosk.ini")) {
       fprintf(stderr, "%s:%d - %s\n",
       config_error_file(cf),
       config_error_line(cf),
       config_error_text(cf));
       config_destroy(cf);
       return -1;
    }
   logos = config_lookup(cf, "logos");
   d->logo_count = config_setting_length(logos);
   g_print("%d logos configured.\n", d->logo_count);
//   d->config= malloc(sizeof(ConfigItem[10]));
   ConfigItem *ci;
   char *b,*i,*p;
   for (n = 0; n < d->logo_count; n++) {
      logo=config_setting_get_elem(logos, n);
      ci=&d->config[n];
      config_setting_lookup_string(logo,"background",
                            &b);
      config_setting_lookup_string(logo,"intro",&i);
      config_setting_lookup_string(logo,"prompter",&p);
      config_setting_lookup_int(logo,"duration",&d->config[n].duration);
      strcpy(d->config[n].background,b);
      strcpy(d->config[n].intro,i);
      strcpy(d->config[n].prompter,p);
  }
   config_destroy(cf);
   return 0;
} 
