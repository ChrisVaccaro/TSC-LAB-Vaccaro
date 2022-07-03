//Comentar la siguiente linea para evitar funciones seriales
#define DEBUG
#ifdef DEBUG
  #define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
  #define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTF(...)
  #define DEBUG_PRINTLN(...)
#endif

#include <WiFi.h>
/**************** WiFi **************************************/
const char *ssid = "Enter your WiFi name"; // Enter your WiFi name
const char *password = "Enter WiFi password";  // Enter WiFi password
bool connectWifi = false;

/************** MQTT *********************************/
#include <PubSubClient.h>

const char *mqtt_broker = "ip broker"; //Enter ip o name domain of broker mqtt
const char *topic = "temperatura";
const char *mqtt_username = "user MQTT"; //Ingresar usuario MQTT
const char *mqtt_password = "pwd MQTT"; //Ingresar pwd MQTT
const int mqtt_port = 1883;
bool connectMQTT = false;
WiFiClient espClient;
PubSubClient client(espClient);


/****************** SENSORES *******************/
#include <OneWire.h>
#include <DallasTemperature.h>

//GPIO pin 4 is set as OneWire bus
OneWire ourWire1(4);
//GPIO pin 0 is set as OneWire bus
OneWire ourWire2(0);

//A variable or object is declared for our sensor 1
DallasTemperature sensors1(&ourWire1);
//A variable or object is declared for our sensor 2
DallasTemperature sensors2(&ourWire2);


//pins of transistor
int trans1 = 17;
int trans2 = 16;
//temperature var
float temp1;
float temp2;

/************************ time and date ***********************/
#include "time.h"

const char* ntpServer = "pool.ntp.org";
unsigned long epochTime;
struct tm timeinfo;

/********************* CODIGO PRINCIPAL ***********************/
void setup() {
  Serial.begin(115200);
  
  //Wifi
  WiFi.mode(WIFI_STA);
  connect_wifi();
  
  //transistor 1
  pinMode(trans1, OUTPUT);
  digitalWrite(trans1,LOW);
  //transitor 2
  pinMode(trans2, OUTPUT);
  digitalWrite(trans2,LOW);
  //Sensores
  sensors1.begin();   //Sensor 1 starts
  sensors2.begin();   //Sensor 2 starts

  //Hora
  configTime(-18000, 0, ntpServer);
  //Debe configurarse por primera vez la hora para empezar
  while(!getLocalTime(&timeinfo));
  
  DEBUG_PRINTLN(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  DEBUG_PRINTLN(getEpoch());

  //MQTT
  client.setServer(mqtt_broker, mqtt_port);
  
  /*
  // Esperar una hora exacta para empezar a enviar datos
  do{
    getLocalTime(&timeinfo);
  }while(timeinfo.tm_hour!=0 || timeinfo.tm_min!=0 || timeinfo.tm_sec!=0 );
  */
}


void loop() {
  //Espera a que sea el segundo exacto 00 (iteración por cada minuto)
  do{
    getLocalTime(&timeinfo);
  }while(timeinfo.tm_sec!=0);
  
  epochTime = getEpoch(); //fecha en formato UNIX/EPOCH
  readData();
  connect_wifi();
  connect_mqtt();  
  
  if(connectWifi && connectMQTT)  publicMQTT();
  else ;//Almacenar en archivo local (SPIFFS)
}


void connect_wifi() {
  const int timeout = 3; //intenta conectarse solo por 15 segundos
  int cnt = 0;
  
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT("Conectando a SSID: ");
    DEBUG_PRINTLN(ssid);
    while (WiFi.status() != WL_CONNECTED) {
      cnt++;
      WiFi.begin(ssid, password);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      DEBUG_PRINT(".");
      delay(5000);
      connectWifi = true;
      if(cnt>=timeout){
        connectWifi = false;
        break;
      }
    }
    if(connectWifi) DEBUG_PRINTLN("\nConectado");
    else DEBUG_PRINTLN("No se logró conectar!!\nLos datos se guardaran localmente");
  }
}

void connect_mqtt(){
  //connecting to a mqtt broker
  if (!client.connected()) {
    String client_id = "TSC-LAB";
    DEBUG_PRINTF("Conectando cliente %s al broker MQTT\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      connectMQTT = true;
      DEBUG_PRINTLN("Mqtt conectado");
    } else {
      connectMQTT = false;
      DEBUG_PRINT("Error al conectar al broker MQTT: ");
      DEBUG_PRINT(client.state());
    }
  }
}
void publicMQTT() {
  String msg = String(epochTime) +","+ String(temp1) +","+ String(temp2);
  client.publish(topic, msg.c_str());
}

void readData() {
  sensors1.requestTemperatures();
  temp1 = sensors1.getTempCByIndex(0);

  sensors2.requestTemperatures();
  temp2 = sensors2.getTempCByIndex(0);
  
  DEBUG_PRINTLN(temp1);
  DEBUG_PRINTLN(temp2);
}

unsigned long getEpoch() {
  time_t now;
  if (!getLocalTime(&timeinfo)) {
    return(0);
  }
  time(&now);
  return now;
}
