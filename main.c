/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * REV 1.6 TESTING
  *
  * author Grant Smith, grantsmith6261@gmail.com 
  * I2C master TX works, added pin toggle for 100 loops at start
  * new CubeMX inits, clock now 4MHz, I2C int enabled
  * Slaves on I2C
  * MAX31875 Temp sensors at Addr 0x90 and 0x91
  * ST25DV04K NFC Tag at Addr 0xA7 for user memory
  * TBD iF PB0 high then RF is talking to the Tag and hold up I2C
  * TAG GPO output level is controlled by Manage GPO Command (set/reset)
  * in the RF control area
  * uP can poll or ITR on GPO -PBO going high (set)to start I2C process
 
  *
  * ALL Writes are read back to check if it worked and TBD compared
  * 1) WRITE the MAX31875 config reg 01 with 0x0040 - default --works
  * 2) READ the MAX31875 config reg --works
  * 3) WRITE Reg 01 with 0x0140 for Conversion shutdown and wait --works
  * 4) READ the MAX31875 config reg  --works
  * While {1} loop for ever
  * 5) WRITE the config reg with 0x0141 - start a conversion --works
  * 6) READ the MAX31875 config reg
  * 7) Wait 70ms for conversion to complete
  * 8) READ the 16-bit Temperature Reg at 0x00, 10bits are temp value --works
  * 8+) send 0140 to MAX to put it into shutdown
  * ######## get aRXBuffer4 [0] and [1]into aTXBuffer5 ############### --works
  * 9) Write EEPROM Addr 0000h with aTXBuffer5 --works
  * 10) READ the  2B temp value from Tag EEPROM at Block 00 
  *  I2C addr  0001h,0000h  --works
  *  with 01 being the high byte and 00 being the low byte
  *  when the other sensor is added, write its temp to block 00
  *  I2C addr 0005h, 0004h


  *  next steps TBD 
  *- fix buffer data move 
  * -- could add compare on W and Reads on MAX if dont match- error
  * look at doing boot code only in flashand get uP image
  * from RF download (may need larger memory tag) 4Kbit now
  * up to 64k bit or 8K Bytes is the biggest
  * Boot routine grabs the image from the tag and runs it
  *
  ***********************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "MAX31875.h"
#include "stm32l0xx_hal_gpio.h"

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

uint16_t Temp;  // better if these are global
uint16_t Count;
uint16_t Val_g;
uint16_t Val;
uint8_t MAX_Addr; // declares only - global
uint8_t TAG_Addr; // will use 7bit addresses

  /* Buffer used for transmission - now local*/
  /* MAX31875 only has 16b Registers, TAG has 8b registers */
  uint8_t aTxBuffer1[] = {0x01,0x00,0x40,0x00} ;  // Write 0x0040 to Config reg at 0x01
  uint8_t aTxBuffer2[] = {0x01,0x01,0x40,0x00} ;  // Write 0x0140 , D8 set, shutdown and wait
  uint8_t aTxBuffer3[] = {0x01,0x01,0x41,0x00} ;  // Write 0x0141 , D0 set to start a conversion
  uint8_t aTxBuffer4[] = {0x00,0x00,0x00,0x00} ;  // READ MAX31875_R0 temp reg  - WORKS 
  uint8_t aTxBuffer5[] = {0x00,0x00,0x00,0x00} ;  // READ Tag eeprom first 2B are Reg Addr 0000h
                                                     // 2B, for write send D_H first and D_L second

  /* Buffersizes used for reception */
  uint8_t aRxBuffer1[4] = {0,0,0,0}; //only need 2 bytes for Config Reg read
  uint8_t aRxBuffer2[4] = {0,0,0,0}; //
  uint8_t aRxBuffer3[4] = {0,0,0,0}; //
  uint8_t aRxBuffer4[4] = {0,0,0,0}; //
  uint8_t aRxBuffer5[4] = {0,0,0,0}; //Read 1  byte at a time Tag eeprom or seq

  
  
  
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);

int main(void)
{

  //HAL_StatusTypeDef ret;
  
  //  use for PW to the TAG EEPROM in ST25DV04K, 8b registers if locked
  //  uint8_t aTxBufferTagpw[8]  = {0,0,0,0,0,0,0,0};  //use to send PW
  //  uint8_t aRxBufferTagpw[8] = {0,0,0,0,0,0,0,0}; //first block of TAG , Block 00, 8 bytes

  static const uint8_t MAX_Addr = 0x90;    //works -Acked
  static const uint8_t TAG_Addr  = 0xA7;   //WORKS - ACKED

  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();

  
  // toggle test points at GPIO8 and GPIO9
  for (Count = 0; Count < 100; Count++)
    { HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_9); // test points on flex bd
      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8); // toggles at 37KHz for 100 loops until >100
    }
    
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_RESET); 
      

   for (Temp = 0; Temp < 2; Temp++) 
   {  
      
     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);
     Val_g = Val_g +2;  // this allows VAl_g and Val to be "watched"
     Val = Val + Val_g ;
   }
      
     Count = 0;
     Temp = 0;
     Val_g = 0;
     Val = 0;
     
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET);
    { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_RESET);  // It is low now
    }
       
     

    // 1) WRITE the MAX31875 config reg at 0x01 with 0040, Default
    if(HAL_I2C_Master_Transmit_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t*)aTxBuffer1, 3)!= HAL_OK)
    {        
     Error_Handler();
    }
      
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    }  
   
    //Insert delay 1 ms then do the read
    HAL_Delay(1);   //
  
  
    // 2) Start READ/ VERIFY process-
    if(HAL_I2C_Master_Transmit_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t*)aTxBuffer1, 1)!= HAL_OK) 
    {
     Error_Handler();
    }
      
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    } 
    
    // READ config reg, recieve 2 bytes
    if(HAL_I2C_Master_Receive_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t *)aRxBuffer1, 2) != HAL_OK)
    {
      Error_Handler();
    }
          
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    } 

    while(HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF);
    {
    } 
    
    HAL_Delay(1);
    
    // 3) WRITE the MAX31875 config reg with 0x0140, D8 set, MAX31875 shutdown and wait
    if(HAL_I2C_Master_Transmit_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t*)aTxBuffer2, 3)!= HAL_OK)
    {        
     Error_Handler();
    }
      
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    }  
   
    HAL_Delay(1);   //
    
    // 4) Start READ/ VERIFY process- send one byte as the reg pointer
    if(HAL_I2C_Master_Transmit_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t*)aTxBuffer2, 1)!= HAL_OK) 
    {
     Error_Handler();
    }
      
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    } 
    
    // READ config reg, recieve 2 bytes
    if(HAL_I2C_Master_Receive_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t *)aRxBuffer2, 2) != HAL_OK)
    {
      Error_Handler();
    }
          
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    } 

    while(HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF);
    {
    } 
    
    HAL_Delay(1);
    
   // ##################################################
    while (1)  //loop forever  
  {

    // 5) WRITE the MAX31875 config reg with 0x0141, D8 and D0 set, MAX31875 start a 10bit conversion
    if(HAL_I2C_Master_Transmit_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t*)aTxBuffer3, 3)!= HAL_OK)
    {        
     Error_Handler();
    }
      
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    }  
   
    HAL_Delay(1);   //
    
    // 6) Start READ/ VERIFY process- send one byt, the reg pointer 00
    if(HAL_I2C_Master_Transmit_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t*)aTxBuffer3, 1)!= HAL_OK) 
    {
     Error_Handler();
    }
      
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    } 
    
    if(HAL_I2C_Master_Receive_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t *)aRxBuffer3, 2) != HAL_OK)
    {
      Error_Handler();
    }
          
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    } 

    while(HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF);
    {
    } 
    
    // 7) wait 70ms
      HAL_Delay(70);
      
   // 8) READ the 16-bit Temperature Reg at 0x00, 10bits resolution D14 to D6 are temp value, D15 is sign bit
    if(HAL_I2C_Master_Transmit_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t*)aTxBuffer4, 1)!= HAL_OK) 
    {
     Error_Handler();
    }
      
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    } 
    
    if(HAL_I2C_Master_Receive_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t *)aRxBuffer4, 2) != HAL_OK)
    {
      Error_Handler();
    }
          
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    } 

    while(HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF);
    {
    } 
    
    HAL_Delay(1);
  // shutdown the MAX31875 so current  goes to 1uA  
  // 1) again WRITE the MAX31875 config reg at 0x01 with 0140, set SD bit D8
    if(HAL_I2C_Master_Transmit_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t*)aTxBuffer2, 3)!= HAL_OK)
    {        
     Error_Handler();
    }
      
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    }  
   
    //Insert delay 1 ms then do the read
    HAL_Delay(1);   //
  
  
    // 2) Again Start READ/ VERIFY process-
    if(HAL_I2C_Master_Transmit_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t*)aTxBuffer2, 1)!= HAL_OK) 
    {
     Error_Handler();
    }
      
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    } 
    
    // READ config reg, recieve 2 bytes
    if(HAL_I2C_Master_Receive_IT(&hi2c1, (uint8_t)MAX_Addr, (uint8_t *)aRxBuffer2, 2) != HAL_OK)
    {
      Error_Handler();
    }
          
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    } 

    while(HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF);
    {
    } 
 
    // aRxBuffer still has the temp value

    HAL_Delay(1);
    
  // for temp calc, left shift 16bit temp registers by 8 to get temp in C
  // or left shit 6 and make a float with shifted val / 4 to get 0.25C resolution
  // eg. 17FF left shifted by 6 is 005F when converted to dec is 95 which when div by 4 as a float
  // gives 23.75C
  
    // uint8_t val = (uint8_t *)aRxBuffer4[0];  //doesnt work
    
   aTxBuffer5[2] = aRxBuffer4[0];
   aTxBuffer5[3] = aRxBuffer4[1];

   // WRITE the REAL  temp values into the TAG --works
     if(HAL_I2C_Master_Transmit_IT(&hi2c1, (uint8_t)TAG_Addr, (uint8_t*)aTxBuffer5, 4)!= HAL_OK)
    {        
     Error_Handler();
    }
      
    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
    {
    }  
    
    
    HAL_Delay(5);  //wait 5ms and READ tag (flash write delay in Tag is 5ms)
   
     // we may need to send a PW for write access to area 1 if protected
     // Area 1 is always readable from RF and I2C, Reg Addresses are 16bit
     // All passwords are 64-bits long
     // the default factory passwords value is 0000000000000000h.
     // Factory default - areas except config reg's not protected
     // I2C host doesn’t have read or write access to RF passwords
     // RF doesn’t have read or write access to I2C password.
   
   // 9) Send REG ptr 0x0000 - for the READ
   if  (HAL_I2C_Master_Transmit_IT(&hi2c1, (uint8_t)TAG_Addr, (uint8_t*)aTxBuffer5, 2)!= HAL_OK)  // send 0000h
    {        
     Error_Handler();
    }
      
   while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
   {
   } 
 
   // finish READ , could read 4Bytes in Block 00 -- works
   if  (HAL_I2C_Master_Receive_IT(&hi2c1, (uint8_t)TAG_Addr, (uint8_t *)aRxBuffer5, 2)!= HAL_OK)  //I2C access is one byte at a time spaced in 5ms
    {        
     Error_Handler();
    }

   while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY)
   {
   } 
   
   while(HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF);
   {
   } 
   
   HAL_Delay(25);
   
   
  
  }

}

/* ########### end of main ##################################*/

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  Power scale 3 (1.2V Core) good for up to 4.2MHz */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);  //  4.2MHz max
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;  //4MHz
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00100E16;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9|GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA9 PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    
  __disable_irq();  // not sure if this helps?
  while (1)
  {
   HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_9); // test points on flex bd
   HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8); // toggles at 37KHz with no delay
  }
  // 
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************  *****END OF FILE****/
