#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   100

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)


// Create ADC node
#define ADC_NODE	DT_NODELABEL(adc)
static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);

// Set ADC parameters
#define ADC_RESOLOUTION 12										// 2^12 = 4096 steps
#define ADC_CHANNEL 	0										// Channel 0 of the ADC
#define ADC_PORT 		SAADC_CH_PSELP_PSELP_AnalogInput0		// AIN0
#define ADC_REFERENCE	ADC_REF_INTERNAL			     		// Internal ref
#define ADC_GAIN		ADC_GAIN_1_6						 		// Gain 1x

struct adc_channel_cfg chl0_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	.channel_id = ADC_CHANNEL,
// use in differential mode
#ifdef CONFIG_ADC_NRFX_SAADC
	.input_positive = ADC_PORT
#endif
};

// TODO: Check if uint can be used
int16_t sample_buffer[1];

struct adc_sequence sequence = {
	// add channels, use: BIT(ADC_CHANNEL) | BIT(ADC_CHANNEL1) etc...  
	.channels = BIT(ADC_CHANNEL),
	.buffer = sample_buffer,
	// buffer size in bytes, not number of samples
	.buffer_size = sizeof(sample_buffer),
	.resolution = ADC_RESOLOUTION
};


/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void) {
	uint8_t err; 

	// Check if ADC is ready
	if (!device_is_ready(adc_dev)) {
		printk("adc_dev is not ready!\n");
		return;
	}

	err = adc_channel_setup(adc_dev, &chl0_cfg);

	if (err != 0) {
		printk("ADC adc_channel_setup failed with error %d.\n", err);
		return;
	}

	while (1) {
		err = adc_read(adc_dev, &sequence);
		if (err != 0) {
			printk("ADC reading failed with error %d.\n", err);
			return;
		}


		int32_t mv_value = sample_buffer[0];
	
		printk("ADC Value:   %d\n", mv_value);
		int32_t adc_vref = adc_ref_internal(adc_dev);
		adc_raw_to_millivolts(adc_vref, ADC_GAIN, ADC_RESOLOUTION, &mv_value);
		printk("ADC Voltage: %d mV\n", mv_value);

		k_msleep(SLEEP_TIME_MS);
	}
}
