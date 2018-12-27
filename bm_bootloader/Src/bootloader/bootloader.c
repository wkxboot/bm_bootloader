#include "main.h"
#include "stdbool.h"
#include "led.h"
#include "flash_utils.h"
#include "bootloader_if.h"
#include "bootloader.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[bootloader]"

/*环境参数*/
static bootloader_env_t env;


/*名称：bootloader
* 功能：bootloader
* 参数：无
* 返回：无
*/
void bootloader(void)
{
 int rc;
 /*等待显示芯片上电稳定*/;
 HAL_Delay(1000);
 
 log_debug("\r\n*************************************************************\r\n"
           "\r\n  BOOTLOADER VER:%s     build date:%s %s \r\n"
           "\r\n*************************************************************\r\n"
           ,BOOTLOADER_VERSION,__DATE__,__TIME__);
 
 led_display_init();
 
 led_display_temperature_unit(LED_DISPLAY_ON);
 led_display_pressure_unit(LED_DISPLAY_ON);
 led_display_capacity_unit(LED_DISPLAY_ON);
 
 led_display_temperature_icon(LED_DISPLAY_ON);
 led_display_pressure_icon(LED_DISPLAY_ON);
 
 led_display_capacity_icon_frame(LED_DISPLAY_ON);
 
 led_display_wifi_icon(3);
 led_display_compressor_icon(LED_DISPLAY_ON,LED_DISPLAY_ON);
 led_display_brand_icon(LED_DISPLAY_ON);
 led_display_capacity_icon_level(5);

 led_display_temperature(BOOTLOADER_INIT_DISPLAY_VALUE);
 led_display_pressure(BOOTLOADER_INIT_DISPLAY_VALUE);
 led_display_capacity(BOOTLOADER_INIT_DISPLAY_VALUE);
 
  /*刷新到芯片*/
 led_display_refresh();
 /*解除写保护*/
 if(bootloader_disable_wr_protection() != 0){
    goto err_exit;   
  }
  
 if(bootloader_init() != 0){
    goto err_exit;   
  }
 
  /*读取当前env*/ 
  log_debug("bootloader read env.\r\n");
  rc = bootloader_get_env(&env); 
  /*执行失败 重启*/
  if(rc != 0){
     goto err_exit;        
  }
  
  /*正常启动程序*/
  if(env.boot_flag == BOOTLOADER_FLAG_BOOT_NORMAL){
     log_debug("bootloader no update.boot normal.\r\n");
     bootloader_boot_user_application(); 
   }
 
  /*需要更新程序*/
  if(env.boot_flag == BOOTLOADER_FLAG_BOOT_UPDATE){
     log_debug("bootloader need update.\r\n");
     if(bootloader_update_user_app(&env) != 0){
        goto err_exit;
     }
     log_debug("done.\r\n");
     /*执行用户程序*/
     bootloader_boot_user_application(); 
  }
 
  /*升级成功，设置启动标志为正常*/
  if(env.boot_flag == BOOTLOADER_FLAG_BOOT_UPDATE_OK){
     log_debug("bootloader set boot flag to normal.\r\n");
     env.boot_flag = BOOTLOADER_FLAG_BOOT_NORMAL;

     if(bootloader_save_env(&env) != 0){
        goto err_exit; 
      }
     log_debug("done.\r\n");
     /*执行用户程序*/
     bootloader_boot_user_application(); 
   }
  
  /*升级失败恢复原程序*/
  if(env.boot_flag == BOOTLOADER_FLAG_BOOT_UPDATE_COMPLETE){
     log_debug("bootloader need recovery.\r\n");
     if(bootloader_recovery_user_app(&env) != 0){
        goto err_exit;
     }
     log_debug("done.\r\n");
     /*执行用户程序*/
     bootloader_boot_user_application(); 
  }
  
 
err_exit:
  bootloader_reset();
  while(1);
}

