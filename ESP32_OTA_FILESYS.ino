/* 
 *  OTA web updater with modest Flash filesystem management  
 ** format FS (SPIFFS or LittelFS)
 ** list dir
 ** upload file
 ** create file 
 ** edit file
 ** delete file
 *  MUST be compiled with one of the partition shemes with OTA + FileSystem 
 *  Adapted from the OTAWebUpdater example and
 *  https://github.com/roberttidey/BaseSupport
 *
 *  Richard Palmer 2023 
 *    Modified by Dan Peirce B.Sc. June 2023
 */

//#define LITTLE_FS 0  // these three lines moved to filecode.h
//#define SPIFFS_FS 1
//#define FILESYSTYPE LITTLE_FS    // Set to on or the other before compile

const char* host = "esp32ota";

#define FILEBUFSIZ 4096

#ifndef WM_PORTALTIMEOUT
  #define WM_PORTALTIMEOUT 180
#endif

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <time.h> 
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFiServer.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager 
#include "filecode.h"
#include "webpages.h"
//#include <HTTPUpdateServer.h>

WebServer server(80);
bool fsFound = false;
long timezone = -8;     // This timezone was set for Vancouver BC Canada (Same as L.A. USA)
byte daysavetime = 1;   // Set in June for DST

void setup(void) 
{
  Serial.begin(115200);
  delay(3000);

  //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  res = wm.autoConnect("ESP32-FS-AP"); // anonymous ap
  //res = wm.autoConnect("ESP-AP","password"); // password protected ap

  if(!res) {
    Serial.println("Failed to connect");
    // ESP.restart();
  } 
  else {
    //if you get here you have connected to the WiFi    
    Serial.println("");
  }

  fsFound = initFS(false, false); // is an FS already in place?

  Serial.println("Contacting Time Server");
  configTime(3600*timezone, daysavetime*3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
  struct tm tmstruct ;
  delay(2000);
  tmstruct.tm_year = 0;
  getLocalTime(&tmstruct, 5000);
  Serial.printf("\nNow is : %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct.tm_year)+1900,( tmstruct.tm_mon)+1, tmstruct.tm_mday,tmstruct.tm_hour , tmstruct.tm_min, tmstruct.tm_sec);
  Serial.println("");
  static char starttime[32];
  sprintf(starttime, "%d-%02d-%02d %02d:%02d:%02d\n",(tmstruct.tm_year)+1900,( tmstruct.tm_mon)+1, tmstruct.tm_mday,tmstruct.tm_hour , tmstruct.tm_min, tmstruct.tm_sec);


  /*use mdns for host name resolution*/
  if (!MDNS.begin(host))  // http://ESP32OTA.local
    Serial.println("Error setting up MDNS responder!");   
  else
    Serial.printf("mDNS responder started. Hotstname = http://%s.local\n", host);
   
  server.on("/", HTTP_GET, handleMain);
  
  // upload file to FS. Three callbacks
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
 
  server.on("/delete", HTTP_GET, handleFileDelete);
  server.on("/main", HTTP_GET, handleMain); // JSON format used by /edit
  // second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []()
    {server.send(200, "text/html", "<meta http-equiv='refresh' content='1;url=/main'>File uploaded. <a href=/main>Back to list</a>"); }, handleFileUpload); 
  server.onNotFound([](){if(!handleFileRead(server.uri())) server.send(404, "text/plain", "404 FileNotFound");});
 
  server.begin();
}

void loop(void) 
{
  server.handleClient();
  delay(1);
}
