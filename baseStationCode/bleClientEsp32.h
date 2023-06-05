#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"


#define bleAdvertPacketSize 33

#define GATTC_TAG "GATTC_DEMO"
#define REMOTE_SERVICE_UUID        0x00FF
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01
#define PROFILE_NUM      1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0

#define maxLinkedSensors 5
#define unlinkedSensorID 0xffffffff

#define defaultScanTime 30 //in secs
#define defaultTriggerLevel 60

//function Event Handlers
void bleSensorEventHandler();


//enum defines

enum bleDeviceScannerEvents{
	
	noDeviceEvent,
	foundNothing,
	foundUnlinkedSensor,
	updatedSensorID,
	updateSensorSoilPer,
	failedToScanLink,
	linkedSensor,
	attemptingToConnect
	
	
};

enum connectedDeviceEvents{
	
	noConnectedEvent,
	connectedToDevice,
	disconnectedDevice,
	updateSensorSoil,
	triggerLevelAlert,
	updatedAdvCycle,
	wroteData
	
	
};

//struct defines

struct scannedDeviceInfoArrary{
	
	esp_bd_addr_t deviceAddr;
	esp_ble_addr_type_t remoteAddrType;
	
	uint32_t arraySize;
	uint32_t maxSize;
	
};

struct deviceInfo{
	
	
	esp_bd_addr_t deviceAddr;
	esp_ble_addr_type_t remoteAddrType;
	
	uint32_t sensorUID;
	
	uint8_t moisturePercentage;
	uint8_t triggerVal;
	
	uint32_t nextAdvCycle;
	
	bool isUsed;
	
	
};

struct bleSensorEvents{
	
	enum bleDeviceScannerEvents eventType;
	
	void (*generalEventHandler)(struct bleSensorEvents*);
	
	void (*foundUnlinkedSensor)(enum bleDeviceScannerEvents);
	
	void (*connectedSensorHandler)(struct deviceInfo*, enum connectedDeviceEvents);

};

struct freeLinkedSensorStack{
	
	size_t freelinkedDeviceSpot;
	struct freeLinkedSensorStack *prevFreeSpot;
	
};

//client globals

extern struct deviceInfo *currentDevice;

extern struct deviceInfo *linkedDevices;

extern uint32_t currentSensorUID;

extern size_t linkedDeviceListSize;

bool isCurrentlyScanning();


//get all ble sensor device in area 
//fails if already scanning
bool scanForDevices(struct scannedDeviceInfoArrary *array, uint32_t scanningTime);

//look for and try to connect to device with given deviceUID
//fails if already scanning
bool scanForDevice(uint32_t deviceUID, uint32_t scanningTime);

bool updateSoilContent();

bool disconnectCurrentDevice();

void removeDeviceFromList(size_t linkedDeviceIter);

bool updateAdvCycle();

bool turnOnWateringDeviceWrite();

bool addDeviceToList(struct scannedDeviceInfoArrary * deviceToAdd);

bool tryConnectToLinkedDevice(size_t linkedDeviceIndex);

bool initESPBle(struct bleSensorEvents *sensorEventStore);