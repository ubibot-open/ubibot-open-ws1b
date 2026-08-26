/*******************************************************************************
  * @file       CRC Check Driver       
  * @author 
  * @version
  * @date 
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/
#ifndef __CRC_CHECK_H__
#define __CRC_CHECK_H__

/*-------------------------------- Includes ----------------------------------*/
#include "stdint.h"

/**
 * @brief  Performs a CRC-8 check on a data buffer to verify whether the transmitted data is correct (the data typically includes the CRC value computed by the sender at the end, which is included in the check).
 * @param  data Pointer to the buffer holding the data to check (input).
 * @param  bytes Number of bytes included in the check.
 * @return The computed CRC-8 check result; 0 means the data passed the check, non-zero means the data has an error.
 */
extern uint8_t Data_Crc_Check(uint8_t *data,uint8_t bytes);

/**
 * @brief  Computes the CRC-8 checksum of a data buffer (polynomial 0x131).
 * @param  data Pointer to the buffer holding the data to compute (input).
 * @param  bytes Number of bytes included in the computation.
 * @return The computed CRC-8 checksum value.
 */
extern uint8_t Data_Crc_Value(uint8_t *data,uint8_t bytes);

/**
 * @brief  specific to the ub_dt_p1 protocol; internally accumulates the result byte-by-byte using a bit-reversed CRC algorithm.
 * @param  data_buf Pointer to the buffer holding the data to compute (input).
 * @param  bytes Number of bytes to compute.
 * @return The computed CRC check value.
 */
extern uint8_t calcrc_bytes(uint8_t *data_buf,uint8_t bytes);

#endif //  __CRC_CHECK_H__

/*******************************************************************************
                                      END         
*******************************************************************************/




