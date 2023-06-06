/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */



/****************************************************************************
*
* This demo showcases BLE GATT client. It can scan BLE devices and connect to one device.
* Run the gatt_server demo, the client demo will automatically connect to the gatt_server demo.
* Client demo will enable gatt_server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/
#include "bleClientEsp32.h"

#define scanSensorNameIter 4

struct freeLinkedSensorStack *freeDeviceList = NULL;

struct deviceInfo *currentDevice = NULL;

static uint32_t scannedListIter = 0;
static struct scannedDeviceInfoArrary *scannedList = NULL;

struct deviceInfo *linkedDevices = NULL;
size_t linkedDeviceListSize = 0;

uint32_t currentSensorUID = unlinkedSensorID;

static struct bleSensorEvents *bleSensorStore = NULL;

static bool currentlyScanning = false;
static bool connectingScanning = false;

static const char remote_device_name[] = "bluetoothGWater-";
static bool connect    = false;
static bool get_server = false;
static esp_gattc_char_elem_t *char_elem_result   = NULL;
static esp_gattc_descr_elem_t *descr_elem_result = NULL;


static uint8_t startADCvalue = 0;
static uint16_t ADCValueSize = sizeof(uint8_t);

static uint8_t WateringCommandValue = 1;
static uint16_t WateringCommandValueSize = sizeof(uint8_t);

static bool justAddingDevice = false;

static bool updateingSoilWrite = false;


/* Declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
static void sensorBLEhandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);


static esp_bt_uuid_t remote_filter_service_uuid = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = {0xBC, 0x8A, 0xBF, 0x45, 0xCA, 0x05, 0x50, 0xBA, 0x40, 0x42, 0xB0, 0x00, 0x00, 0x14, 0x64, 0xF3},},
};

#define deviceIDCharIndex 0
static esp_bt_uuid_t deviceIDChar = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = {0xBC, 0x8A, 0xBF, 0x45, 0xCA, 0x05, 0x50, 0xBA, 0x40, 0x42, 0xB0, 0x00, 0x01, 0x14, 0x64, 0xF3},},
};

#define commandCharIndex 1
static esp_bt_uuid_t commandChar = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = {0xBC, 0x8A, 0xBF, 0x45, 0xCA, 0x05, 0x50, 0xBA, 0x40, 0x42, 0xB0, 0x00, 0x02, 0x14, 0x64, 0xF3},},
};

#define commandOutputCharIndex 2
static esp_bt_uuid_t commandOutputChar = {
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = {0xBC, 0x8A, 0xBF, 0x45, 0xCA, 0x05, 0x50, 0xBA, 0x40, 0x42, 0xB0, 0x00, 0x03, 0x14, 0x64, 0xF3},},
};


static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};


struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle[3];
    esp_bd_addr_t remote_bda;
};

/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = sensorBLEhandler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

static void getConnectedChars(esp_gatt_if_t gattClientDevice,esp_ble_gattc_cb_param_t *p_data, esp_gattc_char_elem_t *charStore,esp_bt_uuid_t charUUIDselect, size_t charIndex,uint16_t *count){
	
	esp_gatt_status_t retVal = ESP_GATT_OK;
	
	retVal = esp_ble_gattc_get_char_by_uuid( gattClientDevice,
                                                             p_data->search_cmpl.conn_id,
                                                             gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                             gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                             charUUIDselect,
                                                             &charStore[charIndex],
                                                             count);
                    if (retVal != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
                    }
					
					
	
}

static void sensorBLEhandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(GATTC_TAG, "REG_EVT");
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
        }
        break;
    case ESP_GATTC_CONNECT_EVT:{
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
        memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
        esp_log_buffer_hex(GATTC_TAG, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
        if (mtu_ret){
            ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
        }
        break;
    }
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "open failed, status %d", p_data->open.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "open success");
        break;
    case ESP_GATTC_DIS_SRVC_CMPL_EVT:
        if (param->dis_srvc_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "discover service failed, status %d", param->dis_srvc_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "discover service complete conn_id %d", param->dis_srvc_cmpl.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_128) {
			
			if(!memcmp( p_data->search_res.srvc_id.uuid.uuid.uuid128, remote_filter_service_uuid.uuid.uuid128, ESP_UUID_LEN_128) ){
			
				ESP_LOGI(GATTC_TAG, "service found");
				get_server = true;
				gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
				gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
				ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
			}
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        if(p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
            ESP_LOGI(GATTC_TAG, "Get service information from remote device");
        } else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
            ESP_LOGI(GATTC_TAG, "Get service information from flash");
        } else {
            ESP_LOGI(GATTC_TAG, "unknown service source");
        }
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SEARCH_CMPL_EVT");
        if (get_server){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }
			
			ESP_LOGI(GATTC_TAG, "Char Count: %d", count);
			
            if (count > 0){
                char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
                if (!char_elem_result){
                    ESP_LOGE(GATTC_TAG, "gattc no mem");
                }else{
					getConnectedChars(gattc_if,p_data, char_elem_result, deviceIDChar, deviceIDCharIndex, &count);
					
					getConnectedChars(gattc_if,p_data,char_elem_result, commandChar, commandCharIndex, &count);
					
					getConnectedChars(gattc_if,p_data,char_elem_result, commandOutputChar, commandOutputCharIndex, &count);

                    /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
                    if (count > 0 && (char_elem_result[commandOutputCharIndex].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
                        gl_profile_tab[PROFILE_A_APP_ID].char_handle[deviceIDCharIndex] = char_elem_result[deviceIDCharIndex].char_handle;
						gl_profile_tab[PROFILE_A_APP_ID].char_handle[commandCharIndex] = char_elem_result[commandCharIndex].char_handle;
						gl_profile_tab[PROFILE_A_APP_ID].char_handle[commandOutputCharIndex] = char_elem_result[commandOutputCharIndex].char_handle;
                        esp_ble_gattc_register_for_notify (gattc_if, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, char_elem_result[commandOutputCharIndex].char_handle);
						ESP_LOGI(GATTC_TAG, "looking to register handle %d", (int)char_elem_result[commandOutputCharIndex].char_handle);
                    }
                }
                /* free char_elem_result */
                free(char_elem_result);
            }else{
                ESP_LOGE(GATTC_TAG, "no char found");
            }
        }
         break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
        }else{
            uint16_t count = 0;
            uint16_t notify_en = 1;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                         ESP_GATT_DB_DESCRIPTOR,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].char_handle[commandOutputCharIndex],
                                                                         &count);
            if (ret_status != ESP_GATT_OK){
                ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
            }
            if (count > 0){
                descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
                if (!descr_elem_result){
                    ESP_LOGE(GATTC_TAG, "malloc error, gattc no mem");
                }else{
                    ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                         gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                         p_data->reg_for_notify.handle,
                                                                         notify_descr_uuid,
                                                                         descr_elem_result,
                                                                         &count);
                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_descr_by_char_handle error");
                    }
                    /* Every char has only one descriptor in our 'ESP_GATTS_DEMO' demo, so we used first 'descr_elem_result' */
                    if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG){
                        ret_status = esp_ble_gattc_write_char_descr( gattc_if,
                                                                     gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                     descr_elem_result[0].handle,
                                                                     sizeof(notify_en),
                                                                     (uint8_t *)&notify_en,
                                                                     ESP_GATT_WRITE_TYPE_RSP,
                                                                     ESP_GATT_AUTH_REQ_NONE);
                    }

                    if (ret_status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_TAG, "esp_ble_gattc_write_char_descr error");
                    }

                    /* free descr_elem_result */
                    free(descr_elem_result);
                }
            }
            else{
                ESP_LOGE(GATTC_TAG, "decsr not found");
            }

        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT:
        if (p_data->notify.is_notify){
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
        }else{
            ESP_LOGI(GATTC_TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
        }
        esp_log_buffer_hex(GATTC_TAG, p_data->notify.value, p_data->notify.value_len);
		
		currentDevice->moisturePercentage = *p_data->notify.value;
		
		currentDevice->moisturePercentage = 100 - currentDevice->moisturePercentage;
		
		if(currentDevice->moisturePercentage < currentDevice->triggerVal){
			
			bleSensorStore->connectedSensorHandler(currentDevice, triggerLevelAlert);
			
		}else{
		
			bleSensorStore->connectedSensorHandler(currentDevice, updateSensorSoil);
		
		}
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write descr failed, error status = %x", p_data->write.status);
			
            break;
        }
		
		if(justAddingDevice){
				
				ESP_LOGI(GATTC_TAG, "writing new device ID of: %d", (int)currentDevice->sensorUID);
				
				esp_ble_gattc_write_char(gattc_if,  gl_profile_tab[PROFILE_A_APP_ID].conn_id,
						gl_profile_tab[PROFILE_A_APP_ID].char_handle[deviceIDCharIndex], 
						sizeof(uint32_t),(uint8_t *)&currentDevice->sensorUID, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE );
		
				
		}else{
				
			ESP_LOGI(GATTC_TAG, "normal mode char descriptor wrote");
			bleSensorStore->connectedSensorHandler(currentDevice, connectedToDevice);
		}
		
        ESP_LOGI(GATTC_TAG, "write descr success ");
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
        esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(GATTC_TAG, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "write char success ");
		
		if(justAddingDevice){
			
			bleSensorStore->foundUnlinkedSensor(linkedSensor);
			
		}else{
			
			if(updateingSoilWrite){
				
				bleSensorStore->connectedSensorHandler(currentDevice, wroteData);
				updateingSoilWrite = false;
			}else{
				
				bleSensorStore->connectedSensorHandler(currentDevice, updatedAdvCycle);
				
			}
			
			
		}
		
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        connect = false;
        get_server = false;
        ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = %d", p_data->disconnect.reason);
		if(p_data->disconnect.reason != 22){
			
			bleSensorStore->connectedSensorHandler(currentDevice, disconnectedDevice);
			
		}
		currentDevice = NULL;
		justAddingDevice = false;
        break;
    default:
        break;
    }
}

static bool lookForName(esp_ble_gap_cb_param_t *input, uint8_t advLength,uint8_t scanLength, uint32_t uid){
	
	
	int compLength = 0;
	bool foundDevice = false;
	uint32_t deviceUID = 0;
	
	if(scanLength){
		if(!(compLength = memcmp(remote_device_name,(input->scan_rst.ble_adv + advLength + scanSensorNameIter), sizeof(remote_device_name) - 1))){
		
			deviceUID = *(uint32_t*)(input->scan_rst.ble_adv + (advLength + scanSensorNameIter + (sizeof(remote_device_name) - 1)));
			ESP_LOGI(GATTC_TAG, "scaned device uid: %d", (int)deviceUID);
			if(deviceUID == uid){
			
				foundDevice = true;
			
			}else{
			
				ESP_LOGI(GATTC_TAG, "sensor uid is diffrent| needed: %d | got: %d", (int)uid, (int)deviceUID);
			
			}
		}else{
			
			ESP_LOGI(GATTC_TAG, "device name differs by %d", compLength);
			esp_log_buffer_hex(GATTC_TAG, (input->scan_rst.ble_adv + input->scan_rst.adv_data_len + scanSensorNameIter), scanLength);

		}
	
	}
	
	
	return foundDevice;
}

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
   // uint8_t *adv_name = NULL;
	
   // uint8_t adv_name_len = 0;
	
	bool diffrentDevice = true;
	
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        ESP_LOGI(GATTC_TAG, "set scam param complete ");
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "scan start success");

        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
			//ESP_LOGI(GATTC_TAG, "New scan result");
           // esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
           // ESP_LOGI(GATTC_TAG, "searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            //adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                              //  ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
           // ESP_LOGI(GATTC_TAG, "searched Device Name Len %d", adv_name_len);
           // esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
			
#if CONFIG_EXAMPLE_DUMP_ADV_DATA_AND_SCAN_RESP
            if (scan_result->scan_rst.adv_data_len > 0) {
           //     ESP_LOGI(GATTC_TAG, "adv data:");
           //     esp_log_buffer_hex(GATTC_TAG, &scan_result->scan_rst.ble_adv[0], scan_result->scan_rst.adv_data_len);
            }
            if (scan_result->scan_rst.scan_rsp_len > 0) {
           //     ESP_LOGI(GATTC_TAG, "scan resp:");
           //     esp_log_buffer_hex(GATTC_TAG, &scan_result->scan_rst.ble_adv[scan_result->scan_rst.adv_data_len], scan_result->scan_rst.scan_rsp_len);
            }
#endif
           // ESP_LOGI(GATTC_TAG, "\n");
			
			if(connectingScanning){
				
				
				if(lookForName(scan_result, scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len, currentSensorUID)){
				
			
					ESP_LOGI(GATTC_TAG, "searched device %s\n", remote_device_name);
					if(connect == false) {
						connect = true;
						ESP_LOGI(GATTC_TAG, "connect to the remote device.");
						bleSensorStore->eventType = attemptingToConnect;
						esp_ble_gap_stop_scanning();
						esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
					}else{
						
						ESP_LOGI(GATTC_TAG, "already connected to device");
							
					}
				
			}
			}else{
				
				
				
				if(lookForName(scan_result, scan_result->scan_rst.adv_data_len,scan_result->scan_rst.scan_rsp_len, currentSensorUID)){
					bleSensorStore->eventType = foundUnlinkedSensor;
					
					for(int i = 0; i < scannedListIter; i++){
						
						if(!memcmp(scan_result->scan_rst.bda, &scannedList[i].deviceAddr, 6)){
							ESP_LOGI(GATTC_TAG, " address already stored");
							diffrentDevice = false;
							break;
							
						}else{
							
							ESP_LOGI(GATTC_TAG, "found free slot");
							
						}
						
					}
					
					if(diffrentDevice){
						ESP_LOGI(GATTC_TAG, "found unlinked scanner ");
						scannedList[0].arraySize = (scannedListIter + 1) ;
						memcpy(&scannedList[scannedListIter].deviceAddr, scan_result->scan_rst.bda, 6);
						scannedList[scannedListIter].remoteAddrType = scan_result->scan_rst.ble_addr_type;
					
						ESP_LOGI(GATTC_TAG, "array spot added: %d", (int)scannedListIter);
						ESP_LOGI(GATTC_TAG, "total array size now: %d", (int)scannedList[0].arraySize);
						ESP_LOGI(GATTC_TAG, "device addr type: %d", (int)scannedList[scannedListIter].remoteAddrType);
						ESP_LOGI(GATTC_TAG, "device addr:");
						esp_log_buffer_hex(GATTC_TAG, &scannedList[scannedListIter].deviceAddr, 6);
					
						if(BLE_ADDR_TYPE_PUBLIC == scannedList[scannedListIter].remoteAddrType){
						
							ESP_LOGI(GATTC_TAG, "device has public addr type");
						
						}
					
						scannedListIter++;
						if(scannedListIter >= scannedList[0].maxSize){
							esp_ble_gap_stop_scanning();
						
						}
					}else{
						diffrentDevice = true;
						
					}
					
				
				}
				
			}
			

            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
			currentlyScanning = false;
			scannedListIter = 0;
			bleSensorStore->foundUnlinkedSensor(bleSensorStore->eventType);
			bleSensorStore->eventType = noDeviceEvent;
			ESP_LOGI(GATTC_TAG, "stop time ended");
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop scan successfully");
		currentlyScanning = false;
		scannedListIter = 0;
		bleSensorStore->foundUnlinkedSensor(bleSensorStore->eventType);
		bleSensorStore->eventType = noDeviceEvent;
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(GATTC_TAG, "adv stop failed, error status = %x", param->adv_stop_cmpl.status);
            break;
        }
        ESP_LOGI(GATTC_TAG, "stop adv successfully");
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gl_profile_tab[idx].gattc_if) {
                if (gl_profile_tab[idx].gattc_cb) {
                    gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}


bool isCurrentlyScanning(){
	
	return currentlyScanning;
	
}

bool scanForDevice(uint32_t deviceUID, uint32_t scanningTime){
	
	currentSensorUID = deviceUID;
	
	if(currentlyScanning){
		ESP_LOGE(GATTC_TAG, "scanForDevices() - already scanning can not scan untill current one is complete");
		return false;
		
	}
	
	currentlyScanning = true;
	ESP_LOGI(GATTC_TAG, "looking for sensor ID of: %d ", (int)currentSensorUID);
    esp_ble_gap_start_scanning(scanningTime);
	
	return true;
}

bool scanForDevices(struct scannedDeviceInfoArrary *array, uint32_t scanningTime){
	
	ESP_LOGI(GATTC_TAG, "began scanning with time of: %d ", (int)scanningTime);
	
	if(!array){
		ESP_LOGI(GATTC_TAG, "scanForDevices() - headNode input is NULL");
		return false;
		
	}
	connectingScanning = false;
	bleSensorStore->eventType = foundNothing;
	scannedList = array;
	return scanForDevice(unlinkedSensorID, scanningTime);;
	
}


bool updateSoilContent(){
	bool retval = false;
	esp_err_t status = ESP_GATT_OK;
	
	
	if(get_server){
		updateingSoilWrite = true;
		status = esp_ble_gattc_write_char(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,  gl_profile_tab[PROFILE_A_APP_ID].conn_id,
						gl_profile_tab[PROFILE_A_APP_ID].char_handle[commandCharIndex], 
						ADCValueSize,&startADCvalue, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE );
		
		if(status != ESP_GATT_OK){
			
			ESP_LOGE(GATTC_TAG, "write error to commandChar: %d", (int)status);
			
		}else{
			
			retval = true;
			
		}
		
	}
	
	
	return retval;
}

bool turnOnWateringDeviceWrite(){
	
	bool retval = false;
	esp_err_t status = ESP_GATT_OK;
	
	ESP_LOGI(GATTC_TAG, "changing water device state");
	if(get_server){
		updateingSoilWrite = true;
		status = esp_ble_gattc_write_char(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,  gl_profile_tab[PROFILE_A_APP_ID].conn_id,
						gl_profile_tab[PROFILE_A_APP_ID].char_handle[commandCharIndex], 
						WateringCommandValueSize,&WateringCommandValue, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE );
		
		if(status != ESP_GATT_OK){
			
			ESP_LOGE(GATTC_TAG, "write error to commandChar: %d", (int)status);
			
		}else{
			
			retval = true;
			
		}
		
	}
	
	
	return retval;
	
	
}


bool updateAdvCycle(){
	bool retval = false;
	esp_err_t status = ESP_OK;
	
	ESP_LOGI(GATTC_TAG, "updating device Adv Status");
	if(get_server){
		
		if(currentDevice->nextAdvCycle > 0){
		
			status = esp_ble_gattc_write_char(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,  gl_profile_tab[PROFILE_A_APP_ID].conn_id,
							gl_profile_tab[PROFILE_A_APP_ID].char_handle[commandCharIndex], 
							sizeof(uint32_t),(uint8_t *)&currentDevice->nextAdvCycle, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE );
		
			if(status != ESP_GATT_OK){
			
				ESP_LOGE(GATTC_TAG, "write error to commandChar: %d", (int)status);
			
			}else{
			
				retval = true;
			
			}
		
		}
		
	}
	
	return retval;
}

bool disconnectCurrentDevice(){
	
	bool retval = false;
	esp_err_t status = ESP_OK;
	
	if(!get_server){
		
		retval = false;
		
	}else{
		
		
		status = esp_ble_gap_disconnect(currentDevice->deviceAddr);
	
		if(status != ESP_OK){
		
			ESP_LOGE(GATTC_TAG, "disconnect gap error: %d", (int)status);
		
		}else{
			retval = true;
		
		}
	
	}
	
	return retval;
	
}


bool tryConnectToLinkedDevice(size_t linkedDeviceIndex){
	bool retVal = false;
	
	if(!get_server){
		
		ESP_LOGI(GATTC_TAG, "device to connect ID: %d", (int)linkedDevices[linkedDeviceIndex].sensorUID);
		ESP_LOGI(GATTC_TAG, "device to connect addres:");
		esp_log_buffer_hex(GATTC_TAG, linkedDevices[linkedDeviceIndex].deviceAddr, 6);
		
		currentDevice = &linkedDevices[linkedDeviceIndex];
		
		if(currentDevice->isUsed){
			
			
		
			bleSensorStore->eventType = failedToScanLink;
			connectingScanning = true;
			if(!(retVal = scanForDevice(currentDevice->sensorUID, defaultScanTime))){
			
				ESP_LOGE(GATTC_TAG, "problem with scanForDevice()");
			
			}
			
		}
		
	}
	
	return retVal;
}


bool addDeviceToList(struct scannedDeviceInfoArrary * deviceToAdd){
	bool retVal = true;
	struct freeLinkedSensorStack *curretStack = freeDeviceList;
	size_t spotToAdd = curretStack->freelinkedDeviceSpot;
	
	if(spotToAdd < linkedDeviceListSize){
		
		memcpy(linkedDevices[spotToAdd].deviceAddr, deviceToAdd->deviceAddr, 6);
		linkedDevices[spotToAdd].remoteAddrType = deviceToAdd->remoteAddrType;
		linkedDevices[spotToAdd].sensorUID = spotToAdd;
		linkedDevices[spotToAdd].isUsed = true;
		linkedDevices[spotToAdd].triggerVal = defaultTriggerLevel;
		
		currentDevice = &linkedDevices[spotToAdd];
	
		justAddingDevice = true;
	
		esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, deviceToAdd->deviceAddr, deviceToAdd->remoteAddrType, true);
	
		if(curretStack->prevFreeSpot){
		
			freeDeviceList = curretStack->prevFreeSpot;
			free(curretStack);
		
		}else{
		
			curretStack->freelinkedDeviceSpot++;
		}
		
	}else{
		
		retVal = false;
		
	}
	
	
	return retVal;
}


void removeDeviceFromList(size_t linkedDeviceIter){
	struct freeLinkedSensorStack *curretStack = freeDeviceList;
	
	if(linkedDeviceIter == (curretStack->freelinkedDeviceSpot - 1)){
		
		curretStack->freelinkedDeviceSpot -= 1;
		memset(&linkedDevices[curretStack->freelinkedDeviceSpot], 0, sizeof(struct deviceInfo));
		
	}else{
		
		memset(&linkedDevices[linkedDeviceIter], 0, sizeof(struct deviceInfo));
		freeDeviceList = calloc(1,sizeof(struct freeLinkedSensorStack));
		
		if(!freeDeviceList){
			
			ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
			
		}
		freeDeviceList->freelinkedDeviceSpot = linkedDeviceIter;
		freeDeviceList->prevFreeSpot = curretStack;
	}
	
}

bool initESPBle(struct bleSensorEvents *sensorEventStore)
{
	
	if(!sensorEventStore){
		
		ESP_LOGI(GATTC_TAG, "initESPBle() - passed null value for input: *sensorEventStore" );
		return false;
		
	}
	
	if(!sensorEventStore->generalEventHandler){
		
		ESP_LOGI(GATTC_TAG, "initESPBle() - passed null value for input: generalEventHandler" );
		return false;
		
	}
	
	if(!sensorEventStore->foundUnlinkedSensor){
		
		ESP_LOGI(GATTC_TAG, "initESPBle() - passed null value for input: foundUnlinkedSensor" );
		return false;
		
	}
	
	if(!sensorEventStore->connectedSensorHandler){
		
		ESP_LOGI(GATTC_TAG, "initESPBle() - passed null value for input: connectedSensorHandler" );
		return false;
		
	}
	
	bleSensorStore = sensorEventStore;
	
	freeDeviceList = calloc(1, sizeof(struct freeLinkedSensorStack));
	
	if(!freeDeviceList){
		
		ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
		
	}
	
	linkedDeviceListSize = maxLinkedSensors;
	
	linkedDevices = calloc(linkedDeviceListSize, sizeof(struct deviceInfo));
	
	if(!linkedDevices){
		
		ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
		
	}
	
    // Initialize NVS.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return false;
    }

    //register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
        return false;
    }

    //register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(esp_gattc_cb);
    if(ret){
        ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
        return false;
    }

    ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
    if (ret){
        ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
		return false;
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(26);
    if (local_mtu_ret){
        ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
		return false;
    }

	return true;
}
