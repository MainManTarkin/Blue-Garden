#pragma once


#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "nrf.h"
#include "nrf_soc.h"
#include "nordic_common.h"
#include "app_timer.h"
#include "app_util.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"

#ifdef SOFTDEVICE_PRESENT

#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "fds.h"

#else
	
#include "nrf_drv_clock.h"
#include "nrf_fstorage_nvmc.h"
#endif

//variables

struct sensorFileContent{

  uint32_t sensorId;
  uint32_t sensorMinADCVal;
  uint32_t sensorMaxADCVal;


};




//fuctions
void initSensorFlash();

ret_code_t writeDeviceInfo(struct sensorFileContent *sensorFile);

ret_code_t readDeviceInfo(struct sensorFileContent *sensorFile);

void waitForWriteComplete();
