/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>


#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
    !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define F_SAMPLE 45
#define T_SAMPLE (1000 / F_SAMPLE)

#define SAMPLES_IN_PACKET 12
#define GET_SEND_WORD(ADC_DATA, INDEX) ((INDEX << 14) | ADC_DATA)

#define LDO_ENABLE_PIN 11
#define DEBUG_PIN 25
#define LED_0_PIN_Mask (1 << 24)
#define LED_1_PIN_Mask (1 << 25)
#define CAL_BUTTON_PIN 26
#define CAL_BUTTON_PIN_Mask (1 << CAL_BUTTON_PIN)


// Advertise with interval of 2 seconds
#define ADVERTISING_PARAMETERS BT_LE_ADV_PARAM(0, (T_SAMPLE * SAMPLES_IN_PACKET / 20), (T_SAMPLE * SAMPLES_IN_PACKET / 20 + 10), NULL)

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define CALIBRATED_NONE     0
#define CALIBRATED_7        1
#define CALIBRATED_7_AND_4  2

#define EXTRAPOLATE (sample, pHA, valA, pHB, valB) (((sample - valA) * (pHB - pHA)) / (valB - valA) + pHA)

/* STEP 4.2.2 - Declare the URL data to include in the scan response */
static unsigned char phData[2 * SAMPLES_IN_PACKET] = {0x17, 'f', 'l', 'o', 0x17, 'f', 'l', 'o'};
// static unsigned char badUrl[] = { 0x17, '/', '/', 'e','x','a','m','p','l','e','.','c','o','m'};

int eend = (1000 / F_SAMPLE);

/* STEP 4.1.1 - Declare the advertising packet */
const static struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_URI, phData, sizeof(phData))
};


/* STEP 4.2.1 - Declare the scan response packet */
static const struct bt_data sd[] = {
    /* 4.2.3 Include the URL data in the scan response packet */
    // BT_DATA(BT_DATA_URI, badUrl, sizeof(badUrl)),
};


/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adcChan = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

int initBle(void);
int initAdc(void);

int main(void)
{

    int16_t buf;
    struct adc_sequence sequence = {
        .buffer = &buf,
        /* buffer size in bytes, not number of samples */
        .buffer_size = sizeof(buf),
    };

    NRF_GPIO->DIRSET = (1 << LDO_ENABLE_PIN) | (1 << DEBUG_PIN);
    NRF_GPIO->OUTSET = (1 << LDO_ENABLE_PIN);

    NRF_GPIO->OUTSET = (1 << DEBUG_PIN);


    // NRF_GPIO->DIRCLR = CAL_BUTTON_PIN_Mask;
    // NRF_GPIO->PIN_CNF[CAL_BUTTON_PIN] |= GPIO_PIN_CNF_PULL_Pullup;

    // Init coms.
    if(initBle()) return -1;
    if(initAdc()) return -1;

    
    // The ADC data is 14 bits.
    // The other 2 bits will be used as indexing to be sure which adc data this is.
    // Every message contains 8 adc values. Every one of these will be sent approximately 2 times.
    // Because we have 8 adc values, each with 2 extra bits, we can embed a whole 2 extra bytes :D

    uint8_t sample = 0;
    uint8_t packetCount = 0;

    // uint8_t calibrationState = CALIBRATED_NONE;
    // uint8_t prevCalButtonState = 0;
    // uint16_t calibrated7pH = 0, calibrated4pH = 0;

    // printf("Calibrate 7pH please :)\n");

    // // Calibration loop.
    // while (1) {

    //     adc_sequence_init_dt(&adcChan, &sequence);
    //     adc_read(adcChan.dev, &sequence);

    //     printf("ADC: %d\n", buf);

    //     // Handle calibration (poorly). (don't look at this)
    //     uint8_t calButtonState = !!(NRF_GPIO->IN & CAL_BUTTON_PIN_Mask);
    //     if(calibrationState < CALIBRATED_7_AND_4 && calButtonState && calButtonState != prevCalButtonState) {
    //         if(calibrationState == CALIBRATED_NONE) {
    //             NRF_GPIO->OUTSET = LED_0_PIN_Mask;
    //             calibrated7pH = buf;
    //             printf("calibrated 7pH = %d\n", calibrated7pH);
    //         }
    //         else {
    //             NRF_GPIO->OUTCLR = LED_0_PIN_Mask;
    //             calibrated4pH = buf;
    //             printf("calibrated 4pH = %d\n", calibrated7pH);
    //         }
    //         calibrationState++;
    //     }
    //     prevCalButtonState = calButtonState;

    // }

    // Sampling loop.
    while(1) {

        adc_sequence_init_dt(&adcChan, &sequence);
        adc_read(adcChan.dev, &sequence);

        // Copy the 2 bytes from the adc buffer into the phData array.
        phData[2*sample] = buf >> 8;
        phData[2*sample + 1] = buf;

        printf("Val: %x %x\n", phData[2*sample], phData[2*sample + 1]);

        sample++;
        if(sample == SAMPLES_IN_PACKET) {
            // Embed packetCount into the packet.
            uint8_t currentBits = packetCount;
            for(uint8_t i = 0; i < 4; i++) {
                // adc:   00dddddd dddddddd 
                //        ^^
                // cBits: cccccccc
                // 0xc0:  11000000

                phData[i * 2] |= currentBits & 0xc0;
                currentBits <<= 2;
            }
            bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
            packetCount++;
            sample = 0;
            printf("\n");
        }
        if(sample & 1) NRF_GPIO->OUTSET = 1 << DEBUG_PIN;
        else NRF_GPIO->OUTCLR = 1 << DEBUG_PIN;
        k_sleep(K_MSEC(T_SAMPLE));
    }
    return 0;
}

int initAdc(void) {
    /* Configure channels individually prior to sampling. */
    if (!adc_is_ready_dt(&adcChan)) {
        printk("ADC controller device is not ready!!!!\n");
        return -1;
    }

    int err = adc_channel_setup_dt(&adcChan);
    if (err < 0) {
        printk("Could not setup channel (%d)\n", err);
        return err;
    }

    return 0;
}

int initBle(void) {

    // Enable bluetooth
    int err = bt_enable(NULL);
    if (err) {
        NRF_GPIO->OUTSET = 1 << DEBUG_PIN;
        printf("Bluetooth init failed (err %d)\n", err);
        return -1;
    }
    printf("Bluetooth initialized\n");

    // Start advertising
    err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        NRF_GPIO->OUTSET = 1 << DEBUG_PIN;
        printf("Advertising failed to start (err %d)\n", err);
        return -1;
    }
    printf("Advertising successfully started\n");

    return 0;
}