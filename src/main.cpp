#include "config.h"
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <WiFi.h>

AsyncWebServer server(80);                                    // Create AsyncWebServer object on port 80

char publishTopic[1024];                                      // adjust this based on your expected file size
#define AWS_IOT_PUBLISH_TOPIC "eGas/smartGas1709126023524661"


WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "connectID";

//Variables to save values from HTML form
String ssid;
String pass;
String connectID;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* connectIDPath = "/connectID.txt";

// Timer variables
unsigned long previousMillis = 0;
unsigned long previousTiming = 0;
const long interval = 10000;  
uint32_t count = 0;

// define flag variables
bool wifiFlag = false;
bool awsFlag = false;

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)){
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}


void saveCertificateToFS(DynamicJsonDocument doc){
  DynamicJsonDocument pem(4000);
  pem["certificatePem"] = doc["certificatePem"];
  pem["privateKey"] = doc["privateKey"];
  File file = SPIFFS.open("/aws.json", "w");
  if (!file)
  {
    Serial.println("failed to open config file for writing");
  }
  serializeJson(pem, Serial);
  serializeJson(pem, file);
  file.close();
}

void registerThing(DynamicJsonDocument doc){
  const char *certificateOwnershipToken = doc["certificateOwnershipToken"];
  DynamicJsonDocument reqBody(4000);
  reqBody["certificateOwnershipToken"] = certificateOwnershipToken;
  reqBody["parameters"]["SerialNumber"] = WiFi.macAddress();
  char jsonBuffer[4000];
  serializeJson(reqBody, jsonBuffer);
  Serial.println("Sending certificate..");
  client.publish("$aws/provisioning-templates/esp32_fleet_prov_template/provision/json", jsonBuffer);
}

void messageHandler(String topic, byte *payload, int length){
  Serial.print("incoming: ");
  Serial.println(topic);
  DynamicJsonDocument doc(length);
  deserializeJson(doc, payload);
  if (topic == "$aws/certificates/create/json/accepted")
  {
    saveCertificateToFS(doc);
    registerThing(doc);
  }
  else if (topic == "$aws/provisioning-templates/esp32_fleet_prov_template/provision/json/accepted"){
    Serial.println("Register things successfully.");
    Serial.println("Restart in 5s.");
    sleep(5);
    ESP.restart();
  }
}

void connectToAWS(DynamicJsonDocument cert){

  awsFlag = false;

  // const char *device_cert = cert["certificatePem"];
  // const char *private_key = cert["privateKey"];
  // net.setCACert(AWS_CERT_CA);
  // net.setCertificate(device_cert);
  // net.setPrivateKey(private_key);

  net.setCACert(AWS_CERT_CA);
  net.setCertificate(cert["certificatePem"]);
  net.setPrivateKey(cert["privateKey"]);

  // net.setCertificate(AWS_CERT_CRT);
  // net.setPrivateKey(AWS_CERT_PRIVATE);

  client.setServer(AWS_IOT_ENDPOINT, 8883);
  // Create a message handler
  client.setCallback(messageHandler);
  client.setBufferSize(4000);
  Serial.print("Connecting to AWS IOT.");
  client.connect(THING_GROUP);
  if (!client.connected()){
    Serial.println("Timeout!");
    ESP.restart();
  }
  String subscriptionTopic = String(AWS_IOT_SUB_TOPIC) + "_" + WiFi.macAddress();
  char topic[50];
  subscriptionTopic.toCharArray(topic, 50);
  client.subscribe(topic);
  Serial.println("Connected to AWS");
  Serial.printf("Subscription topic: %s", topic);Serial.println();
  awsFlag = true;
}

void createCertificate(){
  Serial.println("No file content.");
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  // Create a message handler
  client.setCallback(messageHandler);
  // Set buffer size for receive a certificate.
  client.setBufferSize(4000);
  Serial.print("Connecting to AWS IOT.");
  client.connect(THING_GROUP);
  if (!client.connected())
  {
    Serial.println("Timeout!");
    ESP.restart();
  }
  Serial.println("Connected");
  client.subscribe("$aws/certificates/create/json/accepted");
  client.subscribe("$aws/certificates/create/json/rejected");
  client.subscribe("$aws/provisioning-templates/esp32_fleet_prov_template/provision/json/accepted");
  client.subscribe("$aws/provisioning-templates/esp32_fleet_prov_template/provision/json/rejected");
  Serial.println("Create certificate..");
  client.publish("$aws/certificates/create/json", "");
}

// void getTopic(const char* path, char topic[1024]){ 
//     // Open the connectID file for reading, for the purpose of formating it corectly as a publish topic
//   File file = SPIFFS.open(path, "r");
//   if (!file) {
//     Serial.println("Failed to open file for reading");
//     return;
//   }
//   // Read the file content

//   size_t bytes_read = file.readBytes(topic, sizeof(topic) - 1);
//   topic[bytes_read] = '\0';                                                // Add null terminator manually
//   file.close();   
//                                                                  // Close the file
// }

void readAWS(){
  // Read AWS config file.
  File file = SPIFFS.open("/aws.json", "r");
  if (!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  
  DynamicJsonDocument cert(4000);

  auto deserializeError = deserializeJson(cert, file);

  if (!deserializeError){
    if (cert["certificatePem"]){

      /*String deviceCert = cert["certificatePem"];
      String privateKey = cert["privateKey"];
      Serial.print("Device cert: ");
      Serial.println(deviceCert);
      Serial.print("Private key: ");
      Serial.println(privateKey);
      */
      connectToAWS(cert);
    }
  }else createCertificate();
  file.close();
}

void publishMessage(){
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["sensor_a0"] = analogRead(35);
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);                                           // print to client
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void setup(){

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi..");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval){
      count +=1;
      previousMillis = millis();  
      Serial.println("-> Trying to reconnect.");  
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
      if(count == 5){
        Serial.println("Failed to connect.");
        count = 0;
        ESP.restart();
      }
    }
  }

  Serial.println("Connected.");

  // Init SPIFFS.
  initSPIFFS();
  readAWS();
  // getTopic(connectIDPath, publishTopic);
}

void loop(){
  publishMessage();
  client.loop();  delay(500);     // This at the beginning of the loop improved stability and was recommended
  if(awsFlag){
    if (!client.connected()){
      readAWS();
    }
  }  delay(500);
}