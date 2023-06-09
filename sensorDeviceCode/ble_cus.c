#include "ble_cus.h"
#include "ble_srv_common.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "sdk_common.h"
#include <string.h>


static void on_connect(ble_cus_t *p_cus, ble_evt_t const *p_ble_evt) {
  p_cus->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

  ble_cus_evt_t evt;

  evt.evt_type = BLE_CUS_EVT_CONNECTED;

  p_cus->evt_handler(p_cus, &evt);
}


static void on_disconnect(ble_cus_t *p_cus, ble_evt_t const *p_ble_evt) {
  UNUSED_PARAMETER(p_ble_evt);
  p_cus->conn_handle = BLE_CONN_HANDLE_INVALID;

  ble_cus_evt_t evt;

  evt.evt_type = BLE_CUS_EVT_DISCONNECTED;

  p_cus->evt_handler(p_cus, &evt);
}


static void on_write(ble_cus_t *p_cus, ble_evt_t const *p_ble_evt) {
  ble_gatts_evt_write_t const *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

  ble_gatts_char_handles_t *currentCharHandle = NULL;
  ble_cus_evt_t evt;

  bool foundHandle = false;
  // find handle if its there

  for (size_t i = 0; i < numberOfCharHandles; i++) {

    NRF_LOG_INFO("searched Handle: %d | looking for handle: %d", p_cus->custom_value_handles[i].value_handle, p_evt_write->handle);

    if (p_evt_write->handle == p_cus->custom_value_handles[i].value_handle) {

      currentCharHandle = &p_cus->custom_value_handles[i];
      p_cus->currentHandleInput = currentCharHandle;
      foundHandle = true;
      break;
    }

    // check if modifying cccd

    if (p_evt_write->handle == p_cus->custom_value_handles[i].cccd_handle) {

      currentCharHandle = &p_cus->custom_value_handles[i];
      p_cus->currentHandleInput = currentCharHandle;
      foundHandle = true;
      break;
    }
  }

  if (!foundHandle) {
    NRF_LOG_INFO("did not find handle");
    return;
  }

  NRF_LOG_INFO("write Handle: %d | ccHandle: %d | writeLen: %d", p_evt_write->handle, currentCharHandle->cccd_handle, p_evt_write->len);
  // Check if the Custom value CCCD is written to and that the value is the appropriate length, i.e 2 bytes.
  if ((p_evt_write->handle == currentCharHandle->cccd_handle) && (p_evt_write->len == 2)) {
    // CCCD written, call application event handler
    if (p_cus->evt_handler != NULL) {

      if (ble_srv_is_notification_enabled(p_evt_write->data)) {
        evt.evt_type = BLE_CUS_EVT_NOTIFICATION_ENABLED;
      } else {
        evt.evt_type = BLE_CUS_EVT_NOTIFICATION_DISABLED;
      }
      // Call the application event handler.
      p_cus->evt_handler(p_cus, &evt);
    }
  }

  // check to see if the idChar was writen too so an event can be passed on to the special event handler
  // to update the the device ID advert
  if (p_evt_write->handle == p_cus->custom_value_handles[deviceIDslot].value_handle) {

    evt.evt_type = deviceIDChange;

    memcpy(&p_cus->writeValuePasser, p_evt_write->data, maxDatalength);

    p_cus->evt_handler(p_cus, &evt);
  }

  // check to see if command handler was written too
  if (p_evt_write->handle == p_cus->custom_value_handles[commandSlot].value_handle) {

    evt.evt_type = deviceCommand;

    p_cus->writeValuePasser = *(uint8_t *)p_evt_write->data;
    // memcpy(&p_cus->writeValuePasser,p_evt_write->data, maxDatalength);

    p_cus->evt_handler(p_cus, &evt);
  }
}

void ble_cus_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context) {
  ble_cus_t *p_cus = (ble_cus_t *)p_context;

  NRF_LOG_INFO("BLE event received. Event type = %d\r\n", p_ble_evt->header.evt_id);
  if (p_cus == NULL || p_ble_evt == NULL) {
    return;
  }

  switch (p_ble_evt->header.evt_id) {
  case BLE_GAP_EVT_CONNECTED:
    on_connect(p_cus, p_ble_evt);
    break;

  case BLE_GAP_EVT_DISCONNECTED:
    on_disconnect(p_cus, p_ble_evt);
    break;

  case BLE_GATTS_EVT_WRITE:
    on_write(p_cus, p_ble_evt);
    break;
    /* Handling this event is not necessary
            case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
                NRF_LOG_INFO("EXCHANGE_MTU_REQUEST event received.\r\n");
                break;
    */
  default:
    // No implementation needed.
    break;
  }
}

/**@brief Function for adding the Custom Value characteristic.
 *
 * @param[in]   p_cus        Battery Service structure.
 * @param[in]   p_cus_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static uint32_t custom_value_char_add(ble_cus_t *p_cus, const ble_cus_init_t *p_cus_init, struct charSetUpVals *charValsInput) {
  uint32_t err_code;
  ble_gatts_char_md_t char_md;
  ble_gatts_attr_md_t cccd_md;
  ble_gatts_attr_t attr_char_value;
  ble_uuid_t ble_uuid;
  ble_gatts_attr_md_t attr_md;

  // Add Custom Value characteristic
  memset(&cccd_md, 0, sizeof(cccd_md));

  //  Read  operation on cccd should be possible without authentication.
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);

  cccd_md.write_perm = p_cus_init->custom_value_char_attr_md.cccd_write_perm;
  cccd_md.vloc = BLE_GATTS_VLOC_STACK;

  memset(&char_md, 0, sizeof(char_md));

  char_md.char_props.read = charValsInput->readSet;
  char_md.char_props.write = charValsInput->writeSet;
  char_md.char_props.notify = charValsInput->notifySet;
  char_md.p_char_user_desc = NULL;
  char_md.p_char_pf = NULL;
  char_md.p_user_desc_md = NULL;
  char_md.p_cccd_md = &cccd_md;
  char_md.p_sccd_md = NULL;

  ble_uuid.type = p_cus->uuid_type;
  ble_uuid.uuid = charValsInput->UUID;

  memset(&attr_md, 0, sizeof(attr_md));

  attr_md.read_perm = p_cus_init->custom_value_char_attr_md.read_perm;
  attr_md.write_perm = p_cus_init->custom_value_char_attr_md.write_perm;
  attr_md.vloc = BLE_GATTS_VLOC_STACK;
  attr_md.rd_auth = 0;
  attr_md.wr_auth = 0;
  attr_md.vlen = 0;

  memset(&attr_char_value, 0, sizeof(attr_char_value));

  attr_char_value.p_uuid = &ble_uuid;
  attr_char_value.p_attr_md = &attr_md;
  attr_char_value.init_len = charValsInput->dataSize;
  attr_char_value.init_offs = 0;
  attr_char_value.max_len = charValsInput->dataSize;

  err_code = sd_ble_gatts_characteristic_add(p_cus->service_handle, &char_md,
      &attr_char_value,
      &p_cus->custom_value_handles[charValsInput->charIter]);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  return NRF_SUCCESS;
}

uint32_t ble_cus_init(ble_cus_t *p_cus, const ble_cus_init_t *p_cus_init) {
  if (p_cus == NULL || p_cus_init == NULL) {
    return NRF_ERROR_NULL;
  }

  uint32_t err_code;

  struct charSetUpVals charVals;
  ble_uuid_t ble_uuid;

  // Initialize charVals
  memset(&charVals, 0, sizeof(charVals));

  // Initialize service structure
  p_cus->evt_handler = p_cus_init->evt_handler;
  p_cus->conn_handle = BLE_CONN_HANDLE_INVALID;

  // Add Custom Service UUID
  ble_uuid128_t base_uuid = {CUSTOM_SERVICE_UUID_BASE};
  err_code = sd_ble_uuid_vs_add(&base_uuid, &p_cus->uuid_type);
  VERIFY_SUCCESS(err_code);

  ble_uuid.type = p_cus->uuid_type;
  ble_uuid.uuid = CUSTOM_SERVICE_UUID;

  // Add the Custom Service
  err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cus->service_handle);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // set up char vals for IDchar
  charVals.writeSet = 1;
  charVals.readSet = 1;
  charVals.dataSize = sizeof(uint32_t); // four bytes long
  charVals.UUID = deviceIDCharUUID;

  // Add Custom Value characteristic
  err_code = custom_value_char_add(p_cus, p_cus_init, &charVals);

  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // set up char vals for commandChar
  charVals.writeSet = 1;
  charVals.readSet = 1;
  charVals.dataSize = sizeof(uint8_t); // one byte long
  charVals.UUID = commandCharUUID;
  charVals.charIter = 1;

  err_code = custom_value_char_add(p_cus, p_cus_init, &charVals);

  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // set up char vals for CommandOutputChar
  charVals.writeSet = 0;
  charVals.readSet = 1;
  charVals.notifySet = 1;
  charVals.dataSize = sizeof(uint32_t); // four bytes long
  charVals.UUID = commandOutputUUID;
  charVals.charIter = 2;

  return custom_value_char_add(p_cus, p_cus_init, &charVals);
}

uint32_t ble_cus_custom_value_update(ble_cus_t *p_cus, uint32_t custom_value) {
  NRF_LOG_INFO("In ble_cus_custom_value_update. \r\n");
  if (p_cus == NULL) {
    return NRF_ERROR_NULL;
  }

  uint32_t err_code = NRF_SUCCESS;
  ble_gatts_value_t gatts_value;

  // Initialize value struct.
  memset(&gatts_value, 0, sizeof(gatts_value));

  gatts_value.len = sizeof(uint32_t);
  gatts_value.offset = 0;
  gatts_value.p_value = (uint8_t*)&custom_value;

  // Update database.
  err_code = sd_ble_gatts_value_set(p_cus->conn_handle,
      p_cus->custom_value_handles[commandOutputSlot].value_handle,
      &gatts_value);
  if (err_code != NRF_SUCCESS) {
    return err_code;
  }

  // Send value if connected and notifying.
  if ((p_cus->conn_handle != BLE_CONN_HANDLE_INVALID)) {
    ble_gatts_hvx_params_t hvx_params;

    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = p_cus->custom_value_handles[commandOutputSlot].value_handle;
    hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.offset = gatts_value.offset;
    hvx_params.p_len = &gatts_value.len;
    hvx_params.p_data = gatts_value.p_value;

    err_code = sd_ble_gatts_hvx(p_cus->conn_handle, &hvx_params);
    NRF_LOG_INFO("sd_ble_gatts_hvx result: %x. \r\n", err_code);
  } else {
    err_code = NRF_ERROR_INVALID_STATE;
    NRF_LOG_INFO("sd_ble_gatts_hvx result: NRF_ERROR_INVALID_STATE. \r\n");
  }

  return err_code;
}