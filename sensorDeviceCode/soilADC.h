#pragma  once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_drv_saadc.h"
#include "app_error.h"
#include "app_util_platform.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "startState.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_ppi.h"


extern int32_t maxCalbriValue, minCalbriValue;

extern uint8_t soilMoisturePercent;

extern bool newValue;

void initSaadc();

void calibrateSaadcDevice();

void getSoilMoistureContent();