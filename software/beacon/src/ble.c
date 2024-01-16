#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>

#include "ble.h"

#define ADVERTISING_PARAMETERS BT_LE_ADV_PARAM(0, 0x0200, 0x0210, NULL)

/* STEP 4.2.2 - Declare the URL data to include in the scan response */
static unsigned char goodUrl[] = { 0x69 };
// static unsigned char badUrl[] = { 0x17, '/', '/', 'e','x','a','m','p','l','e','.','c','o','m'};


/* STEP 4.1.1 - Declare the advertising packet */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_URI, goodUrl, sizeof(goodUrl))
};

/* STEP 4.2.1 - Declare the scan response packet */
static const struct bt_data sd[] = {
    /* 4.2.3 Include the URL data in the scan response packet */
    // BT_DATA(BT_DATA_URI, badUrl, sizeof(badUrl)),
};


int initBle(void) {
    /* STEP 6 - Start advertising */
    int err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printf("Advertising failed to start (err %d)\n", err);
        return -1;
    }
    printf("Advertising successfully started\n");
}