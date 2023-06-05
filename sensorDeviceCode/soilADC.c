#include "soilADC.h"

#define indicationADCLight 15
#define SAMPLES_IN_BUFFER 1
static const nrf_drv_timer_t sampTimer = NRF_DRV_TIMER_INSTANCE(4);
static nrf_ppi_channel_t ppiChannel;
static nrf_saadc_value_t sampleBuffer[2][SAMPLES_IN_BUFFER];
static nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
volatile static bool blockState = true;

int32_t maxCalbriValue = 2910;
int32_t minCalbriValue = 2480;
uint8_t soilMoisturePercent = 0;

static int32_t fetchedADCvalue = 0;

static bool setRange = false;
bool newValue = false;

static void samplingEventDisable();

static void saadcEventHandler(nrf_drv_saadc_evt_t const *p_event) {
  nrf_saadc_value_t resultStore = 0;
  int32_t finalValue = 0;

  if (p_event->type == NRF_DRV_SAADC_EVT_DONE) {

    ret_code_t err_code;

    samplingEventDisable();

    err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);

    for (int i = 0; i < SAMPLES_IN_BUFFER; i++) {
      resultStore = p_event->data.done.p_buffer[i];
      finalValue += (int32_t)resultStore;
    }
    finalValue /= SAMPLES_IN_BUFFER;
    finalValue = finalValue * 3600 / 4096;
    fetchedADCvalue = finalValue;

    if (setRange) {

      if (fetchedADCvalue < minCalbriValue) {

        fetchedADCvalue = minCalbriValue;
      }

      if (fetchedADCvalue > maxCalbriValue) {

        fetchedADCvalue = maxCalbriValue;
      }

      soilMoisturePercent = ((fetchedADCvalue - minCalbriValue) * 100) / (maxCalbriValue - minCalbriValue);
      newValue = true;
    }

    blockState = false;
  }
}

void timer_handler(nrf_timer_event_t event_type, void *p_context) {
}

static void samplePowerWait(void) {
#ifdef SOFTDEVICE_PRESENT
  (void)sd_app_evt_wait();
#else
  __WFE();
#endif
}

static void turnOnADClight() {

  nrf_gpio_cfg(indicationADCLight, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
}

static void turnOffADClight() {

  nrf_gpio_cfg_default(indicationADCLight);
}

static void waitForCalButtonPress() {

  bool lastButtonState = buttonPressed;

  while (buttonPressed == lastButtonState) {

    samplePowerWait();
  }
}

static void holdUntillSampleComplete() {

  while (blockState) {

    ;
  }

  blockState = true;
}

void samplingEventInit() {
  ret_code_t err_code;

  err_code = nrf_drv_ppi_init();
  APP_ERROR_CHECK(err_code);

  timer_cfg.bit_width = NRF_TIMER_BIT_WIDTH_32;
  err_code = nrf_drv_timer_init(&sampTimer, &timer_cfg, timer_handler);
  APP_ERROR_CHECK(err_code);

  /* setup m_timer for compare event every 400ms */
  uint32_t ticks = nrf_drv_timer_ms_to_ticks(&sampTimer, 10);
  nrf_drv_timer_extended_compare(&sampTimer,
      NRF_TIMER_CC_CHANNEL4,
      ticks,
      NRF_TIMER_SHORT_COMPARE4_CLEAR_MASK,
      false);
 //  nrf_drv_timer_enable(&sampTimer);

  uint32_t timer_compare_event_addr = nrf_drv_timer_compare_event_address_get(&sampTimer,
      NRF_TIMER_CC_CHANNEL4);
  uint32_t saadc_sample_task_addr = nrf_drv_saadc_sample_task_get();

  /* setup ppi channel so that timer compare event is triggering sample task in SAADC */
  err_code = nrf_drv_ppi_channel_alloc(&ppiChannel);
  APP_ERROR_CHECK(err_code);

  err_code = nrf_drv_ppi_channel_assign(ppiChannel,
      timer_compare_event_addr,
      saadc_sample_task_addr);
  APP_ERROR_CHECK(err_code);
}

static void saadc_sampling_event_enable() {
  nrf_drv_timer_enable(&sampTimer);
  ret_code_t err_code = nrf_drv_ppi_channel_enable(ppiChannel);

  APP_ERROR_CHECK(err_code);
}

static void samplingEventDisable() {

  nrf_drv_timer_disable(&sampTimer);
  ret_code_t err_code = nrfx_ppi_channel_disable(ppiChannel);

  APP_ERROR_CHECK(err_code);
}

void calibrateSaadcDevice() {
  // get max value (saturated soil) and then min value (dry soil) in that order
  ret_code_t errCode = 0;

  if (setupCalbriIntrupt()) {
    NRF_LOG_INFO("failed to setup calibration button");

    APP_ERROR_CHECK(1);
  }
  
  
  turnOnADClight();

  enablecalbriButton();

  waitForCalButtonPress();

  disableCalbriButton();
  turnOffADClight();

  saadc_sampling_event_enable();

  holdUntillSampleComplete();

  maxCalbriValue = fetchedADCvalue;
  
  turnOnADClight();
  enablecalbriButton();

  waitForCalButtonPress();
  disableCalbriButton();
  turnOffADClight();

  saadc_sampling_event_enable();

  holdUntillSampleComplete();

  minCalbriValue = fetchedADCvalue;

  NRF_LOG_INFO("min val: %d", minCalbriValue);
  NRF_LOG_INFO("max val: %d", maxCalbriValue);
}

void initSaadc() {

  ret_code_t errCode = 0;

  nrfx_saadc_config_t confT = {// NRFX_SAADC_DEFAULT_CONFIG;
      .resolution = (nrf_saadc_resolution_t)NRF_SAADC_RESOLUTION_12BIT,
      .oversample = (nrf_saadc_oversample_t)NRF_SAADC_OVERSAMPLE_256X,
      .interrupt_priority = 6,
      .low_power_mode = 0};

  nrf_saadc_channel_config_t channel_config = // NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN0);

      {
          .resistor_p = NRF_SAADC_RESISTOR_DISABLED,
          .resistor_n = NRF_SAADC_RESISTOR_DISABLED,
          .gain = NRF_SAADC_GAIN1_6,
          .reference = NRF_SAADC_REFERENCE_INTERNAL,
          .acq_time = NRF_SAADC_ACQTIME_40US,
          .mode = NRF_SAADC_MODE_SINGLE_ENDED,
          .burst = NRF_SAADC_BURST_DISABLED,
          .pin_p = NRF_SAADC_INPUT_AIN0,
          .pin_n = NRF_SAADC_INPUT_DISABLED};

  errCode = nrf_drv_saadc_init(&confT, saadcEventHandler);
  APP_ERROR_CHECK(errCode);

  errCode = nrf_drv_saadc_channel_init(0, &channel_config);
  APP_ERROR_CHECK(errCode);

  errCode = nrfx_saadc_calibrate_offset();
  APP_ERROR_CHECK(errCode);

  while (nrfx_saadc_is_busy()) {

    ;
  }

  errCode = nrf_drv_saadc_buffer_convert(sampleBuffer[0], SAMPLES_IN_BUFFER);
  APP_ERROR_CHECK(errCode);

  errCode = nrf_drv_saadc_buffer_convert(sampleBuffer[1], SAMPLES_IN_BUFFER);
  APP_ERROR_CHECK(errCode);

  samplingEventInit();
}

void getSoilMoistureContent() {

  setRange = true;
  saadc_sampling_event_enable();
}