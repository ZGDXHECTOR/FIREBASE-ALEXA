#include "DHTesp.h" 
#include <Ticker.h>
#include "Display.h"
#include "Network.h"

#ifndef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP32 ONLY!)
#error Select ESP32 board.
#endif

Display *display;
Network *network;
DHTesp dht;

void tempTask(void *pvParameters);
bool getTemperature();
void triggerGetTemp();


TaskHandle_t tempTaskHandle = NULL;

Ticker tempTicker;

ComfortState cf;

bool tasksEnabled = false;

int dhtPin = 21;

/** 
 @return     
 */
bool initTemp() {
  byte resultValue = 0;

	dht.setup(dhtPin, DHTesp::DHT11);
	Serial.println("DHT initiated");

  
	xTaskCreatePinnedToCore(
			tempTask,                       
			"tempTask ",                    
			10000,                          
			NULL,                           
			5,                              
			&tempTaskHandle,                
			1);                            

  if (tempTaskHandle == NULL) {
    Serial.println("Failed to start task for temperature update");
    return false;
  } else {
    
    tempTicker.attach(20, triggerGetTemp);
  }
  return true;
}


void triggerGetTemp() {
  if (tempTaskHandle != NULL) {
	   xTaskResumeFromISR(tempTaskHandle);
  }
}

/**
  @param pvParameters   
 */
void tempTask(void *pvParameters) {
	Serial.println("tempTask loop started");
	while (1) 
  {
    if (tasksEnabled) {
      
			getTemperature();
		}
    
		vTaskSuspend(NULL);
	}
}

/** 
  @return 
*/
bool getTemperature() {

  TempAndHumidity newValues = dht.getTempAndHumidity();
	
	if (dht.getStatus() != 0) {
		Serial.println("DHT11 error status: " + String(dht.getStatusString()));
		return false;
	}

	float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  float cr = dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);

  String comfortStatus;
  switch(cf) {
    case Comfort_OK:
      comfortStatus = "OK";
      break;
    case Comfort_TooHot:
      comfortStatus = "TooHot";
      break;
    case Comfort_TooCold:
      comfortStatus = "TooCold";
      break;
    case Comfort_TooDry:
      comfortStatus = "TooDry";
      break;
    case Comfort_TooHumid:
      comfortStatus = "TooHumid";
      break;
    case Comfort_HotAndHumid:
      comfortStatus = "Hot&Humid";
      break;
    case Comfort_HotAndDry:
      comfortStatus = "Hot&Dry";
      break;
    case Comfort_ColdAndHumid:
      comfortStatus = "Cold&Humid";
      break;
    case Comfort_ColdAndDry:
      comfortStatus = "Cold&Dry";
      break;
    default:
      comfortStatus = "Unknown";
      break;
  };

  Serial.println(" T:" + String(newValues.temperature) + " H:" + String(newValues.humidity) + " I:" + String(heatIndex) + " D:" + String(dewPoint) + " " + comfortStatus);

  display->tempUpdates("Temp " +  String(newValues.temperature, 1) +"'C",
  "Humidity " + String(newValues.humidity,0) + "%",
  comfortStatus);

  network->firestoreDataUpdate(newValues.temperature, newValues.humidity);
  
	return true;
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("DHT ESP32 example with tasks");
  
  initDisplay();
  initNetwork();
  initTemp();
 
  tasksEnabled = true;
}

void loop() {
  if (!tasksEnabled) {
    
    delay(2000);
    
    tasksEnabled = true;
    if (tempTaskHandle != NULL) {
			vTaskResume(tempTaskHandle);
		}
  }
  yield();
}

void initDisplay(){
  display = new Display();
  display->initTFT();
  display->showWiFiIcon(false);
  display->showFirebaseIcon(false);
  display->centerMsg("System Init...");
}

void initNetwork(){
  void (*ptr)(Network_State_t) = &networkEvent;
  network = new Network(ptr);
  network->initWiFi();
}

void networkEvent(Network_State_t event){
  switch(event){
    case NETWORK_CONNECTED:
      display->showWiFiIcon(true);
      break;
    case NETWORK_DISCONNECTED:
      display->showWiFiIcon(false);
      break; 
    case FIREBASE_CONNECTED:  
      display->showFirebaseIcon(true);
      break;
    case FIREBASE_DISCONNECTED:  
      display->showFirebaseIcon(false);
      break;
    default: break;
  }
}


















