#include "main.h"
#include "stdbool.h"
#include "led.h"
#include "flash_utils.h"
#include "bootloader_env.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[bootloader]"


#define  BOOTLOADER_VERSION                         "v1.0.0"
#define  BOOTLOADER_INIT_DISPLAY_VALUE              0xFF



/*判断env是否经过了修复*/
static bool env_repair = false;
/*环境参数*/
static bootloader_env_t main_env,backup_env;


/*任务*/
void bootloader_task(void const * argument)
{

 /*等待显示芯片上电稳定*/;
 //osDelay(200);
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
 flash_utils_init();
 
 log_debug("\r\n*************************************************************\r\n");
 log_debug("\r\n  BOOTLOADER VER:%s      build date:%s %s \r\n",BOOTLOADER_VERSION);
 log_debug("\r\n*************************************************************\r\n");

 /*解除写保护*/
 if(flash_utils_write_protection_config(FLASH_UTILS_WR_PROTECTION_NONE) != 0){  
 log_error("bootloader disable write protect err.\r\n");  
 bootloader_reset();
 }
 
  while(1){
  /*启动检查ENV是否完整 保持一致*/
    
  /*读取主要ENV参数*/ 
  log_debug("bootloader check env...\r\n");
  bootloader_read_main_env(&main_env);
  /*读取备份ENV参数*/ 
  bootloader_read_backup_env(&backup_env);
  /*如果ENV都有效，一定是第一次启动，尝试启动APP*/
  if(main_env.status != BOOTLOADER_ENV_STATUS_VALID && backup_env.status != BOOTLOADER_ENV_STATUS_VALID){
  log_warning("bootloader all env invalid.\r\n");  
  bootloader_boot_user_application(); 
  }
  
  /*如果备份env有效，同时主要env无效，恢复主要env*/
  if(main_env.status != BOOTLOADER_ENV_STATUS_VALID &&  backup_env.status == BOOTLOADER_ENV_STATUS_VALID){
  log_debug("bootloader main env is invlaid.\r\n"); 
  main_env = backup_env;
  bootloader_write_main_env(&main_env);
  env_repair = true;
  }else if(main_env.status == BOOTLOADER_ENV_STATUS_VALID &&  backup_env.status != BOOTLOADER_ENV_STATUS_VALID){
  log_debug("bootloader backup env is invlaid.\r\n"); 
  backup_env = main_env;
  bootloader_write_backup_env(&backup_env);
  env_repair = true;
  }
  
  /*检查ENV全是有效的情况下 在ENV不一致的情况下同步ENV*/
  if(backup_env.boot_flag != main_env.boot_flag){
  log_debug("bootloader env is not sync. start sync...\r\n");
  /*查看启动标志，判断哪个是最新的ENV*/
  int boot_step_interval = main_env.boot_flag > backup_env.boot_flag \
                           ? main_env.boot_flag -  backup_env.boot_flag \
                           : backup_env.boot_flag -  main_env.boot_flag;
  /*步骤相邻，boot_flag越大，代表最新*/
 if(boot_step_interval == 1){
 /*如果是主要ENV最新，同步备份ENV*/
 if(main_env.boot_flag > backup_env.boot_flag){
  log_debug("bootloader backup env is old.\r\n");
  backup_env = main_env;
  bootloader_write_backup_env(&backup_env); 
 }else{
  log_debug("bootloader main env is old.\r\n"); 
  main_env = backup_env;
  bootloader_write_main_env(&main_env);
 }
 }else if(boot_step_interval == (BOOTLOADER_FLAG_BOOT_UPDATE_OK - BOOTLOADER_FLAG_BOOT_NORMAL)){
 /*如果在步骤极限的两端 BOOTLOADER_FLAG_BOOT_NORMAL标志的是最新的*/
 if(main_env.boot_flag == BOOTLOADER_FLAG_BOOT_NORMAL){
  backup_env = main_env;
  bootloader_write_backup_env(&backup_env);     
 }else{
  main_env = backup_env;
  bootloader_write_main_env(&main_env);
 }
 }else{
 log_warning("bootloader all env invalid.wait manual reset...\r\n");  
 while(1);     
 }
 env_repair = true;
 } 
   
 /*现在所有ENV一致*/  
 /*判断是否升级*/
 /*没有更新，正常启动*/
 if(main_env.boot_flag == BOOTLOADER_FLAG_BOOT_NORMAL)
 {
  log_debug("bootloader no update.boot normal...\r\n");
  bootloader_boot_user_application(); 
 }
 
 /*需要更新程序*/
 /*此时任意ENV存储着UPDATE程序信息*/
 if(main_env.boot_flag == BOOTLOADER_FLAG_BOOT_UPDATE)
 {
 log_debug("bootloader need update.\r\n");
 bootloader_copy_update_to_user_app(&main_env);
 log_debug("bootloader start update main env.\r\n");

 main_env.boot_flag = BOOTLOADER_FLAG_BOOT_UPDATE_COMPLETE;
 main_env.old_fw =  main_env.new_fw;
 log_debug("old fw size:%d,update size :%d.\r\n",main_env.old_fw.size,main_env.new_fw.size);
 bootloader_write_main_env(&main_env);
 
 log_debug("bootloader start sync backup env.\r\n");  
 backup_env = main_env;
 bootloader_write_main_env(&backup_env);

 /*执行用户程序*/
 bootloader_boot_user_application(); 
 }
 
 /*需要备份新程序*/
 if(main_env.boot_flag == BOOTLOADER_FLAG_BOOT_UPDATE_OK)
 {
 log_debug("bootloader need backup app.\r\n");
 log_debug("bootloader start copy user app to backup.\r\n");
 bootloader_copy_user_app_to_backup(&main_env);
 
 log_debug("bootloader start update main env.\r\n");
 main_env.backup_fw = main_env.old_fw;
 main_env.boot_flag = BOOTLOADER_FLAG_BOOT_NORMAL;
 bootloader_write_main_env(&main_env);

 log_debug("bootloader start update backup env.\r\n");
 backup_env = main_env;
 bootloader_write_backup_env(&backup_env);

 /*执行用户程序*/
 bootloader_boot_user_application(); 
 }
 
 /*如果升级标志BOOTLOADER_FLAG_BOOT_UPDATE_COMPLETE*/
 /*并且没有新的应用程序改为BOOTLOADER_FLAG_BOOT_UPDATE_OK*/
 /*同时升级过程没有被破坏和修复，就证明新程序升级失败，进行回滚*/
 if(main_env.boot_flag == BOOTLOADER_FLAG_BOOT_UPDATE_COMPLETE ){
 if(env_repair == false){ 
 log_debug("bootloader need  roll back app.\r\n");
 log_debug("bootloader start copy backup to user app...\r\n");
 bootloader_copy_backup_to_user_app(&main_env);
 
 log_debug("bootloader start update main env.\r\n");
 main_env.boot_flag = BOOTLOADER_FLAG_BOOT_NORMAL;
 main_env.old_fw = main_env.backup_fw;
 bootloader_write_main_env(&main_env);
 
 log_debug("bootloader start update backup env.\r\n");
 backup_env = main_env;
 bootloader_write_backup_env(&backup_env);
 
 /*执行用户程序*/
 bootloader_boot_user_application();    
 }else{
 log_debug("bootloader need retry run update app.\r\n");
 /*执行用户程序*/
 bootloader_boot_user_application();   
 }
 }
 
 
 }
}