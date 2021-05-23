//*******************************************************
//
//  (c) Dietmar Krause, DL2SBA 2021
//
//  filemanager code taken from https://fipsok.de/
//
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <list>
#include <detail/RequestHandlersImpl.h>
#include <DHT.h>
#include <DHT_U.h>

#include "data.h"
#include "measure.h"
#include "AccessPointHandler.h"

//  type for storing fileName, fileSize
typedef std::pair<String, int> fileProperty;

WebServer server(HTTP_SERVER_PORT);

//  signal, when the AP should be stopped
bool stopAP = false;


//*******************************************************
//
const String formatBytes(size_t const& bytes) {            // lesbare Anzeige der Speichergrößen
  return bytes < 1024 ? static_cast<String>(bytes) + " Byte" : bytes < 1048576 ? static_cast<String>(bytes / 1024.0) + " KB" : static_cast<String>(bytes / 1048576.0) + " MB";
}

//*******************************************************
//
bool freeSpace(uint16_t const& printsize) {               // Funktion um beim speichern in Logdateien zu prüfen ob noch genügend freier Platz verfügbar ist.
  D_PRINTF("Funktion: %s meldet in Zeile: %d FreeSpace: %s\n", __PRETTY_FUNCTION__, __LINE__, formatBytes(SPIFFS.totalBytes() - (SPIFFS.usedBytes() * 1.05)).c_str());
  return (SPIFFS.totalBytes() - (SPIFFS.usedBytes() * 1.05) > printsize) ? true : false;
}

//*******************************************************
//
// upload a file
//
void handleFileUpload() {
  static File fsUploadFile;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    if (upload.filename.length() > 30) {
      upload.filename = upload.filename.substring(upload.filename.length() - 30, upload.filename.length());  // Dateinamen auf 30 Zeichen kürzen
    }
    D_PRINTF("handleFileUpload Name: /%s\n", upload.filename.c_str());
    fsUploadFile = SPIFFS.open("/" + server.urlDecode(upload.filename), "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    D_PRINTF("handleFileUpload Data: %u\n", upload.currentSize);
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile)
      fsUploadFile.close();
    D_PRINTF("handleFileUpload Size: %u OK\n", upload.totalSize);
  }
}

//*******************************************************
//
// function to compare to filenames
//
int compareString(const fileProperty & f, const fileProperty & l) {
  //  compare case-sensitive
  if (server.arg(0) == "1") {
    return f.second > l.second;
  } else {
    //  compare only first 32 bytes of the names case-insensitive
    for (uint8_t i = 0; i < 31; i++) {
      if (tolower(f.first[i]) < tolower(l.first[i]))  {
        return true;
      }
      else if (tolower(f.first[i]) > tolower(l.first[i])) {
        return false;
      }
    }
    return false;
  }
}

//*******************************************************
//
//  ??
//
void handleListFiles() {                                                                    // Senden aller Daten an den Client
  File root = SPIFFS.open("/");
  std::list<fileProperty> dirList;                                                          // Liste anlegen
  while (File f = root.openNextFile()) dirList.emplace_back(f.name(), f.size());            // Liste füllen
  dirList.sort(compareString);
  String temp = "[";
  for (auto& p : dirList) {
    if (temp != "[") temp += ',';
    temp += "{\"name\":\"" + p.first.substring(1) + "\",\"size\":\"" + formatBytes(p.second) + "\"}";
  }
  temp += R"(,{"usedBytes":")" + formatBytes(SPIFFS.usedBytes() * 1.05) + R"(",)";         // Berechnet den verwendeten Speicherplatz + 5% Sicherheitsaufschlag
  temp += R"("totalBytes":")" + formatBytes(SPIFFS.totalBytes()) + R"(","freeBytes":")" +   // Zeigt die Größe des Speichers
          (SPIFFS.totalBytes() - (SPIFFS.usedBytes() * 1.05)) + R"("}])";                   // Berechnet den freien Speicherplatz + 5% Sicherheitsaufschlag
  server.send(HTTP_CODE_OK, "application/json", temp);
}

//*******************************************************
//
//  send a redirect response to the filemanager 
//
void redirectToFilemanagerPage() {
  server.sendHeader("Location", PAGE_FILEMANAGER);
  server.send(HTTP_CODE_SEEOTHER, "message/http");
}


//*******************************************************
//
//  System info
//
void handleSysInfo(void) {
  String json = "";
  json += "{\"Hostname\":\"" + String(WiFi.softAPgetHostname()) + "\"";
  json += ",\"Broadcast\":\"" + WiFi.softAPBroadcastIP().toString() + "\"";
  json += ",\"MACAdress\":\"" + String(WiFi.softAPmacAddress()) + "\"";
  json += ",\"IPAdress\":\"" + WiFi.softAPIP().toString() + "\"";
  json += ",\"Uptime\":\"" + String(time(NULL)) + "\"";
  json += ",\"Vbatt\":\"" + String(readVbatt()) + "\"";
  json += ",\"Vsupp\":\"" + String(readVsupp()) + "\"";
  json += "}";

  server.send(HTTP_CODE_OK, "application/json", json);
}


//*******************************************************
//
//  ??
//
bool handleFile(String&& path) {
  bool rc;
  if (server.hasArg(HTTP_COMMAND_DELETE)) {
    SPIFFS.remove(server.arg(HTTP_COMMAND_DELETE));                                                    // Datei löschen
    redirectToFilemanagerPage();
    rc = true;
  } else {
    if (path.endsWith("/")) {
      path += PAGE_INDEX;
    }
    if ( SPIFFS.exists(path) ) {
      File f = SPIFFS.open(path);
      server.streamFile(f, StaticRequestHandler::getContentType(path));
      f.close();
      rc = true;
    } else {
      rc = false;
    }
  }
  return rc;
}

//*******************************************************
//
//  ??
//
void handleStopAP( void ) {
  //  send a redirect to the index page 
  server.sendHeader("Location", PAGE_INDEX);
  server.send(HTTP_CODE_SEEOTHER, "message/http");

  D_PRINTLN("Stopping AP");
  stopAP = true;
}

//*******************************************************
//
//  ??
//
void handleNotFound( void ) {
  if (!handleFile(server.urlDecode(server.uri()))) {
    server.send(HTTP_CODE_NOTFOUND, "text/plain", "FileNotFound");
  }
}

//*******************************************************
//
//  start AP
//
//  https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer/src
//  https://fipsok.de/Esp32-Webserver/accesspoint-Esp32.tab
//
void startAP( void ) {
  D_PRINTLN("Setting AP ...");

  WiFi.mode(WIFI_AP);

  if (MDNS.begin("esp32")) {
    D_PRINTLN("MDNS responder started");
  }

  server.on("/stopAP", handleStopAP);
  server.on("/list", handleListFiles);
  server.on("/sysinfo", handleSysInfo);
  server.on("/upload", HTTP_POST, redirectToFilemanagerPage, handleFileUpload);
  server.onNotFound(handleNotFound);

  server.begin();
  D_PRINTLN("^^^^^^^^^^^^^^ http-Server started ...");


  while ( !stopAP ) {
    server.handleClient();
  }

  server.stop();
  D_PRINTLN("vvvvvvvvvvvvvv http-Server stopped ...");
}
