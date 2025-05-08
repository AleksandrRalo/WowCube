#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h" 
#include <stdio.h> 
#include "esp_log.h"
#include "chg.h"
#include "ble.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc_cal.h"
#include "esp_adc/adc_cali.h"
#include <esp_adc/adc_cali_scheme.h>

static const char *TAG = "example";

void adc_timer_callback(TimerHandle_t xTimer);

// Таймеры
TimerHandle_t adc_timer;


// Определяем используемые каналы ADC
#define ADC1_CHANNEL_0 ADC1_CHANNEL_0 // петля массы
#define ADC1_CHANNEL_1 ADC1_CHANNEL_1 // входное напряжение
#define ADC1_CHANNEL_2 ADC1_CHANNEL_2 // напряжение на гребне
#define ADC1_CHANNEL_3 ADC1_CHANNEL_3 // ток на гребне

static const float adc_coefficients[] = {
  1.204 / 958,   // Коэффициент для канала 0
  4.986 / 775,   // Коэффициент для канала 1 0.005449
  2.650 / 402,   // Коэффициент для канала 2
  1.000 / 507,   // Замените на реальный коэффициент для канала 3
};

// Определяем коэффициент усиления
#define DEFAULT_VREF 1100 // Опорное напряжение в мВ (по умолчанию)

// Дескриптор для ADC
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc_cali_channel_handle;
static adc_cali_handle_t cali_handle;


// Функция для инициализации ADC
static void init_adc(void)
{
  esp_err_t ret;
  // Конфигурация ADC
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT_1,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

  // Настройка каналов
  adc_oneshot_chan_cfg_t channel_config = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN_DB_12, // Усиление 0 дБ
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHANNEL_0, &channel_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHANNEL_1, &channel_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHANNEL_2, &channel_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHANNEL_3, &channel_config));


     // Конфигурация калибровки
     adc_cali_curve_fitting_config_t cali_config = {
      .unit_id = ADC_UNIT_1,
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
  };

  // Создаем схему калибровки Curve Fitting
  ret = adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);
  if (ret == ESP_OK) {
      ESP_LOGI(TAG, "Калибровка ADC успешно создана.");
  } else {
      ESP_LOGE(TAG, "Ошибка создания калибровочной схемы: %s", esp_err_to_name(ret));
      return;
  }

  // // Инициализация ADC каналов 1–4
  // for (int channel = 1; channel <= 4; channel++) {
  //     adc_gpio_init(ADC_UNIT_1, channel);
  //     ESP_LOGI(TAG, "Канал %d инициализирован.", channel);
  // }

  
}

// Функция для чтения значения ADC
static uint32_t read_adc(adc_channel_t channel)
{
  int raw_value = 0;
  ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &raw_value));

  int voltage = 0;
  ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw_value, &voltage));

  return voltage; // Возвращаем необработанное значение
}

void vTaskLed( void * pvParameters );

void app_main(void)
{
    TaskHandle_t xHandle = NULL;

    init_adc();

    // Создаем таймер для замеров ADC (каждую 1 секунду)
    adc_timer = xTimerCreate("ADC Timer", pdMS_TO_TICKS(2000), pdTRUE, NULL, adc_timer_callback);
    if (adc_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create ADC timer");
        return;
    }

    

    if (xTimerStart(adc_timer, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to start ADC timer");
    }

    xTaskCreate( vTaskLed, "TaskLed", 2048, 0, tskIDLE_PRIORITY, &xHandle );
    configASSERT( xHandle );

    // Основной таск ничего не делает, так как все выполняется через таймеры
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// Callback для таймера АЦП
void adc_timer_callback(TimerHandle_t xTimer)
{
    //char *log_buffer = (char *)malloc(128);
    //if (log_buffer == NULL) {
    //    ESP_LOGE(TAG, "Failed to allocate memory for log buffer");
    //    return;
    //}

    uint32_t voltage0 = read_adc(ADC1_CHANNEL_0);
    vTaskDelay(pdMS_TO_TICKS(10)); // Задержка 10 мс
/*    uint32_t voltage1 = read_adc(ADC1_CHANNEL_1);
    vTaskDelay(pdMS_TO_TICKS(10)); // Задержка 10 мс
    uint32_t voltage2 = read_adc(ADC1_CHANNEL_2);
    vTaskDelay(pdMS_TO_TICKS(10)); // Задержка 10 мс
    uint32_t voltage3 = read_adc(ADC1_CHANNEL_3);
    */

    ESP_LOGI(TAG, "ADC Channel 0: %lu mV", (unsigned long)voltage0);
/*    ESP_LOGI(TAG, "ADC Channel 1: %lu mV", (unsigned long)voltage1);
    ESP_LOGI(TAG, "ADC Channel 2: %lu mV", (unsigned long)voltage2);
    ESP_LOGI(TAG, "ADC Channel 3: %lu mV", (unsigned long)voltage3);
*/

    // Вывод значений в лог


  /*  
    snprintf(log_buffer, 128, "Ground_loop: %.3f V\n", (float)voltage0 * adc_coefficients[0]);
    send_log(log_buffer);

    snprintf(log_buffer, 128, "Input_voltage: %.3f V\n", (float)voltage1 * adc_coefficients[1]);
    send_log(log_buffer);

    snprintf(log_buffer, 128, "Output_voltage: %.3f V\n", (float)voltage2 * adc_coefficients[2]);
    send_log(log_buffer);

    snprintf(log_buffer, 128, "Target_current: %.3f A\n", (float)voltage3 * adc_coefficients[3]);
    send_log(log_buffer);
    */
   
    ESP_LOGI(TAG, "Ground_loop: %.3f V", (float)voltage0*adc_coefficients[0]); 
/*    ESP_LOGI(TAG, "Input_voltage: %.3f V", (float)voltage1*adc_coefficients[1]);
    ESP_LOGI(TAG, "Outputr_voltage: %.3f V", (float)voltage2*adc_coefficients[2]);
    ESP_LOGI(TAG, "Target_current: %.3f A", (float)voltage3*adc_coefficients[3]);
*/

     // Освобождаем память
     //free(log_buffer);
}


void vTaskLed( void * pvParameters )
{
  for( ;; )
  {
      // Task code goes here.
      vTaskDelay(100);
      printf("Hello\n\r");
      chg_func();
      ble_func();
      ESP_LOGE(TAG, "ESP test log");
  }
}
