#include "startState.h"

// give proper pin number value when fiqured out



nrf_drv_gpiote_in_config_t calbButtonConfig = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
bool buttonPressed = true;

uint32_t getDipSwitchVal(uint8_t *dipVal) {
  uint8_t dipState = 0;
  uint8_t pinVal1 = 0;
  uint8_t pinVal2 = 0;

  if (!dipVal) {

    return nullInputVal;

  } else {

    *dipVal = 0;
  }

  pinVal1 = nrf_gpio_pin_read(dipPin1);
  pinVal2 = nrf_gpio_pin_read(dipPin2);

  dipState = pinVal1;
  dipState = (dipState << 1) | pinVal2;
  
  *dipVal = dipState;

  return 0;
}

void calbriateCompleted(){
	
	nrf_gpio_cfg(calbriCompletePin, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
	
}

static uint32_t setupPins() {

  nrf_gpio_cfg_input(dipPin1, NRF_GPIO_PIN_PULLDOWN);

  if (nrf_gpio_pin_input_get(dipPin1) == NRF_GPIO_PIN_INPUT_DISCONNECT) {

    return 1;
  }

  nrf_gpio_cfg_input(dipPin2, NRF_GPIO_PIN_PULLDOWN);

  if (nrf_gpio_pin_input_get(dipPin2) == NRF_GPIO_PIN_INPUT_DISCONNECT) {

    return 2;
  }

  return 0;
}

void calbriPinHandler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {

  bool maxTrue = ~(0);

  if (pin == calbiButton) {

    switch (action) {

    case NRF_GPIOTE_POLARITY_LOTOHI:
      buttonPressed = !(buttonPressed);
      break;
    case NRF_GPIOTE_POLARITY_HITOLO:
      buttonPressed = !(buttonPressed);
      break;
    case NRF_GPIOTE_POLARITY_TOGGLE:

      buttonPressed = !(buttonPressed);

      break;
    default:

      break;
    };
  }
}

void disableCalbriButton() {

  buttonPressed = false;
  nrfx_gpiote_in_event_disable(calbiButton);
}

void enablecalbriButton() {

  buttonPressed = true;
  nrfx_gpiote_in_event_enable(calbiButton, true);
}

uint32_t setupCalbriIntrupt() {

  ret_code_t errCode = 0;
  calbButtonConfig.pull = NRF_GPIO_PIN_PULLUP;

  errCode = nrf_drv_gpiote_init();
  if (errCode != NRF_ERROR_INVALID_STATE) {

    APP_ERROR_CHECK(errCode);

  } else {

    errCode = 0;
  }

  errCode = nrf_drv_gpiote_in_init(calbiButton, &calbButtonConfig, calbriPinHandler);
  APP_ERROR_CHECK(errCode);

  return errCode;
}

uint32_t getStartStateHandler(uint32_t *stateValOutput) {

  uint32_t err_code = 0;
  uint32_t stateVal = defualtState;
  uint8_t dipSwitchVal = 0;

  // setup io pins

  if ((err_code = setupPins())) {

    return err_code;
  }

  if ((err_code = getDipSwitchVal(&dipSwitchVal))) {

    return err_code;
  }

  stateVal = dipSwitchVal;

  if (stateVal > resetMode) {

    stateVal = defualtState;
  }

  *stateValOutput = stateVal;

  return err_code;
}