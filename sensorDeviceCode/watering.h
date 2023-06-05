#pragma once

#include "app_error.h"
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include <stdbool.h>
#include <stdint.h>

#define waterStatePin 25

void changeWateringState();