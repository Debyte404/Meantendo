/**
 * @file input.c
 * @brief Meantendo Input System - Implementation
 * @author Debyte
 * @version 2.0.0 (ESP-IDF Native)
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

#include "input.h"

static const char *TAG = "INPUT";

// ============================================================================
//  Pin Mappings
// ============================================================================

static const gpio_num_t s_button_pins[BTN_COUNT] = {
    [BTN_SELECT] = INPUT_BTN_SELECT,
    [BTN_A]      = INPUT_BTN_A,
    [BTN_B]      = INPUT_BTN_B,
    [BTN_X]      = INPUT_BTN_X,
    [BTN_Y]      = INPUT_BTN_Y,
    [BTN_BACK]   = INPUT_BTN_BACK,
};

// ============================================================================
//  Static Variables
// ============================================================================

static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static bool s_initialized = false;

// Button state tracking
static bool s_button_state[BTN_COUNT] = {false};
static bool s_button_prev[BTN_COUNT] = {false};
static uint32_t s_button_debounce_time[BTN_COUNT] = {0};

// Joystick state tracking
static input_direction_t s_last_direction = INPUT_DIR_NONE;

// ============================================================================
//  Initialization
// ============================================================================

esp_err_t input_init(void) {
    if (s_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing input system...");
    
    // Configure button GPIOs
    for (int i = 0; i < BTN_COUNT; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << s_button_pins[i]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&io_conf);
    }
    
    // Configure ADC for joystick
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &s_adc_handle));
    
    // Configure ADC channels for joystick X and Y
    adc_oneshot_chan_cfg_t chan_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    
    // GPIO34 = ADC1_CH6, GPIO35 = ADC1_CH7
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, ADC_CHANNEL_6, &chan_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, ADC_CHANNEL_7, &chan_config));
    
    s_initialized = true;
    ESP_LOGI(TAG, "Input system initialized");
    
    return ESP_OK;
}

void input_deinit(void) {
    if (s_adc_handle) {
        adc_oneshot_del_unit(s_adc_handle);
        s_adc_handle = NULL;
    }
    s_initialized = false;
}

// ============================================================================
//  Joystick Reading
// ============================================================================

int input_get_joystick_x(void) {
    int value = 0;
    if (s_adc_handle) {
        adc_oneshot_read(s_adc_handle, ADC_CHANNEL_6, &value);
    }
    return value;
}

int input_get_joystick_y(void) {
    int value = 0;
    if (s_adc_handle) {
        adc_oneshot_read(s_adc_handle, ADC_CHANNEL_7, &value);
    }
    return value;
}

input_direction_t input_read_joystick(void) {
    int x = input_get_joystick_x();
    int y = input_get_joystick_y();
    
    // Check Y axis first (up/down takes priority)
    if (y < INPUT_JOY_CENTER - INPUT_JOY_DEADZONE) {
        return INPUT_DIR_UP;
    }
    if (y > INPUT_JOY_CENTER + INPUT_JOY_DEADZONE) {
        return INPUT_DIR_DOWN;
    }
    
    // Check X axis
    if (x < INPUT_JOY_CENTER - INPUT_JOY_DEADZONE) {
        return INPUT_DIR_LEFT;
    }
    if (x > INPUT_JOY_CENTER + INPUT_JOY_DEADZONE) {
        return INPUT_DIR_RIGHT;
    }
    
    return INPUT_DIR_NONE;
}

input_direction_t input_read_joystick_change(void) {
    input_direction_t current = input_read_joystick();
    
    if (current != s_last_direction && current != INPUT_DIR_NONE) {
        s_last_direction = current;
        return current;
    }
    
    if (current == INPUT_DIR_NONE) {
        s_last_direction = INPUT_DIR_NONE;
    }
    
    return INPUT_DIR_NONE;
}

// ============================================================================
//  Button Reading
// ============================================================================

bool input_button_pressed(input_button_t button) {
    if (button >= BTN_COUNT) return false;
    return gpio_get_level(s_button_pins[button]) == 0;  // Active low
}

bool input_button_just_pressed(input_button_t button) {
    if (button >= BTN_COUNT) return false;
    return s_button_state[button] && !s_button_prev[button];
}

bool input_button_just_released(input_button_t button) {
    if (button >= BTN_COUNT) return false;
    return !s_button_state[button] && s_button_prev[button];
}

void input_update(void) {
    // Update button states
    for (int i = 0; i < BTN_COUNT; i++) {
        s_button_prev[i] = s_button_state[i];
        s_button_state[i] = input_button_pressed((input_button_t)i);
    }
}

bool input_any_button_pressed(void) {
    for (int i = 0; i < BTN_COUNT; i++) {
        if (input_button_pressed((input_button_t)i)) {
            return true;
        }
    }
    return false;
}

bool input_button_debounced(input_button_t button) {
    if (button >= BTN_COUNT) return false;
    
    if (input_button_pressed(button)) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now - s_button_debounce_time[button] > INPUT_DEBOUNCE_MS) {
            s_button_debounce_time[button] = now;
            return true;
        }
    }
    return false;
}
