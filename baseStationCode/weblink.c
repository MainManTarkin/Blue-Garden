#include "weblink.h"

 TaskHandle_t weblinkHandle = NULL;
 SemaphoreHandle_t xMutex = NULL;

static struct scannedDeviceInfoArrary *allScannedDevices = NULL;

static bool completedScan = false;

static bool beginTransAction = false;

static bool endTransAction = false;

static bool soildUpdated = false;

static bool addedDevice = false;

bool workingCommandHalt = true;

static bool usingTimer = false;

static bool triggerLoop = false;

static bool wateringOn = false;

static uint8_t currentSoilMoisture = 0;

void scannedDevicesHandler(enum bleDeviceScannerEvents scanBleEvent){
	
	
	switch(scanBleEvent){
		
		case foundUnlinkedSensor:
			
			completedScan = true;
			
		break;
		
		case linkedSensor:
		
			disconnectCurrentDevice();
			addedDevice = true;
			
			
		break;
		
		case foundNothing:
			completedScan = true;
			workingCommandHalt = true;
		
		break;
		
		default:
		
		
		break;
		
	}
	
	
}

void connectedDeviceSensorHandler(struct deviceInfo* deviceInfoInput, enum connectedDeviceEvents deviceConEvent){
	
	switch(deviceConEvent){
		
		case connectedToDevice:
		
			beginTransAction = true;
			
		break;
		
		case triggerLevelAlert:
			currentSoilMoisture = deviceInfoInput->moisturePercentage;
			triggerLoop = true;
		
		
		break;
		
		case updateSensorSoil:
		
			soildUpdated = true;
			
		break;
		
		
		case updatedAdvCycle:
		
			endTransAction = true;
		
		break;
		
		case disconnectedDevice:
			beginTransAction = false;
			triggerLoop = false;
			soildUpdated = false;
			endTransAction = false;
			wateringOn = false;
			workingCommandHalt = true;
			
		break;
		
		default:
		
		
		break;
		
		
		
	}
	
	
}

int beginScan(size_t scanListSize){
	
	int retVal = 0;
	
	if(allScannedDevices){
		
		free(allScannedDevices);
		allScannedDevices = NULL;
	}
	
	if(scanListSize > 0){
		
		printf( "doing scan");
		allScannedDevices = calloc(scanListSize, sizeof(struct scannedDeviceInfoArrary));
		if(!allScannedDevices){
				
			ESP_LOGE(webLinkTask, "memory alloc failed ");
			retVal = 1;
		}else{
				
				
			workingCommandHalt = false;
			allScannedDevices[0].maxSize = scanListSize;
			scanForDevices(allScannedDevices, 6);
			
		}
	}else{
	
		printf("beginScan() - scan list size input is too small\n");
		retVal = 1;
	}
	
			
		
	return retVal;
}

void printScanList(){
	
	if(allScannedDevices){
		
		if(allScannedDevices[0].arraySize > 0){
		
			for(int i = 0; i < allScannedDevices[0].arraySize; i++){
				printf("device array index: %d\n", i);
				printf("Device address: 0X%x\n", (unsigned int)allScannedDevices[i].deviceAddr);
				//esp_log_buffer_hex(webLinkTask, allScannedDevices[i].deviceAddr, 6);
				printf("device address type: %d\n", allScannedDevices[i].remoteAddrType);
				//do what comes after completed scan
			}
		}else{
			
			printf("no devices in list \n");
			
		}
		
	}else{
		
		printf("there is no device list\n");
		
	}
	
}

bool updateTriggerValue(size_t deviceIter, uint8_t triggerVal){
	bool retVal = true;
	
	
	if(deviceIter < linkedDeviceListSize){
		
		if(linkedDevices){
			
			if(triggerVal > 100){
				
				triggerVal = 100;
				
			}
			
			linkedDevices[deviceIter].triggerVal = triggerVal;
			
		}else{
			
			printf("linkedDevices is not initialized\n");
			retVal = false;
			
		}
		
	}else{
		
		printf("deviceIter is larger then linked device list size\n");
		retVal = false;
		
	}
	
	return retVal;
}

bool printLinkedDevice(size_t deviceIter){
	bool retVal = true;
	
	
	if(deviceIter < linkedDeviceListSize){
		
		if(linkedDevices){
			
			printf("-----------------SENSOR-----------------\n");
			printf("device address: %s\n", linkedDevices[deviceIter].deviceAddr);
			printf("device ID: %d\n", (int)linkedDevices[deviceIter].sensorUID);
			printf("device soil moisture percentage: %d\n", (int)linkedDevices[deviceIter].moisturePercentage);
			printf("device next advertise cycle: %d\n", (int)linkedDevices[deviceIter].nextAdvCycle);
			printf("----------------------------------------\n\n");
			
		}else{
			
			printf("linkedDevices is not initialized\n");
			retVal = false;
			
		}
		
	}else{
		
		printf("deviceIter is larger then linked device list size\n");
		retVal = false;
		
	}
	
	return retVal;
	
}

void addToLinkDeviceList(size_t scannedDeviceIter){
	
	if(allScannedDevices){
		
		if(addDeviceToList(&allScannedDevices[scannedDeviceIter])){
			
			workingCommandHalt = false;
			
		}
		
	}else{
		
		printf("there is no scanned device list yet\n");
		
	}
	
}



void printLinkedDeviceList(){
	
	if(linkedDevices){
	
		for(size_t i =0; i < linkedDeviceListSize; i++){
			
			printLinkedDevice(i);
		}
	
	}else{
		
		printf("there is no linked device list\n");
		
	}
	
}

void removeSensorFromList(size_t linkedDeviceIter){
	
	if(linkedDeviceIter > linkedDeviceListSize){
		
		printf("given device iter is larger then list\n");
		
	}else{
		
		removeDeviceFromList(linkedDeviceIter);
		
	}
	
}

void forceSoilUpdateCheck(size_t linkedDeviceIter){
	
	if(usingTimer){
		
		printf("cannot force update when auto timer is used\n");
		
	}else{
	
		if(linkedDeviceIter > linkedDeviceListSize){
		
			printf("given device iter is larger then list\n");
		
		}else{
			
			if(tryConnectToLinkedDevice(linkedDeviceIter)){
				workingCommandHalt = false;
				
			
			}else{
				
				printf("failed to connect to device: %d", (int)linkedDeviceIter);
				
			}
			
		}
	
	}
}

void webLinkLoop(){
	ESP_LOGI(webLinkTask, "entering web link loop");
	while(true){
		
		if(completedScan){
			
			printScanList();
			
			workingCommandHalt = true;
			completedScan = false;
		}
		
		if(addedDevice){
			printf("added sensor\n");
			workingCommandHalt = true;
			addedDevice = false;
		}
		
		if(beginTransAction){
			
			updateSoilContent();
			beginTransAction = false;
		}
		
		if(triggerLoop){
			printf("trigger watering \n Soil Moisture is now: %d", (int)currentSoilMoisture);
			if(!wateringOn){
				wateringOn = true;
				turnOnWateringDeviceWrite();
				
			}
			beginTransAction = true;
			triggerLoop = false;
		}
		
		if(soildUpdated){
			
			if(!updateAdvCycle()){
				
				endTransAction = true;
				
			}
			if(wateringOn){
				wateringOn = false;
				turnOnWateringDeviceWrite();
				
			}
			soildUpdated = false;
			
		}
		
		if(endTransAction){
			
			disconnectCurrentDevice();
			workingCommandHalt = true;
			endTransAction = false;
		}
		
		vTaskDelay(600 / portTICK_RATE_MS);
	}
	
}

