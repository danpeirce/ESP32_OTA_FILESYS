/*
 * filesystem code - local and via webserver
*/
#ifndef FILECODE_H_
#define FILECODE_H_

#define LITTLE_FS 0
#define SPIFFS_FS 1
#define FILESYSTYPE LITTLE_FS    // Set to on or the other before compile

void fsList(void);
bool initFS(bool format, bool force);

extern String logStr;
extern char tempBuf[];

#if FILESYSTYPE == LITTLE_FS
	#include "LittleFS.h"
	#define FILESYS LittleFS
  extern char fsName[];
#else
	#include "FS.h"
  #include <SPIFFS.h>
	#define FILESYS SPIFFS
  extern char fsName[];
#endif

//holds the current upload
extern File fsUploadFile;

void handleFileSysFormat();

String getContentType(String filename);

bool handleFileRead(String path);

// uses edit mode uploader
void handleFileUpload();

void handleFileDelete();

void handleFileCreate();

// main page JS scripts 
extern const char edit_script[] PROGMEM;

// generate HTML for main page
void handleMain();

#endif
