#include "gpio.h"
#include "spi.h"
#include "main.h"
#include "board.h"


void bsp_board_init(void)
{
bsp_gsm_pwr_key_release();  
 
}


/*压缩机控制*/
void bsp_compressor_ctrl_on(void)
{
  HAL_GPIO_WritePin(BSP_COMPRESSOR_CTRL_POS_GPIO_Port, BSP_COMPRESSOR_CTRL_POS_Pin, BSP_CTRL_COMPRESSOR_ON); 
}
void bsp_compressor_ctrl_off(void)
{
  HAL_GPIO_WritePin(BSP_COMPRESSOR_CTRL_POS_GPIO_Port, BSP_COMPRESSOR_CTRL_POS_Pin, BSP_CTRL_COMPRESSOR_OFF); 
}

/*蜂鸣器控制*/
void bsp_buzzer_ctrl_on(void)
{
  HAL_GPIO_WritePin(BSP_BUZZER_CTRL_POS_GPIO_Port, BSP_BUZZER_CTRL_POS_Pin, BSP_CTRL_BUZZER_ON); 
}
void bsp_buzzer_ctrl_off(void)
{
  HAL_GPIO_WritePin(BSP_BUZZER_CTRL_POS_GPIO_Port, BSP_BUZZER_CTRL_POS_Pin, BSP_CTRL_BUZZER_OFF); 
}

/*GSM PWR控制*/
void bsp_gsm_pwr_key_press(void)
{
HAL_GPIO_WritePin(BSP_2G_PWR_CTRL_POS_GPIO_Port, BSP_2G_PWR_CTRL_POS_Pin, BSP_GSM_PWR_ON);   
}

void bsp_gsm_pwr_key_release(void)
{
HAL_GPIO_WritePin(BSP_2G_PWR_CTRL_POS_GPIO_Port, BSP_2G_PWR_CTRL_POS_Pin, BSP_GSM_PWR_OFF);   
}

bsp_gsm_pwr_status_t bsp_get_gsm_pwr_status(void)
{
return HAL_GPIO_ReadPin(BSP_2G_STATUS_POS_GPIO_Port,BSP_2G_STATUS_POS_Pin) == GPIO_PIN_SET ? BSP_GSM_STATUS_PWR_ON : BSP_GSM_STATUS_PWR_OFF;
}

/*tm1629a cs控制*/
void bsp_tm1629a_cs_ctrl_set(void)
{
  HAL_GPIO_WritePin(BSP_TM1629A_CS_POS_GPIO_Port, BSP_TM1629A_CS_POS_Pin, BSP_TM1629A_CS_SET); 
}
void bsp_tm1629a_cs_ctrl_clr(void)
{
  HAL_GPIO_WritePin(BSP_TM1629A_CS_POS_GPIO_Port, BSP_TM1629A_CS_POS_Pin, BSP_TM1629A_CS_CLR); 
}

/*tm1629a interface*/
void bsp_tm1629a_write_byte(uint8_t byte)
{
 HAL_SPI_Transmit(&hspi2,&byte,1,0xff); 
}
uint8_t bsp_tm1629a_read_byte(void)
{
return 0;
}
