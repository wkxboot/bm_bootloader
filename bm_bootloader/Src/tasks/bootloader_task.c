#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "led.h"
#include "flash_utils.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[log]"


typedef void (*user_application_func_t)(void);

#define  BOOTLOADER_VERSION                   "v1.0.0"

#define  BOOTLOADER_INIT_DISPLAY_VALUE         0xFF

#define  BOOTLOADER_FLASH_BASE_ADDR                 (0x8000000)

#define  BOOTLOADER_FLASH_ENV_ADDR_OFFSET           (0x2800)
#define  BOOTLOADER_FLASH_ENV_SIZE                  (0x800)

#define  BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET    (0x3000)
#define  BOOTLOADER_FLASH_ENV_BACKUP_SIZE           (0x800)

#define  BOOTLOADER_FLASH_USER_APPLICATION_OFFSET   (0x3800)
#define  BOOTLOADER_FLASH_USER_APPLICATION_SIZE     (0x14000)

#define  BOOTLOADER_FLASH_UPDATE_APPLICATION_OFFSET (0x17800)
#define  BOOTLOADER_FLASH_UPDATE_APPLICATION_SIZE   (0x14000)

#define  BOOTLOADER_FLASH_BACKUP_APPLICATION_OFFSET (0x2B800)
#define  BOOTLOADER_FLASH_BACKUP_APPLICATION_SIZE   (0x14000)

#define  BOOTLOADER_FLASH_SIZE                      (0x3FFFF)


typedef enum
{
BOOTLOADER_ENV_STATUS_VALID  = 0x11223344,
BOOTLOADER_ENV_STATUS_INVALID
}bootloader_env_status_t;

/*更新完成 需要新的应用程序改回BOOTLOADER_FLAG_BOOT_UPDATE_OK代表升级成功*/
/*如果第二次bootloader启动时是BOOTLOADER_FLAG_BOOT_UPDATE_OK 代表升级成功 设置为 BOOTLOADER_FLAG_BOOT_NORMAL*/
/*如果第二次bootloader启动时仍然是BOOTLOADER_FLAG_BOOT_UPDATE_COMPLETE 代表升级失败 进行回滚操作 回滚完成设置BOOTLOADER_FLAG_BOOT_NORMAL*/
typedef enum
{
BOOTLOADER_FLAG_BOOT_NORMAL = 0x1234000U, /*正常启动*/
BOOTLOADER_FLAG_BOOT_UPDATE,              /*需要更新*/
BOOTLOADER_FLAG_BOOT_UPDATE_COMPLETE,     /*更新完成,待验证是否成功*/
BOOTLOADER_FLAG_BOOT_UPDATE_OK,           /*更新成功*/
}bootloader_flag_t;

typedef struct
{
char value[16];
}bootloader_fw_md5_t;

typedef struct
{
char value[4];
}bootloader_fw_version_t;

typedef struct
{
bootloader_fw_version_t version;
bootloader_fw_md5_t     md5;
uint32_t size;
}bootloader_fw_t;


typedef struct
{
uint32_t                reserved[32]; /*保留32个参数，供应用程序使用*/
bootloader_flag_t       boot_flag;    /*启动标志*/
bootloader_fw_t         new_fw;       /*新固件*/
bootloader_fw_t         old_fw;       /*老固件*/
bootloader_fw_t         backup;       /*备份固件*/
bootloader_env_status_t status;       /*环境参数状态*/
}bootloader_env_t;

static bootloader_env_t bootloader_env,bootloader_env_backup;

static user_application_func_t user_application_func;

static uint32_t user_application_msp;


#define  BOOTLOADER_RESET_LATER_TIME           5000

static void bootloader_boot_user_application()
{
  /*初始化栈指针*/
  user_application_msp = *(__IO uint32_t*)(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET);
  __set_MSP(user_application_msp);
  
  /*指定用户应用地址*/
  user_application_func = (user_application_func_t)(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET + 4);
  log_debug("bootloader boot user app --> addr:0x%X stack:0x%X....\r\n",user_application_func,user_application_msp);
  /*跳转*/
  user_application_func();  
  
  while(1);
}

static void bootloader_reset()
{
  uint32_t reset_later = BOOTLOADER_RESET_LATER_TIME;
  while(reset_later > 0){
  log_error("bootloader will reset %ds later.\r\n",reset_later); 
  reset_later --;
  }  
  NVIC_SystemReset();
}


static void bootloader_erase_env(uint32_t env_addr)
{
  int rc;
  /*擦除原ENV参数区域*/
  rc = flash_utils_erase(env_addr,BOOTLOADER_FLASH_ENV_SIZE);
  if(rc != 0){
  log_error("bootloader erase env addr:% err.\r\n",env_addr);  
  bootloader_reset();
  }   
  
}
static void bootloader_save_env(bootloader_env_t *env,uint32_t addr)
{
  int rc;
  /*编程原ENV参数区域*/
  rc = flash_utils_write(env_addr,(uint32_t*)env,sizeof(bootloader_env_t) / 4);
  if(rc != 0){
  log_error("bootloader save env addr:% err.\r\n",env_addr);  
  bootloader_reset();
  }
}

static void bootloader_read_env(bootloader_env_t *env,uint32_t addr)
{
  int rc;
  rc = flash_utils_read((char *)&env,addr,sizeof(bootloader_env_t));
  if(rc != 0){
  bootloader_reset();  
  }   
}

static void bootloader_write_fw(uint32_t fw_dest_addr,uint32_t fw_src_addr,uint32_t size)
{
  int rc;
  
  /*拷贝fw*/
  /*擦除原FW区域*/
  rc = flash_utils_erase(fw_dest_addr,size); 
  if(rc != 0){
  log_error("bootloader erase flash old fw addr:% err.\r\n",fw_dest_addr);  
  bootloader_reset();
  }
  /*编程原FW区域*/
  rc = flash_utils_write(fw_dest_addr,(uint32_t *)fw_src_addr,size); 
  if(rc != 0){
  log_error("bootloader write fw addr:% err.\r\n",fw_dest_addr);  
  bootloader_reset();
  }  
   
}

  /*如果备份env有效，同时主要env无效，恢复主要env*/
  if(bootloader_env.status != BOOTLOADER_ENV_STATUS_VALID &&  bootloader_env_backup.status == BOOTLOADER_ENV_STATUS_VALID){
  log_debug("bootloader main env invalid.copy backup env to main...\r\n");  

  bootloader_write_fw(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET,
                    BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_BACKUP_APPLICATION_OFFSET,
                    bootloader_env_backup.backup.size);
  bootloader_write_env(&bootloader_env_backup,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET);
  bootloader_env = bootloader_env_backup;/*保持一致*/
  log_debug("bootloader restore main env done.\r\n");
  }else if(bootloader_env.status == BOOTLOADER_ENV_STATUS_VALID &&  bootloader_env_backup.status != BOOTLOADER_ENV_STATUS_VALID){
  log_debug("bootloader backup env invalid.copy main env to backup...\r\n");  
  bootloader_write_fw(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_BACKUP_APPLICATION_OFFSET,
                    BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET,
                    bootloader_env.backup.size);
  bootloader_write_env(&bootloader_env, BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET);  
  bootloader_env_backup = bootloader_env;/*保持一致*/
  log_debug("bootloader restore backup env done.\r\n");
  }

void bootloader_task(void const * argument)
{
 int rc;

 /*等待显示芯片上电稳定*/;
 osDelay(200);
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
 
 log_debug("\r\n*******************************************************\r\n");
 log_debug("\r\n* WKXBOOT BOOTLOADER VER:%s        *\r\n",BOOTLOADER_VERSION);
 log_debug("\r\n* build date:%s time:%s            *\r\n",__DATE__,__TIME__);  
 log_debug("\r\n*******************************************************\r\n");

 /*解除写保护*/
 if(flash_utils_write_protection_config(FLASH_UTILS_WR_PROTECTION_NONE) != 0){  
 log_error("bootloader disable write protect err.\r\n");  
 bootloader_reset();
 }
 
  while(1){
  /*启动检查ENV是否完整 保持一致 始终至少有一个是正确的ENV*/
    
  /*读取主要ENV参数*/ 
  log_debug("bootloader check env...\r\n");
  bootloader_read_env(&bootloader_env,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET);
  /*读取备份ENV参数*/ 
  bootloader_read_env(&bootloader_env_backup,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET);
 
  if(bootloader_env.status != BOOTLOADER_ENV_STATUS_VALID && bootloader_env_backup.status != BOOTLOADER_ENV_STATUS_VALID){
  log_warning("bootloader all env invalid.wait manual reset...\r\n");  
  while(1);  
  }
  
  /*检查ENV是否一致,如果不一致，肯定有一个是无效的ENV*/
  if(bootloader_env_backup != bootloader_env){
  log_debug("bootloader env is not sync. start sync...\r\n");
  if(bootloader_env.status != BOOTLOADER_ENV_STATUS_VALID){
  log_debug("bootloader main env is invlaid.\r\n"); 
  log_debug("bootloader erase main env...\r\n");  
  bootloader_erase_env(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET);
  log_debug("done.\r\n"); 
  bootloader_env = bootloader_env_backup;
  log_debug("bootloader sync env to main...\r\n");  
  bootloader_save_env(&bootloader_env_backup,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET);
  log_debug("done.\r\n");

  }else{
  log_debug("bootloader backup env is invlaid.\r\n"); 
  log_debug("bootloader erase backup env...\r\n");  
  bootloader_erase_env(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET);
  log_debug("done.\r\n"); 
  bootloader_env_backup = bootloader_env;
  log_debug("bootloader sync env to backup...\r\n");  
  bootloader_save_env(&bootloader_env_backup,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET);
  log_debug("done.\r\n");
  }
 }
 /*在同步的条件下判断是否升级*/
 
 /*没有更新，正常启动*/
 if(bootloader_env.boot_flag == BOOTLOADER_FLAG_BOOT_NORMAL)
 {
  log_debug("bootloader no update.boot normal...\r\n");
  bootloader_boot_user_application(); 
 }
 
 /*需要更新程序*/
 /*此时任意ENV存储着UPDATE程序信息*/
 if(bootloader_env.boot_flag == BOOTLOADER_FLAG_BOOT_UPDATE)
 {
 log_debug("bootloader need update.\r\n");
 log_debug("bootloader erase main env...\r\n");
 bootloader_erase_env(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET);
 log_debug("done.\r\n");
 log_debug("bootloader copy update fw to user app...\r\n");
 bootloader_write_fw(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET,
                   BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_UPDATE_APPLICATION_OFFSET,
                   bootloader_env.update.size);
 log_debug("done.\r\n");
 
 log_debug("bootloader save main env...\r\n");
 bootloader_env.status = BOOTLOADER_FLAG_BOOT_UPDATE_COMPLETE;
 bootloader_env.old_fw =  bootloader_env.update_fw;
 bootloader_save_env(&bootloader_env,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET);
 log_debug("done.\r\n");
 
 log_debug("bootloader erase backup env...\r\n");  
 bootloader_erase_env(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET);
 log_debug("done.\r\n"); 
 bootloader_env_backup = bootloader_env;
 log_debug("bootloader sync env to backup...\r\n");  
 bootloader_save_env(&bootloader_env_backup,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET);
 log_debug("done.\r\n");
 
 /*执行用户程序*/
 user_application_func(); 
 }
 
 /*需要备份新程序*/
 if(bootloader_env.boot_flag == BOOTLOADER_FLAG_BOOT_UPDATE_OK)
 {
 log_debug("bootloader need backup app.\r\n");
 log_debug("bootloader erase main env...\r\n");
 bootloader_erase_env(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET);
 log_debug("done.\r\n");  
 log_debug("bootloader copy app fw to backup...\r\n");
 bootloader_write_fw(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_BACKUP_APPLICATION_OFFSET,
                   BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET,
                   bootloader_env.update.size);
 log_debug("done.\r\n");
 
 
 
 

 log_debug("bootloader copy update fw to user app...\r\n");
 bootloader_write_fw(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET,
                   BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_UPDATE_APPLICATION_OFFSET,
                   bootloader_env.update.size);
 log_debug("done.\r\n");
 
 log_debug("bootloader save main env...\r\n");
 bootloader_env.status = BOOTLOADER_FLAG_BOOT_UPDATE_COMPLETE;
 bootloader_save_env(&bootloader_env,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_ADDR_OFFSET);
 log_debug("done.\r\n");
 
 log_debug("bootloader erase backup env...\r\n");  
 bootloader_erase_env(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET);
 log_debug("done.\r\n"); 
 bootloader_env_backup = bootloader_env;
 log_debug("bootloader sync env to backup...\r\n");  
 bootloader_save_env(&bootloader_env_backup,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET);
 log_debug("done.\r\n");
 
 /*执行用户程序*/
 user_application_func(); 
 }
    
    
    
    
    
    
    
    
    
  }
  
  
  
}