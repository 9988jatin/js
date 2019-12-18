#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_system.h"
#include <math.h>

//#define FRAME_SIZE      4
#define FRAME_SIZE      32
#define SAMPLE_RATE     (44100)
#define I2S_NUM         I2S_NUM_0
#define WAVE_FREQ_HZ    (100)
#define PI              (3.14159265)
#define I2S_BCK_IO      (GPIO_NUM_26)
#define I2S_WS_IO       (GPIO_NUM_25)
#define I2S_DO_IO       (GPIO_NUM_22)
#define I2S_DI_IO       (-1)

const double twoPi = 2.0 * PI;

double sin_phase = 0.0;
double sin_phase_step = 200.0 / SAMPLE_RATE * twoPi;

static void output_audio(int bits)
{
    static double frequency = 100.0;
    sin_phase_step = frequency / SAMPLE_RATE * twoPi;
    frequency += 0.01;
    
    const int num_frames = FRAME_SIZE;
    const int sample_size = (bits == 16) ? 2 : 4;

    // allocate stereo frames
    int * samples_data = (int*)malloc(num_frames * sample_size * 2);

/*
    printf("\r\nTest bits=%d, free mem=%d, written data=%d\n",
      bits,
      esp_get_free_heap_size(),
      num_frames * sample_size * 2);
*/

    for (int i = 0; i < num_frames; ++i)
    {
        double sin_float = sin(sin_phase);

        sin_phase = sin_phase + sin_phase_step;
        if (sin_phase >= twoPi)
          sin_phase -= twoPi;

        sin_float *= 1 << (bits - 3);

        if (bits == 16)
        {
            int sample_val = 0;
            sample_val += (short)sin_float;
            sample_val = sample_val << 16;
            sample_val += (short)sin_float;
            samples_data[i] = sample_val;
        }
        else if (bits == 24)
        {
            samples_data[i * 2 + 0] = (int)sin_float << 8;
            samples_data[i * 2 + 1] = (int)sin_float << 8;
        }
        else
        {
            samples_data[i * 2 + 0] = (int)sin_float;
            samples_data[i * 2 + 1] = (int)sin_float;
        }
    }

#if 0
    i2s_set_clk(
      I2S_NUM,
      SAMPLE_RATE,
      bits == 16
      ? I2S_BITS_PER_SAMPLE_16BIT
      : bits == 24
      ? I2S_BITS_PER_SAMPLE_24BIT
      : I2S_BITS_PER_SAMPLE_32BIT,
      I2S_CHANNEL_STEREO);
#endif

    

    size_t i2s_bytes_write = 0;
    
    i2s_write(
      I2S_NUM,
      samples_data,
      num_frames * sample_size * 2,
      &i2s_bytes_write,
      portMAX_DELAY);

    free(samples_data);
    samples_data = NULL;
}

void setup()
{
    //for 36Khz sample rates, we create 100Hz sine wave, every cycle need 36000/100 = 360 samples (4-bytes or 8-bytes each sample)
    //depend on bits_per_sample
    //using 6 buffers, we need 60-samples per buffer
    //if 2-channels, 16-bit each channel, total buffer is 360*4 = 1440 bytes
    //if 2-channels, 24/32-bit each channel, total buffer is 360*8 = 2880 bytes
    i2s_config_t i2s_config;
    i2s_config.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX);
    i2s_config.sample_rate = SAMPLE_RATE;
    i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    i2s_config.communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB);
    i2s_config.dma_buf_count = 2;
    i2s_config.dma_buf_len = FRAME_SIZE * 4 * 2;
    i2s_config.use_apll = false;
    i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;

 
  
    esp_chip_info_t out_info;
    esp_chip_info(&out_info);
    if(out_info.revision > 0)
    {
        i2s_config.use_apll = true;
        ESP_LOGI(TAG, "chip revision %d, enabling APLL for I2S", out_info.revision);
    }
    
    i2s_pin_config_t pin_config;
    pin_config.bck_io_num = I2S_BCK_IO;
    pin_config.ws_io_num = I2S_WS_IO;
    pin_config.data_out_num = I2S_DO_IO;
    pin_config.data_in_num = I2S_DI_IO;
    
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    
    i2s_set_sample_rates(I2S_NUM, SAMPLE_RATE);
}

void loop()
{
    int test_bits = 16;
    
    while (1)
    {
        output_audio(test_bits);
    }
}

