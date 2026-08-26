/*******************************************************************************
  * @file       ub_dt_p1 Temperature Sensor Application
  * @author
  * @version
  * @date
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/

/*-------------------------------- Includes ----------------------------------*/
#include <stdlib.h>
#include "osi.h"
#include "ub_dt_p1.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "pinmux.h"
#include "crc_check.h"
#include "MsgType.h"

extern void ets_delay_us(uint32_t us);

// The board has two identical single-wire temperature probes wired to two
// different GPIOs (ub_dt_p1_pin1 / ub_dt_p1_pin2). Every helper below is
// parameterized by pin so the protocol implementation only exists once;
// ub_dt_p1_get_temp() below drives it once per probe.

/**
 * @brief  Drive the given probe's single-wire data pin high
 * @param  pin GPIO the probe's data line is wired to
 */
static void ub_dt_p1_data_on(gpio_num_t pin)
{
  gpio_set_level(pin, 1);
}

/**
 * @brief  Drive the given probe's single-wire data pin low
 * @param  pin GPIO the probe's data line is wired to
 */
static void ub_dt_p1_data_off(gpio_num_t pin)
{
  gpio_set_level(pin, 0);
}

/**
 * @brief  Configure the given probe's data pin as GPIO input mode
 * @param  pin GPIO the probe's data line is wired to
 */
static void ub_dt_p1_io_in(gpio_num_t pin)
{
  gpio_set_direction(pin, GPIO_MODE_INPUT);
}

/**
 * @brief  Configure the given probe's data pin as GPIO output mode
 * @param  pin GPIO the probe's data line is wired to
 */
static void ub_dt_p1_io_out(gpio_num_t pin)
{
  gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}

/**
 * @brief  Perform a reset on the probe wired to the given pin (single-wire bus): pull the bus low for a period, then release it, and wait for the sensor's presence-pulse response
 * @param  pin GPIO the probe's data line is wired to
 * @return Returns SUCCESS(0) if the probe's presence pulse is detected and reset succeeds; returns FAILURE(-1) if waiting for the response times out
 */
static short ub_dt_p1_reset(gpio_num_t pin)
{
  uint8_t retry=0;

  ub_dt_p1_io_out(pin);
  ub_dt_p1_data_off(pin);
  ets_delay_us(750);

  ub_dt_p1_io_in(pin);
  ets_delay_us(30);

  while(gpio_get_level(pin))  //waite ub_dt_p1 respon
  {
    if(retry++>100)
    {
      return FAILURE;
    }
    ets_delay_us(3);
  }

  ets_delay_us(480);
  ub_dt_p1_io_out(pin);  //data pin out mode;

  return SUCCESS;
}

/**
 * @brief  Read one data bit from the probe wired to the given pin's single-wire bus
 * @param  pin GPIO the probe's data line is wired to
 * @return The bit value read, 0 or 1
 */
static uint8_t ub_dt_p1_read_bit(gpio_num_t pin)
{
  uint8_t data;

  ub_dt_p1_io_out(pin);
  ub_dt_p1_data_off(pin);
  ets_delay_us(3);

  ub_dt_p1_io_in(pin);
  ets_delay_us(10);

  data = gpio_get_level(pin) ? 1 : 0;

  ets_delay_us(60);

  return data;
}

/**
 * @brief  Read 8 data bits sequentially from the probe wired to the given pin's single-wire bus and assemble them into one byte (LSB first)
 * @param  pin GPIO the probe's data line is wired to
 * @return The byte of data read
 */
static uint8_t ub_dt_p1_read_byte(gpio_num_t pin)
{
  uint8_t i,j,data=0;

  for(i=0;i<8;i++)
  {
    j=ub_dt_p1_read_bit(pin);

    data=(j<<7)|(data>>1);
  }

  return data;
}

/**
 * @brief  Write the 8 data bits of one byte sequentially to the probe wired to the given pin's single-wire bus (LSB first)
 * @param  pin  GPIO the probe's data line is wired to
 * @param  data The byte of data to write
 */
static void ub_dt_p1_write_byte(gpio_num_t pin,uint8_t data)
{
  uint8_t i,data_bit;

  ub_dt_p1_io_out(pin);  //data pin out mode

  for(i=0;i<8;i++)
  {
    data_bit=data&0x01;

    if(data_bit)
    {
      ub_dt_p1_data_off(pin);
      ets_delay_us(5);

      ub_dt_p1_data_on(pin);
      ets_delay_us(75);
    }
    else
    {
      ub_dt_p1_data_off(pin);
      ets_delay_us(75);

      ub_dt_p1_data_on(pin);
      ets_delay_us(5);
    }
    data=data>>1;
  }
}

/**
 * @brief  Reset the probe wired to the given pin and, if present, send Skip ROM + Start Conversion
 * @param  pin GPIO the probe's data line is wired to
 * @return true if the probe reset successfully and conversion was started, false otherwise
 */
static bool ub_dt_p1_start_one(gpio_num_t pin)
{
  if(ub_dt_p1_reset(pin)!=SUCCESS)
  {
    return false;
  }
  ub_dt_p1_write_byte(pin,0xcc);   //skip rom
  ub_dt_p1_write_byte(pin,0x44);   //start convert
  ub_dt_p1_data_on(pin);
  return true;
}

/**
 * @brief  Start temperature conversion on both probes: reset each single-wire bus in turn, and on a successful reset send the Skip ROM and Start Conversion commands; if at least one probe started successfully, wait for the conversion to complete
 * @return Start status for each probe: bit0 set to 1 means probe 1 reset successfully and conversion was started; bit1 set to 1 means probe 2 reset successfully and conversion was started; 0 means the corresponding probe did not start successfully
 */
static uint8_t ub_dt_p1_start(void)
{
  uint8_t resp_val=0;

  ets_delay_us(5000);
  if(ub_dt_p1_start_one(ub_dt_p1_pin1)) resp_val |= 0x01;
  if(ub_dt_p1_start_one(ub_dt_p1_pin2)) resp_val |= 0x02;

  if(resp_val) osi_Sleep(750);  //
  return resp_val;
}

/**
 * @brief  Read back the temperature conversion result from the probe wired to the given pin: reset the bus, read its 9-byte scratchpad, verify the CRC, then convert the raw reading to degrees Celsius
 * @param  pin GPIO the probe's data line is wired to
 * @return The converted temperature in degrees Celsius, or ERROR_CODE if the reset or the CRC check failed
 */
static float ub_dt_p1_read_one(gpio_num_t pin)
{
  uint8_t data_buf[9] = {0};

  if(ub_dt_p1_reset(pin)!=SUCCESS)
  {
    return ERROR_CODE;
  }

  ub_dt_p1_write_byte(pin,0xcc);   //skip rom
  ub_dt_p1_write_byte(pin,0xbe);   //read data
  for(uint8_t j=0;j<9;j++)
  {
    data_buf[j] = ub_dt_p1_read_byte(pin);
  }

  if(calcrc_bytes(data_buf,9)!=0)
  {
    return ERROR_CODE;
  }

  uint8_t data_l = data_buf[0];
  uint8_t data_h = data_buf[1];
  short temp;
  if(data_h>7)  //temperature value<0
  {
    data_l=~data_l;
    data_h=~data_h;

    temp=data_h;
    temp<<=8;
    temp+=data_l;
    return (float)-temp*0.0625;
  }
  else //temperature value>=0
  {
    temp=data_h;
    temp<<=8;
    temp+=data_l;
    return (float)temp*0.0625;
  }
}

/**
 * @brief  Read the temperature conversion results from probes 1 and 2: after starting conversion, reset each probe and read its 9 bytes of scratchpad data, then compute the actual temperature value after CRC verification
 * @param  temp_value1 Output parameter used to return probe 1's temperature value, in degrees Celsius; remains ERROR_CODE if the read is not successful
 * @param  temp_value2 Output parameter used to return probe 2's temperature value, in degrees Celsius; remains ERROR_CODE if the read is not successful
 */
void ub_dt_p1_get_temp(float *temp_value1,float *temp_value2)
{
  *temp_value1 = ERROR_CODE;
  *temp_value2 = ERROR_CODE;

  uint8_t ext_status = ub_dt_p1_start();  //ub_dt_p1 start convert
  ets_delay_us(5000);

  if(ext_status&0x01)
  {
    *temp_value1 = ub_dt_p1_read_one(ub_dt_p1_pin1);
  }
  if(ext_status&0x02)
  {
    *temp_value2 = ub_dt_p1_read_one(ub_dt_p1_pin2);
  }
}

/*******************************************************************************
                                      END
*******************************************************************************/
