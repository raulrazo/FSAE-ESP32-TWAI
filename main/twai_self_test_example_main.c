/*
 * SPDX-FileCopyrightText: 2010-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/*
 * The following example demonstrates the self testing capabilities of the TWAI
 * peripheral by utilizing the No Acknowledgment Mode and Self Reception Request
 * capabilities. This example can be used to verify that the TWAI peripheral and
 * its connections to the external transceiver operates without issue. The example
 * will execute multiple iterations, each iteration will do the following:
 * 1) Start the TWAI driver
 * 2) Transmit and receive 100 messages using self reception request
 * 3) Stop the TWAI driver
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "freertos/semphr.h"
 #include "esp_err.h"
 #include "esp_log.h"
 #include "driver/twai.h"
 
 /* --------------------- Definitions and static variables ------------------ */
 
 //Example Configurations
 #define TX_GPIO_NUM             CONFIG_EXAMPLE_TX_GPIO_NUM
 #define RX_GPIO_NUM             CONFIG_EXAMPLE_RX_GPIO_NUM
 #define TX_TASK_PRIO            8       //Sending task priority
 #define RX_TASK_PRIO            9       //Receiving task priority
 #define MSG_ID                  0x555   //11 bit standard format ID
 #define EXAMPLE_TAG             "TWAI Self Test"
 
 static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_50KBITS();
 //Filter all other IDs except MSG_ID
 static const twai_filter_config_t f_config = {.acceptance_code = (MSG_ID << 21),
                                               .acceptance_mask = ~(TWAI_STD_ID_MASK << 21),
                                               .single_filter = true
                                              };
 //Set to NO_ACK mode due to self testing with single module with loopback 
 static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NO_ACK);
 
 /* --------------------------- Tasks and Functions -------------------------- */
 
 static void twai_transmit_task(void *arg)
 {
    twai_message_t tx_msg = {
        .extd = 0,              // Standard ID (11-bit)
        .rtr = 0,               // Not a remote frame
        .ss = 0,                // Not single-shot
        .self = 1,              // Enable self-reception
        .dlc_non_comp = 0,      // Default DLC interpretation
        .identifier = MSG_ID,   // ID: 0x555
        .data_length_code = 1,  // 1 byte of data
        .data = {36},         // The data being sent
    };
 
     while (1) {
         ESP_ERROR_CHECK(twai_transmit(&tx_msg, portMAX_DELAY));
         // vTaskDelay(pdMS_TO_TICKS(10));  // Reduce or remove for faster signal
     }
 }
 
 
 static void twai_receive_task(void *arg)
 {
     twai_message_t rx_message;
     while (1) {
         ESP_ERROR_CHECK(twai_receive(&rx_message, portMAX_DELAY));
         ESP_LOGI(EXAMPLE_TAG, "Msg received\tID 0x%lx\tData = %d", rx_message.identifier, rx_message.data[0]);
     }
 }
 
 void app_main(void)
 {
     // Install and start the TWAI driver
     ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
     ESP_ERROR_CHECK(twai_start());
     ESP_LOGI(EXAMPLE_TAG, "Driver installed and started");
 
     // Launch TX and RX tasks (no control task needed)
     xTaskCreatePinnedToCore(twai_transmit_task, "TWAI_tx", 8192, NULL, TX_TASK_PRIO, NULL, tskNO_AFFINITY);
     xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 8192, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);
 }