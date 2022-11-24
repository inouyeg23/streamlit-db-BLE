/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by Evandro Copercini
   
   Based on K. Suwatchai (Mobizt)
  Github: https://github.com/mobizt/Firebase-ESP-Client
 
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"

//BUTTON AND LED ===================
#define PIN_LED    2
#define PIN_BUTTON 13

int ledState = LOW;     // the current state of LED
int lastButtonState;    // the previous state of button
int currentButtonState; // the current state of button

//TIME =================
#define MY_TZ "PST8PDT,M3.2.0,M11.1.0" //shoule help with instability

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -28800; //not used bc manually setting timezone
const int   daylightOffset_sec = 3600;

//BLE SCAN =================
int scanTime = 5; //In seconds
BLEScan* pBLEScan;

//FIREBASE =================
// Provide the token generation process info.
#include <addons/TokenHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "UPIoT"

/* 2. Define the API Key */
#define API_KEY "AIzaSyDmJ-e9l9JeMxPjvNYT34FO1U-p3awR9XA"

/* 3. Define the project ID */
#define FIREBASE_PROJECT_ID "streamlit-db"

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "ghinouye21@gmail.com"
#define USER_PASSWORD "poggers"

#define UPDATE_TIME 300000

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long dataMillis = 0;
int count = 0;

// The Firestore payload upload callback function
void fcsUploadCallback(CFS_UploadStatusInfo info)
{
    if (info.status == fb_esp_cfs_upload_status_init)
    {
        Serial.printf("\nUploading data (%d)...\n", info.size);
    }
    else if (info.status == fb_esp_cfs_upload_status_upload)
    {
        Serial.printf("Uploaded %d%s\n", (int)info.progress, "%");
    }
    else if (info.status == fb_esp_cfs_upload_status_complete)
    {
        Serial.println("Upload completed ");
    }
    else if (info.status == fb_esp_cfs_upload_status_process_response)
    {
        Serial.print("Processing the response... ");
    }
    else if (info.status == fb_esp_cfs_upload_status_error)
    {
        Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
    }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID);
  Serial.print(F("Connecting to WiFi .."));
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print(F("Connected to WiFi network with IP Address: "));
  Serial.println(WiFi.localIP());
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      //Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    }
};

String getHour(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";
  }
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  return timeHour;
}

String getMinutes(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";
  }
  char timeMinutes[3];
  strftime(timeMinutes,3, "%M", &timeinfo);
  return timeMinutes;
}

String getDate(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";  }
  char timeDate[10];
  strftime(timeDate,10, "%A", &timeinfo);
  return timeDate;
}

void setup() {
    Serial.begin(115200);
    
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUTTON, INPUT);
    currentButtonState = digitalRead(PIN_BUTTON);
    
    initWiFi();
  
    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  
    /* Assign the api key (required) */
    config.api_key = API_KEY;
  
    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
  
    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
    
    // Limit the size of response payload to be collected in FirebaseData
    fbdo.setResponseSize(2048);

    Firebase.begin(&config, &auth);

    Firebase.reconnectWiFi(true);
  
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); //create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);  // less or equal setInterval value

    configTime(0, daylightOffset_sec, ntpServer); // 0, 0 because we will use TZ in the next line
    setenv("TZ", MY_TZ, 1); // Set environment variable with your time zone

    String startPath = "crowd/crowd_doc/crowd_col/count";

    Serial.print("Get a document... ");
    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", startPath.c_str(), "")) {
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
  
      // Create a FirebaseJson object and set content with received payload
      FirebaseJson payload;
      payload.setJsonData(fbdo.payload().c_str());
  
      // Get the data from FirebaseJson object 
      FirebaseJsonData jsonData;
      payload.get(jsonData, "fields/count/integerValue", true);
      Serial.println(jsonData.intValue);
      count = jsonData.intValue + 1;
    }
    
}

void loop() {
  // put your main code here, to run repeatedly:
  lastButtonState    = currentButtonState;      // save the last state
  currentButtonState = digitalRead(PIN_BUTTON); // read new state

  if(lastButtonState == HIGH && currentButtonState == LOW) {

    // toggle state of LED
    ledState = !ledState;
    // control LED arccoding to the toggled state
    digitalWrite(PIN_LED, ledState); 
  }
  if (ledState == 1){
    if (Firebase.ready() && (millis() - dataMillis > UPDATE_TIME || dataMillis == 0))
    {
        Serial.println(F("Scanning..."));
        BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
        int countRssi = 0;
        for (int i = 0; i < foundDevices.getCount(); i++){
          BLEAdvertisedDevice device = foundDevices.getDevice(i);
          int rssi = device.getRSSI();
          if (rssi <= -90){
            countRssi++;
          }
        }
        Serial.print("Devices found: ");
        Serial.println(foundDevices.getCount());
        Serial.print("Nearby Devices found: ");
        Serial.println(foundDevices.getCount() - countRssi);
        Serial.println("Scan done!");
        pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
        
        dataMillis = millis();

        // For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create.ino
        FirebaseJson content;

        // We will create the nested document in the parent path "a0/b0/c0
        // a0 is the collection id, b0 is the document id in collection a0 and c0 is the collection id in the document b0.
        // and d? is the document id in the document collection id c0 which we will create.
        String documentPath = "crowd/crowd_doc/crowd_col/a" + String(count);
        String countPath = "crowd/crowd_doc/crowd_col/count";

        // If the document path contains space e.g. "a b c/d e f"
        // It should encode the space as %20 then the path will be "a%20b%20c/d%20e%20f"
        content.set("fields/dayoftheweek/stringValue", getDate());

        String currentTime = String(getHour()) + String(getMinutes());
        content.set("fields/time/stringValue", currentTime);

        content.set("fields/num/integerValue", foundDevices.getCount() - countRssi); // no value set

        Serial.print("Create a document... ");

        if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw()))
            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        else
            Serial.println(fbdo.errorReason());

        content.clear();
        content.set("fields/count/integerValue",String(count).c_str());
        
        if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, countPath.c_str(), content.raw(), "count,status" /* updateMask */))
            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
        else
            Serial.println(fbdo.errorReason());
            
        count++;
    }
  }
}
