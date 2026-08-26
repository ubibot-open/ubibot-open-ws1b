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

/*-------------------------------- Includes ----------------------------------*/
#include "string.h"
#include "stdlib.h"
#include "osi.h"
#include "user_spi.h"
#include "w25q128.h"
#include "MsgType.h"
#include "esp_log.h"

#define TAG "w25q128"

/**
 * @brief  Sends a read-register command to the w25q128 over SPI and reads back the register value
 * @param  com_val Read-register command byte to send
 * @return The register value read back
 */
uint8_t w25q_ReadReg(uint8_t com_val)
{
  SET_SPI1_CS_OFF();  //
  SPI_SendReciveByte(com_val);  //send the read command
  uint8_t value=SPI_SendReciveByte(0x00);  //clk signal,get register value
  SET_SPI1_CS_ON();  //
  return value;
}

/**
 * @brief  Reads the manufacturer ID and device ID of the w25q128
 * @return 16-bit chip ID, with the manufacturer ID in the high byte and the device ID in the low byte
 */
uint16_t w25q_ReadId(void)
{
  uint16_t w25qid=0;
  SET_SPI1_CS_OFF();  //
  SPI_SendReciveByte(W25Q_DEVICE_ID);  //send the read manufacturer id
  SPI_SendReciveByte(0x00);  //recive data
  SPI_SendReciveByte(0x00);  //recive data
  SPI_SendReciveByte(0x00);  //recive data
  w25qid=SPI_SendReciveByte(0x00);  //recive data
  w25qid=w25qid<<8;
  w25qid+=SPI_SendReciveByte(0x00);  //recive data
  SET_SPI1_CS_ON();  //
  ESP_LOGI(TAG, "%d,%04x", __LINE__,w25qid);
  return w25qid;
}

/**
 * @brief  Sends a single-byte command (with no accompanying data) to the w25q128 over SPI
 * @param  com_val Command byte to send
 */
void w25q_WriteCommand(uint8_t com_val)
{
  SET_SPI1_CS_OFF();  //
  SPI_SendReciveByte(com_val);  //send the write command
  SET_SPI1_CS_ON();  //
}

/**
 * @brief  Polls the w25q128 status register waiting for a write/erase operation to finish (busy flag cleared), timing out after roughly 1 minute
 */
static void w25q_WaitCompleted(void)
{
  uint16_t retry=0; 
  while(w25q_ReadReg(READ_STATUS_REGISTER)&0x01) //read status register,bit0=0:Ready,bit0=1:Busy
  {
    if(retry++>6000) break; //time out 1min
    osi_Sleep(10);
  }
}

/**
 * @brief  Writes one byte of data to the specified w25q128 register; enables write mode beforehand and waits for the operation to complete afterward
 * @param  com_val Write-register command byte
 * @param  value   Data to write to the register
 */
void w25q_WriteReg(uint8_t com_val,uint8_t value)
{
  w25q_WriteCommand(WRITE_ENABLE);  //write enable
  
  SET_SPI1_CS_OFF();  //
  SPI_SendReciveByte(com_val);  //send the write register command
  SPI_SendReciveByte(value);     //write register value
  SET_SPI1_CS_ON();  //
  
  w25q_WaitCompleted();  //waite command completed
}

/**
 * @brief  Erases the 4KB subsector of the w25q128 that contains the specified address; enables write mode before erasing and waits for the operation to complete afterward
 * @param  addr 24-bit address within the subsector to be erased
 */
void w25q_EraseSubsector(uint32_t addr)
{
  w25q_WriteCommand(WRITE_ENABLE);  //write enable
  
  SET_SPI1_CS_OFF();  //
  SPI_SendReciveByte(SECTOR_ERASE);  //subsector eaase code
  SPI_SendReciveByte((uint8_t)((addr)>>16));  //24bit address first 8 bit addres
  SPI_SendReciveByte((uint8_t)((addr)>>8));
  SPI_SendReciveByte((uint8_t)(addr));  //24bit address last 8 bit address
  SET_SPI1_CS_ON();  //
  
  w25q_WaitCompleted();  //wait command completed
}

/**
 * @brief  Erases all data on the entire w25q128 chip; enables write mode before erasing and waits for the operation to complete afterward
 */
void w25q_EraseChip(void)
{
  w25q_WriteCommand(WRITE_ENABLE);  //write enable
  w25q_WriteCommand(CHIP_ERASE);  //chip eaase code
  w25q_WaitCompleted();  //wait command completed
}

/**
 * @brief  Sends the release-power-down command to wake the w25q128 from power-down mode
 */
void w25q_WakeUp(void)
{
  w25q_WriteCommand(RELEASE_POWER_DOWN);  //release power down code
}

/**
 * @brief  Sends the power-down command to put the w25q128 into low-power power-down mode
 */
void w25q_PowerDown(void)
{
  w25q_WriteCommand(POWER_DOWN);  //power down code
}

/**
 * @brief  Reads a number of consecutive bytes from the w25q128 starting at the specified address
 * @param  addr   Starting read address (24-bit)
 * @param  buffer Output parameter; buffer that receives the data read
 * @param  size   Number of bytes to read
 */
void w25q_Read_Data(uint32_t addr,char *buffer,uint16_t size)
{
  SET_SPI1_CS_OFF();  //
  SPI_SendReciveByte(READ_DATA);  //read operations code 
  SPI_SendReciveByte((uint8_t)((addr)>>16));   //24bit address first 8 bit address
  SPI_SendReciveByte((uint8_t)((addr)>>8));
  SPI_SendReciveByte((uint8_t)(addr));  //24bit address last 8 bit address
  for(uint16_t i=0;i<size;i++)
  {
    buffer[i]=SPI_SendReciveByte(0x00);	//read byte
  }
  SET_SPI1_CS_ON();  //
}

/**
 * @brief  Writes up to one page (256 bytes) of data to the specified address within a w25q128 page; enables write mode beforehand and waits for the operation to complete afterward
 * @param  addr   Starting write address within the page (24-bit)
 * @param  buffer Buffer holding the data to write
 * @param  Size   Number of bytes to write; must not exceed the remaining space in the page
 */
static void w25q_WritePage(uint32_t addr,char *buffer,uint8_t Size)
{
  w25q_WriteCommand(WRITE_ENABLE);  //write enable
  
  SET_SPI1_CS_OFF();  //
  SPI_SendReciveByte(PAGE_PROGRAM);  //page program code
  SPI_SendReciveByte((uint8_t)(addr>>16));   //24bit address first 8 bit address
  SPI_SendReciveByte((uint8_t)(addr>>8));
  SPI_SendReciveByte((uint8_t)addr);	//24bit address last 8 bit address
  for(uint16_t i=0;i<Size;i++)
  {
    SPI_SendReciveByte(buffer[i]);  //write data
  }
  SET_SPI1_CS_ON();  //

  w25q_WaitCompleted();  //w25q128 wait command completed
}

/**
 * @brief  Writes data of arbitrary length to the specified address on the w25q128, automatically splitting the write into multiple page writes at page boundaries (no pre-erase check is performed)
 * @param  addr   Starting write address (24-bit)
 * @param  buffer Buffer holding the data to write
 * @param  Size   Total number of bytes to write
 */
void w25q_Write_Data(uint32_t addr,char *buffer,uint16_t Size)
{
  uint16_t i,n_i;
  uint16_t remain;
  
  n_i=Size/256+2;
  remain=256-addr%256;  //page remain byte number
  remain=remain>Size?Size:remain;

  for(i=0;i<=n_i;i++)
  {
    if(remain>0)
    {
      w25q_WritePage(addr,buffer,remain);  //
    } 
    if(remain==Size)
    {
      break;
    }
    
    buffer+=remain;
    addr+=remain;
    Size-=remain;
    remain=Size>256?256:Size;
  }
}

/*******************************************************************************
                                      END         
*******************************************************************************/




