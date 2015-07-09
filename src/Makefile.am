# name of your binary
bin_PROGRAMS = kiosk

# list of source files
# the prefix is the name of the binary
kiosk_SOURCES = main.c cambin.c displaybin.c bkgbin.c recordbin.c gesturebin.c \
                qrcode.c dropbox_upload.c gpithread.c introbin.c prompterbin.c \
		parseconfig.c

# list of headers we're not going to install
noinst_HEADERS = 

# our CFLAGS and LDFLAGS used for compiling and linking
# make sure you prefix these with the name of your binary
kiosk_CFLAGS = $(GST_CFLAGS)
kiosk_LDFLAGS = $(GST_LIBS) 
kiosk_LDADD = -lgstvideo-1.0 -lpng -lqrencode -ldropbox -lcurl -loauth -ljansson \
              -lconfig
