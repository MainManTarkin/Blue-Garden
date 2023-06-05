#include "watering.h"

static bool waterTriggerState = false;

static void turnOnWaterDevice(){

  nrf_gpio_cfg(waterStatePin, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);

}

static void turnOffWaterDevice(){

   nrf_gpio_cfg_default(waterStatePin);

}

void changeWateringState(){

  if(waterTriggerState){
     waterTriggerState = false;
     turnOffWaterDevice();
  }else{
    waterTriggerState = true;
    turnOnWaterDevice();
  }

}