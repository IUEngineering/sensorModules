/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>


#include <dk_buttons_and_leds.h>

#define ADVERTISING_PARAMETERS BT_LE_ADV_PARAM(0, 0x0200, 0x0210, NULL)


#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define RUN_LED_BLINK_INTERVAL 1000


/* STEP 4.2.2 - Declare the URL data to include in the scan response */
static unsigned char goodUrl[] = { 0x69 };
// static unsigned char badUrl[] = { 0x17, '/', '/', 'e','x','a','m','p','l','e','.','c','o','m'};


/* STEP 4.1.1 - Declare the advertising packet */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_URI, goodUrl, sizeof(goodUrl))
};

// Scan response is empty because we really don't want to be responding to scans.
static const struct bt_data scanResponse[] = {

};

static struct bt_le_ext_adv_start_param extendedParameters = {
    .num_events = 1,
    .timeout=0
};

// void fuck(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info) {
//     printf("haha KLEINE KATTEN.\n");
// }

// const struct bt_le_ext_adv_cb advertiseCallback = {
//     .sent = fuck
// };


int main(void)
{
    int err;
    uint16_t pHBuf;
    struct adc_sequence sequence = {

    };

    // adc_is_ready_dt();

    printf("Waiting for a second\n");
    // dk_buttons_init(handleButton);

    // k_msleep(1000);
    struct bt_le_ext_adv *advertisement;
    bt_le_ext_adv_create(BT_LE_ADV_NCONN_IDENTITY, &advertiseCallback, &advertisement);

    /* STEP 5 - Enable the Bluetooth LE stack */
    err = bt_enable(NULL);
    if (err) {
        printf("Bluetooth init failed (err %d)\n", err);
        return -1;
    }

    printf("Bluetooth initialized\n");


    /* STEP 6 - Start advertising */
    // err = bt_le_ext_adv_start(advertisement, &extendedParameters);
    err = bt_le_adv_start(ADVERTISING_PARAMETERS, ad, sizeof(ad), scanResponse, sizeof(scanResponse));
    if (err) {
        printf("Advertising failed to start (err %d)\n", err);
        return -1;
    }
    printf("Advertising successfully started\n");

    return 0;
}

