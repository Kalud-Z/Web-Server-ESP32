#include "ntp_time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/apps/sntp.h"

const char *TAG_NTP = "NTP_TIME";


void initialize_sntp(void) {
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "de.pool.ntp.org"); 
    sntp_init();
}

void obtain_time(void) {
   initialize_sntp();

    // Set timezone to Central European Time (CET) with DST
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG_NTP, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        time(&now);
        localtime_r(&now, &timeinfo); //transform time in &now into struct tm format.
    }
}

