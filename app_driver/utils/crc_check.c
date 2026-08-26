/*******************************************************************************
  * @file       CRC Checksum Driver       
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
#include <stdint.h>
#include <string.h>
#include "crc_check.h"

#define polynomial      0x131   //P(x)=x^8+x^5+x^4+1=100110001

/**
 * @brief  Performs a CRC-8 check on a data buffer to verify whether the transmitted data is correct (the data typically includes the CRC value computed by the sender at the end, which is included in the check).
 * @param  data_buf Pointer to the buffer holding the data to check (input).
 * @param  bytes Number of bytes included in the check.
 * @return The computed CRC-8 check result; 0 means the data passed the check, non-zero means the data has an error.
 */
uint8_t Data_Crc_Check(uint8_t *data_buf,uint8_t bytes)
{
  uint8_t bit,byte;
  uint8_t crc_val=0xff;  //calculated checksum
  
  for(byte=0;byte<bytes;byte++)
  {
    crc_val^=*data_buf++;
    
    for(bit=0;bit<8;bit++)
    {
      if(crc_val&0x80)
      {
        crc_val=(crc_val<<1)^polynomial;
      }
      else
      {
        crc_val=(crc_val<<1);
      }
    }
  }
  return crc_val;
}

/**
 * @brief  Computes the CRC-8 checksum of a data buffer (polynomial 0x131).
 *         Identical algorithm to Data_Crc_Check(); kept as a separate name
 *         because callers use it purely to compute a checksum to append,
 *         rather than to verify one that's already present in the buffer.
 * @param  data_buf Pointer to the buffer holding the data to compute (input).
 * @param  bytes Number of bytes included in the computation.
 * @return The computed CRC-8 checksum value.
 */
uint8_t Data_Crc_Value(uint8_t *data_buf,uint8_t bytes)
{
  return Data_Crc_Check(data_buf,bytes);
}

/**
 * @brief  Computes the CRC value of a single byte of data (bit-reversed CRC algorithm specific to the ub_dt_p1 protocol).
 * @param  byte The single byte of data to compute.
 * @return The computed single-byte CRC value.
 */
uint8_t calcrc_byte(uint8_t byte)
{
	uint8_t i,crc_byte = 0;
	for(i=0;i<8;i++)
	{
		if((crc_byte^byte)&0x01)
		{
			crc_byte^=0x18;
			crc_byte>>=1;
			crc_byte|=0x80;
		}
		else
		{
			crc_byte>>=1;
		}
		byte>>=1;
	}
	return crc_byte;
}

/**
 * @brief specific to the ub_dt_p1 protocol; calls calcrc_byte on each byte in turn to accumulate the result.
 * @param  data_buf Pointer to the buffer holding the data to compute (input).
 * @param  bytes Number of bytes to compute.
 * @return The computed CRC check value.
 */
uint8_t calcrc_bytes(uint8_t *data_buf,uint8_t bytes)
{
	uint8_t crc_val = 0;
	while(bytes--)
	{
		crc_val = calcrc_byte(crc_val^*data_buf++);
	}
	return crc_val;
}

/*******************************************************************************
                                      END         
*******************************************************************************/




