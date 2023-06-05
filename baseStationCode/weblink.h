#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "bleClientEsp32.h"

#define webLinkTask "web link log"

extern  TaskHandle_t weblinkHandle;
extern SemaphoreHandle_t xMutex;

extern bool workingCommandHalt;

void scannedDevicesHandler(enum bleDeviceScannerEvents scanBleEvent);

void connectedDeviceSensorHandler(struct deviceInfo*, enum connectedDeviceEvents deviceConEvent);

void forceSoilUpdateCheck(size_t linkedDeviceIter);

void removeSensorFromList(size_t linkedDeviceIter);

void printLinkedDeviceList();

void addToLinkDeviceList(size_t scannedDeviceIter);

void printScanList();

bool printLinkedDevice(size_t deviceIter);

bool updateTriggerValue(size_t deviceIter, uint8_t triggerVal);

int beginScan(size_t scanListSize);

void webLinkLoop();


