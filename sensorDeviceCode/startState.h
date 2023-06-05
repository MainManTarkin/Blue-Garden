#pragma once

#include "app_error.h"
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include <stdbool.h>
#include <stdint.h>

#define dipPin1 11
#define dipPin2 12
#define calbiButton 25
#define calbriCompletePin 16

extern bool buttonPressed;
extern nrf_drv_gpiote_in_config_t calbButtonConfig;

enum stateFucReturnCodes {

  nullInputVal = 1

};

enum startStateModes {

  defualtState = 1,
  calibrationMode,
  resetMode

};

uint32_t getStartStateHandler(uint32_t *stateValOutput);

uint32_t setupCalbriIntrupt();

void enablecalbriButton();

void disableCalbriButton();

void calbriateCompleted();

