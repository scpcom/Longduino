#ifndef _VARIANT_SIPEED_LONGAN_NANO
#define _VARIANT_SIPEED_LONGAN_NANO

#include "Arduino.h"
#include "gd32vf103.h"
#include "gd32vf103_libopt.h"

/* BOARD  PIN DEFINE */

/* LEDs */
#define LED_BUILTIN  PA8

#define LED1_PIN                         GPIO_PIN_8
#define LED1_GPIO_PORT                   GPIOA
#define LED1_GPIO_CLK                    RCU_GPIOA


/* SD card chip select */
#define SDCARD_SS_PIN PB1

#define SD_CS_GPIO_PIN                    GPIO_PIN_1
#define SD_CS_GPIO_PORT                   GPIOB
#define SD_CS_GPIO_CLK                    RCU_GPIOB

#ifdef __cplusplus
extern "C" {
#endif

/* GD32VF103CBT6 package LQFP48 (PC0-PC12 n/a, PD0/PD1 not used here) */
#define VARIANT_GPIO_PORT_NUM (3)
#define VARIANT_GPIO_NUM (48)
#define VARIANT_AF_NUM (40)
#define VARIANT_SPI_NUM (3)
#define VARIANT_TIMER_NUM (7)

/* Pin aliases: these give the GPIO port/bit for each pin as an
 * enum. These are optional, but recommended. They make it easier to
 * write code using low-level GPIO functionality. */
enum {
PA0,PA1,PA2,PA3,PA4,PA5,PA6,PA7,PA8,PA9,PA10,PA11,PA12,PA13,PA14,PA15,
PB0,PB1,PB2,PB3,PB4,PB5,PB6,PB7,PB8,PB9,PB10,PB11,PB12,PB13,PB14,PB15,
PC0,PC1,PC2,PC3,PC4,PC5,PC6,PC7,PC8,PC9,PC10,PC11,PC12,PC13,PC14,PC15,
#if VARIANT_GPIO_NUM > 48
PD0,PD1,PD2,PD3,PD4,PD5,PD6,PD7,PD8,PD9,PD10,PD11,PD12,PD13,PD14,PD15,
#endif
#if VARIANT_GPIO_NUM > 64
PE0,PE1,PE2,PE3,PE4,PE5,PE6,PE7,PE8,PE9,PE10,PE11,PE12,PE13,PE14,PE15,
#endif
};

typedef struct _gpio_dev_t
{
    uint32_t gpio_port;
    rcu_periph_enum clk_id;

}gpio_dev_t;

typedef struct _adc_dev_t
{
    uint32_t adc_dev;
    rcu_periph_enum clk_id;

}adc_dev_t;

typedef struct _timer_dev_t {
    uint32_t timer_dev;
    rcu_periph_enum clk_id;
}timer_dev_t;

typedef struct _spi_dev_t {
    uint32_t spi_dev;
    rcu_periph_enum clk_id;
}spi_dev_t;

typedef struct _spi_map_t
{
    spi_dev_t * spi_device;
    uint8_t nss_pin;
    uint8_t sck_pin;
    uint8_t miso_pin;
    uint8_t mosi_pin;
} spi_map_t;

typedef struct _gd32v_pin_info_t
{
    timer_dev_t * timer_device;
    adc_dev_t * adc_device;
    uint8_t timer_channel;
    uint8_t adc_channel;
} gd32v_pin_info_t;

extern const gpio_dev_t * PORT_MAP[VARIANT_GPIO_PORT_NUM];
extern const gd32v_pin_info_t PIN_MAP[VARIANT_AF_NUM];
extern const spi_map_t SPI_MAP[VARIANT_SPI_NUM];
extern const timer_dev_t * TIMER_MAP[VARIANT_TIMER_NUM];

#define digitalPinToPort(p) ((PORT_MAP[((p) >> 4)])->gpio_port)
#define digitalPinToBitMask(p) (BIT((p) & 0xF))
#define digitalPinToClkid(p) ((PORT_MAP[(p) >> 4])->clk_id)

spi_dev_t * digitalPinToSPIInfo(uint8_t pinNumber);
#define digitalPinSPIAvailiable(p) (digitalPinToSPIInfo(p) != 0)
#define digitalPinToSPIDevice(p) (digitalPinToSPIInfo(p)->spi_dev)
#define digitalPinToSPIClockId(p) (digitalPinToSPIInfo(p)->clk_id)


#define VARIANT_GPIO_OSPEED GPIO_OSPEED_50MHZ //

#ifdef __cplusplus
}
#endif

#endif
