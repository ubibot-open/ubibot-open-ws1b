/*******************************************************************************
  * @file       Power Voltage Application Task
  * @author 
  * @version
  * @date 
  * @brief
  ******************************************************************************
  * @attention
  *
  *
*******************************************************************************/
#ifndef PWR_ADC_H
#define PWR_ADC_H

/*-------------------------------- Includes ----------------------------------*/

/**
 * @brief  Initializes ADC1, takes multiple samples on the specified channel and averages them, converts the average to a
 *         calibrated voltage, then scales it by the voltage-divider resistor ratio to recover the actual supply voltage
 * @return Computed supply voltage, in volts (V)
 */
extern float power_adcValue(void);

#endif  //PWR_ADC_H

/*******************************************************************************
                                      END         
*******************************************************************************/




