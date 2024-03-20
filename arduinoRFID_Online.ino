#include <Arduino.h>
#include <ESP32_Supabase.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>


#define SS_PIN  5 
#define RST_PIN 4
MFRC522 rfid(SS_PIN, RST_PIN);

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#define LED_CONNECTED 15
#define PORTE 13
#define LED_NEWCARD 2
#define LED_AUTORIZED 12
#define LED_REFUSED 14


// Put your supabase URL and Anon key here...
// Because Login already implemented, there's no need to use secretrole key
String supabase_url = "https://ttrjntfzhfivtebsgeeq.supabase.co";
String anon_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InR0cmpudGZ6aGZpdnRlYnNnZWVxIiwicm9sZSI6ImFub24iLCJpYXQiOjE3MDA3MTA4NjgsImV4cCI6MjAxNjI4Njg2OH0._xcglVrE3Vs-xCM9nRkluX0eN7sfbZKoLFdZCT0ROX0";

// put your WiFi credentials (SSID and Password) here
const char *ssid = "WIFI INPHB@";
const char *psswd = "";

JsonDocument doc;
JsonDocument newData;
String jsonData;

void setup()
{
  Serial.begin(9600);

  // Connecting to Wi-Fi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid);
  pinMode(LED_CONNECTED,OUTPUT);
  pinMode(PORTE,OUTPUT);
  pinMode(LED_NEWCARD,OUTPUT);
  pinMode(LED_AUTORIZED,OUTPUT);
  pinMode(LED_REFUSED,OUTPUT);
  
  SPI.begin(); // init SPI bus
  rfid.PCD_Init();
 
  
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  digitalWrite(LED_CONNECTED,HIGH);
  rfid.PCD_DumpVersionToSerial();
}

void loop()
{
  //CHECK STATUS WIFI
  if(WiFi.status() != WL_CONNECTED)
  {
      digitalWrite(LED_CONNECTED,LOW);
  }
  readCard();
}

void readCard(){
    if (rfid.PICC_IsNewCardPresent()) { // new tag is available
    if (rfid.PICC_ReadCardSerial()) { // NUID has been readed      
      // print UID in Serial Monitor in the hex format
      String uid="";
      for (int i = 0; i < rfid.uid.size; i++) {
        uid=uid+String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        uid=uid+String(rfid.uid.uidByte[i], HEX);
      }
      
      digitalWrite(LED_NEWCARD,HIGH);
      delay(500);
      digitalWrite(LED_NEWCARD,LOW);
      httpDB_Request(uid);
        Serial.println(uid);
            
    }
  }
   
}
void httpDB_Request(String code_badge){

  HTTPClient http;
  
  String url_get_personnel = supabase_url + "/rest/v1/Personnel?Code_empreinte=eq."+code_badge; // Remplacez XXXXXXXXXX par le matricule de l'utilisateur
  http.begin(url_get_personnel);
  http.addHeader("apikey", anon_key);

  int httpResponseCode = http.GET();
  String payload_personnel = http.getString();
  DeserializationError error = deserializeJson(doc, payload_personnel);
    if (error)
        Serial.println("wf");
  Serial.print("Response payload:");
  Serial.println(payload_personnel);


  if (httpResponseCode > 0 && httpResponseCode<202&&!payload_personnel.equals("[]")) {
      const char* matriculeValue = doc[0]["Matricule"];
      String matriculeString = String(matriculeValue);
      String url_get_etat = supabase_url + "/rest/v1/Etat?matricule=eq." + matriculeString;
      url_get_etat=url_get_etat+"&order=created_at.desc&limit=1";
      http.begin(url_get_etat);
      http.addHeader("apikey", anon_key);

      int httpResponseCode_getEtat = http.GET();
      String payload = http.getString();

      DeserializationError error = deserializeJson(doc, payload);
      if (error)
        Serial.println("wf");

      Serial.print("Response payload:");
      Serial.println(payload);
      Serial.println("Value Etat:");
      const char* etat = doc[0]["etat"];
      Serial.println(etat);

      String url_update = supabase_url + "/rest/v1/Etat"; // Remplacez XXXXXXXXXX par le matricule de l'utilisateur
      http.begin(url_update);
      http.addHeader("apikey", anon_key);
      newData["matricule"]=doc[0]["matricule"];

      if (strcmp(etat, "Absent") == 0) {
        newData["etat"]="Present";
      } else if (strcmp(etat, "Present") == 0) {
          newData["etat"]="Absent";
      } 
      serializeJson(newData, jsonData);
      int httpResponseCode_update = http.POST(jsonData);

      if (httpResponseCode_update > 0) {
      Serial.println("Utilisateur Modifier avec succes");
    } else {
            int i =0;
             while(i==4){
              digitalWrite(LED_REFUSED,HIGH);
              delay(80);
              digitalWrite(LED_REFUSED,LOW);
              i++;
             }
    }



    digitalWrite(LED_AUTORIZED,HIGH);
    delay(500);
    digitalWrite(LED_AUTORIZED,LOW);  
    digitalWrite(PORTE,HIGH);
    delay(3000);
    digitalWrite(PORTE,LOW);  

  } else {

    digitalWrite(LED_REFUSED,HIGH);
    delay(1000);
    digitalWrite(LED_REFUSED,LOW);
  }

  // Libérer les ressources
  http.end();
}