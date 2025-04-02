/**
 * Copyright (c) 2025 - LSITEC - Kaue Rodrigues Barbosa
 * Copyright (c) 2015 - present LibDriver All rights reserved
 * 
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. 
 *
 * @file      tsc34725_priv.h
 * @brief     tcs34725 driver header private file
 * @version   2.0.0
 * @author    Kaue Rodrigues Barbosa
 * @date      2025-03-19
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2025/03/20  <td>2.0      <td>Kaue Rodrigues Barbosa <td>adaptation to Zephyr
 * <tr><td>2021/02/28  <td>2.0      <td>Shifeng Li  <td>format the code
 * <tr><td>2020/10/30  <td>1.0      <td>Shifeng Li  <td>first upload
 * </table>
 */

#ifndef TCS34725_PRIV_H
#define TCS34725_PRIV_H

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <drivers/tcs34725.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C"{
#endif

/**
 * @brief chip information definition
 */
#define CHIP_NAME                 "AMS TCS34725"        /**< chip name */
#define MANUFACTURER_NAME         "AMS"                 /**< manufacturer name */
#define SUPPLY_VOLTAGE_MIN        2.7f                  /**< chip min supply voltage */
#define SUPPLY_VOLTAGE_MAX        3.6f                  /**< chip max supply voltage */
#define MAX_CURRENT               20.0f                 /**< chip max current */
#define TEMPERATURE_MIN           -40.0f                /**< chip min operating temperature */
#define TEMPERATURE_MAX           85.0f                 /**< chip max operating temperature */
#define DRIVER_VERSION            2000                  /**< driver version */

/**
 * @brief chip register definition
 */
#define TCS34725_REG_ENABLE         0x80        /**< enable register */
#define TCS34725_REG_ATIME          0x81        /**< atime register */
#define TCS34725_REG_WTIME          0x83        /**< wtime register */
#define TCS34725_REG_AILTL          0xA4        /**< ailtl register */
#define TCS34725_REG_AILTH          0xA5        /**< ailth register */
#define TCS34725_REG_AIHTL          0xA6        /**< aihtl register */
#define TCS34725_REG_AIHTH          0xA7        /**< aihtl register */
#define TCS34725_REG_PERS           0x8C        /**< pers register */
#define TCS34725_REG_CONFIG         0x8D        /**< config register */
#define TCS34725_REG_CONTROL        0x8F        /**< control register */
#define TCS34725_REG_ID             0x92        /**< id register */
#define TCS34725_REG_STATUS         0x93        /**< status register */
#define TCS34725_REG_CDATAL         0xB4        /**< cdatal register */
#define TCS34725_REG_CDATAH         0xB5        /**< cdatah register */
#define TCS34725_REG_RDATAL         0xB6        /**< rdatal register */
#define TCS34725_REG_RDATAH         0xB7        /**< rdatah register */
#define TCS34725_REG_GDATAL         0xB8        /**< gdatal register */
#define TCS34725_REG_GDATAH         0xB9        /**< gdatah register */
#define TCS34725_REG_BDATAL         0xBA        /**< bdatal register */
#define TCS34725_REG_BDATAH         0xBB        /**< bdatah register */
#define TCS34725_REG_CLEAR          0xE6        /**< clear register */

/**
 * @defgroup tcs34725_driver tcs34725 driver function
 * @brief    tcs34725 driver modules
 * @{
 */

/**
 * @addtogroup tcs34725_base_driver
 * @{
 */

/**
 * @brief tcs34725 bool enumeration definition
 */
typedef enum
{
    TCS34725_BOOL_FALSE = 0x00,        /**< disable function */
    TCS34725_BOOL_TRUE  = 0x01,        /**< enable function */
} tcs34725_bool_t;

/**
 * @brief tcs34725 integration time enumeration definition
 */
typedef enum
{
    TCS34725_INTEGRATION_TIME_2P4MS = 0xFF,        /**< integration time 2.4 ms */
    TCS34725_INTEGRATION_TIME_24MS  = 0xF6,        /**< integration time 24 ms */
    TCS34725_INTEGRATION_TIME_50MS  = 0xEB,        /**< integration time 50 ms */
    TCS34725_INTEGRATION_TIME_101MS = 0xD5,        /**< integration time 101 ms */
    TCS34725_INTEGRATION_TIME_154MS = 0xC0,        /**< integration time 154 ms */
    TCS34725_INTEGRATION_TIME_700MS = 0x00,        /**< integration time 700 ms */
} tcs34725_integration_time_t;

/**
 * @brief tcs34725 gain enumeration definition
 */
typedef enum
{
    TCS34725_GAIN_1X  = 0x00,        /**< 1x gain */
    TCS34725_GAIN_4X  = 0x01,        /**< 4x gain */
    TCS34725_GAIN_16X = 0x02,        /**< 16x gain */
    TCS34725_GAIN_60X = 0x03,        /**< 60x gain */
} tcs34725_gain_t;    

/**
 * @brief tcs34725 wait time enumeration definition
 */
typedef enum
{
    TCS34725_WAIT_TIME_2P4MS  = 0x0FF,        /**< 2.4 ms wait time */
    TCS34725_WAIT_TIME_204MS  = 0x0AB,        /**< 204 ms wait time */
    TCS34725_WAIT_TIME_614MS  = 0x000,        /**< 614 ms wait time */
    TCS34725_WAIT_TIME_29MS   = 0x1FF,        /**< 29 ms wait time */
    TCS34725_WAIT_TIME_2450MS = 0x1AB,        /**< 2450 ms wait time */
    TCS34725_WAIT_TIME_7400MS = 0x100,        /**< 7400 ms wait time */
} tcs34725_wait_time_t;

/**
 * @}
 */

/**
 * @addtogroup tcs34725_interrupt_driver
 * @{
 */

/**
 * @brief tcs34725 interrupt mode enumeration definition
 */
typedef enum  
{
    TCS34725_INTERRUPT_MODE_EVERY_RGBC_CYCLE                  = 0x00,        /**< every rgbc cycle interrupt */
    TCS34725_INTERRUPT_MODE_1_CLEAR_CHANNEL_OUT_OF_THRESHOLD  = 0x01,        /**< 1 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_2_CLEAR_CHANNEL_OUT_OF_THRESHOLD  = 0x02,        /**< 2 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_3_CLEAR_CHANNEL_OUT_OF_THRESHOLD  = 0x03,        /**< 3 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_5_CLEAR_CHANNEL_OUT_OF_THRESHOLD  = 0x04,        /**< 5 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_10_CLEAR_CHANNEL_OUT_OF_THRESHOLD = 0x05,        /**< 10 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_15_CLEAR_CHANNEL_OUT_OF_THRESHOLD = 0x06,        /**< 15 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_20_CLEAR_CHANNEL_OUT_OF_THRESHOLD = 0x07,        /**< 20 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_25_CLEAR_CHANNEL_OUT_OF_THRESHOLD = 0x08,        /**< 25 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_30_CLEAR_CHANNEL_OUT_OF_THRESHOLD = 0x09,        /**< 30 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_35_CLEAR_CHANNEL_OUT_OF_THRESHOLD = 0x0A,        /**< 35 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_40_CLEAR_CHANNEL_OUT_OF_THRESHOLD = 0x0B,        /**< 40 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_45_CLEAR_CHANNEL_OUT_OF_THRESHOLD = 0x0C,        /**< 45 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_50_CLEAR_CHANNEL_OUT_OF_THRESHOLD = 0x0D,        /**< 50 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_55_CLEAR_CHANNEL_OUT_OF_THRESHOLD = 0x0E,        /**< 55 cycle out of threshold interrupt */
    TCS34725_INTERRUPT_MODE_60_CLEAR_CHANNEL_OUT_OF_THRESHOLD = 0x0F,        /**< 60 cycle out of threshold interrupt */
} tcs34725_interrupt_mode_t;

/**
 * @}
 */

/**
 * @addtogroup tcs34725_base_driver
 * @{
 */

/**
 * @brief tcs34725 handle structure definition
 */
typedef struct tcs34725_handle_s
{
    uint8_t (*i2c_init)(void);                                                          /**< point to an i2c_init function address */
    uint8_t (*i2c_deinit)(void);                                                        /**< point to an i2c_deinit function address */
    uint8_t (*i2c_read)(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len);         /**< point to an i2c_read function address */
    uint8_t (*i2c_write)(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len);        /**< point to an i2c_write function address */
    void (*delay_ms)(uint32_t ms);                                                      /**< point to a delay_ms function address */
    void (*debug_print)(const char *const fmt, ...);                                    /**< point to a debug_print function address */
    uint8_t  inited;                                                                    /**< inited flag */
} tcs34725_handle_t;

/**
 * @brief tcs34725 information structure definition
 */
typedef struct tcs34725_info_s
{
    char chip_name[32];                /**< chip name */
    char manufacturer_name[32];        /**< manufacturer name */
    char interface[8];                 /**< chip interface name */
    float supply_voltage_min_v;        /**< chip min supply voltage */
    float supply_voltage_max_v;        /**< chip max supply voltage */
    float max_current_ma;              /**< chip max current */
    float temperature_min;             /**< chip min operating temperature */
    float temperature_max;             /**< chip max operating temperature */
    uint32_t driver_version;           /**< driver version */
} tcs34725_info_t;


struct tcs34725_data {
	uint16_t red;
    uint16_t green;
    uint16_t blue;
    uint16_t clear;
    // Not sure if this is how it should be done
    struct tcs34725_handle_s *handle;
};

struct tcs34725_config {
	struct i2c_dt_spec i2c;
};

// Writes data to a register
static int tcs34725_register_write(const struct device *dev, uint8_t *buf, uint32_t size);

// Reads data from a register
static int tcs34725_register_read(const struct device *dev, uint8_t *buf, uint32_t size);

// Writes command in the command register
// Maybe not useful
static int tcs34725_command_write(const struct device *dev,  uint8_t cmd);

// Fetches sample data
static int tcs34725_sample_fetch(const struct device *dev, enum sensor_channel channel);

static int tcs34725_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val);

// Gets desired attribute
static int tcs34725_attr_get(const struct device *dev, enum sensor_channel channel, enum sensor_attribute attribute, struct sensor_value *value);
// Sets desired attribute 
static int tcs34725_attr_set(const struct device *dev, enum sensor_channel channel, enum sensor_attribute attribute, const struct sensor_value *value);

/**
 * @}
 */

/**
 * @defgroup tcs34725_link_driver tcs34725 link driver function
 * @brief    tcs34725 link driver modules
 * @ingroup  tcs34725_driver
 * @{
 */

/**
 * @brief     initialize tcs34725_handle_t structure
 * @param[in] HANDLE pointer to a tcs34725 handle structure
 * @param[in] STRUCTURE tcs34725_handle_t
 * @note      none
 */
#define DRIVER_TCS34725_LINK_INIT(HANDLE, STRUCTURE)   memset(HANDLE, 0, sizeof(STRUCTURE))

/**
 * @brief     link i2c_init function
 * @param[in] HANDLE pointer to a tcs34725 handle structure
 * @param[in] FUC pointer to an i2c_init function address
 * @note      none
 */
#define DRIVER_TCS34725_LINK_I2C_INIT(HANDLE, FUC)    (HANDLE)->i2c_init = FUC

/**
 * @brief     link i2c_deinit function
 * @param[in] HANDLE pointer to a tcs34725 handle structure
 * @param[in] FUC pointer to an i2c_deinit function address
 * @note      none
 */
#define DRIVER_TCS34725_LINK_I2C_DEINIT(HANDLE, FUC)  (HANDLE)->i2c_deinit = FUC

/**
 * @brief     link i2c_read function
 * @param[in] HANDLE pointer to a tcs34725 handle structure
 * @param[in] FUC pointer to an i2c_read function address
 * @note      none
 */
#define DRIVER_TCS34725_LINK_I2C_READ(HANDLE, FUC)    (HANDLE)->i2c_read = FUC

/**
 * @brief     link i2c_write function
 * @param[in] HANDLE pointer to a tcs34725 handle structure
 * @param[in] FUC pointer to an i2c_write function address
 * @note      none
 */
#define DRIVER_TCS34725_LINK_I2C_WRITE(HANDLE, FUC)   (HANDLE)->i2c_write = FUC

/**
 * @brief     link delay_ms function
 * @param[in] HANDLE pointer to a tcs34725 handle structure
 * @param[in] FUC pointer to a delay_ms function address
 * @note      none
 */
#define DRIVER_TCS34725_LINK_DELAY_MS(HANDLE, FUC)    (HANDLE)->delay_ms = FUC

/**
 * @brief     link debug_print function
 * @param[in] HANDLE pointer to a tcs34725 handle structure
 * @param[in] FUC pointer to a debug_print function address
 * @note      none
 */
#define DRIVER_TCS34725_LINK_DEBUG_PRINT(HANDLE, FUC) (HANDLE)->debug_print = FUC

/**
 * @}
 */

/**
 * @defgroup tcs34725_base_driver tcs34725 base driver function
 * @brief    tcs34725 base driver modules
 * @ingroup  tcs34725_driver
 * @{
 */

/**
 * @brief      get chip information
 * @param[out] *info pointer to a tcs34725 info structure
 * @return     status code
 *             - 0 success
 *             - 2 handle is NULL
 * @note       none
 */
uint8_t tcs34725_info(tcs34725_info_t *info);

/**
 * @brief     initialize the chip
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @return    status code
 *            - 0 success
 *            - 1 i2c initialization failed
 *            - 2 handle is NULL
 *            - 3 linked functions is NULL
 * @note      none
 */
static int tcs34725_init(const struct device *dev);

/**
 * @brief     close the chip
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @return    status code
 *            - 0 success
 *            - 1 i2c deinit failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t tcs34725_deinit(tcs34725_handle_t *handle);

/**
 * @brief      read the rgbc data
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *red pointer to a red color buffer
 * @param[out] *green pointer to a green color buffer
 * @param[out] *blue pointer to a blue color buffer
 * @param[out] *clear pointer to a clear color buffer
 * @return     status code
 *             - 0 success
 *             - 1 read rgbc failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_read_rgbc(const struct device *dev, uint16_t *red, uint16_t *green, uint16_t *blue, uint16_t *clear);

// uint8_t tcs34725_read_rgbc(tcs34725_handle_t *handle, uint16_t *red, uint16_t *green, uint16_t *blue, uint16_t *clear);

/**
 * @brief      read the rgb data
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *red pointer to a red color buffer
 * @param[out] *green pointer to a green color buffer
 * @param[out] *blue pointer to a blue color buffer
 * @return     status code
 *             - 0 success
 *             - 1 read rgb failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_read_rgb(tcs34725_handle_t *handle, uint16_t *red, uint16_t *green, uint16_t *blue);

/**
 * @brief      read the clear data
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *clear pointer to a clear color buffer
 * @return     status code
 *             - 0 success
 *             - 1 read clear failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_read_c(tcs34725_handle_t *handle, uint16_t *clear);

/**
 * @brief     enable or disable the wait time
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @param[in] enable bool value
 * @return    status code
 *            - 0 success
 *            - 1 set wait failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t tcs34725_set_wait(tcs34725_handle_t *handle, tcs34725_bool_t enable);

/**
 * @brief      get the wait time
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *enable pointer to a bool value buffer
 * @return     status code
 *             - 0 success
 *             - 1 get wait failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_get_wait(tcs34725_handle_t *handle, tcs34725_bool_t *enable);

/**
 * @brief     enable or disable the rgbc adc
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @param[in] enable bool value
 * @return    status code
 *            - 0 success
 *            - 1 set rgbc failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t tcs34725_set_rgbc(tcs34725_handle_t *handle, tcs34725_bool_t enable);

/**
 * @brief      get the rgbc status
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *enable pointer to a bool value buffer
 * @return     status code
 *             - 0 success
 *             - 1 get rgbc failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_get_rgbc(tcs34725_handle_t *handle, tcs34725_bool_t *enable);

/**
 * @brief     enable or disable the power
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @param[in] enable bool value
 * @return    status code
 *            - 0 success
 *            - 1 set power on failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t tcs34725_set_power_on(tcs34725_handle_t *handle, tcs34725_bool_t enable);

/**
 * @brief      get the power status
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *enable pointer to a bool value buffer
 * @return     status code
 *             - 0 success
 *             - 1 get power on failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_get_power_on(tcs34725_handle_t *handle, tcs34725_bool_t *enable);

/**
 * @brief     set the rgbc adc integration time
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @param[in] t adc integration time
 * @return    status code
 *            - 0 success
 *            - 1 set rgbc integration time failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t tcs34725_set_rgbc_integration_time(tcs34725_handle_t *handle, tcs34725_integration_time_t t);

/**
 * @brief      get the rgbc adc integration time
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *t pointer to an integration time buffer
 * @return     status code
 *             - 0 success
 *             - 1 get rgbc integration time failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_get_rgbc_integration_time(tcs34725_handle_t *handle, tcs34725_integration_time_t *t);

/**
 * @brief     set the wait time
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @param[in] t wait time
 * @return    status code
 *            - 0 success
 *            - 1 set wait time failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t tcs34725_set_wait_time(tcs34725_handle_t *handle, tcs34725_wait_time_t t);

/**
 * @brief      get the wait time
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *t pointer to a wait time buffer
 * @return     status code
 *             - 0 success
 *             - 1 get wait time failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_get_wait_time(tcs34725_handle_t *handle, tcs34725_wait_time_t *t);

/**
 * @brief     set the adc gain
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @param[in] gain adc gain
 * @return    status code
 *            - 0 success
 *            - 1 set gain failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t tcs34725_set_gain(tcs34725_handle_t *handle, tcs34725_gain_t gain);

/**
 * @brief      get the adc gain
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *gain pointer to an adc gain buffer
 * @return     status code
 *             - 0 success
 *             - 1 get gain failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_get_gain(tcs34725_handle_t *handle, tcs34725_gain_t *gain);

/**
 * @}
 */

/**
 * @defgroup tcs34725_interrupt_driver tcs34725 interrupt driver function
 * @brief    tcs34725 interrupt driver modules
 * @ingroup  tcs34725_driver
 * @{
 */

/**
 * @brief     enable or disable the rgbc interrupt
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @param[in] enable bool value
 * @return    status code
 *            - 0 success
 *            - 1 set rgbc interrupt failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t tcs34725_set_rgbc_interrupt(tcs34725_handle_t *handle, tcs34725_bool_t enable);

/**
 * @brief      get the rgbc interrupt
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *enable pointer to a bool value buffer
 * @return     status code
 *             - 0 success
 *             - 1 get rgbc interrupt failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_get_rgbc_interrupt(tcs34725_handle_t *handle, tcs34725_bool_t *enable);

/**
 * @brief     set the interrupt mode
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @param[in] mode interrupt mode
 * @return    status code
 *            - 0 success
 *            - 1 set interrupt mode failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t tcs34725_set_interrupt_mode(tcs34725_handle_t *handle, tcs34725_interrupt_mode_t mode);

/**
 * @brief      get the interrupt mode
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *mode pointer to an interrupt mode buffer
 * @return     status code
 *             - 0 success
 *             - 1 get interrupt mode failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_get_interrupt_mode(tcs34725_handle_t *handle, tcs34725_interrupt_mode_t *mode);

/**
 * @brief     set the rgbc clear low interrupt threshold
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @param[in] threshold low interrupt threshold
 * @return    status code
 *            - 0 success
 *            - 1 set rgbc clear low interrupt threshold failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t tcs34725_set_rgbc_clear_low_interrupt_threshold(tcs34725_handle_t *handle, uint16_t threshold);

/**
 * @brief      get the rgbc clear low interrupt threshold
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *threshold pointer to a low interrupt threshold buffer
 * @return     status code
 *             - 0 success
 *             - 1 get rgbc clear low interrupt threshold failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_get_rgbc_clear_low_interrupt_threshold(tcs34725_handle_t *handle, uint16_t *threshold);

/**
 * @brief     set the rgbc clear high interrupt threshold
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @param[in] threshold high interrupt threshold
 * @return    status code
 *            - 0 success
 *            - 1 set rgbc clear high interrupt threshold failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t tcs34725_set_rgbc_clear_high_interrupt_threshold(tcs34725_handle_t *handle, uint16_t threshold);

/**
 * @brief      get the rgbc clear high interrupt threshold
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[out] *threshold pointer to a high interrupt threshold buffer
 * @return     status code
 *             - 0 success
 *             - 1 get rgbc clear high interrupt threshold failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_get_rgbc_clear_high_interrupt_threshold(tcs34725_handle_t *handle, uint16_t *threshold);

/**
 * @}
 */

/**
 * @defgroup tcs34725_extend_driver tcs34725 extend driver function
 * @brief    tcs34725 extend driver modules
 * @ingroup  tcs34725_driver
 * @{
 */

/**
 * @brief     set the chip register
 * @param[in] *handle pointer to a tcs34725 handle structure
 * @param[in] reg i2c register address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len data buffer length
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 *            - 2 handle is NULL
 *            - 3 handle is not initialized
 * @note      none
 */
uint8_t tcs34725_set_reg(tcs34725_handle_t *handle, uint8_t reg, uint8_t *buf, uint16_t len);

/**
 * @brief      get the chip register
 * @param[in]  *handle pointer to a tcs34725 handle structure
 * @param[in]  reg i2c register address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len data buffer length
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 *             - 2 handle is NULL
 *             - 3 handle is not initialized
 * @note       none
 */
uint8_t tcs34725_get_reg(tcs34725_handle_t *handle, uint8_t reg, uint8_t *buf, uint16_t len);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif