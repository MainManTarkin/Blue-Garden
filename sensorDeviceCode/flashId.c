#include "flashId.h"

#define FILE_ID 0x0001      /* The ID of the file to write the records into. */
#define RECORD_KEY_1 0x1111 /* A key for the first record. */
#define sensorStructWordCount 3

static fds_stat_t stat = {0};
static bool volatile fdsInit = false;
static fds_record_t record;
static fds_record_desc_t recordDesc;
static volatile bool writingToFlash = false;

static void flashSystemHandler(fds_evt_t const *p_evt) {
  if (p_evt->result == NRF_SUCCESS) {
    NRF_LOG_INFO("flash event success");
  } else {
    NRF_LOG_INFO("flash event failed");
  }

  switch (p_evt->id) {
  case FDS_EVT_INIT:
    if (p_evt->result == NRF_SUCCESS) {
      fdsInit = true;
    }
    break;

  case FDS_EVT_WRITE: {
    if (p_evt->result == NRF_SUCCESS) {
      NRF_LOG_INFO("Record ID:\t0x%04x", p_evt->write.record_id);
      NRF_LOG_INFO("File ID:\t0x%04x", p_evt->write.file_id);
      NRF_LOG_INFO("Record key:\t0x%04x", p_evt->write.record_key);
      writingToFlash = false;
    }
  } break;

  case FDS_EVT_DEL_RECORD: {
    if (p_evt->result == NRF_SUCCESS) {
      NRF_LOG_INFO("Record ID:\t0x%04x", p_evt->del.record_id);
      NRF_LOG_INFO("File ID:\t0x%04x", p_evt->del.file_id);
      NRF_LOG_INFO("Record key:\t0x%04x", p_evt->del.record_key);
    }

  } break;

  default:
    break;
  }
}

static void flashPowerSave(void) {
#ifdef SOFTDEVICE_PRESENT
    nrf_pwr_mgmt_run();
#else
  __WFE();
#endif
}

static void waitForFlashSystem(void) {
  while (!fdsInit) {
    flashPowerSave();
  }
}

void waitForWriteComplete(){

  while(writingToFlash){
    flashPowerSave();

  }

}

inline void initSensorFlash() {
  ret_code_t errCode = NRF_SUCCESS;

  errCode = fds_register(flashSystemHandler);
   APP_ERROR_CHECK(errCode);

  errCode = fds_init();
  APP_ERROR_CHECK(errCode);

  waitForFlashSystem();

  errCode = fds_stat(&stat);
  APP_ERROR_CHECK(errCode);

  record.file_id = FILE_ID;
  record.key = RECORD_KEY_1;

  errCode = fds_gc();
  APP_ERROR_CHECK(errCode);
}

ret_code_t writeDeviceInfo(struct sensorFileContent *sensorFile) {

  ret_code_t errCode = NRF_SUCCESS;

  if (!fdsInit) {

    errCode = FDS_ERR_NOT_INITIALIZED;

  } else {

    record.data.p_data = sensorFile;
    record.data.length_words = sensorStructWordCount;

    do {
      errCode = fds_record_write(&recordDesc, &record);
      if (errCode != NRF_SUCCESS) {

        if (errCode == FDS_ERR_NO_SPACE_IN_FLASH) {

          errCode = fds_gc();
        }
      }else{

        writingToFlash = true;

      }

    } while (errCode);
  }

  return errCode;
}

ret_code_t readDeviceInfo(struct sensorFileContent *sensorFile) {

  ret_code_t errCode = NRF_SUCCESS;
  fds_find_token_t ftok;
  fds_flash_record_t recoverdRecord;

  memset(&ftok, 0, sizeof(fds_find_token_t));

  if (!fdsInit) {

    errCode = FDS_ERR_NOT_INITIALIZED;

  } else {

    while ((errCode = fds_record_find(FILE_ID, RECORD_KEY_1, &recordDesc, &ftok)) == NRF_SUCCESS) {

      if ((errCode = fds_record_open(&recordDesc, &recoverdRecord)) == NRF_SUCCESS) {
        
        memcpy(sensorFile, recoverdRecord.p_data, sizeof(struct sensorFileContent));
        break;
      }
    }
  }

  return errCode;
}