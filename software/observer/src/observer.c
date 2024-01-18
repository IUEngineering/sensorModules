/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <ctype.h>

#include "observer.h"

#define MAX_NAME_LENGTH 30
#define MAX_URI_LENGTH 252 // Let's just be safe

#define SAMPLES_PER_PACKET 12

static void foundDevice(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad);
static void receivedScan(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf);
static bool dataCallback(struct bt_data *data, void *user_data);
void printCharBuffer(char *buf);
void printByteBuffer(uint8_t *buf, uint8_t len);


static struct bt_le_scan_cb scan_callbacks = {
    .recv = receivedScan,
};

int startObserver(void) {

    printk("Welcome to the Observer.\n\n");

    // Set the scanning parameters.
    struct bt_le_scan_param scan_param = {
        .type       = BT_LE_SCAN_TYPE_PASSIVE,
        .options    = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
        .interval   = BT_GAP_SCAN_FAST_INTERVAL,
        .window     = BT_GAP_SCAN_FAST_WINDOW,
    };

    int err;

    // Set extended callbacks.
    bt_le_scan_cb_register(&scan_callbacks);
    printk("Registered scan callbacks\n");

    // Start scanning, with foundDevice as normal callback.
    err = bt_le_scan_start(&scan_param, NULL);
    if (err) {
        printk("Start scanning failed (err %d)\n", err);
        return err;
    }
    printk("Started scanning...\n");

    return 0;
}


static void foundDevice(const bt_addr_le_t *addr, int8_t rssi, uint8_t type, struct net_buf_simple *ad) {
    if(addr->type == 1) return;
    char addr_str[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    printk("Device found: %s (RSSI %d), type %u, AD data len %u\n",
           addr_str, rssi, type, ad->len);
}

struct relevantData_t {
    char name[MAX_NAME_LENGTH];
    char uri[MAX_URI_LENGTH];
    uint8_t uriLength;
};

static void receivedScan(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf) {
    //E3:34:4D:E3:0B:60 is adress!!!
    
    // static uint8_t moduleAddress[6] = {0x00, 0xdf, 0xb9, 0x9f, 0x18, 0x0b};
    // static uint8_t setModuleAddress = 0;
    // if(memcmp(info->addr->a.val, moduleAddress, 6)) return;

    char address[BT_ADDR_LE_STR_LEN];
    struct relevantData_t relevantData = {0};

    // Use the built-in parser because literally why not.
    bt_data_parse(buf, dataCallback, &relevantData);

    // if(strcmp(relevantData.name, "poezen")) return;
    // if(relevantData.name[0] == '\0') return;
    if(relevantData.uriLength == 0) return;
    bt_addr_le_to_str(info->addr, address, sizeof(address));

    // if(!setModuleAddress && strcmp(relevantData.name, "poezen") == 0) { 
    //     memcpy(moduleAddress, info->addr->a.val, 6);
    //     setModuleAddress = 1;
    // }


    // printf("%03d: Found %s. Name is %s.\n", i++, address, relevantData.name);

    if(relevantData.uriLength) {
        // printf("URI is:\n");
        // printByteBuffer(relevantData.uri, relevantData.uriLength);
        static uint8_t lastSampleIndex = 3;
        if((relevantData.uri[6] & 0xc0) == lastSampleIndex) return;
        lastSampleIndex = relevantData.uri[6] & 0xc0;

        for(uint8_t i = 0; i < SAMPLES_PER_PACKET; i++) {
            // printf("Value: %d\n", (int16_t)((relevantData.uri[2*i] & 0x3f) << 8) | relevantData.uri[2*i + 1]);
            printf("Rec: %2x %2x: %d\n", relevantData.uri[2*i] & 0x3f, relevantData.uri[2*i+1], (int16_t)(((relevantData.uri[2*i] & 0x3f) << 8) | relevantData.uri[2*i+1]));
        }
        printf("\n");
    }
}


static bool dataCallback(struct bt_data *data, void *user_data) {
    struct relevantData_t *relevant = user_data;
    uint8_t length;
    uint8_t hadUri = 0, hadName = 0;
    
    switch(data->type) {
        case BT_DATA_URI:
            length = MIN(data->data_len, MAX_URI_LENGTH - 1);
            memcpy(relevant->uri, data->data, length);
            relevant->uri[length] = '\0';
            relevant->uriLength = length;
            hadUri = 1;
            break;

        case BT_DATA_NAME_COMPLETE:
        case BT_DATA_NAME_SHORTENED:
            length = MIN(data->data_len, MAX_NAME_LENGTH - 1);
            memcpy(relevant->name, data->data, length);
            relevant->name[length] = '\0';
            hadName = 1;
            break;
    }
    
    // Don't continue if we've had both the name and the uri.
    return !(hadUri && hadName);
}


void printCharBuffer(char *buf) {
    printf("\t");

    char *c = buf;
    while(*c) {
        printf("%02x ", *c);
        c++;
    }
    printf("\n\t");

    c = buf;
    while(*c) {
        if(isprint(*c)) printf("%c  ", *c);
        else printf("   ");
    }
    printf("\n\n");
}


void printByteBuffer(uint8_t *buf, uint8_t len) {
    printf("\t");
    for(uint8_t i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n\t");
    for(uint8_t i = 0; i < len; i++) {
        if(isprint(buf[i])) printf("%c  ", buf[i]);
        else printf("   ");
    }
    printf("\n\n");
}
