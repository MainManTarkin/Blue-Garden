/*
* the main application for ble base station sensor esp32 
* inits all required stacks and sets up freertos tasks
*
*/

#include "bleClientEsp32.h"
#include "weblink.h"
#include "bleConsole.h"

#define appLog "appLog"

void mainSensorHandler(struct bleSensorEvents* sensorEvents){
	
	
	
	
	
	
};


struct bleSensorEvents sensorDeviceEventStore = {
	
	.eventType = noDeviceEvent,
	.generalEventHandler = mainSensorHandler,
	.foundUnlinkedSensor = scannedDevicesHandler,
	.connectedSensorHandler = connectedDeviceSensorHandler
	
	
	
};

void app_main(void)
{

	if(!initESPBle(&sensorDeviceEventStore)){
		
		ESP_LOGE(appLog, "failed to init ESP ble ");
		
	}else{
		
		ESP_LOGI(appLog, "init of ble success ");
		
	}
	
	BaseType_t xReturned;
	// init the eventList
	
	ESP_LOGI(appLog, "creating mutex");
    xMutex = xSemaphoreCreateMutex();
	
	ESP_LOGI(appLog, "creating task");
	xReturned = xTaskCreate(webLinkLoop, "weblink loop", 4096, NULL, 10, &weblinkHandle);
	
	if(xReturned != pdPASS ){
		
		ESP_LOGE(appLog, "weblink task failed to create");
		
	}else{
		
		ESP_LOGI(appLog, "weblink task created");
		
	}
	
	initConsole();
	
	xReturned = xTaskCreate(consoleLoop, "console loop", 4096, NULL, 10, &consolelinkHandle);
	
	if(xReturned != pdPASS ){
		
		ESP_LOGE(appLog, "weblink task failed to create");
		
	}else{
		
		ESP_LOGI(appLog, "weblink task created");
		
	}

	//the main thread dies 
}