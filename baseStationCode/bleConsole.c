#include "bleConsole.h"

TaskHandle_t consolelinkHandle = NULL;
static const char* prompt = "central Device Console >";

static int scanCommandHandler(int argc, char **argv);
static int checkScanListCommandHandler(int argc, char **argv);
static int linkCommandHandler(int argc, char **argv);
static int checkCommandHandler(int argc, char **argv);
static int checkLinkedListCommandHandler(int argc, char **argv);
static int removeDeviceCommandHandler(int argc, char **argv);
static int forceUpdateCommandHandler(int argc, char **argv);
static int seteTriggerCommandHandler(int argc, char **argv);

static esp_console_cmd_t scanCommand = {
	
	.command = "scanArea",
	.help = "scan up to listed number of devices in the area which are not linked",
	.func = scanCommandHandler,
	
	
};

static esp_console_cmd_t checkScanList = {
	
	.command = "checkScanList",
	.help = "check current list of scanned devices to connect too",
	.func = checkScanListCommandHandler,
	
	
};

static esp_console_cmd_t linkCommand = {
	
	.command = "linkDevice",
	.help = "link to given device id",
	.func = linkCommandHandler,
	
	
};

static esp_console_cmd_t checkLinkedList = {
	
	.command = "checkLinkedList",
	.help = "check currently linked devices and their status",
	.func = checkLinkedListCommandHandler,
	
	
};

static esp_console_cmd_t checkCommand = {
	
	.command = "checkDevice",
	.help = "check current status on given device id",
	.func = checkCommandHandler,
	
	
};

static esp_console_cmd_t removeDevice = {
	
	.command = "removeDevice",
	.help = "remove linked device by given ID",
	.func = removeDeviceCommandHandler,
	
	
};

static esp_console_cmd_t setTriggerCommand = {
	
	.command = "setDeviceTrigger",
	.help = "set given water trigger threshold level for given device id| deviceID, newTriggerVal",
	.func = seteTriggerCommandHandler,
	
	
};

static esp_console_cmd_t forceUpdate = {
	
	.command = "forceUpdate",
	.help = "force update of given linked device id",
	.func = forceUpdateCommandHandler,
	
	
};



static int scanCommandHandler(int argc, char **argv){
	int retVal = ESP_OK;
	size_t possibleSensorsToList = 0;
	
	ESP_LOGI(consoleLogs, "scan command given");
	if(argc > 1){
		
		possibleSensorsToList = (size_t)atoi(argv[1]);
		printf("size of sensor list: %d\n", (int)possibleSensorsToList);
	}
	
	
	retVal = beginScan(possibleSensorsToList);
	
	return retVal;
}

static int checkScanListCommandHandler(int argc, char **argv){
	
	ESP_LOGI(consoleLogs, "check scan list command given");
	
	printScanList();
	
	return ESP_OK;
}

static int linkCommandHandler(int argc, char **argv){
	int retVal = ESP_OK;
	size_t deviceToLink = 0;
	
	ESP_LOGI(consoleLogs,"link command given");
	
	if(argc > 1){
		
		deviceToLink = atoi(argv[1]);
		printf("sensor to link to: %d\n", (int)deviceToLink);
		addToLinkDeviceList(deviceToLink);
		
	}else{
		
		printf("need device to link to number\n");
		retVal = 1;
		
	}
	
	
	
	return retVal;
}

static int checkCommandHandler(int argc, char **argv){
	int retVal = ESP_OK;
	size_t deviceToCheck = 0;
	
	ESP_LOGI(consoleLogs,"check command given");
	
	if(argc > 1){
		
		deviceToCheck = atoi(argv[1]);
		printf("sensor to check is: %d\n", (int)deviceToCheck);
		if(!printLinkedDevice(deviceToCheck)){
			
			retVal = 1;
			
		}
		
	}else{
		
		printf("need device to check number\n");
		retVal = 1;
		
	}
	
	
	
	return retVal;
}

static int checkLinkedListCommandHandler(int argc, char **argv){
	ESP_LOGI(consoleLogs, "checkLinkedList command given");
	
	printLinkedDeviceList();
	
	return ESP_OK;
}

static int removeDeviceCommandHandler(int argc, char **argv){
	int retVal = ESP_OK;
	size_t deviceToRemove = 0;
	
	ESP_LOGI(consoleLogs,"removeDevice command given");
	
	if(argc > 1){
		
		deviceToRemove = atoi(argv[1]);
		printf("sensor to remove: %d\n", (int)deviceToRemove);
		removeSensorFromList(deviceToRemove);
		
	}else{
		
		printf("need device to remove number\n");
		retVal = 1;
		
	}
	
	
	
	return retVal;
}

static int seteTriggerCommandHandler(int argc, char **argv){
	int retVal = ESP_OK;
	size_t deviceToUpdateTrigger = 0;
	uint8_t newTriggerVal = 0;
	
	ESP_LOGI(consoleLogs,"setTrigger command given");
	
	if(argc > 2){
		
		deviceToUpdateTrigger = atoi(argv[1]);
		newTriggerVal = atoi(argv[2]);
		printf("sensor to update: %d| new trigger value: %d\n", (int)deviceToUpdateTrigger, (int)newTriggerVal);
		
		if(!updateTriggerValue(deviceToUpdateTrigger, newTriggerVal)){
			
			retVal = 1;
			
		}
		
	}else{
		
		printf("need device id number and new trigger number\n");
		retVal = 1;
		
	}
	
	
	
	return retVal;
}

static int forceUpdateCommandHandler(int argc, char **argv){
	int retVal = ESP_OK;
	size_t deviceToUpdate = 0;
	
	ESP_LOGI(consoleLogs,"forceUpdate command given");
	
	if(argc > 1){
		
		deviceToUpdate = atoi(argv[1]);
		printf("sensor to update: %d\n", (int)deviceToUpdate);
		forceSoilUpdateCheck(deviceToUpdate);
		
	}else{
		
		printf("need device to update number\n");
		retVal = 1;
		
	}
	
	
	
	return retVal;
}

void initConsole(){
	
	/* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
#if SOC_UART_SUPPORT_REF_TICK
        .source_clk = UART_SCLK_REF_TICK,
#elif SOC_UART_SUPPORT_XTAL_CLK
        .source_clk = UART_SCLK_XTAL,
#endif
    };
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM,
            256, 0, 0, NULL, 0) );
    ESP_ERROR_CHECK( uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_args = 8,
            .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
            .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

    /* Set command maximum length */
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);

    /* Don't return empty lines */
    linenoiseAllowEmpty(false);

#if CONFIG_STORE_HISTORY
    /* Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
#endif

printf("\n Type 'help' to get the list of commands.\n");

	ESP_ERROR_CHECK(esp_console_cmd_register(&scanCommand));
	ESP_ERROR_CHECK(esp_console_cmd_register(&checkScanList));
	ESP_ERROR_CHECK(esp_console_cmd_register(&linkCommand));
	ESP_ERROR_CHECK(esp_console_cmd_register(&checkCommand));
	ESP_ERROR_CHECK(esp_console_cmd_register(&setTriggerCommand));
	ESP_ERROR_CHECK(esp_console_cmd_register(&checkLinkedList));
	ESP_ERROR_CHECK(esp_console_cmd_register(&removeDevice));
	ESP_ERROR_CHECK(esp_console_cmd_register(&forceUpdate));
	ESP_ERROR_CHECK(esp_console_register_help_command());
	ESP_LOGI(consoleLogs, "completed console Initialize");
	
}

void consoleLoop(){
	char* givenLine = NULL;
	int ret = 0;
	
	ESP_LOGI(consoleLogs, "main console loop entered");
	
	while(true){
		
		if(workingCommandHalt){
			
			givenLine = linenoise(prompt);
			if (givenLine == NULL) { /* Break on EOF or error */
				break;
			}
		
			esp_err_t err = esp_console_run(givenLine, &ret);
			if (err == ESP_ERR_NOT_FOUND) {
				printf("Unrecognized command\n");
			} else if (err == ESP_ERR_INVALID_ARG) {
				// command was empty
			} else if (err == ESP_OK && ret != ESP_OK) {
				printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
			} else if (err != ESP_OK) {
				printf("Internal error: %s\n", esp_err_to_name(err));
			}
		
			linenoiseFree(givenLine);
		}
		
		
		vTaskDelay(600 / portTICK_RATE_MS);
		
	}


	esp_console_deinit();
	vTaskDelete(NULL);
}