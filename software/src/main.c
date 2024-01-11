/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <nrf.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>


/* Data of ADC io-channels specified in devicetree. */
// static const struct adc_dt_spec adc_channels = ADC_DT_SPEC_GET(DT_PATH(soc, adc_40007000));
static const struct adc_dt_spec adc_chan0 = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);

void readADC(struct adc_sequence* seq);
void doTX(void);
void initRadioRX(void);

int main(void) {

    // Init ADC.
    int16_t buf;
    struct adc_sequence seq = {
        .buffer = &buf,
        /* buffer size in bytes, not number of samples */
        .buffer_size = sizeof(buf),
    };
    int setupErr = adc_channel_setup_dt(&adc_chan0);
    while(!adc_is_ready_dt(&adc_chan0));

    doTX();


    while(1) {
        readADC(&seq);
        printf("%d %d\t", buf, buf * 3300 / 4096);
        for(uint8_t i = 0; i < (buf >> 6); i++) {
            printf("#");
        }
        printf("\n");

        k_msleep(50);
    }
    
    return 0;
}

void doTX(void) {
    // Initializing
    NRF_RADIO->MODE = RADIO_MODE_MODE_Nrf_2Mbit;


    // 16 bit for BLE_2Mbit
    // NRF_RADIO->PCNF0 = RADIO_PCNF0_PLEN_16bit;
    // No LENGTH, no S0, no S1.
    NRF_RADIO->PCNF0 = RADIO_PCNF0_PLEN_8bit;

    NRF_RADIO->PCNF1 = (3 << RADIO_PCNF1_MAXLEN_Pos) | (2 << RADIO_PCNF1_BALEN_Pos);

    NRF_RADIO->BASE0 = 0xdead;
    NRF_RADIO->BASE1 = 0xbeef;
    NRF_RADIO->TXADDRESS = RADIO_DACNF_TXADD0_Msk;
    NRF_RADIO->RXADDRESSES = RADIO_RXADDRESSES_ADDR0_Enabled;




    NRF_RADIO->TASKS_TXEN = RADIO_TASKS_TXEN_TASKS_TXEN_Msk;
    while(!NRF_RADIO->EVENTS_READY);
    NRF_RADIO->TASKS_START = RADIO_TASKS_START_TASKS_START_Msk;

}


void readADC(struct adc_sequence* seq) {
    adc_sequence_init_dt(&adc_chan0, seq);
    adc_read(adc_chan0.dev, seq);
}
