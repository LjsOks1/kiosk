#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dropbox.h>
#include <memStream.h>
#include <gst/gst.h>
#include "kiosk.h"

void displayAccountInfo(drbAccountInfo *info);
void displayMetadata(drbMetadata *meta, char *title);
void displayMetadataList(drbMetadataList* list, char* title);

gpointer upload_file (gpointer data) {
    
    int err;
    void* output;
    
    // Application key and secret. To get them, create an app here:
    // https://www.dropbox.com/developers/apps
    char *c_key    = "lmg4kv8jw45nlsr";  //< consumer key
    char *c_secret = "xmq9x391l0y8nwy";  //< consumer secret
    char filename[200];
    CustomData *cdata=(CustomData*)data;
    strcpy(filename,cdata->sink_filename);
    
    // User key and secret. Leave them NULL or set them with your AccessToken.
   // char *t_key    = NULL; //< access token key
   // char *t_secret = NULL;  //< access token secret
     char *t_key    = "5j55oq1lf8h1zi2w"; //< access token key
     char *t_secret = "nvv5ps3hy4v4wy3";  //< access token secret
    
    // Global initialisation
    drbInit();
    
    // Create a Dropbox client
    drbClient* cli = drbCreateClient(c_key, c_secret, t_key, t_secret);
    
    // Request a AccessToken if undefined (NULL)
    if(!t_key || !t_secret) {
        drbOAuthToken* reqTok = drbObtainRequestToken(cli);
        
        if (reqTok) {
            char* url = drbBuildAuthorizeUrl(reqTok);
            printf("Please visit %s\nThen press Enter...\n", url);
            free(url);
            fgetc(stdin);
            
            drbOAuthToken* accTok = drbObtainAccessToken(cli);
            
            if (accTok) {
                // This key and secret can replace the NULL value in t_key and
                // t_secret for the next time.
                printf("key:    %s\nsecret: %s\n", accTok->key, accTok->secret);
            } else{
                fprintf(stderr, "Failed to obtain an AccessToken...\n");
                return NULL;
            }
        } else {
            fprintf(stderr, "Failed to obtain a RequestToken...\n");
            return NULL;
        }
    }
    
    // Set default arguments to not repeat them on each API call
    drbSetDefault(cli, DRBOPT_ROOT, DRBVAL_ROOT_AUTO, DRBOPT_END);
    

    // upload current file
   
    FILE *file = fopen(filename, "r");
    char fn[200];
    memset(fn,0,sizeof(fn));
    strcpy(fn,"/");
    strcat(fn,filename);
	output = NULL;	
        err = drbPutFile(cli, &output,
                         DRBOPT_PATH, fn,
                         DRBOPT_IO_DATA, file,
                         DRBOPT_IO_FUNC, fread,
                         DRBOPT_END);
        printf("File upload: %s\n", err ? "Failed" : "Successful");
    
   fclose(file); 
    

   output = NULL; 

    //share public link to file
	drbShare(cli, &output,
					DRBOPT_PATH, fn,
					DRBOPT_END);
	drbLink* url = (drbLink*)output;
//	strcpy(link,url->url);  We should return here!!!!
 
    drbDestroyClient(cli);
    
    // Global Cleanup
    drbCleanup();
    gst_bus_post(cdata->bus,gst_message_new_application(
        GST_OBJECT_CAST(cdata->pipeline),
        gst_structure_new_empty("UploadDone")));
    return url->url;
}


char* strFromBool(bool b) { return b ? "true" : "false"; }

/*!
 * \brief  Display a drbAccountInfo item in stdout.
 * \param  info    account informations to display.
 * \return  void
 */
void displayAccountInfo(drbAccountInfo* info) {
    if(info) {
        printf("---------[ Account info ]---------\n");
        if(info->referralLink)         printf("referralLink: %s\n", info->referralLink);
        if(info->displayName)          printf("displayName:  %s\n", info->displayName);
        if(info->uid)                  printf("uid:          %d\n", *info->uid);
        if(info->country)              printf("country:      %s\n", info->country);
        if(info->email)                printf("email:        %s\n", info->email);
        if(info->quotaInfo.datastores) printf("datastores:   %u\n", *info->quotaInfo.datastores);
        if(info->quotaInfo.shared)     printf("shared:       %u\n", *info->quotaInfo.shared);
        if(info->quotaInfo.quota)      printf("quota:        %u\n", *info->quotaInfo.quota);
        if(info->quotaInfo.normal)     printf("normal:       %u\n", *info->quotaInfo.normal);
    }
}

/*!
 * \brief  Display a drbMetadata item in stdout.
 * \param  meta    metadata to display.
 * \param   title   display the title before the metadata.
 * \return  void
 */
void displayMetadata(drbMetadata* meta, char* title) {
    if (meta) {
        if(title) printf("---------[ %s ]---------\n", title);
        if(meta->hash)        printf("hash:        %s\n", meta->hash);
        if(meta->rev)         printf("rev:         %s\n", meta->rev);
        if(meta->thumbExists) printf("thumbExists: %s\n", strFromBool(*meta->thumbExists));
        if(meta->bytes)       printf("bytes:       %d\n", *meta->bytes);
        if(meta->modified)    printf("modified:    %s\n", meta->modified);
        if(meta->path)        printf("path:        %s\n", meta->path);
        if(meta->isDir)       printf("isDir:       %s\n", strFromBool(*meta->isDir));
        if(meta->icon)        printf("icon:        %s\n", meta->icon);
        if(meta->root)        printf("root:        %s\n", meta->root);
        if(meta->size)        printf("size:        %s\n", meta->size);
        if(meta->clientMtime) printf("clientMtime: %s\n", meta->clientMtime);
        if(meta->isDeleted)   printf("isDeleted:   %s\n", strFromBool(*meta->isDeleted));
        if(meta->mimeType)    printf("mimeType:    %s\n", meta->mimeType);
        if(meta->revision)    printf("revision:    %d\n", *meta->revision);
        if(meta->contents)    displayMetadataList(meta->contents, "Contents");
    }
}

/*!
 * \brief  Display a drbMetadataList item in stdout.
 * \param  list    list to display.
 * \param   title   display the title before the list.
 * \return  void
 */
void displayMetadataList(drbMetadataList* list, char* title) {
    int i;
    if (list){
        printf("---------[ %s ]---------\n", title);
        for ( i = 0; i < list->size; i++) {
            
            displayMetadata(list->array[i], list->array[i]->path);
        }
    }
}
