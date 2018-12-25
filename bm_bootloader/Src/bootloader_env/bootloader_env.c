#include "main.h"
#include "stdbool.h"
#include "flash_utils.h"
#include "bootloader_env.h"
#include "log.h"
#define  LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#define  LOG_MODULE_NAME     "[env]"

#define  BOOTLOADER_RESET_LATER_TIME                5000


typedef void (*user_application_func_t)(void);


static user_application_func_t user_application_func;

/*名称：bootloader_boot_user_application
* 功能：启动用户区APP
* 参数：无
* 返回：无
*/
void bootloader_boot_user_application()
{
  uint32_t user_application_msp;
  uint32_t user_app_addr;
  
  /*初始化栈指针*/
  user_application_msp = *(uint32_t*)(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET);
  
  /*获取用户APP地址和栈指针*/
  user_app_addr = *(uint32_t *)(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET + 4);
  user_application_func = (user_application_func_t)user_app_addr;
  log_debug("bootloader boot user app --> addr:0x%X stack:0x%X....\r\n",user_application_func,user_application_msp);
  /*等待日志输出完毕*/
  HAL_Delay(500);
  /*跳转*/
  __disable_irq();
  __set_MSP(user_application_msp);
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
  log_error("bootloader will reset %dS later.\r\n",reset_later); 
  reset_later --;
  HAL_Delay(1000);
  }  
  NVIC_SystemReset();
}

/*名称：bootloader_read_env
* 功能：读取ENV参数
* 参数：env 环境参数指针
* 参数：env 环境参数地址
* 返回：无
*/
static int bootloader_read_env(bootloader_env_t *env,uint32_t addr)
{
  int rc;
  rc = flash_utils_read((uint32_t *)env,addr,sizeof(bootloader_env_t) / 4);
  if(rc != 0){
  log_error("bootloader read env value addr:%d err.\r\n",(uint32_t)addr);  
  return -1;  
  }   
  return 0;
}

/*名称：bootloader_read_env
* 功能：写入ENV参数
* 参数：env 环境参数指针
* 参数：env 环境参数地址
* 返回：无
*/
static int bootloader_write_env(bootloader_env_t *env,uint32_t addr)
{
  int rc;
  rc = flash_utils_write(addr,(uint32_t *)env,sizeof(bootloader_env_t) / 4);
  if(rc != 0){
  log_error("bootloader read env value addr:%d err.\r\n",(uint32_t)addr);  
  return -1;  
  }   
  return 0;
}

 
/*名称：bootloader_find_valid_env_addr
* 功能：找到有效的ENV的地址
* 参数：addr 开始查找的地址
* 参数：size 查找范围
* 返回：0 没有有效的env 其他 ENV地址
*/
static int bootloader_find_valid_env_addr(uint32_t *env_addr,uint32_t addr_start,uint32_t size)
{
    int rc;
    uint32_t env_size;
    bootloader_env_t env_search;
    bootloader_env_t *env_addr_search;
  
    env_addr_search = (bootloader_env_t *)addr_start;
    env_size = sizeof(bootloader_env_t);
 
    do{
    rc = flash_utils_read((uint32_t *)&env_search,(uint32_t)env_addr_search,env_size / 4);
    if ( rc != 0 ){
    log_error("bootloader read env addr:% err.\r\n",env_addr);  
    return -1;
    }     
    env_addr_search ++;
    }while( env_addr_search <= (bootloader_env_t *)(addr_start + size - env_size) && env_search.status == BOOTLOADER_ENV_STATUS_VALID);
      
    *env_addr =(uint32_t) (env_addr_search - 2 >= (bootloader_env_t *)addr_start ? env_addr_search - 2 : 0);
    return 0;    
}

/*名称：bootloader_find_free_env_addr
* 功能：找到有效的ENV的地址
* 参数：addr 开始查找的地址
* 参数：size 查找范围
* 返回：0 没有有效的env 其他 ENV地址
*/
static int bootloader_find_free_env_addr(uint32_t *env_addr,uint32_t addr_start,uint32_t size)
{
    int rc;
    uint32_t env_size;
    bootloader_env_t env_search;
    bootloader_env_t *env_addr_search;
  
    env_addr_search = (bootloader_env_t *)addr_start;
    env_size = sizeof(bootloader_env_t);
 
    do{
        rc = flash_utils_read((uint32_t *)&env_search,(uint32_t)env_addr_search,env_size / 4);
        if ( rc != 0 ){
            log_error("bootloader read env addr:% err.\r\n",env_addr);  
            return -1;
        }     
    env_addr_search ++;
    }while( env_addr_search <= (bootloader_env_t *)(addr_start + size - env_size) && env_search.status != BOOTLOADER_ENV_STATUS_VALID);
    /*如果没有搜到，证明全被写入，需要擦出*/
    if(env_addr_search >= (bootloader_env_t *)(addr_start + size)){
        log_debug("env start addr: bank are all used.\r\n");
        log_debug("need erase.\r\n");
        /*擦除ENV参数区域*/
        rc = flash_utils_erase(addr_start,size);
        if(rc != 0){
        log_error("bootloader erase env addr:% err.\r\n",addr_start);  
        return -1;
        }  
        /*偏移为0的地址就是可用地址*/
        *env_addr =addr_start;
        return 0;    
    }
    
    *env_addr = (uint32_t)(env_addr_search - 1);
    return 0;
}


/*名称：bootloader_get_env
* 功能：获取ENV参数
* 参数：env 环境参数指针
* 返回：0：成功 1：成功 但是ENV参数是无效的 其他：失败
*/
int bootloader_get_env(bootloader_env_t *env)
{
  int rc;
  uint32_t env_addr,env_addr_bank1,env_addr_bank2;

  rc = bootloader_find_valid_env_addr(&env_addr_bank1,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BANK1_ADDR_OFFSET,BOOTLOADER_FLASH_ENV_BANK1_SIZE);
  if( rc != 0){
    return -1;
  }
  
  if(env_addr_bank1 == 0){
    log_error("bootloader main env is invalid.\r\n");
    rc = bootloader_find_valid_env_addr(&env_addr_bank2,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BANK2_ADDR_OFFSET,BOOTLOADER_FLASH_ENV_BANK2_SIZE);  
    if(rc != 0 ){
      return -1;
    }
    if(env_addr_bank2 == 0){
      log_debug("bootloader are all invalid.\r\n");
      return 1;
    }
    env_addr = env_addr_bank2;
  }else {
   env_addr = env_addr_bank1;
  }
  /*读取ENV参数*/
  rc = bootloader_read_env(env,env_addr);
  if(rc != 0 ){
    return -1;
  }
  
 return 0;
}

/*名称：bootloader_save_env
* 功能：保存ENV参数
* 参数：env 环境参数指针
* 返回：0：成功 其他：失败
*/
int bootloader_save_env(bootloader_env_t *env)
{
  int rc;
  uint32_t env_addr,env_addr_bank1,env_addr_bank2;
  
  log_debug("search bank2...\r\n");
  rc = bootloader_find_free_env_addr(&env_addr_bank1,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BANK1_ADDR_OFFSET,BOOTLOADER_FLASH_ENV_BANK1_SIZE);
  if(rc != 0 ){
    return -1;
  } 
  
  /*BANK1已经存满，需要写入BANK2，然后擦除BANK1*/    
  if(env_addr_bank1 == 0){
    log_debug("bootloader bank1 env is full.\r\n");
    log_debug("search bank2...\r\n");
    rc = bootloader_find_free_env_addr(&env_addr_bank2,BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BANK2_ADDR_OFFSET,BOOTLOADER_FLASH_ENV_BANK2_SIZE);  
    if(rc != 0 ){
      return -1;
   }
    
   /*如果BANK2也是写满的，擦除BANK2，然后在BANK2写入ENV*/
   if(env_addr_bank2 == 0){
     log_debug("bootloader are all full.\r\n");
     log_debug("erase bank2...\r\n");
        /*擦除BANK2 ENV参数区域*/
     rc = flash_utils_erase(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BANK2_ADDR_OFFSET,BOOTLOADER_FLASH_ENV_BANK2_SIZE);
     if(rc != 0){
     return -1;
     }
     env_addr = BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BANK2_ADDR_OFFSET;      
   }else{
   env_addr = env_addr_bank2; 
  }
  }else{
  env_addr = env_addr_bank1;
  }
  /*写入BANK*/
  rc = bootloader_write_env(env,env_addr);
  if(rc != 0){
    return -1;
  }
  /*如果BANK1写满，擦除BANK1 ENV参数区域*/
  if(env_addr_bank1 == 0){
   log_debug("erase bank2...\r\n");
   rc = flash_utils_erase(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_ENV_BANK2_ADDR_OFFSET,BOOTLOADER_FLASH_ENV_BANK2_SIZE);
   if(rc != 0){
     return -1;
   }  
  }
   
  return 0;
}


/*名称：bootloader_write_fw
* 功能：flash固件写入参数
* 参数：fw_dest_addr 固件目标地址
* 参数：fw_src_addr  固件源地址
* 参数：size         固件大小
* 返回：0：成功 其他：失败
*/
static int bootloader_write_fw(uint32_t fw_dest_addr,uint32_t fw_src_addr,uint32_t size)
{
  int rc;
  /*拷贝fw*/
  /*擦除FW区域*/
  log_debug("write fw from addr:%d to addr:%d size:%d...\r\n",fw_src_addr,fw_dest_addr,size);
  log_debug("erase fw addr:%d size:%d...\r\n",fw_src_addr,size);
  rc = flash_utils_erase(fw_dest_addr,size); 
  if(rc != 0){
  return -1;
  }
  log_debug("done.\r\n");
  /*编程FW区域*/
  log_debug("write fw addr:%d size:%d...\r\n",fw_dest_addr,size);
  rc = flash_utils_write(fw_dest_addr,(uint32_t *)fw_src_addr,size); 
  if(rc != 0){
  return -1;
  }
  log_debug("done.\r\n");
  return 0; 
}

static int bootloader_copy_update_to_user(bootloader_env_t *env)
{
  int rc;
  uint32_t size;
  
  size = env->fw_update.size - env->swap_ctrl.update_offset >= BOOTLOADER_FLASH_SWAP_BLOCK_SIZE ? BOOTLOADER_FLASH_SWAP_BLOCK_SIZE  :env->fw_update.size - env->swap_ctrl.update_offset;
  rc = bootloader_write_fw(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_ADDR_OFFSET + env->swap_ctrl.update_offset,
                            BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_UPDATE_APPLICATION_ADDR_OFFSET + env->swap_ctrl.update_offset,
                            size / 4);
  if(rc != 0){
  return -1;
  } 
  env->swap_ctrl.update_offset += size;
  /*保存当前ENV*/ 
  rc = bootloader_save_env(env);
  if(rc != 0){
  return -1;  
  }
   
  return 0;
}

static int bootloader_copy_swap_to_update(bootloader_env_t *env)
{
  int rc;

  rc = bootloader_write_fw(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_UPDATE_APPLICATION_ADDR_OFFSET + env->swap_ctrl.origin_offset,
                           BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_SWAP_BLOCK_ADDR_OFFSET,
                           env->swap_ctrl.size / 4);
  if(rc != 0){
  return -1;
  }           
  env->swap_ctrl.origin_offset += env->swap_ctrl.size;
  env->swap_ctrl.size = 0;
  /*保存当前ENV*/ 
  rc = bootloader_save_env(env);
  if(rc != 0){
   return -1;  
  }
  
  return 0;
 } 
  
 static int bootloader_copy_user_to_swap(bootloader_env_t *env)
{
  int rc;
  uint32_t size;
  
  size = env->fw_origin.size - env->swap_ctrl.origin_offset >= BOOTLOADER_FLASH_SWAP_BLOCK_SIZE ? BOOTLOADER_FLASH_SWAP_BLOCK_SIZE  : env->fw_origin.size - env->swap_ctrl.origin_offset;
  rc = bootloader_write_fw(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_SWAP_BLOCK_ADDR_OFFSET,
                           BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_ADDR_OFFSET + env->swap_ctrl.origin_offset,
                           size / 4);
  if(rc != 0){
  return -1;
  }           
  env->swap_ctrl.origin_offset += env->swap_ctrl.size;
  env->swap_ctrl.size = size;
  /*保存当前ENV*/ 
  rc = bootloader_save_env(env);
  if(rc != 0){
   return -1;  
  }
  
  return 0;
 } 
  
/*名称：bootloader_copy_update_to_user_app
* 功能：根据ENV把更新程序复制到用户APP区域
* 参数：env  环境参数
* 返回：无
*/
int bootloader_update_user_app(bootloader_env_t* env)
{
 int rc;
 uint32_t size;
 
 /*复制相同大小的数据*/
 while(env->fw_update.swap_ctrl.update_offset != env->fw_update.size || env->fw_origin.swap_ctrl.origin_offset != env->fw_origin.size){ 
   /*如果用户程序没有交换完毕*/
   if(env->swap_ctrl.origin_offset < env->fw_origin.size){
     /*如果有用户数据保留在交换区，说明没有写入更新区备份，先把这部分的更新程序写入到用户区*/
    if(env->swap_ctrl.size > 0 ){
    /*如果更新区程序还没有复制完毕*/
      if(env->swap_ctrl.update_offset < env->fw_update.size){ 
        /*如果交换的更新区程序还没有复制用户区*/
        rc = bootloader_copy_update_to_user(env);
        if(rc != 0){
        return -1; 
        }
      }else{
        /*如果交换的UPDATE区程序已经复制到用户区，把交换区数据写入UPDATE区*/    
        if(env->swap_ctrl.update_offset == env->swap_ctrl.origin_offset){
          rc = bootloader_copy_swap_to_update(env);
          if(rc != 0){
          return -1; 
          }
        }
          
          
        }
    log_debug("copy swap data to update...\r\n");   
      
      
    log_debug("copy swap data to update...\r\n");
   
   log_debug("done.\r\n");
   env->fw_origin.swap_ctrl.offset += env->fw_origin.swap_ctrl.size; //BOOTLOADER_FLASH_SWAP_BLOCK_SIZE;
   env->fw_origin.swap_ctrl.size = 0;
   /*保存当前ENV*/ 
   rc = bootloader_save_env(&env);
   if(rc != 0){
   return -1;  
   }
   
   if(env->fw_origin.swap_ctrl.offset >= env->fw_origin.size){
   env->fw_origin.swap_ctrl.offset = env->fw_origin.size  
   }
   /*保存当前ENV*/ 
   rc = bootloader_save_env(&env);
   if(rc != 0){
   return -1;  
   }
   log_debug("copy user to swap...\r\n");
   rc = bootloader_write_fw(BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_SWAP_BLOCK_ADDR_OFFSET,
                            BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_ADDR_OFFSET + env.fw_origin.swap_ctrl.offset,
                            BOOTLOADER_FLASH_SWAP_BLOCK_SIZE);
   if(rc != 0){
     return -1;
   }
   log_debug("done.\r\n");
   env.fw_origin.swap_ctrl.offset += BOOTLOADER_FLASH_SWAP_BLOCK_SIZE;
   if(env.fw_origin.swap_ctrl.offset >= env.fw_origin.size){
   env.fw_origin.swap_ctrl.offset = env.fw_origin.size  
   }
   /*保存当前ENV*/ 
   rc = bootloader_save_env(&env);
   if(rc != 0){
   return -1;  
   }
   /*从更新区拷贝一段数据到用户区*/
   
   
   
   
   
   
   
   
   
   
   
   
 }
   
 (BOOTLOADER_FLASH_BASE_ADDR + BOOTLOADER_FLASH_USER_APPLICATION_OFFSET + ,
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
