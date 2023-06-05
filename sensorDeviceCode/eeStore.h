#pragma  once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nrf.h"
#include "nrf_soc.h"
#include "nordic_common.h"
#include "app_util.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_drv_twi.h"
#include "nrfx_twi.h"

#define twiEEPROMaddr 80

#define minCalValAddr 0
#define maxCalValAddr 4
#define deviceIDAddr 8

#define sdaPinNum 26
#define sclPinNum 27

extern bool readComplete;
extern bool noTransAction;

extern uint8_t currentAddr;


void initEEPROM(uint8_t eepromDeviceAddress, uint32_t maxAddresses, bool blocking);

void uninitEEPROM();

ret_code_t writeEEPROM(uint8_t address, uint32_t data);

ret_code_t setEEPROMAddr(uint8_t address);

ret_code_t readEEOROM(uint8_t address,  uint32_t *data);