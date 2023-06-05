#include "eeStore.h"

#define valueEEPROM 0

bool readComplete = false;
bool noTransAction = true;
bool transActionCompleted = false;

uint8_t currentAddr = 0;

static const nrf_drv_twi_t masterTWI = NRF_DRV_TWI_INSTANCE(valueEEPROM);
static uint8_t deviceAddress = 0;

static uint32_t eepromMaxAddresses = 0;

static bool readStart = false;

void twiHandler(nrf_drv_twi_evt_t const *p_event, void *p_context) {

  switch (p_event->type) {

  case NRF_DRV_TWI_EVT_DONE:

    noTransAction = true;
    transActionCompleted = true;

    if (!readStart) {

      readComplete = true;
      readStart = false;
    }

    break;

  case NRF_DRV_TWI_EVT_ADDRESS_NACK:

    noTransAction = false;
    transActionCompleted = false;
    break;

  case NRF_DRV_TWI_EVT_DATA_NACK:

    noTransAction = false;
    transActionCompleted = false;
    break;

  default:

    break;
  }
}

void initEEPROM(uint8_t eepromDeviceAddress, uint32_t maxAddresses, bool blocking) {

  ret_code_t errCode = NRF_SUCCESS;

  deviceAddress = eepromDeviceAddress;
  eepromMaxAddresses = maxAddresses;

  nrf_drv_twi_config_t config = NRF_DRV_TWI_DEFAULT_CONFIG;
  config.scl = sclPinNum;
  config.sda = sdaPinNum;

  blocking = true;
  if (blocking) {

    errCode = nrf_drv_twi_init(&masterTWI, &config, NULL, NULL);

  } else {

    errCode = nrf_drv_twi_init(&masterTWI, &config, twiHandler, NULL);
  }

  if (NRF_SUCCESS == errCode) {
    nrf_drv_twi_enable(&masterTWI);
  } else {

    APP_ERROR_CHECK(errCode);
  }
}

void uninitEEPROM() {

  nrf_drv_twi_uninit(&masterTWI);
}

ret_code_t writeEEPROM(uint8_t address, uint32_t data) {

  ret_code_t errCode = NRF_SUCCESS;

  
  uint8_t sendBuffer[sizeof(uint8_t) + sizeof(uint32_t)] = {};
  sendBuffer[0] = address;
  memcpy((sendBuffer + sizeof(uint8_t)), &data, sizeof(uint32_t));
  
  if ((uint32_t)address > eepromMaxAddresses) {

    errCode = NRF_ERROR_INVALID_ADDR;
    APP_ERROR_CHECK(errCode);
    return errCode;
  }

  transActionCompleted = false;
  errCode = nrf_drv_twi_tx(&masterTWI, deviceAddress, sendBuffer, sizeof(sendBuffer), false);

  if (errCode == NRF_ERROR_DRV_TWI_ERR_ANACK || errCode == NRF_ERROR_DRV_TWI_ERR_DNACK) {

    noTransAction = false;
    errCode = -1;

  } else {

    APP_ERROR_CHECK(errCode);
  }

  if (errCode == NRF_SUCCESS) {
    noTransAction = true;
    currentAddr = address;
  }
  return errCode;
}

ret_code_t setEEPROMAddr(uint8_t address) {

  ret_code_t errCode = NRF_SUCCESS;

  if (address > eepromMaxAddresses) {

    errCode = NRF_ERROR_INVALID_ADDR;
    APP_ERROR_CHECK(errCode);
    return errCode;
  }

  transActionCompleted = false;
  errCode = nrf_drv_twi_tx(&masterTWI, deviceAddress, &address, 1, false);

  if (errCode == NRF_ERROR_DRV_TWI_ERR_ANACK || errCode == NRF_ERROR_DRV_TWI_ERR_DNACK) {

    noTransAction = false;
    errCode = -1;

  } else {

    APP_ERROR_CHECK(errCode);
  }

  if (errCode == NRF_SUCCESS) {
    noTransAction = true;
    currentAddr = address;
  }

  return errCode;
}

ret_code_t readEEOROM(uint8_t address,  uint32_t *data) {

  ret_code_t errCode = NRF_SUCCESS;

  if (address > eepromMaxAddresses) {

    errCode = NRF_ERROR_INVALID_ADDR;
    APP_ERROR_CHECK(errCode);
    return errCode;
  }

  if (address != currentAddr) {

    errCode = -1;
    return errCode;
  }

  transActionCompleted = false;
  readComplete = false;
  errCode = nrf_drv_twi_rx(&masterTWI, deviceAddress, (uint8_t*)data, sizeof(uint32_t));

  if (errCode == NRF_ERROR_DRV_TWI_ERR_ANACK || errCode == NRF_ERROR_DRV_TWI_ERR_DNACK) {

    noTransAction = false;
    errCode = -1;

  } else {

    APP_ERROR_CHECK(errCode);
  }

  if (errCode == NRF_SUCCESS) {
    noTransAction = true;
    readStart = true;
  }

  return errCode;
}