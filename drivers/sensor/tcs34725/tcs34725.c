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
 * @file      tcs34725.c
 * @brief     driver tcs34725 source file
 * @version   2.0.0
 * @author    Kaue Rodrigues Barbosa
 * @date      2025-03-20
 *
 * <h3>history</h3>
 * <table>
 * <tr><th>Date        <th>Version  <th>Author      <th>Description
 * <tr><td>2025/03/20  <td>2.0      <td>Kaue Rodrigues Barbosa <td>adaptation to Zephyr
 * <tr><td>2021/02/28  <td>2.0      <td>Shifeng Li  <td>format the code
 * <tr><td>2020/10/30  <td>1.0      <td>Shifeng Li  <td>first upload
 * </table>
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <drivers/tcs34725.h>

#define DT_DRV_COMPAT ams_tcs34725

#include "tcs34725_priv.h"

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "TCS34725 driver enabled without any devices"
#endif

LOG_MODULE_REGISTER(TCS34725, CONFIG_SENSOR_LOG_LEVEL);

// Writes command in the command register
// returns 0 or -EIO
static int tcs34725_command_write(const struct device *dev,  uint8_t cmd)
{
    const struct tcs34725_config *cfg = dev->config;

    uint8_t cmd_reg[1]; 
    cmd_reg[0] = cmd | TCS34725_COMMAND;

    // printk("Comando enviado: %.2X\n\t", cmd_reg[0]);
    // printk("I2C address: %.2X\n\t", cfg->i2c.addr);

    return i2c_write_dt(&cfg->i2c, (uint8_t *)cmd_reg, 1);

    // return 0; // Maybe it isnt necessary. Not sure yet. Read datasheet for more info
}

// Writes data to a register // Maybe receive a size to?
// returns 0 or -eio
static int tcs34725_register_write(const struct device *dev, uint8_t reg, uint8_t *data, uint8_t size)
{
    const struct tcs34725_config *cfg = dev->config;
    uint8_t buf[1+size];

    if (data == NULL && size != 0) 
    {
        return -EINVAL;
    }

    buf[0] = reg;

    if (data != NULL) {
        memcpy(buf + 1, data, size);
    }
    
    
    return i2c_write_dt(&cfg->i2c, buf, (data == NULL) ? 1 : 1 + size);
}

// Reads data from a register
// returns 0 or -eio
static int tcs34725_register_read(const struct device *dev, uint8_t reg, uint8_t *buf, uint32_t size)
{
    // ARG_UNUSED(size); // Maybe ill mantain like this or pass the responsa to set the size to the fuction caller

    // slave_adress, reg_adress, buf_guarda_dados, byte_quantity
    const struct tcs34725_config *cfg = dev->config;

    int ret = 0;

    ret = tcs34725_command_write(dev, reg);
    if (ret != 0) {
        printk("Sequer conseguiu escrevere o comando!\n\t");
        return ret;
    }

    ret = i2c_read_dt(&cfg->i2c, buf, size);
    if (ret != 0) {
        printk("Escreveu o comando, mas não conseguiu fazer a leitura!\n\t");
        return ret;
    }

    return 0;

    // reg |= COMMAND_BIT;

    // return i2c_write_read_dt(&(cfg->i2c), (void *)&reg, 1, buf, size);
    // return i2c_burst_read_dt(&cfg->i2c, reg, buf, size);
}

// static float tcs34725_bytes_to_float(const uint8_t *value) {

//     union {
//         uint16_t unsigned16;
//         float float16;
//     } temp;

//     temp.unsigned16 = sys_get_be16(value);

//     return temp.float16;
// }


// Fetches sample data
// returns 0 or -eio
static int tcs34725_sample_fetch(const struct device *dev, enum sensor_channel channel)
{

    // Atribuir handle ao device pela macro
    // struct tcs34725_handle_t *handle = dev->handle; 

    // Pegar o struct data do device
    // struct tcs34725_data *data = dev->data; // Nao necessario ??

    // printk("Sample fetch ...\n\t");


    // chamar a read_rgbc
    int ret = tcs34725_read_rgbc(dev);
    if (ret)
    {
        LOG_DBG("Error at reading RGBC");
        return ret; // Verificar depois a lógica do retorno
    }

    // Calculates color temperature and lux. Stores in data structure
    tcs34725_calculate_lux_and_temp(dev);

    // // Temporary prints
    // printk("Red: %d\n\t Green: %d \n\t Blue: %d \n\t Clear: %d \n\t", data->red, data->green, data->blue, data->clear);
    return 0;
}

// returns 0 or -enotsup
static int tcs34725_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{

    const struct tcs34725_data *data = dev->data;

    switch (chan)
    {
    case SENSOR_CHAN_CLEAR_RAW:
        val->val1 = data->clear;
        val->val2 = 0;
        break;
    
    case SENSOR_CHAN_RED_RAW:
        val->val1 = data->red;
        val->val2 = 0;
        break;

    case SENSOR_CHAN_GREEN_RAW:
        val->val1 = data->green;
        val->val2 = 0;
        break;

    case SENSOR_CHAN_BLUE_RAW:
        val->val1 = data->blue;
        val->val2 = 0;
        break;

    case SENSOR_CHAN_LIGHT:
        val->val1 = (uint32_t)(data->luminosity / 100);
        val->val2 = (uint32_t)(data->luminosity % 100 * 10000); // Maybe will adjust this later

    case SENSOR_CHAN_COLOR_TEMP:
        val->val1 = (uint32_t)(data->color_temperature);
        val->val2 = 0;
    
    default:
        return -ENOTSUP; 
    }

    return 0; 
}

// Gets desired attribute
static int tcs34725_attr_get(const struct device *dev, enum sensor_channel channel, enum sensor_attribute attribute, struct sensor_value *value)
{
    return 0; // Temporary return
}

// Sets desired attribute 
static int tcs34725_attr_set(const struct device *dev, enum sensor_channel channel, enum sensor_attribute attribute, const struct sensor_value *value)
{
    return 0; // Temporary return
}

static int tcs34725_hex_to_gain(uint8_t gain_hex)
{
    int gain;

    switch (gain_hex)
    {
    case 0X00:
        return gain = 1;
    
    case 0X01:
        return gain = 4;

    case 0X02:
        return gain = 16;

    case 0X03:
        return gain = 60;
    
    default:
        return gain = 1;
    } 
}

// Adapted from https://github.com/hideakitai/TCS34725/blob/master/TCS34725.h
// See if any copyrigth info is nedded after
static void tcs34725_calculate_lux_and_temp(const struct device *dev)
{
    struct tcs34725_data *data = dev->data;
    int ret = 0;

    ret = tcs34725_get_rgbc_integration_time(dev);
    if (ret) { 
        return;
    }
    ret = tcs34725_get_gain(dev);
    if (ret) {
        return;
    }

    int gain_value = tcs34725_hex_to_gain((uint8_t)data->gain); 

    const uint8_t atime = data->int_time;
    const float integration_time = (256 - atime)*2.4f; // Integration time in ms

    // Device specific values (DN40 Table 1 in Appendix I)
    static const float GA = 1.f;                      // Glass Attenuation Factor
    static const float DF = 310.f;             // Device Factor
    static const float R_Coef = 0.136f;        //
    static const float G_Coef = 1.f;           // used in lux computation
    static const float B_Coef = -0.444f;       //
    static const float CT_Coef = 3810.f;       // Color Temperature Coefficient
    static const float CT_Offset = 1391.f;     // Color Temperatuer Offset

    // Analog/Digital saturation (DN40 3.5)
    float saturation = (256 - atime > 63) ? 65535 : 1024 * (256 - atime);

    // Ripple saturation (DN40 3.7)
    if (integration_time < 150)
        saturation -= saturation / 4;

    // Check for saturation and mark the sample as invalid if true
    if (data->clear >= saturation)
        return;

    // IR Rejection (DN40 3.1)
    float sum = data->red + data->green + data->blue;
    float c = data->clear;
    float ir = (sum > c) ? ((sum - c) / 2.f) : 0.f;
    float r2 = data->red - ir;
    float g2 = data->green - ir;
    float b2 = data->blue - ir;

    // Lux Calculation (DN40 3.2)
    float g1 = R_Coef * r2 + G_Coef * g2 + B_Coef * b2;
    float cpl = (integration_time * gain_value) / (GA * DF);
    data->luminosity = (uint64_t) g1 / cpl;

    // CT Calculations (DN40 3.4)
    data->color_temperature = (uint64_t)(CT_Coef * b2 / r2 + CT_Offset);
    
}

/**
 * @brief i2c address definition
 */
#define TCS34725_ADDRESS        (0x29 << 1)        /**< i2c address */ // 

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
// returns 0 or -eio
int tcs34725_set_rgbc_interrupt(const struct device *dev, tcs34725_bool_t enable)
{
    int ret = 0; 
    uint8_t prev;
    
    // res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);          /* read enable config */
    ret = tcs34725_register_read(dev, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);
    if (ret)                                                                                /* check the result */
    {
        LOG_DBG("Fail at reading enable register");                                /* read register failed */
        
        return ret;                                                                                /* return error */
    }
    prev &= ~(1 << 4);                                                                           /* clear interrupt */
    prev |= enable << 4;                                                                         /* set enable */
    // ret = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);         /* write config */
    ret = tcs34725_register_write(dev, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);
    if (ret)                                                                                /* check the result */
    {
        LOG_DBG("Fail at writting in enable register");                               /* write register failed */
        
        return ret;                                                                                /* return error */
    }
    
    return 0;                                                                                    /* success return 0 */
}

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
// returns 0 or -eio
int tcs34725_get_rgbc_interrupt(const struct device *dev)
{
    struct tcs34725_data *data = dev->data;

    int ret;
    uint8_t prev;
    
    // res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);        /* read enable config */
    ret = tcs34725_register_read(dev, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);
    if (ret)                                                                              /* check the result */
    {
        LOG_DBG("Fail at reading enable register");                              /* read register failed */
        
        return ret;                                                                              /* return error */
    }
    prev &= 1 << 4;                                                                            /* get interrupt */
    data->enable = (tcs34725_bool_t)((prev >> 4) & 0x01);                                           /* set interrupt */
    
    return 0;                                                                                  /* success return 0 */
}

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
// returns 0 or -eio
int tcs34725_set_wait_enable(const struct device *dev, tcs34725_bool_t enable)
{
    int ret = 0;
    uint8_t prev;
    
    // res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);          /* read enable config */
    ret = tcs34725_register_read(dev, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);
    if (ret)                                                                                /* check result */
    {
        LOG_DBG("Fail at reading enable register");                                /* read register failed */
        
        return ret;                                                                                /* return error */
    }
    prev &= ~(1 << 3);                                                                           /* clear enable bit */
    prev |= enable << 3;                                                                         /* set enable */
    ret = tcs34725_register_write(dev, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);         /* write config */
    if (ret)                                                                                /* check the result */
    {
        LOG_DBG("Fail at writting in enable register");                               /* write register failed */
        
        return ret;                                                                                /* return error */
    }
    
    return 0;                                                                                    /* success return 0 */
}

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
// returns 0 or -eio
int tcs34725_get_wait_enable(const struct device *dev)
{
    struct tcs34725_data *data = dev->data;
    
    int ret;
    uint8_t prev;

    ret = tcs34725_register_read(dev, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);        /* read config */
    if (ret)                                                                              /* check result */
    {
        LOG_DBG("Fail at reading enable register");                              /* read register failed */
        
        return ret;                                                                              /* return error */
    }
    prev &= 1 << 3;                                                                            /* get wait bit */
    data->enable = (tcs34725_bool_t)((prev >> 3) & 0x01);                                           /* get wait */
    
    return 0;                                                                                  /* success return 0 */
}

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
// returns 0 or -eio
int tcs34725_set_rgbc_status(const struct device *dev, tcs34725_bool_t enable)
{
    int ret = 0;
    uint8_t prev;
    
    ret = tcs34725_register_read(dev, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);          /* read config */
    if (ret)                                                                                /* check result */
    {
        LOG_DBG("Fail at reading enable register");                                /* read register failed */
        
        return ret;                                                                                /* return error */
    }
    prev &= ~(1 << 1);                                                                           /* clear enable bit */
    prev |= enable << 1;                                                                         /* set enable */
    ret = tcs34725_register_write(dev, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);         /* write config */
    if (ret)                                                                                /* check the result */
    {
        LOG_DBG("Fail at writting in enable register");                               /* write register failed */
        
        return ret;                                                                                /* return error */
    }
    
    return 0;                                                                                    /* success return 0 */
}

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
// returns 0 or -eio
int tcs34725_get_rgbc_status(const struct device *dev)
{
    struct tcs34725_data *data = dev->data;

    int ret;
    uint8_t prev;
    
    ret = tcs34725_register_read(dev, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);        /* read enable config */
    if (ret)                                                                              /* check result */
    {
        LOG_DBG("Fail at reading enable register");                              /* read register failed */
        
        return ret;                                                                              /* return error */
    }
    prev &= 1 << 1;                                                                            /* get rgbc bit */ 
    data->enable = (tcs34725_bool_t)((prev >> 1) & 0x01);                                           /* get enable */
    
    return 0;                                                                                  /* success return 0 */
}

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
// returns 0 or -eio
int tcs34725_set_power_on(const struct device *dev, tcs34725_bool_t enable)
{
    int ret = 0;
    uint8_t prev;
    
    ret = tcs34725_register_read(dev, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);          /* read config */
    if (ret)                                                                                /* check result */
    {
        LOG_DBG("Fail at reading enable register");                                /* read register failed */
        
        return ret;                                                                                /* return error */
    }
    prev &= ~(1 << 0);                                                                           /* clear enable bit */
    prev |= enable << 0;                                                                         /* set enable */
    ret = tcs34725_register_write(dev, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);         /* write config */
    if (ret)                                                                                /* check the result */
    {
        LOG_DBG("Fail at writting at enable register");                               /* write register failed */
        
        return ret;                                                                                /* return error */
    }
    
    return 0;                                                                                    /* success return 0 */
}

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
// returns 0 or -eio
int tcs34725_get_power_on(const struct device *dev)
{
    struct tcs34725_data *data = dev->data;

    int ret;
    uint8_t prev;
    
    ret = tcs34725_register_read(dev, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);        /* read enable config */
    if (ret)                                                                              /* check result */
    {
        LOG_DBG("Fail at reading enable register");                              /* read register failed */
        
        return ret;                                                                              /* return error */
    }
    prev &= 1 << 0;                                                                            /* get enable bit */
    data->enable = (tcs34725_bool_t)((prev >> 0) & 0x01);                                           /* get enable */
    
    return 0;                                                                                  /* success return 0 */
}

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
// return 0 or -eio
int tcs34725_set_rgbc_integration_time(const struct device *dev, tcs34725_integration_time_t time)
{
    int ret = 0;
    
    ret = tcs34725_register_write(dev, TCS34725_REG_ATIME, (uint8_t *)&time, 1);           /* write config */
    if (ret)                                                                              /* check the result */
    {
        LOG_DBG("Fail at writting in rgbc time register");                             /* write register failed */
        
        return ret;                                                                              /* return error */
    }
    
    return 0;                                                                                  /* success return 0 */
}


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
// returns 0 or -eio
int tcs34725_get_rgbc_integration_time(const struct device *dev)
{
    struct tcs34725_data *data = dev->data;
    int ret = 0;

    tcs34725_integration_time_t time;

    // res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ATIME, (uint8_t *)t, 1);           /* read config */
    ret = tcs34725_register_read(dev, TCS34725_REG_ATIME, (uint8_t *)&time, 1);
    if (ret)                                                                            /* check the result */
    {
        LOG_DBG("Fail at getting rgbc integration time");                           /* write register failed */
        return ret;                                                                            /* return error */
    }

    data->int_time = time;

    return 0;                                                                                /* success return 0 */
}

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
// returns 0 or -eio
int tcs34725_set_wait_time(const struct device *dev, tcs34725_wait_time_t time)
{
    int ret = 0;
    uint8_t prev, bit;
    
    ret = tcs34725_register_read(dev, TCS34725_REG_CONFIG, (uint8_t *)&prev, 1);         /* read config */
    if (ret)                                                                               /* check result */
    {
        LOG_DBG("Fail at reading config register");                               /* read register failed */
        
        return ret;                                                                               /* return error */
    }
    bit = (time & 0x100) >> 8;                                                                     /* get bit */
    prev &= ~(1 << 1);                                                                          /* clear wait time bit */
    prev |= bit << 1;                                                                           /* set bit */
    ret = tcs34725_register_write(dev, TCS34725_REG_CONFIG, (uint8_t *)&prev, 1);        /* write config */
    if (ret)                                                                               /* check result */
    {
        LOG_DBG("Fail at writting in config register");                              /* write register failed */
        
        return ret;                                                                               /* return error */
    }
    prev = time & 0xFF;                                                                            /* get time */
    ret = tcs34725_register_write(dev, TCS34725_REG_WTIME, (uint8_t *)&prev, 1);         /* write config */
    if (ret)                                                                               /* check the result */
    {
        LOG_DBG("Fail at reading wtime register");                              /* write register failed */
        
        return ret;                                                                               /* return error */
    }
    
    return 0;                                                                                   /* success return 0 */
}

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
// returns 0 or -eio
int tcs34725_get_wait_time(const struct device *dev)
{
    struct tcs34725_data *data = dev->data;
    
    int ret = 0;
    uint8_t prev, bit;
    
    ret = tcs34725_register_read(dev, TCS34725_REG_CONFIG, (uint8_t *)&prev, 1);        /* read config */
    if (ret)                                                                              /* check result */
    {
        LOG_DBG("Fail at reading CONFIG register");                              /* read failed */
        
        return ret;                                                                              /* return error */
    }
    prev &= 1 << 1;                                                                            /* get wait time bit */
    bit = (prev >> 1) & 0x01;                                                                  /* get wait time */
    ret = tcs34725_register_read(dev, TCS34725_REG_WTIME, (uint8_t *)&prev, 1);         /* read config */
    if (ret)                                                                              /* check result */
    {
        LOG_DBG("Fail at reading WTIME register");                              /* read register failed */
        
        return ret;                                                                              /* return error */
    }
    data->wait_time = (tcs34725_wait_time_t)((bit << 8) | prev);                                            /* get time */

    return 0;                                                                                  /* success return 0 */
}

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
// returns 0 or -eio
int tcs34725_set_rgbc_clear_low_interrupt_threshold(const struct device *dev, uint16_t threshold)
{
    int ret = 0;
    uint8_t buf[2];
    
    buf[0] = threshold & 0xFF;                                                               /* get threshold LSB */
    buf[1] = (threshold >> 8) & 0xFF;                                                        /* get threshold MSB */
    ret = tcs34725_register_write(dev, TCS34725_REG_AILTL, (uint8_t *)buf, 2);        /* write config */
    if (ret != 0)                                                                            /* check the result */
    {
        LOG_DBG("Failed at writting AILTL register");                           /* write register failed */
        
        return ret;                                                                            /* return error */
    }
    
    return 0;                                                                                /* success return 0 */
}

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
int tcs34725_get_rgbc_clear_low_interrupt_threshold(const struct device *dev)
{
    struct tcs34725_data *data = dev->data;

    int ret = 0;
    uint8_t buf[2];
    
    memset(buf, 0, sizeof(uint8_t) * 2);                                                    /* clear the buffer */
    ret = tcs34725_register_write(dev, TCS34725_REG_AILTL, (uint8_t *)buf, 2);        /* read ailtl */
    if (ret)                                                                           /* check result */
    {
        LOG_DBG("Failed at writting AILTL register");                           /* read register failed */
        
        return ret;                                                                           /* return error */
    }
    data->low_threshold = ((uint16_t)buf[1] << 8) | buf[0];                                          /* get threshold */
    
    return 0;                                                                               /* success return 0 */
}

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
int tcs34725_set_rgbc_clear_high_interrupt_threshold(const struct device *dev, uint16_t threshold)
{
    int ret = 0;
    uint8_t buf[2];
    
    buf[0] = threshold & 0xFF;                                                               /* get threshold LSB */
    buf[1] = (threshold >> 8) & 0xFF;                                                        /* get threshold MSB */
    ret = tcs34725_register_write(dev, TCS34725_REG_AIHTL, (uint8_t *)buf, 2);        /* write config */
    if (ret)                                                                            /* check the result */
    {
        LOG_DBG("Failed at writting AIHTL register");                           /* write register failed */
        
        return ret;                                                                            /* return error */
    }
    
    return 0;                                                                                /* success return 0 */
}

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
int tcs34725_get_rgbc_clear_high_interrupt_threshold(const struct device *dev)
{
    struct tcs34725_data *data = dev->data;

    int ret = 0;
    uint8_t buf[2];
       
    memset(buf, 0, sizeof(uint8_t) * 2);                                                    /* clear the buffer */
    ret = tcs34725_register_write(dev, TCS34725_REG_AIHTL, (uint8_t *)buf, 2);        /* read aihtl */
    if (ret)                                                                           /* check result */
    {
        LOG_DBG("Failed at writting into AIHTL register");                           /* read register failed */
        
        return ret;                                                                           /* return error */
    }
    data->high_threshold = ((uint16_t)buf[1] << 8) | buf[0];                                          /* get threshold */
    
    return 0;                                                                               /* success return 0 */
}

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
int tcs34725_set_interrupt_mode(const struct device *dev, tcs34725_interrupt_mode_t mode)
{
    int ret = 0;
    uint8_t prev;
    
    ret = tcs34725_register_read(dev, TCS34725_REG_PERS, (uint8_t *)&prev, 1);          /* read pers */
    if (ret)                                                                              /* check result */
    {
        LOG_DBG("Failed at reading PERS register");                              /* read register failed */
        
        return ret;                                                                              /* return error */
    }
    prev &= ~0x0F;                                                                             /* clear mode bit */
    prev |= mode;                                                                              /* set mode */
    ret = tcs34725_register_write(dev, TCS34725_REG_PERS, (uint8_t *)&prev, 1);         /* write config */
    if (ret)                                                                              /* check result */
    {
        LOG_DBG("Failed at writting PERS register");                             /* write register failed */
        
        return ret;                                                                              /* return error */
    }

    return 0;                                                                                  /* success return 0 */
}

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
int tcs34725_get_interrupt_mode(const struct device *dev)
{
    struct tcs34725_data *data = dev->data;

    int ret = 0;
    uint8_t prev;
    
    ret = tcs34725_register_read(dev, TCS34725_REG_PERS, (uint8_t *)&prev, 1);        /* read pers */
    if (ret)                                                                            /* check result */
    {
        LOG_DBG("Failed at reading PERS register");                            /* read register failed */
        
        return ret;                                                                            /* return error */
    }
    prev &= 0x0F;                                                                            /* get interrupt mode bits */
    data->int_mode = (tcs34725_interrupt_mode_t)(prev & 0x0F);                                        /* get interrupt mode */
    
    return 0;                                                                                /* success return 0 */
}

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
int tcs34725_set_gain(const struct device *dev, tcs34725_gain_t gain)
{
    struct tcs34725_data *data = dev->data;

    int ret; 
    uint8_t buf = 0;
    

    // Pq ler o que está no registrador antes???????
    // ret = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_CONTROL, (uint8_t *)&buf, 1);          /* read control */
    ret = tcs34725_register_read(dev, TCS34725_REG_CONTROL, (uint8_t *)&buf, 1);
    if (ret)                                                                                 /* check retult */
    {
        LOG_DBG("Fail at reading control register");                                 /* read register failed */
        
        return ret;                                                                                 /* return error */
    }


    buf &= ~0x03;                                                                                /* get gain bits */
    buf |= gain;                                                                                 /* set gain */
    ret = tcs34725_register_write(dev, TCS34725_REG_CONTROL, (uint8_t *)&buf, 1);         /* write config */
    if (ret)                                                                                 /* check result */
    {
        LOG_DBG("Fail at writting to control register");                                /* write register failed */
        
        return ret;                                                                                 /* return error */
    }

    return 0;                                                                                     /* success return 0 */
}

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
int tcs34725_get_gain(const struct device *dev)
{
    struct tcs34725_data *data = dev->data;

    int ret = 0; 
    uint8_t prev;
    
    // res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_CONTROL, (uint8_t *)&prev, 1);        /* read config */
    ret = tcs34725_register_read(dev, TCS34725_REG_CONTROL, &prev, 1);
    if (ret)                                                                               /* check result */
    {
        LOG_DBG("Error at getting gain");                               /* read register failed */
        return ret;                                                                               /* return error */
    }

    prev &= 0x03;                                                                               /* get gain bits */
    data->gain = (tcs34725_gain_t)(prev & 0x03);                                                     /* get gain */
    
    return 0;                                                                                   /* success return 0 */
}

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
int tcs34725_read_rgbc(const struct device *dev)
{
    int ret;
    uint8_t status = 0x99; // arbitrary value

    struct tcs34725_data *data = dev->data;

    // uint8_t buf[8];

    // printk("Status antes: %x\n\t", status);


    struct __attribute__((packed)) tcs34725_rx_data {
        uint8_t clear[2];
        uint8_t red[2];
        uint8_t green[2];
        uint8_t blue[2];
    } raw_data;

    // tcs34725_handle_t *handle = dev->data->handle;
    
    // Will check this later
    // if (handle == NULL)                                                                          /* check handle */
    // {
    //     return 2;                                                                                /* return error */
    // }
    // if (handle->inited != 1)                                                                     /* check handle initialization */
    // {
    //     return 3;                                                                                /* return error */
    // }
    

    //res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_STATUS, (uint8_t *)&prev, 1);          /* read status */
    ret = tcs34725_register_read(dev, TCS34725_REG_STATUS, &status, 1);                    /* read status */

   

    if (ret != 0)                                                                                /* check result */
    {
        LOG_DBG("Error at reading status register");                                /* read register failed */
        
        return ret;                                                                                /* return error */
    }

    // printk("Status: %x\n\t", status);

    if ((status & TCS34725_STATUS_AINT) != 0)  // Sei la. Vejo depois                        /* find interrupt */
    {
        // Se tem interrupt ele limpa o interrupt
        //ret = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_CLEAR, NULL, 0);                  /* clear interrupt */
        ret = tcs34725_register_write(dev, TCS34725_COMMAND_SPECIAL_FUNCTION | TCS34725_COMMAND_CLEAR_FUNCTION,
                                      NULL, 0);                          /* clear interrupt */
        if (ret) {                                                                     /* check result */
            LOG_DBG("tcs34725: clear interrupt failed.\n");                          /* clear interrupt failed */
            
            return ret;                                                                            /* return error */
        }

        // LOG_DBG("Deu AINT, mas sera ignorado ...\n\t");

        //return 1;
    }
    if ((status & TCS34725_STATUS_AVALID) != 0)                                                                      /* if data ready */
    {
        //ret = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_CDATAL, (uint8_t *)buf, 8);        /* read data */

        // Will read 8 consecutive bytes of data
        ret = tcs34725_register_read(dev, TCS34725_COMMAND_AUTO_INCREMENT | TCS34725_REG_CDATAL,
                                     (uint8_t *)&raw_data, sizeof(raw_data));
        if (ret) {
            LOG_DBG("Error at reading RGC data");
            return ret;
        }


        // ret = tcs34725_register_read(dev, TCS34725_REG_CDATAL, &raw_data.clear[0], 1);                 /* read data */
        // if (ret) {
        //     LOG_DBG("Error at reading lower byte of clear");
        //     return ret;
        // }
        // ret = tcs34725_register_read(dev, TCS34725_REG_CDATAH, &raw_data.clear[1], 1); 
        // if (ret) {
        //     LOG_DBG("Error at reading higher byte of clear");
        //     return ret;
        // }
        // ret = tcs34725_register_read(dev, TCS34725_REG_RDATAL, &raw_data.red[0], 1); 
        // if (ret) {
        //     LOG_DBG("Error at reading lower byte of red");
        //     return ret;
        // }
        // ret = tcs34725_register_read(dev, TCS34725_REG_RDATAH, &raw_data.red[1], 1); 
        // if (ret) {
        //     LOG_DBG("Error at reading higher byte of red");
        //     return ret;
        // }
        // ret = tcs34725_register_read(dev, TCS34725_REG_GDATAL, &raw_data.green[0], 1); 
        // if (ret) {
        //     LOG_DBG("Error at reading lower byte of green");
        //     return ret;
        // }
        // ret = tcs34725_register_read(dev, TCS34725_REG_GDATAH, &raw_data.green[1], 1); 
        // if (ret) {
        //     LOG_DBG("Error at reading higher byte of green");
        //     return ret;
        // }
        // ret = tcs34725_register_read(dev, TCS34725_REG_BDATAL, &raw_data.blue[0], 1); 
        // if (ret) {
        //     LOG_DBG("Error at reading lower byte of blue");
        //     return ret;
        // }
        // ret = tcs34725_register_read(dev, TCS34725_REG_BDATAH, &raw_data.blue[1], 1); 
        // if (ret) {
        //     LOG_DBG("Error at reading higher byte of blue");
        //     return ret;
        // }

        data->clear = sys_get_be16(raw_data.clear);
        data->red = sys_get_be16(raw_data.red);
        data->green = sys_get_be16(raw_data.green);
        data->blue = sys_get_be16(raw_data.blue);
        
        // temp prints
        // printk("Valores lidos ...\n\t");
        // printk("Clear: %f\n\t", data->clear);
        // printk("Red: %f\n\t", data->red);
        // printk("Green: %f\n\t", data->green);
        // printk("Blue: %f\n\t", data->blue);

        return 0;                                                                                /* success return 0 */
    }
    else
    {
        LOG_DBG("Data still not ready.\n");                                      /* data not ready */
            
        return ret;                                                                                /* return error */
    }
}

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
int tcs34725_read_rgb(const struct device *dev)
{
    int ret = 0;
    uint8_t status;
    // uint8_t buf[8];

    struct tcs34725_data *data = dev->data;

    struct __attribute__((packed)) tcs34725_rx_data {
        uint8_t red[2];
        uint8_t green[2];
        uint8_t blue[2];
    } raw_data;

    
    ret = tcs34725_register_read(dev, TCS34725_REG_STATUS, (uint8_t *)&status, 1);          /* read config */
    if (ret)                                                                                /* check result */
    {
        LOG_DBG("Failed at reading STATUS register");                                /* read register failed */
        
        return ret;                                                                                /* return error */
    }
    if ((status & TCS34725_STATUS_AINT) != 0)                                                                  /* find interrupt */
    {
        ret = tcs34725_register_write(dev, TCS34725_REG_CLEAR, NULL, 0);                  /* clear interrupt */
        if (ret)                                                                            /* check result */
        {
            LOG_DBG("Error at writting at CLEAR register");                          /* clear interrupt failed */
            
            return ret;                                                                            /* return error */
        }
    }
    if ((status & TCS34725_STATUS_AVALID) != 0)                                                                      /* if data ready */
    {
        // ret = tcs34725_register_read(dev, TCS34725_REG_CDATAL, (uint8_t *)buf, 8);        /* read data */
        ret = tcs34725_register_read(dev, TCS34725_COMMAND_AUTO_INCREMENT | TCS34725_REG_RDATAL,
                                    (uint8_t *)&raw_data, sizeof(raw_data));
        if (ret != 0)                                                                            /* check result */
        {
            LOG_DBG("Reading RDATAL register failed");                                     /* read data failed */
            
            return ret;                                                                            /* return error */
        }
        // *red   = ((uint16_t)buf[3] << 8) | buf[2];                                               /* get red */
        // *green = ((uint16_t)buf[5] << 8) | buf[4];                                               /* get green */
        // *blue  = ((uint16_t)buf[7] << 8) | buf[6];                                               /* get blue */

        data->red = sys_get_be16(raw_data.red);
        data->green = sys_get_be16(raw_data.green);
        data->blue = sys_get_be16(raw_data.blue);
   
        return 0;                                                                               /* success return 0 */
    }
    else
    {
        LOG_DBG("Data still not ready.\n");                                      /* data not ready */
            
        return ret;                                                                                /* return error */
    }
}

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
int tcs34725_read_c(const struct device *dev)
{
    int ret = 0;
    uint8_t status;
    uint8_t buf[2];
    
    struct tcs34725_data *data = dev->data;
    
    ret = tcs34725_register_read(dev, TCS34725_REG_STATUS, (uint8_t *)&status, 1);          /* read status */
    if (ret)                                                                                /* check result */
    {
        LOG_DBG("Failed at reading STATUS register");                                /* read register failed */
        
        return ret;                                                                                /* return error */
    }
    if ((status & TCS34725_STATUS_AINT) != 0)                                                                  /* find interrupt */
    {
        ret = tcs34725_register_write(dev, TCS34725_REG_CLEAR, NULL, 0);                  /* clear interrupt */
        if (ret)                                                                            /* check result */
        {
            LOG_DBG("Error at writting into CLEAR register");                          /* clear interrupt failed */
            
            return ret;                                                                            /* return error */
        }
    }
    if ((status & TCS34725_STATUS_AVALID) != 0)                                                                      /* if data ready */
    {
        ret = tcs34725_register_read(dev, TCS34725_REG_CDATAL, (uint8_t *)buf, 2);        /* read data */
        if (ret)                                                                            /* check result */
        {
            LOG_DBG("Failed at reading CDATAL register");                                     /* read failed */
            
            return ret;                                                                            /* return error */
        }
        
        data->clear = sys_get_be16(buf);                                               /* get clear */
   
        return 0;                                                                               /* success return 0 */
    }
    else
    {
        LOG_DBG("tcs34725: data not ready.\n");                                      /* data not ready */
            
        return ret;                                                                                /* return error */
    }
}

static int tcs34725_chip_enable(const struct device *dev)
{
    // const struct tcs34725_config *cfg = dev->config;

    int ret = 0;
    uint8_t temp_check;

    ret = tcs34725_register_read(dev, TCS34725_REG_ENABLE, &temp_check, 1);
    if (ret) {
        printk("Deu ruim ...\n\t");
    }

    // printk("Reg enable: %0.2x\n\t", temp_check);

    ret = tcs34725_register_write(dev, TCS34725_REG_ENABLE, (uint8_t *)TCS34725_ENABLE_PON, 1);
    if (ret != 0) 
    {
        return ret;
    }

    // This delay is specified in datasheet
    k_sleep(K_MSEC(3)); // delay as done in arduino implementarion. Maybe not necessary

    ret = tcs34725_register_write(dev, TCS34725_REG_ENABLE,(uint8_t *)(TCS34725_ENABLE_AEN | TCS34725_ENABLE_PON), 1);
    if (ret != 0)
    {
        return ret;
    }

    // ret = tcs34725_register_read(dev, TCS34725_REG_ENABLE, &temp_check, 1);
    // if (ret) {
    //     printk("Deu ruim 2 ...\n\t");
    // }

    // // printk("Reg enable fim: %0.2x\n\t", temp_check);

    // ret = tcs34725_register_read(dev, TCS34725_REG_ATIME, &temp_check, 1);
    // if (ret) {
    //     printk("Deu ruim 3 ...\n\t");
    // }

    // // printk("Tempo de integração no init: %0.2x\n\t", temp_check);


    // Maybe its needed to set the integration time here
    temp_check = 0xFF;
    ret = tcs34725_register_write(dev, TCS34725_REG_ATIME, &temp_check, 1);
    if (ret != 0) 
    {
        return ret;
    }

    k_sleep(K_MSEC(3)); // arduino sets an delay here for immediate readings. Maybe will be necessary

    return 0;
}

// Struct used to integrate this driver functions into Zephyr sensor's API
static const struct sensor_driver_api tcs34725_driver_api = {
	.sample_fetch = tcs34725_sample_fetch,
	.channel_get = tcs34725_channel_get,
	.attr_get = tcs34725_attr_get,
	.attr_set = tcs34725_attr_set,
};


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
static int tcs34725_init(const struct device *dev)
{
    LOG_DBG("Initializing TCS34725");

    // printk("This is the device init function\n\t");

    int ret; 
    uint8_t id;

    // tcs34725_handle_t *handle = dev->data.handle; // not sure if ill use

    const struct tcs34725_config *cfg = dev->config;
    const struct tcs34725_data *data = dev->data;

    if (!device_is_ready(cfg->i2c.bus)) 
    {
        LOG_ERR("I2C device %s is not ready", cfg->i2c.bus->name);
        return -ENODEV;
    }

    // printk("Device ready do driver check \n\t");

    // printk("Starting writing battery ...\n\t");

    // k_sleep(K_NSEC(50));


    // ret = 1;
    // while(ret != 0) {
    //     ret = tcs34725_command_write(dev, 0x92);
    //     printk("Ret: %d \n\t", ret);
    //     // k_sleep(K_NSEC(50));
    // }
    
    // if (handle == NULL)                                                                  /* check handle */
    // {
    //     return 2;                                                                        /* return error */
    // }
    // if (handle->debug_print == NULL)                                                     /* check debug_print */
    // {
    //     return 3;                                                                        /* return error */
    // }
    // if (handle->i2c_init == NULL)                                                        /* check i2c_init */
    // {
    //     handle->debug_print("tcs34725: i2c_init is null.\n");                            /* i2c_init is null */
        
    //     return 3;                                                                        /* return error */
    // }
    // if (handle->i2c_deinit == NULL)                                                      /* check i2c_init */
    // {
    //     handle->debug_print("tcs34725: i2c_deinit is null.\n");                          /* i2c_deinit is null */
        
    //     return 3;                                                                        /* return error */
    // }
    // if (handle->i2c_read == NULL)                                                        /* check i2c_read */
    // {
    //     handle->debug_print("tcs34725: i2c_read is null.\n");                            /* i2c_read is null */
        
    //     return 3;                                                                        /* return error */
    // }
    // if (handle->i2c_write == NULL)                                                       /* check i2c_write */
    // {
    //     handle->debug_print("tcs34725: i2c_write is null.\n");                           /* i2c_write is null */
        
    //     return 3;                                                                        /* return error */
    // }
    // if (handle->delay_ms == NULL)                                                        /* check delay_ms */
    // {
    //     handle->debug_print("tcs34725: delay_ms is null.\n");                            /* delay_ms is null */
        
    //     return 3;                                                                        /* return error */
    // }
    
    // if (handle->i2c_init() != 0)                                                         /* i2c init */
    // {
    //     handle->debug_print("tcs34725: i2c init failed.\n");                             /* i2c init failed */
        
    //     return 1;                                                                        /* return error */
    // }

    
    // Wait a little to stabilize chip
    // k_sleep(K_MSEC(5000));

    
    // Will send a dummy message to wake up the sensor out of sleep mode
    // OBS: if done more than enough, the systems halts. Not investigated yet
    for (int try = 0; try < MAX_TRIES_I2C_WRITES_AT_WAKE_UP && ret != 0; try++)
    {
        ret = tcs34725_register_read(dev, TCS34725_REG_ID, &id, 1);
        // k_sleep(K_MSEC(10));
        // if (ret == 0)
        //     break;
    }

    //ret = tcs34725_command_write(dev, 0x12);

    // res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ID, (uint8_t *)&id, 1);        /* read id */
    // ret = tcs34725_register_read(dev, TCS34725_REG_ID, &id, 1);                    /* read id */
    // ret = i2c_reg_read_byte_dt(&cfg->i2c, TCS34725_REG_ID, (uint8_t *) id);
    if (ret != 0)                                                                        /* check result */
    {
        printk("Erro na aquisição do ID. Return code: %d \n\t", ret);
        //handle->debug_print("tcs34725: read id failed.\n");                              /* read id failed */
        LOG_DBG("read id failed");
        // (void)handle->i2c_deinit();                                                      /* i2c deinit */
        
        return ret;  // -EIO                                                                      /* return error */
    }
   
    if ((id != 0x44) && (id != 0x4D))                                                    /* check id */
    {
        // handle->debug_print("tcs34725: id is error.\n");                                 /* id is error */
        LOG_DBG("tcs34725: id is error.\n");
        // (void)handle->i2c_deinit();                                                      /* i2c deinit */
        
        return ret;    // mudar isso aqui                                                                    /* return error */
    }

    ret = tcs34725_chip_enable(dev);
    if (ret != 0)
    {
        LOG_DBG("Could not enable chip");
        return 0;
    }
    // handle->inited = 1;                                                                  /* flag finish initialization */

    // Chamar a função de enable


    // printk("Fim da init ...\n\t");

    return 0;                                                                            /* success return 0 */
}

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
uint8_t tcs34725_deinit(tcs34725_handle_t *handle)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                       /* check handle */
    {
        return 2;                                                                             /* return error */
    }
    if (handle->inited != 1)                                                                  /* check handle initialization */
    {
        return 3;                                                                             /* return error */
    }
 
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);       /* read enable */
    if (res != 0)                                                                             /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                             /* read register failed */
        
        return 1;                                                                             /* return error */
    }
    prev &= ~(1 << 0);                                                                        /* disable */
    if (handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1) != 0)   /* write enable */
    {
         handle->debug_print("tcs34725: write register failed.\n");                           /* write register failed */
        
        return 1;                                                                             /* return error */
    }
    if (handle->i2c_deinit() != 0)                                                            /* i2c deinit */
    {
        handle->debug_print("tcs34725: i2c deinit failed.\n");                                /* i2c deinit failed */
        
        return 1;                                                                             /* return error */
    }   
    handle->inited = 0;                                                                       /* flag close */
    
    return 0;                                                                                 /* success return 0 */
}

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
uint8_t tcs34725_set_reg(tcs34725_handle_t *handle, uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t res;
    
    if (handle == NULL)                                                   /* check handle */
    {
        return 2;                                                         /* return error */
    }
    if (handle->inited != 1)                                              /* check handle initialization */
    {
        return 3;                                                         /* return error */
    }
    
    res = handle->i2c_write(TCS34725_ADDRESS, reg, buf, len);             /* write data */
    if (res != 0)                                                         /* check result */
    {
        handle->debug_print("tcs34725: write register failed.\n");        /* write register failed */
        
        return 1;                                                         /* return error */
    }

    return 0;                                                             /* success return 0 */
}

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
uint8_t tcs34725_get_reg(tcs34725_handle_t *handle, uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t res;
    
    if (handle == NULL)                                                  /* check handle */
    {
        return 2;                                                        /* return error */
    }
    if (handle->inited != 1)                                             /* check handle initialization */
    {
        return 3;                                                        /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, reg, buf, len);             /* read data */
    if (res != 0)                                                        /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");        /* read register failed */
        
        return 1;                                                        /* return error */
    }

    return 0;                                                            /* success return 0 */
}

/**
 * @brief      get chip information
 * @param[out] *info pointer to a tcs34725 info structure
 * @return     status code
 *             - 0 success
 *             - 2 handle is NULL
 * @note       none
 */
uint8_t tcs34725_info(tcs34725_info_t *info)
{
    if (info == NULL)                                               /* check handle */
    {
        return 2;                                                   /* return error */
    }
    
    memset(info, 0, sizeof(tcs34725_info_t));                       /* initialize tcs34725 info structure */
    strncpy(info->chip_name, CHIP_NAME, 32);                        /* copy chip name */
    strncpy(info->manufacturer_name, MANUFACTURER_NAME, 32);        /* copy manufacturer name */
    strncpy(info->interface, "i2c", 8);                             /* copy interface name */
    info->supply_voltage_min_v = SUPPLY_VOLTAGE_MIN;                /* set minimal supply voltage */
    info->supply_voltage_max_v = SUPPLY_VOLTAGE_MAX;                /* set maximum supply voltage */
    info->max_current_ma = MAX_CURRENT;                             /* set maximum current */
    info->temperature_max = TEMPERATURE_MAX;                        /* set minimal temperature */
    info->temperature_min = TEMPERATURE_MIN;                        /* set maximum temperature */
    info->driver_version = DRIVER_VERSION;                          /* set driver version */
    
    return 0;                                                       /* success return 0 */
}

#define TCS34725_DEFINE(inst)                                                                        \
	static const struct tcs34725_config tcs34725_config_##inst = {                                   \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                           \
	};                                                                                               \
                                                                                                     \
    static struct tcs34725_data tcs34725_data_##inst;                                                \
                                                                                                     \
	DEVICE_DT_INST_DEFINE(inst, tcs34725_init, NULL, &tcs34725_data_##inst, &tcs34725_config_##inst, \
						  POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &tcs34725_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TCS34725_DEFINE);

// if extern data is needed 

// DEVICE_DT_INST_DEFINE(inst, tcs34725_init, NULL, &tcs34725_data_##inst, &tcs34725_config_##inst, \
//     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &tcs34725_driver_api);