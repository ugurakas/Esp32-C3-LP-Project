/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/rtc_io.h"
#include "driver/adc.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "time.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_sleep.h"
#include "esp_event_loop.h"
#include "esp_sntp.h"
#include "sdkconfig.h"
#include "esp_mac.h"
#include "esp_tls.h"
#include "esp_transport_tcp.h"

static const char *TAG = "MQTT_EXAMPLE";
static RTC_DATA_ATTR  uint8_t buffer[32]={0};
volatile RTC_DATA_ATTR int counter = 0;
volatile char str[128];
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN
#define MY_TOPIC    "sade/develop/ugur"
/*
#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_eclipseprojects_io_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_eclipseprojects_io_pem_start[]   asm("_binary_mqtt_eclipseprojects_io_pem_start");
#endif
extern const uint8_t mqtt_eclipseprojects_io_pem_end[]   asm("_binary_mqtt_eclipseprojects_io_pem_end");
*/
/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
void IntArrayToString()
{
    strcpy(str, "Device-2 ledsiz Data:");
    for (int i = 1; i <= 20; i++) {
        char temp[6];
        sprintf(temp, "%d", buffer[i]);
        strcat(str, temp);
        if (i < 20) strcat(str, ",");
    }
}
/*
void IntArrayToString() 
{
    sprintf(str,"Device-2 ledsiz Data:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7],buffer[8],buffer[9],buffer[10]
    ,buffer[11],buffer[12],buffer[13],buffer[14],buffer[15],buffer[16],buffer[17],buffer[18],buffer[19],buffer[20]); 
}
*/
static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        printf("WiFi connecting ... \n");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        printf("WiFi connected ... \n");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        printf("WiFi lost connection ... \n");
        break;
    case IP_EVENT_STA_GOT_IP:
        printf("WiFi got IP ... \n\n");
        break;
    default:
        break;
    }
}
void wifi_connection()
{
    // 1 - Wi-Fi/LwIP Init Phase
    esp_netif_init();                    // TCP/IP initiation 			     s1.1
    esp_event_loop_create_default();     // event loop 			             s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station 	                 s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation);     // 					             s1.4
    // 2 - Wi-Fi Configuration Phase
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    //esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS}};
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();
    //ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    //SP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_configuration));
    //ESP_ERROR_CHECK(esp_wifi_start());
    //ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI(TAG, "wifi_init_sta finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);

}
//"11111111111111111111111111111111111111111"
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        printf("buradan index %s \n",str);
        msg_id = esp_mqtt_client_publish(client, MY_TOPIC,str, 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;    
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        printf("Subscrıbe index %s \n",str);
        msg_id = esp_mqtt_client_publish(client, MY_TOPIC, str, 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {    
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);     
        break;
    }
}
static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://91.229.44.58:59461",
        .credentials.username = "sadedevtst",
        .credentials.authentication.password = "kuDQjtbn",
        //.broker.address.port = CONFIG_BROKER_PORT
    };    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);  
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}
void wakeup_wifi() {
    IntArrayToString(); 
    wifi_connection();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    mqtt_app_start();
    //ESP_ERROR_CHECK(esp_event_loop_create_default());
    //ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    ESP_LOGI(TAG, "Waiting for time sync");
    //sntp_setoperatingmode(SNTP_OPMODE_POLL);
    //sntp_setservername(0, "pool.ntp.org");
    //sntp_init();
    ESP_LOGI(TAG, "Ugur Wifi WakeupTime synced");
    // 1 saatte bir uyan
    esp_sleep_enable_timer_wakeup(180000000); 
    esp_deep_sleep_start();
    ESP_LOGI(TAG, "Time synsadasdASqsq ds<cxdqdwced");
}

static void single_read(void *arg)
{
    esp_err_t ret;
    //int adc1_reading[1] = {0xcc};
    //const char TAG_CH[2][10] = {"ADC1_CH3"};
    adc1_config_width(ADC_WIDTH_BIT_DEFAULT);
    adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_0);
    //adc1_reading[counter] = adc1_get_raw(ADC1_CHANNEL_3);
    buffer[counter] = adc1_get_raw(ADC1_CHANNEL_3); 
    esp_sleep_enable_timer_wakeup(180000000); // 3 dakika
    esp_deep_sleep_start();
    assert(ret == ESP_OK);
}

void deep_sleep()
{
    // Verileri bellekte saklama
    /*   // requesting time knowledge
    struct timeval now;
    gettimeofday(&now, NULL);
    int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 + (now.tv_usec - sleep_enter_time.tv_usec) / 1000;
    // Deep sleep moduna geçiş
    printf("Time:%d \n",sleep_time_ms);*/
    printf("Count:%d",counter);
    int i=0;
    counter++;
    if((counter % 20)==0)
    {       
        ESP_LOGI(TAG, "Waiting for time sync");
        printf("Counter içindeki sayi:%d",counter);
        if(i==0)
        {   
            esp_err_t ret = nvs_flash_init();
            if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
            {
                ESP_ERROR_CHECK(nvs_flash_erase());
                ret = nvs_flash_init();
            }
            ESP_ERROR_CHECK(ret);
            ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
            counter=0;
            wakeup_wifi();
            i++;          
        }        
    }
    else(single_read(NULL)); 
}
void app_main(void)
{   
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
    nvs_flash_init();
    ESP_ERROR_CHECK(esp_netif_init());
    deep_sleep();
}
