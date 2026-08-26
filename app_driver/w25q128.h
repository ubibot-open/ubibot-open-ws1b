/*******************************************************************************
  * @file       W25Q128 NOR FLASH CHIP DRIVER APPLICATION      
  * @author 
  * @version
  * @date 
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/
#ifndef W25Q128_H
#define W25Q128_H

/*-------------------------------- Includes ----------------------------------*/
#include "stdint.h"

#define W25Q80                  0xef13
#define W25Q16                  0xef14
#define W25Q32                  0xef15
#define W25Q64                  0xef16
#define W25Q128                 0xef17

#define READ_STATUS_REGISTER    0x05
#define WRITE_STATUS_REGISTER   0x10
#define WRITE_ENABLE            0x06
#define W25Q_DEVICE_ID          0x90
#define READ_DATA               0x03
#define PAGE_PROGRAM            0x02
#define SECTOR_ERASE            0x20
#define CHIP_ERASE              0xc7
#define POWER_DOWN              0xb9
#define RELEASE_POWER_DOWN      0xab

/**
 * @brief  Sends a read-register command to the w25q128 over SPI and reads back the register value
 * @param  com_val Read-register command byte to send
 * @return The register value read back
 */
extern uint8_t w25q_ReadReg(uint8_t com_val);

/**
 * @brief  Reads the manufacturer ID and device ID of the w25q128
 * @return 16-bit chip ID, with the manufacturer ID in the high byte and the device ID in the low byte
 */
extern uint16_t w25q_ReadId(void);

/**
 * @brief  Sends a single-byte command (with no accompanying data) to the w25q128 over SPI
 * @param  com_val Command byte to send
 */
extern void w25q_WriteCommand(uint8_t com_val);

/**
 * @brief  Writes one byte of data to the specified w25q128 register; enables write mode beforehand and waits for the operation to complete afterward
 * @param  com_val Write-register command byte
 * @param  value   Data to write to the register
 */
extern void w25q_WriteReg(uint8_t com_val,uint8_t value);

/**
 * @brief  Erases the 4KB subsector of the w25q128 that contains the specified address; enables write mode before erasing and waits for the operation to complete afterward
 * @param  addr 24-bit address within the subsector to be erased
 */
extern void w25q_EraseSubsector(uint32_t addr);

/**
 * @brief  Erases all data on the entire w25q128 chip; enables write mode before erasing and waits for the operation to complete afterward
 */
extern void w25q_EraseChip(void);

/**
 * @brief  Sends the release-power-down command to wake the w25q128 from power-down mode
 */
extern void w25q_WakeUp(void);

/**
 * @brief  Sends the power-down command to put the w25q128 into low-power power-down mode
 */
extern void w25q_PowerDown(void);

/**
 * @brief  Reads a number of consecutive bytes from the w25q128 starting at the specified address
 * @param  addr   Starting read address (24-bit)
 * @param  buffer Output parameter; buffer that receives the data read
 * @param  size   Number of bytes to read
 */
extern void w25q_Read_Data(uint32_t addr,char *buffer,uint16_t size);

/**
 * @brief  Writes data of arbitrary length to the specified address on the w25q128, automatically splitting the write into multiple page writes at page boundaries (no pre-erase check is performed)
 * @param  addr   Starting write address (24-bit)
 * @param  buffer Buffer holding the data to write
 * @param  Size   Total number of bytes to write
 */
extern void w25q_Write_Data(uint32_t addr,char *buffer,uint16_t Size);

#endif //  W25Q128_H

/*******************************************************************************
                                      END         
*******************************************************************************/




