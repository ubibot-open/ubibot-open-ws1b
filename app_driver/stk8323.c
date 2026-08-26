/*******************************************************************************
  * @file       stk8323 Sensor Application      
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
#include "stdio.h"
#include "stk8323.h"
#include "iic.h"
#include "math.h"
#include "MsgType.h"

uint8_t stk8323_chipid = 0;

/**
 * @brief  Reads data from the specified STK8323 register(s) over I2C
 * @param  reg  Starting register address
 * @param  data Output parameter; buffer that receives the data read
 * @param  len  Number of consecutive registers to read
 */
void stk8323_read_reg(uint8_t reg,uint8_t *data,uint8_t len)
{
  IIC_Read_Buf(STK8323_ADDR,reg,data,len);
}

/**
 * @brief  Writes data to the specified STK8323 register(s) over I2C
 * @param  reg  Starting register address
 * @param  data Buffer holding the data to write to the register(s)
 * @param  len  Number of consecutive registers to write
 */
void stk8323_write_reg(uint8_t reg,uint8_t *data,uint8_t len)
{
  IIC_Write_Buf(STK8323_ADDR,reg,data,len);
}

/**
 * @brief  Reads the STK8323 chip ID register (used to verify device identity)
 * @param  buff Output parameter; buffer that receives the chip ID read
 */
void stk8323_device_id_get(uint8_t *buff)
{
  stk8323_read_reg(STK832X_REG_CHIPID,buff,1);
}

/**
 * @brief  Sends a soft-reset command to the STK8323 and delays to wait for the reset to complete
 */
void stk8323_reset(void)
{
	uint8_t reg_val;
	reg_val = STK832X_SWRST_VAL;
	stk8323_write_reg(STK832X_REG_SWRST,&reg_val,1); //
	ets_delay_ms(50);
}

/**
 * @brief  Soft-resets the STK8323 and then sets it to suspend (low-power) mode, used to deinitialize the sensor
 */
void stk8323_uinit(void)
{
	uint8_t reg_val;
	stk8323_reset(); /* soft-reset */  
	/* set power mode */
	reg_val = STK832X_PWMD_SUSPEND;	// low-power mode
	stk8323_write_reg(STK832X_REG_POWMODE,&reg_val,1); //
}

/**
 * @brief  Initializes the STK8323 accelerometer: verifies the chip ID, performs a soft reset, then configures the
 *         range, bandwidth, low-power mode, watchdog, data settings, slope-detection threshold, step-counter enable,
 *         and any-motion interrupt (INT1) registers
 */
void stk8323_init(void)
{
	uint8_t reg_val;
	stk8323_read_reg(STK832X_REG_CHIPID,&stk8323_chipid,1);
  
	if(stk8323_chipid == STK8323_ID)
	{
		stk8323_reset(); /* soft-reset */       

		/* set range, resolution */
		reg_val = STK832X_RANGESEL_4G;                  /*  4G   */
		stk8323_write_reg(STK832X_REG_RANGESEL,&reg_val,1); 

	 	/* set bandwidth */
		reg_val = STK832X_BWSEL_BW_125;  /* 125 Hz */ 
		stk8323_write_reg(STK832X_REG_BWSEL,&reg_val,1); 
     
		/* set Low_Power mode */
		// reg_val = STK832X_PWMD_LOWPOWER;  //Low_Power, sleep duration = 0ms EDM mode
		reg_val = 0x7A;  // Low_Power,sleep duration = 100ms  ESM mode
		stk8323_write_reg(STK832X_REG_POWMODE,&reg_val,1); //
                
		/* set i2c watch dog */
		reg_val =  STK832X_INTFCFG_I2C_WDT_EN;  // enable watch dog  Watchdog timer period 1ms
		stk8323_write_reg(STK832X_REG_INTFCFG,&reg_val,1); //
                                            
		 reg_val = 0x40;  /* set : Disable the data protection function.  Data output filtered */
   		stk8323_write_reg(STK832X_REG_DATASETUP,&reg_val,1); //  
                                                                  
		/* set the number of samples needed in slope detection */
		reg_val = 0x00;
		stk8323_write_reg(STK832X_REG_SLOPEDLY,&reg_val,1); //
   		
		reg_val = 0x14;
		stk8323_write_reg(STK832X_REG_SLOPETHD,&reg_val,1); //
                               
		reg_val = STK832X_STEPCNT2_STEP_CNT_EN;
		stk8323_write_reg(STK832X_REG_STEPCNT2,&reg_val,1); 
                
		reg_val = 0x07;  /*  set  0 SLP_EN_X,Y,Z any-motion (slope) interrupt*/
		stk8323_write_reg(STK832X_REG_INTEN1,&reg_val,1); //
        
		/* INT1 int config */
		reg_val = 0x01;  //Int PIN 1/Int PIN2   Active high
		stk8323_write_reg(STK832X_REG_INTCFG1,&reg_val,1); //
                                   
//		reg_val = 0x00;  /* non -latched  */
		reg_val = 0x03;  /*temporary, 1s */
//		reg_val = 0x07;  /* latched  */
		stk8323_write_reg(STK832X_REG_INTCFG2,&reg_val,1); //
                          
	 	reg_val = 0x04;  /* ANY_MOT_EN：Enable any-motion */
		//reg_val = 0x02;  /*Enable significant motion*/
	 	stk8323_write_reg(STK832X_REG_SIGMOT2,&reg_val,1); //

	 	 /* INT1 pin */
	 	reg_val = 0x0F; 
	 	stk8323_write_reg(STK832X_REG_INTMAP1,&reg_val,1); 

		/*INT pin  */
		reg_val = 0x00;  //enable Map FIFO full interrupt to INT1
		stk8323_write_reg(STK832X_REG_INTMAP2,&reg_val,1); 
	}
	else
	{
#ifdef DEBUG_MODE
		printf("stk8323 CHIP_ID ERR");
#endif 
	}
}

/**
 * @brief  Reads the current step count from the STK8323's internal step counter
 * @return Current cumulative step count
 */
int stk8323_get_step(void) 
{
	int counter;
	uint8_t cnt_l,cnt_h;     
	stk8323_read_reg(STK832X_REG_STEPOUT1,&cnt_l,1);
	stk8323_read_reg(STK832X_REG_STEPOUT2,&cnt_h,1);
	counter = cnt_h*256 + cnt_l;
#ifdef STK8323_DEBUG
	printf("counter= %d  cnt_l=%d  cnt_h=%d ",counter,cnt_l,cnt_h);
#endif
	return counter;
}

/**
 * @brief  Clears the STK8323's internal step counter
 */
void stk8323_Steps_Clear(void) 
{
	uint8_t reg_val;
	reg_val = 0x0C;
	stk8323_write_reg(STK832X_REG_STEPCNT2,&reg_val,1); //
}

/**
 * @brief  Reads the raw X/Y/Z-axis acceleration data from the STK8323 (12-bit effective resolution, with sign extension and right-shift already applied)
 * @param  X_DataOut Output parameter; receives the X-axis acceleration data
 * @param  Y_DataOut Output parameter; receives the Y-axis acceleration data
 * @param  Z_DataOut Output parameter; receives the Z-axis acceleration data
 */
void stk832x_getdata(short int *X_DataOut, short int *Y_DataOut, short int *Z_DataOut)
{
	unsigned char  RegReadValue[6]={0};	
  	stk8323_read_reg(STK832X_REG_XOUT1, (uint8_t*)RegReadValue, 6);
	*X_DataOut = (short int)((((int)((char)RegReadValue[1])) << 8) | (RegReadValue[0] & 0xF0)) >> 4;  //resolution = 12 bit
	*Y_DataOut = (short int)((((int)((char)RegReadValue[3])) << 8) | (RegReadValue[2] & 0xF0)) >> 4;  //resolution = 12 bit
	*Z_DataOut = (short int)((((int)((char)RegReadValue[5])) << 8) | (RegReadValue[4] & 0xF0)) >> 4;  //resolution = 12 bit
}

/**
 * @brief  Reads the STK8323 interrupt status register 1 (INTSTS1) to get the currently triggered interrupt status
 * @return Value of the interrupt status register
 */
uint8_t stk8323_intstatus(void)
{
   uint8_t reg_val;
   stk8323_read_reg(STK832X_REG_INTSTS1,&reg_val,1);  // 
#ifdef STK8323_DEBUG
   printf("REG_INTSTS1= %x\n  REG_INTSTS2=%x\r\n  ",REG_INTSTS1,REG_INTSTS2);
#endif  
   return reg_val;
}

/*******************************************************************************
                                      END         
*******************************************************************************/



