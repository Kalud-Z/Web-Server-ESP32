#include "ntp_time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/apps/sntp.h"
#include <inttypes.h>


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
        localtime_r(&now, &timeinfo);
    }


    // Log the time continuously every 3 seconds for 15 seconds
    for (int i = 0; i < 1; i++) {
        // Count to 3
        for (int count = 1; count <= 3; count++) {
            ESP_LOGI(TAG_NTP, "Count: %d", count);
            vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for 1 second
        }

        // Log the time after counting to 3
        struct timeval tv_now;
        gettimeofday(&tv_now, NULL);

        // Convert to local time
        struct tm timeinfo;
        localtime_r(&tv_now.tv_sec, &timeinfo);

        // Buffer to hold the formatted time
        char time_str[50];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);

        // Append milliseconds
        int milliseconds = tv_now.tv_usec / 1000;
        char final_time_str[60];
        snprintf(final_time_str, sizeof(final_time_str), "%s:%03d", time_str, milliseconds);

        // Log the time
        ESP_LOGI(TAG_NTP, "Current time: %s", final_time_str);

        // Wait for 3 seconds before the next iteration
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }


}
