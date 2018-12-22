#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "stdbool.h"
#include "flash_utils.h"
#include "bootloader_env.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[env]"

#define  BOOTLOADER_RESET_LATER_TIME                5000


typedef void (*user_application_func_t)(void);

/*名称：bootloader_write_fw
* 功能：flash固件写入参数
* 参数：fw_dest_addr 固件目标地址
* 参数：fw_src_addr  固件源地址
* 参数：size         固件大小
* 返回：无
*/
static void bootloader_write_fw(uint32_t fw_dest_addr,uint32_t fw_src_addr,uint32_t size);



static user_application_func_t user_application_func;

/*名称：bootloader_boot_user_application
* 功能：启动用户区APP
* 参数：无
* 返回：无
*/
void bootloader_boot_user_application()
{
  uint32_t user_application_msp;
  /*初始化栈指针*/
  user_application_msp = *(uint32_t*)(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET);
  __set_MSP(user_application_msp);
  
  /*指定用户应用地址*/
  user_application_func = (user_application_func_t)(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET + 4);
  log_debug("bootloader boot user app --> addr:0x%X stack:0x%X....\r\n",user_application_func,user_application_msp);
  /*跳转*/
  user_application_func();  
  
  while(1);
}

/*名称：bootloader_reset
* 功能：应用程序复位
* 参数：无
* 返回：无
*/
void bootloader_reset()
{
  uint32_t reset_later = BOOTLOADER_RESET_LATER_TIME;
  while(reset_later > 0){
  log_error("bootloader will reset %ds later.\r\n",reset_later); 
  reset_later --;
  }  
  NVIC_SystemReset();
}



/*名称：bootloader_read_main_env
* 功能：读取主要ENV参数
* 参数：env 环境参数存储指针
* 返回：无
*/
void bootloader_read_main_env(bootloader_env_t *env)
{
  int rc;
  rc = flash_utils_read((char *)&env,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET,sizeof(bootloader_env_t));
  if(rc != 0){
  log_error("bootloader read main env addr:% err.\r\n",BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET);  
  bootloader_reset();  
  }   
}

/*名称：bootloader_write_main_env
* 功能：写入主要ENV参数
* 参数：env 环境参数存储指针
* 返回：无
*/
void bootloader_write_main_env(bootloader_env_t *env)
{
  int rc;
  /*擦除原ENV参数区域*/
  rc = flash_utils_erase(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET,BOOTLOADER_FLASH_ENV_SIZE);
  if(rc != 0){
  log_error("bootloader erase main env addr:% err.\r\n",BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET);  
  bootloader_reset();
  }  
  /*编程原ENV参数区域*/
  rc = flash_utils_write(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET,(uint32_t*)env,sizeof(bootloader_env_t) / 4);
  if(rc != 0){
  log_error("bootloader write main env addr:% err.\r\n",BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET);  
  bootloader_reset();
  }
}

/*名称：bootloader_read_backup_env
* 功能：读取备份ENV参数
* 参数：env 环境参数存储指针
* 返回：无
*/
void bootloader_read_backup_env(bootloader_env_t *env)
{
  int rc;
  rc = flash_utils_read((char *)&env,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET,sizeof(bootloader_env_t));
  if(rc != 0){
  log_error("bootloader read backup env addr:% err.\r\n",BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET);  
  bootloader_reset();  
  }   
}

/*名称：bootloader_write_backup_env
* 功能：写入备份ENV参数
* 参数：env 环境参数存储指针
* 返回：无
*/
void bootloader_write_backup_env(bootloader_env_t *env)
{
  int rc;
  /*擦除原ENV参数区域*/
  rc = flash_utils_erase(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET,BOOTLOADER_FLASH_ENV_SIZE);
  if(rc != 0){
  log_error("bootloader erase backup env addr:% err.\r\n",BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET);  
  bootloader_reset();
  }  
  /*编程原ENV参数区域*/
  rc = flash_utils_write(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET,(uint32_t*)env,sizeof(bootloader_env_t) / 4);
  if(rc != 0){
  log_error("bootloader write backup env addr:% err.\r\n",BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET);  
  bootloader_reset();
  }
}



/*名称：bootloader_write_fw
* 功能：flash固件写入参数
* 参数：fw_dest_addr 固件目标地址
* 参数：fw_src_addr  固件源地址
* 参数：size         固件大小
* 返回：无
*/
static void bootloader_write_fw(uint32_t fw_dest_addr,uint32_t fw_src_addr,uint32_t size)
{
  int rc;
  
  /*拷贝fw*/
  /*擦除原FW区域*/
  rc = flash_utils_erase(fw_dest_addr,size); 
  if(rc != 0){
  log_error("bootloader erase fw addr:% err.\r\n",fw_dest_addr);  
  bootloader_reset();
  }
  /*编程原FW区域*/
  rc = flash_utils_write(fw_dest_addr,(uint32_t *)fw_src_addr,size); 
  if(rc != 0){
  log_error("bootloader write fw addr:% err.\r\n",fw_dest_addr);  
  bootloader_reset();
  }  
   
}

/*名称：bootloader_copy_update_to_user_app
* 功能：根据ENV把更新程序复制到用户APP区域
* 参数：env  环境参数
* 返回：无
*/
void bootloader_copy_update_to_user_app(bootloader_env_t *env)
{
 log_debug("copy update and write to user app...\r\n");
 bootloader_write_fw(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET,
                     BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_UPDATE_APPLICATION_OFFSET,
                     env->new_fw.size);
 log_debug("done.\r\n");
}

/*名称：bootloader_copy_user_app_to_backup
* 功能：根据ENV把用户APP程序复制到备份区域
* 参数：env  环境参数
* 返回：无
*/
void bootloader_copy_user_app_to_backup(bootloader_env_t *env)
{
 log_debug("copy user app and write to backup...\r\n");
 bootloader_write_fw(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_BACKUP_APPLICATION_OFFSET,
                     BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET,
                     env->old_fw.size);
 log_debug("done.\r\n")
}

/*名称：bootloader_copy_backup_to_user_app
* 功能：根据ENV把备份程序复制到用户APP区域
* 参数：env  环境参数
* 返回：无
*/
void bootloader_copy_backup_to_user_app(bootloader_env_t *env)
{
 log_debug("copy backup and write to user...\r\n");
 bootloader_write_fw(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET,
                     BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_BACKUP_APPLICATION_OFFSET,
                     env->backup_fw.size);
 log_debug("done.\r\n")
}
