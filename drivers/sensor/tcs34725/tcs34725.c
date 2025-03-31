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

// Writes data to a register
static int tcs34725_register_write(const struct device *dev, uint8_t buf, uint32_t size)
{
    const struct tcs34725_config *cfg = dev->config;
    return i2c_write_dt(&cfg->i2c, buf, size);
}

// Reads data from a register
static int tcs34725_register_read(const struct device *dev, uint8_t buf, uint32_t size)
{
    const struct tcs34725_config *cfg = dev->config;
    return i2c_read_dt(&cfg->i2c, buf, size);
}

// Writes command in the command register
static int tcs34725_command_write(const struct device *dev,  uint8_t cmd)
{
    return 0; // Maybe it isnt necessary. Not sure yet. Read datasheet for more info
}

// Fetches sample data
static int tcs34725_sample_fetch(const struct device *dev, enum sensor_channel channel)
{

    // Atribuir handle ao device pela macro
    // struct tcs34725_handle_t *handle = dev->handle; 

    // Pegar o struct data do device
    struct tcs34725_data *data = dev->data;


    // chamar a read_rgbc
    uint8_t ret = tcs34725_read_rgbc(dev, &(data->red), &(data->green), &(data->blue), &(data->clear));
    if (!ret)
    {
        return ret; // Verificar depois a lógica do retorno
    }
}

static int tcs34725_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
    return 0; // Temporary return
}

// Gets desired attribute
static int tcs34725_attr_get(const struct device *dev, enum sensor_channel channel, struct sensor_attibute attribute, struct sensor_value *value)
{
    return 0; // Temporary return
}

// Sets desired attribute 
static int tcs34725_attr_set(const struct device *dev, enum sensor_channel channel, struct sensor_attibute attribute, struct sensor_value *value)
{
    return 0; // Temporary return
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
uint8_t tcs34725_set_rgbc_interrupt(tcs34725_handle_t *handle, tcs34725_bool_t enable)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                          /* check handle */
    {
        return 2;                                                                                /* return error */
    }
    if (handle->inited != 1)                                                                     /* check handle initialization */
    {
        return 3;                                                                                /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);          /* read enable config */
    if (res != 0)                                                                                /* check the result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                                /* read register failed */
        
        return 1;                                                                                /* return error */
    }
    prev &= ~(1 << 4);                                                                           /* clear interrupt */
    prev |= enable << 4;                                                                         /* set enable */
    res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);         /* write config */
    if (res != 0)                                                                                /* check the result */
    {
        handle->debug_print("tcs34725: write register failed.\n");                               /* write register failed */
        
        return 1;                                                                                /* return error */
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
uint8_t tcs34725_get_rgbc_interrupt(tcs34725_handle_t *handle, tcs34725_bool_t *enable)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                        /* check handle */
    {
        return 2;                                                                              /* return error */
    }
    if (handle->inited != 1)                                                                   /* check handle initialization */
    {
        return 3;                                                                              /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);        /* read enable config */
    if (res != 0)                                                                              /* check the result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                              /* read register failed */
        
        return 1;                                                                              /* return error */
    }
    prev &= 1 << 4;                                                                            /* get interrupt */
    *enable = (tcs34725_bool_t)((prev >> 4) & 0x01);                                           /* set interrupt */
    
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
uint8_t tcs34725_set_wait(tcs34725_handle_t *handle, tcs34725_bool_t enable)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                          /* check handle */
    {
        return 2;                                                                                /* return error */
    }
    if (handle->inited != 1)                                                                     /* check handle initialization */
    {
        return 3;                                                                                /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);          /* read enable config */
    if (res != 0)                                                                                /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                                /* read register failed */
        
        return 1;                                                                                /* return error */
    }
    prev &= ~(1 << 3);                                                                           /* clear enable bit */
    prev |= enable << 3;                                                                         /* set enable */
    res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);         /* write config */
    if (res != 0)                                                                                /* check the result */
    {
        handle->debug_print("tcs34725: write register failed.\n");                               /* write register failed */
        
        return 1;                                                                                /* return error */
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
uint8_t tcs34725_get_wait(tcs34725_handle_t *handle, tcs34725_bool_t *enable)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                        /* check handle */
    {
        return 2;                                                                              /* return error */
    }
    if (handle->inited != 1)                                                                   /* check handle initialization */
    {
        return 3;                                                                              /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);        /* read config */
    if (res != 0)                                                                              /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                              /* read register failed */
        
        return 1;                                                                              /* return error */
    }
    prev &= 1 << 3;                                                                            /* get wait bit */
    *enable = (tcs34725_bool_t)((prev >> 3) & 0x01);                                           /* get wait */
    
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
uint8_t tcs34725_set_rgbc(tcs34725_handle_t *handle, tcs34725_bool_t enable)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                          /* check handle */
    {
        return 2;                                                                                /* return error */
    }
    if (handle->inited != 1)                                                                     /* check handle initialization */
    {
        return 3;                                                                                /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);          /* read config */
    if (res != 0)                                                                                /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                                /* read register failed */
        
        return 1;                                                                                /* return error */
    }
    prev &= ~(1 << 1);                                                                           /* clear enable bit */
    prev |= enable << 1;                                                                         /* set enable */
    res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);         /* write config */
    if (res != 0)                                                                                /* check the result */
    {
        handle->debug_print("tcs34725: write register failed.\n");                               /* write register failed */
        
        return 1;                                                                                /* return error */
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
uint8_t tcs34725_get_rgbc(tcs34725_handle_t *handle, tcs34725_bool_t *enable)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                        /* check handle */
    {
        return 2;                                                                              /* return error */
    }
    if (handle->inited != 1)                                                                   /* check handle initialization */
    {
        return 3;                                                                              /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);        /* read enable config */
    if (res != 0)                                                                              /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                              /* read register failed */
        
        return 1;                                                                              /* return error */
    }
    prev &= 1 << 1;                                                                            /* get rgbc bit */ 
    *enable = (tcs34725_bool_t)((prev >> 1) & 0x01);                                           /* get enable */
    
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
uint8_t tcs34725_set_power_on(tcs34725_handle_t *handle, tcs34725_bool_t enable)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                          /* check handle */
    {
        return 2;                                                                                /* return error */
    }
    if (handle->inited != 1)                                                                     /* check handle initialization */
    {
        return 3;                                                                                /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);          /* read config */
    if (res != 0)                                                                                /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                                /* read register failed */
        
        return 1;                                                                                /* return error */
    }
    prev &= ~(1 << 0);                                                                           /* clear enable bit */
    prev |= enable << 0;                                                                         /* set enable */
    res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);         /* write config */
    if (res != 0)                                                                                /* check the result */
    {
        handle->debug_print("tcs34725: write register failed.\n");                               /* write register failed */
        
        return 1;                                                                                /* return error */
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
uint8_t tcs34725_get_power_on(tcs34725_handle_t *handle, tcs34725_bool_t *enable)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                        /* check handle */
    {
        return 2;                                                                              /* return error */
    }
    if (handle->inited != 1)                                                                   /* check handle initialization */
    {
        return 3;                                                                              /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ENABLE, (uint8_t *)&prev, 1);        /* read enable config */
    if (res != 0)                                                                              /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                              /* read register failed */
        
        return 1;                                                                              /* return error */
    }
    prev &= 1 << 0;                                                                            /* get enable bit */
    *enable = (tcs34725_bool_t)((prev >> 0) & 0x01);                                           /* get enable */
    
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
uint8_t tcs34725_set_rgbc_integration_time(tcs34725_handle_t *handle, tcs34725_integration_time_t t)
{
    uint8_t res;
    
    if (handle == NULL)                                                                        /* check handle */
    {
        return 2;                                                                              /* return error */
    }
    if (handle->inited != 1)                                                                   /* check handle initialization */
    {
        return 3;                                                                              /* return error */
    }
    
    res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_ATIME, (uint8_t *)&t, 1);           /* write config */
    if (res != 0)                                                                              /* check the result */
    {
        handle->debug_print("tcs34725: write register failed.\n");                             /* write register failed */
        
        return 1;                                                                              /* return error */
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
uint8_t tcs34725_get_rgbc_integration_time(tcs34725_handle_t *handle, tcs34725_integration_time_t *t)
{
    uint8_t res;
    
    if (handle == NULL)                                                                      /* check handle */
    {
        return 2;                                                                            /* return error */
    }
    if (handle->inited != 1)                                                                 /* check handle initialization */
    {
        return 3;                                                                            /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ATIME, (uint8_t *)t, 1);           /* read config */
    if (res != 0)                                                                            /* check the result */
    {
        handle->debug_print("tcs34725: write register failed.\n");                           /* write register failed */
        
        return 1;                                                                            /* return error */
    }
    
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
uint8_t tcs34725_set_wait_time(tcs34725_handle_t *handle, tcs34725_wait_time_t t)
{
    uint8_t res, prev, bit;
    
    if (handle == NULL)                                                                         /* check handle */
    {
        return 2;                                                                               /* return error */
    }
    if (handle->inited != 1)                                                                    /* check handle initialization */
    {
        return 3;                                                                               /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_CONFIG, (uint8_t *)&prev, 1);         /* read config */
    if (res != 0)                                                                               /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                               /* read register failed */
        
        return 1;                                                                               /* return error */
    }
    bit = (t & 0x100) >> 8;                                                                     /* get bit */
    prev &= ~(1 << 1);                                                                          /* clear wait time bit */
    prev |= bit << 1;                                                                           /* set bit */
    res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_CONFIG, (uint8_t *)&prev, 1);        /* write config */
    if (res != 0)                                                                               /* check result */
    {
        handle->debug_print("tcs34725: write register failed.\n");                              /* write register failed */
        
        return 1;                                                                               /* return error */
    }
    prev = t & 0xFF;                                                                            /* get time */
    res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_WTIME, (uint8_t *)&prev, 1);         /* write config */
    if (res != 0)                                                                               /* check the result */
    {
        handle->debug_print("tcs34725: write register failed.\n");                              /* write register failed */
        
        return 1;                                                                               /* return error */
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
uint8_t tcs34725_get_wait_time(tcs34725_handle_t *handle, tcs34725_wait_time_t *t)
{
    uint8_t res, prev, bit;
    
    if (handle == NULL)                                                                        /* check handle */
    {
        return 2;                                                                              /* return error */
    }
    if (handle->inited != 1)                                                                   /* check handle initialization */
    {
        return 3;                                                                              /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_CONFIG, (uint8_t *)&prev, 1);        /* read config */
    if (res != 0)                                                                              /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                              /* read failed */
        
        return 1;                                                                              /* return error */
    }
    prev &= 1 << 1;                                                                            /* get wait time bit */
    bit = (prev >> 1) & 0x01;                                                                  /* get wait time */
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_WTIME, (uint8_t *)&prev, 1);         /* read config */
    if (res != 0)                                                                              /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                              /* read register failed */
        
        return 1;                                                                              /* return error */
    }
    *t = (tcs34725_wait_time_t)((bit << 8) | prev);                                            /* get time */

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
uint8_t tcs34725_set_rgbc_clear_low_interrupt_threshold(tcs34725_handle_t *handle, uint16_t threshold)
{
    uint8_t res;
    uint8_t buf[2];
    
    if (handle == NULL)                                                                      /* check handle */
    {
        return 2;                                                                            /* return error */
    }
    if (handle->inited != 1)                                                                 /* check handle initialization */
    {
        return 3;                                                                            /* return error */
    }
    
    buf[0] = threshold & 0xFF;                                                               /* get threshold LSB */
    buf[1] = (threshold >> 8) & 0xFF;                                                        /* get threshold MSB */
    res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_AILTL, (uint8_t *)buf, 2);        /* write config */
    if (res != 0)                                                                            /* check the result */
    {
        handle->debug_print("tcs34725: write register failed.\n");                           /* write register failed */
        
        return 1;                                                                            /* return error */
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
uint8_t tcs34725_get_rgbc_clear_low_interrupt_threshold(tcs34725_handle_t *handle, uint16_t *threshold)
{
    uint8_t res, buf[2];
    
    if (handle == NULL)                                                                     /* check handle */
    {
        return 2;                                                                           /* return error */
    }
    if (handle->inited != 1)                                                                /* check handle initialization */
    {
        return 3;                                                                           /* return error */
    }
    
    memset(buf, 0, sizeof(uint8_t) * 2);                                                    /* clear the buffer */
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_AILTL, (uint8_t *)buf, 2);        /* read ailtl */
    if (res != 0)                                                                           /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                           /* read register failed */
        
        return 1;                                                                           /* return error */
    }
    *threshold = ((uint16_t)buf[1] << 8) | buf[0];                                          /* get threshold */
    
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
uint8_t tcs34725_set_rgbc_clear_high_interrupt_threshold(tcs34725_handle_t *handle, uint16_t threshold)
{
    uint8_t res;
    uint8_t buf[2];
    
    if (handle == NULL)                                                                      /* check handle */
    {
        return 2;                                                                            /* return error */
    }
    if (handle->inited != 1)                                                                 /* check handle initialization */
    {
        return 3;                                                                            /* return error */
    }
    
    buf[0] = threshold & 0xFF;                                                               /* get threshold LSB */
    buf[1] = (threshold >> 8) & 0xFF;                                                        /* get threshold MSB */
    res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_AIHTL, (uint8_t *)buf, 2);        /* write config */
    if (res != 0)                                                                            /* check the result */
    {
        handle->debug_print("tcs34725: write register failed.\n");                           /* write register failed */
        
        return 1;                                                                            /* return error */
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
uint8_t tcs34725_get_rgbc_clear_high_interrupt_threshold(tcs34725_handle_t *handle, uint16_t *threshold)
{
    uint8_t res, buf[2];
    
    if (handle == NULL)                                                                     /* check handle */
    {
        return 2;                                                                           /* return error */
    }
    if (handle->inited != 1)                                                                /* check handle initialization */
    {
        return 3;                                                                           /* return error */
    }
    
    memset(buf, 0, sizeof(uint8_t) * 2);                                                    /* clear the buffer */
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_AIHTL, (uint8_t *)buf, 2);        /* read aihtl */
    if (res != 0)                                                                           /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                           /* read register failed */
        
        return 1;                                                                           /* return error */
    }
    *threshold = ((uint16_t)buf[1] << 8) | buf[0];                                          /* get threshold */
    
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
uint8_t tcs34725_set_interrupt_mode(tcs34725_handle_t *handle, tcs34725_interrupt_mode_t mode)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                        /* check handle */
    {
        return 2;                                                                              /* return error */
    }
    if (handle->inited != 1)                                                                   /* check handle initialization */
    {
        return 3;                                                                              /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_PERS, (uint8_t *)&prev, 1);          /* read pers */
    if (res != 0)                                                                              /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                              /* read register failed */
        
        return 1;                                                                              /* return error */
    }
    prev &= ~0x0F;                                                                             /* clear mode bit */
    prev |= mode;                                                                              /* set mode */
    res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_PERS, (uint8_t *)&prev, 1);         /* write config */
    if (res != 0)                                                                              /* check result */
    {
        handle->debug_print("tcs34725: write register failed.\n");                             /* write register failed */
        
        return 1;                                                                              /* return error */
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
uint8_t tcs34725_get_interrupt_mode(tcs34725_handle_t *handle, tcs34725_interrupt_mode_t *mode)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                      /* check handle */
    {
        return 2;                                                                            /* return error */
    }
    if (handle->inited != 1)                                                                 /* check handle initialization */
    {
        return 3;                                                                            /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_PERS, (uint8_t *)&prev, 1);        /* read pers */
    if (res != 0)                                                                            /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                            /* read register failed */
        
        return 1;                                                                            /* return error */
    }
    prev &= 0x0F;                                                                            /* get interrupt mode bits */
    *mode = (tcs34725_interrupt_mode_t)(prev & 0x0F);                                        /* get interrupt mode */
    
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
uint8_t tcs34725_set_gain(tcs34725_handle_t *handle, tcs34725_gain_t gain)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                           /* check handle */
    {
        return 2;                                                                                 /* return error */
    }
    if (handle->inited != 1)                                                                      /* check handle initialization */
    {
        return 3;                                                                                 /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_CONTROL, (uint8_t *)&prev, 1);          /* read control */
    if (res != 0)                                                                                 /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                                 /* read register failed */
        
        return 1;                                                                                 /* return error */
    }
    prev &= ~0x03;                                                                                /* get gain bits */
    prev |= gain;                                                                                 /* set gain */
    res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_CONTROL, (uint8_t *)&prev, 1);         /* write config */
    if (res != 0)                                                                                 /* check result */
    {
        handle->debug_print("tcs34725: write register failed.\n");                                /* write register failed */
        
        return 1;                                                                                 /* return error */
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
uint8_t tcs34725_get_gain(tcs34725_handle_t *handle, tcs34725_gain_t *gain)
{
    uint8_t res, prev;
    
    if (handle == NULL)                                                                         /* check handle */
    {
        return 2;                                                                               /* return error */
    }
    if (handle->inited != 1)                                                                    /* check handle initialization */
    {
        return 3;                                                                               /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_CONTROL, (uint8_t *)&prev, 1);        /* read config */
    if (res != 0)                                                                               /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                               /* read register failed */
        
        return 1;                                                                               /* return error */
    }
    prev &= 0x03;                                                                               /* get gain bits */
    *gain = (tcs34725_gain_t)(prev & 0x03);                                                     /* get gain */
    
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
uint8_t tcs34725_read_rgbc(const struct device *dev, uint16_t *red, uint16_t *green, uint16_t *blue, uint16_t *clear)
{
    uint8_t ret, prev;
    uint8_t buf[8];

    tcs34725_handle_t *handle = dev->data->handle;
    
    if (handle == NULL)                                                                          /* check handle */
    {
        return 2;                                                                                /* return error */
    }
    if (handle->inited != 1)                                                                     /* check handle initialization */
    {
        return 3;                                                                                /* return error */
    }
    

    //res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_STATUS, (uint8_t *)&prev, 1);          /* read status */
    ret = tcs34725_register_read(dev, (uint8_t *)&prev, TCS34725_REG_STATUS);                    /* read status */
    if (ret != 0)                                                                                /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                                /* read register failed */
        
        return 1;                                                                                /* return error */
    }
    if ((prev & (1 << 4)) != 0)                                                                  /* find interrupt */
    {
        //ret = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_CLEAR, NULL, 0);                  /* clear interrupt */
        ret = tcs34725_register_write(dev, NULL, TCS34725_REG_CLEAR);                          /* clear interrupt */
        if (res != 0)                                                                            /* check result */
        {
            handle->debug_print("tcs34725: clear interrupt failed.\n");                          /* clear interrupt failed */
            
            return 1;                                                                            /* return error */
        }
    }
    if ((prev & 0x01) != 0)                                                                      /* if data ready */
    {
        //ret = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_CDATAL, (uint8_t *)buf, 8);        /* read data */
        ret = tcs34725_register_read(dev, (uint8_t *)buf, TCS34725_REG_CDATAL);                 /* read data */
        if (res != 0)                                                                            /* check result */
        {
            handle->debug_print("tcs34725: read failed.\n");                                     /* read failed */
            
            return 1;                                                                            /* return error */
        }
        *clear = ((uint16_t)buf[1] << 8) | buf[0];                                               /* get clear */
        *red   = ((uint16_t)buf[3] << 8) | buf[2];                                               /* get red */
        *green = ((uint16_t)buf[5] << 8) | buf[4];                                               /* get green */
        *blue  = ((uint16_t)buf[7] << 8) | buf[6];                                               /* get blue */
   
        return 0;                                                                                /* success return 0 */
    }
    else
    {
        handle->debug_print("tcs34725: data not ready.\n");                                      /* data not ready */
            
        return 1;                                                                                /* return error */
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
uint8_t tcs34725_read_rgb(tcs34725_handle_t *handle, uint16_t *red, uint16_t *green, uint16_t *blue)
{
    uint8_t res, prev;
    uint8_t buf[8];
    
    if (handle == NULL)                                                                          /* check handle */
    {
        return 2;                                                                                /* return error */
    }
    if (handle->inited != 1)                                                                     /* check handle initialization */
    {
        return 3;                                                                                /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_STATUS, (uint8_t *)&prev, 1);          /* read config */
    if (res != 0)                                                                                /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                                /* read register failed */
        
        return 1;                                                                                /* return error */
    }
    if ((prev & (1 << 4)) != 0)                                                                  /* find interrupt */
    {
        res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_CLEAR, NULL, 0);                  /* clear interrupt */
        if (res != 0)                                                                            /* check result */
        {
            handle->debug_print("tcs34725: clear interrupt failed.\n");                          /* clear interrupt failed */
            
            return 1;                                                                            /* return error */
        }
    }
    if ((prev & 0x01) != 0)                                                                      /* if data ready */
    {
        res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_CDATAL, (uint8_t *)buf, 8);        /* read data */
        if (res != 0)                                                                            /* check result */
        {
            handle->debug_print("tcs34725: read failed.\n");                                     /* read data failed */
            
            return 1;                                                                            /* return error */
        }
        *red   = ((uint16_t)buf[3] << 8) | buf[2];                                               /* get red */
        *green = ((uint16_t)buf[5] << 8) | buf[4];                                               /* get green */
        *blue  = ((uint16_t)buf[7] << 8) | buf[6];                                               /* get blue */
   
         return 0;                                                                               /* success return 0 */
    }
    else
    {
        handle->debug_print("tcs34725: data not ready.\n");                                      /* data not ready */
            
        return 1;                                                                                /* return error */
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
uint8_t tcs34725_read_c(tcs34725_handle_t *handle, uint16_t *clear)
{
    uint8_t res, prev;
    uint8_t buf[8];
    
    if (handle == NULL)                                                                          /* check handle */
    {
        return 2;                                                                                /* return error */
    }
    if (handle->inited != 1)                                                                     /* check handle initialization */
    {
        return 3;                                                                                /* return error */
    }
    
    res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_STATUS, (uint8_t *)&prev, 1);          /* read status */
    if (res != 0)                                                                                /* check result */
    {
        handle->debug_print("tcs34725: read register failed.\n");                                /* read register failed */
        
        return 1;                                                                                /* return error */
    }
    if ((prev & (1 << 4)) != 0)                                                                  /* find interrupt */
    {
        res = handle->i2c_write(TCS34725_ADDRESS, TCS34725_REG_CLEAR, NULL, 0);                  /* clear interrupt */
        if (res != 0)                                                                            /* check result */
        {
            handle->debug_print("tcs34725: clear interrupt failed.\n");                          /* clear interrupt failed */
            
            return 1;                                                                            /* return error */
        }
    }
    if ((prev & 0x01) != 0)                                                                      /* if data ready */
    {
        res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_CDATAL, (uint8_t *)buf, 8);        /* read data */
        if (res != 0)                                                                            /* check result */
        {
            handle->debug_print("tcs34725: read failed.\n");                                     /* read failed */
            
            return 1;                                                                            /* return error */
        }
        *clear = ((uint16_t)buf[1] << 8) | buf[0];                                               /* get clear */
   
         return 0;                                                                               /* success return 0 */
    }
    else
    {
        handle->debug_print("tcs34725: data not ready.\n");                                      /* data not ready */
            
        return 1;                                                                                /* return error */
    }
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
uint8_t tcs34725_init(const struct device *dev)
{
    LOG_DBG("Initializing TCS34725");

    uint8_t ret, id;
    tcs34725_handle_t *handle = dev->data->handle; // not sure if ill use

    const struct tcs34725_config *cfg = dev->config;
    const struct tcs34725_data *data = dev->data;

    if (!device_is_ready(cfg->bus.bus)) 
    {
        LOG_ERR("I2C device %s is not ready", cfg->bus.bus->name);
        return -ENODEV;
    }
    
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


    // res = handle->i2c_read(TCS34725_ADDRESS, TCS34725_REG_ID, (uint8_t *)&id, 1);        /* read id */
    ret = tcs34725_register_read(dev, (uint8_t *)&id, TCS34725_REG_ID);                    /* read id */
    if (ret != 0)                                                                        /* check result */
    {
        //handle->debug_print("tcs34725: read id failed.\n");                              /* read id failed */
        LOG_DBG("read id failed");
        // (void)handle->i2c_deinit();                                                      /* i2c deinit */
        
        return 1;  // -EIO                                                                      /* return error */
    }
    if ((id != 0x44) && (id != 0x4D))                                                    /* check id */
    {
        // handle->debug_print("tcs34725: id is error.\n");                                 /* id is error */
        LOG_DBG("tcs34725: id is error.\n");
        // (void)handle->i2c_deinit();                                                      /* i2c deinit */
        
        return 1;                                                                        /* return error */
    }
    // handle->inited = 1;                                                                  /* flag finish initialization */

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

#define TCS34725_DEFINE(inst)                                                                    \
	static const struct tcs34725_config tcs34725_config_##inst = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                  \
	};                                                                                      \
                                                                                            \
	DEVICE_DT_INST_DEFINE(inst, tcs34725_init, NULL, &tcs34725_data_##inst, &tcs34725_config_##inst, \
						  POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &tcs34725_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TCS34725_DEFINE);