/**
 * @file mqtt_application.c
 * @author
 * @date
 * @brief
 */

//=============================================================================
// [Inclusions] ===============================================================
#include <math.h>

#include "esp_log.h"
#include "driver/uart.h"

#include "cJSON.h"

#include "mqtt_application.h"
#include "nvs_application.h"
#include "nvs_keys.h"

#include "nets_application.h"
#include "device.h"

//=============================================================================

//=============================================================================
// [Private Defines] ==========================================================
#define MQTT_TAG    		"[MQTT]"

#define MQTT_HOST   		""
#define MQTT_USER   		""
#define MQTT_PASS   		""
#define MQTT_PORT    		0

//=============================================================================

//=============================================================================
// [Local Typedef] ============================================================

//=============================================================================

//=============================================================================
// [External Data Definitions] ================================================

// Const ---------------------------------------------
//----------------------------------------------------

// Vars ----------------------------------------------
esp_mqtt_client_handle_t mqtt_client = NULL;

//----------------------------------------------------

// Task Handlers -------------------------------------
//----------------------------------------------------

// Queue Handlers ------------------------------------
//----------------------------------------------------

// Event Group Handlers ------------------------------
EventGroupHandle_t mqtt_event_group = NULL;

//----------------------------------------------------

// Semaphore Handlers --------------------------------
//----------------------------------------------------

//=============================================================================

//=============================================================================
// [Local Data Declarations] ==================================================

// Const ---------------------------------------------
//----------------------------------------------------

// Vars ----------------------------------------------
static mqtt_config_t mqtt_config = {0};
static mqtt_state_t mqtt_state = {0};

//----------------------------------------------------

// Task Handlers -------------------------------------
//----------------------------------------------------

// Queue Handlers ------------------------------------
//----------------------------------------------------

// Event Group Handlers ------------------------------
//----------------------------------------------------

// Semaphore Handlers --------------------------------
static SemaphoreHandle_t mqtt_semaphore = NULL;

//----------------------------------------------------

//=============================================================================

//=============================================================================
// [Local Function Declarations] ==============================================

/**
 * @details
 * @param[in]
 * @param[in]
 * @return
 */
static void mqtt_read_config(void);


/**
 * @details
 * @param[in]
 * @param[in]
 * @return
 */
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);


/**
 * @details
 * @param[in]
 * @param[in]
 * @return
 */
static void mqtt_receive_data(esp_mqtt_event_handle_t event);


/**
 * @details
 * @param[in]
 * @param[in]
 * @return
 */
void mqtt_parse_config(char * data);


/**
 * @details
 * @param[in]
 * @param[in]
 * @return
 */
void mqtt_cmd_read_config(void);

//=============================================================================

//=============================================================================
// [FreeRTOS Task Definitions] ================================================


void mqtt_subscribe_task(void *pvParameters)
{
	char topic[64] = {0};

	mqtt_make_topic("cmd/#", topic);

	while(1)
	{
		vTaskDelay(2000 / portTICK_RATE_MS);

		if( !(xEventGroupGetBits(mqtt_event_group) & MQTT_SUBSCRIBED_BIT) && (xEventGroupGetBits(mqtt_event_group) & MQTT_CONNECTED_BIT) )
		{
			if (esp_mqtt_client_subscribe(mqtt_client, "test2", 0) != -1 )
			{
				xEventGroupSetBits(mqtt_event_group, MQTT_SUBSCRIBED_BIT);
			}
		}
	}
}

//=============================================================================

//=============================================================================
// [Local Function Definitions] ===============================================

static void mqtt_read_config(void)
{
    //D&T server by defaul
    mqtt_config.server = true;
	/* Set default values */
	mqtt_config.delay = 2;
	mqtt_config.logs_delay = 300;

    nvs_read_bool(NVS_KEY_MQTT_SERVER, &mqtt_config.server);
    nvs_read_string(NVS_KEY_MQTT_HOST, mqtt_config.host);
    nvs_read_string(NVS_KEY_MQTT_USER, mqtt_config.user);
    nvs_read_string(NVS_KEY_MQTT_PASS, mqtt_config.pass);
    nvs_read_u32(NVS_KEY_MQTT_PORT, &mqtt_config.port);
    nvs_read_u16(NVS_KEY_MQTT_DELAY, &mqtt_config.delay);
    nvs_read_u16(NVS_KEY_MQTT_LOGS_DELAY, &mqtt_config.logs_delay);

	/* Check min values */
    if (mqtt_config.delay < 2)
    {
        mqtt_config.delay = 2;   //2 sec
    }

    if (mqtt_config.logs_delay < 300)
    {
        mqtt_config.logs_delay = 5 * 60;    //5 min
    }
}
// End static void mqtt_read_config(void)


static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    switch(event->event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
            mqtt_state.connected = true;
            mqtt_send_config();
        break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
            mqtt_state.connected = false;
            mqtt_state.subscribed = false;
            vTaskDelay(pdMS_TO_TICKS(5000));
        break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DATA");
            mqtt_receive_data(event);
        break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
        break;

        default:
            ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
        break;
    }

    return ESP_OK;
}
// End static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)


static void mqtt_receive_data(esp_mqtt_event_handle_t event)
{
    cJSON * json_root = NULL;
	cJSON * json_item = NULL;

	json_root = cJSON_Parse(event->data);

	if (json_root != NULL)
	{
		json_item = cJSON_GetObjectItemCaseSensitive(json_root, "str");
		if ((true == cJSON_IsString(json_item)) && (json_item->valuestring != NULL))
		{
			uart_write_bytes(UART_NUM_0, json_item->valuestring, strlen(json_item->valuestring));
		}
	}

	cJSON_Delete(json_root);
}
// End static void mqtt_receive_data(esp_mqtt_event_handle_t event)


void mqtt_parse_config(char * data)
{

}
// End void mqtt_parse_config(char * data)

void mqtt_cmd_read_config(void)
{

}
// End void mqtt_cmd_read_config(char *data)

//=============================================================================

//=============================================================================
// [External Function Definition] =============================================

void mqtt_init(void)
{
    mqtt_read_config();

    esp_mqtt_client_config_t mqtt_client_config = {0};

    if (true == mqtt_config.server)
    {
        mqtt_client_config.host = MQTT_HOST;
        mqtt_client_config.username = MQTT_USER;
        mqtt_client_config.password = MQTT_PASS;
        mqtt_client_config.port = MQTT_PORT;
    }
    else
    {
        mqtt_client_config.host = mqtt_config.host;
        mqtt_client_config.username = mqtt_config.user;
        mqtt_client_config.password = mqtt_config.pass;
        mqtt_client_config.port = mqtt_config.port;
    }

    mqtt_client_config.event_handle = mqtt_event_handler;

    mqtt_client = esp_mqtt_client_init(&mqtt_client_config);
    esp_mqtt_client_start(mqtt_client);

    mqtt_event_group = xEventGroupCreate();

    mqtt_semaphore = xSemaphoreCreateBinary();
	mqtt_exit_critical();

    xTaskCreate(mqtt_subscribe_task, "mqtt_sub_task", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY + 1, NULL);
}
// End void mqtt_init(char * type, char * name, char * id)


void mqtt_make_topic(const char * label, char * dest)
{
    device_config_t device_config = device_get_config();

    sprintf(dest, "%s/%s/%s/%s", device_config.family, device_config.model, device_config.id, label);
}
// End void mqtt_make_topic(const char * label, char * dest)


mqtt_config_t mqtt_get_config(void)
{
    return mqtt_config;
}
// End mqtt_config_t mqtt_get_config(void)


mqtt_state_t mqtt_get_state(void)
{
    return mqtt_state;
}
// End mqtt_state_t mqtt_get_state(void)


void mqtt_set_host(char * host)
{
    if (nvs_write_string(NVS_KEY_MQTT_HOST, host))
    {
        strncpy(mqtt_config.host, host, sizeof(mqtt_config.host));
    }
}
// End void mqtt_set_host(char * host)


void mqtt_set_user(char * user)
{
    if (nvs_write_string(NVS_KEY_MQTT_USER, user))
    {
        strncpy(mqtt_config.user, user, sizeof(mqtt_config.user));
    }
}
// End void mqtt_set_user(char * user)


void mqtt_set_pass(char * pass)
{
    if (nvs_write_string(NVS_KEY_MQTT_PASS, pass))
    {
        strncpy(mqtt_config.pass, pass, sizeof(mqtt_config.pass));
    }
}
// End void mqtt_set_pass(char * pass)


void mqtt_set_port(uint32_t port)
{
    if (nvs_write_u32(NVS_KEY_MQTT_PORT, port))
    {
        mqtt_config.port = port;
    }
}
// End void mqtt_set_port(uint32_t port)


void mqtt_set_delay(uint16_t delay)
{
    if (nvs_write_u16(NVS_KEY_MQTT_DELAY, delay))
    {
        mqtt_config.delay = delay;
    }
}
// End void mqtt_set_delay(uint16_t delay)


void mqtt_set_logs_delay(uint16_t logs_delay)
{
    if (nvs_write_u16(NVS_KEY_MQTT_LOGS_DELAY, logs_delay))
    {
        mqtt_config.logs_delay = logs_delay;
    }
}
// End void mqtt_set_logs_delay(uint16_t logs_delay)


void mqtt_set_server(bool server)
{
    if (nvs_write_bool(NVS_KEY_MQTT_SERVER, server))
    {
        mqtt_config.server = server;
    }
}
// End void mqtt_set_server(bool server)

void mqtt_send_config(void)
{
    mqtt_cmd_read_config();
}
// End void mqtt_send_config(void)

bool mqtt_exit_critical(void)
{
	return (bool)xSemaphoreGive(mqtt_semaphore);
}
// End bool mqtt_exit_critical(void)

bool mqtt_enter_critical(void)
{
	return (bool)xSemaphoreTake(mqtt_semaphore, portMAX_DELAY);
}
// End bool mqtt_enter_critical(void)

//=============================================================================

//====================== [End Document] =======================================
