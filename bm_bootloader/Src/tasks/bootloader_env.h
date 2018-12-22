#ifndef  __BOOTLOADER_ENV_H__
#define  __BOOTLOADER_ENV_H__



#define  BOOTLOADER_FLASH_BASE_ADDR                 (0x8000000)

#define  BOOTLOADER_FLASH_ENV_ADDR_OFFSET           (0x6C00)
#define  BOOTLOADER_FLASH_ENV_SIZE                  (0x800)

#define  BOOTLOADER_FLASH_ENV_BACKUP_ADDR_OFFSET    (0x7400)
#define  BOOTLOADER_FLASH_ENV_BACKUP_SIZE           (0x800)

#define  BOOTLOADER_FLASH_USER_APPLICATION_OFFSET   (0x7C00)
#define  BOOTLOADER_FLASH_USER_APPLICATION_SIZE     (0x12C00) /*75K*/

#define  BOOTLOADER_FLASH_UPDATE_APPLICATION_OFFSET (0x1A800)
#define  BOOTLOADER_FLASH_UPDATE_APPLICATION_SIZE   (0x12C00)

#define  BOOTLOADER_FLASH_BACKUP_APPLICATION_OFFSET (0x2D400)
#define  BOOTLOADER_FLASH_BACKUP_APPLICATION_SIZE   (0x12C00)

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
bootloader_fw_t         backup_fw;    /*备份固件*/
bootloader_env_status_t status;       /*环境参数状态*/
}bootloader_env_t;




/*名称：bootloader_boot_user_application
* 功能：启动用户区APP
* 参数：无
* 返回：无
*/
void bootloader_boot_user_application();

/*名称：bootloader_reset
* 功能：应用程序复位
* 参数：无
* 返回：无
*/
void bootloader_reset();

/*名称：bootloader_read_main_env
* 功能：读取主要ENV参数
* 参数：env 环境参数存储指针
* 返回：无
*/
void bootloader_read_main_env(bootloader_env_t *env);

/*名称：bootloader_write_main_env
* 功能：写入主要ENV参数
* 参数：env 环境参数存储指针
* 返回：无
*/
void bootloader_write_main_env(bootloader_env_t *env);

/*名称：bootloader_read_backup_env
* 功能：读取备份ENV参数
* 参数：env 环境参数存储指针
* 返回：无
*/
void bootloader_read_backup_env(bootloader_env_t *env);

/*名称：bootloader_write_backup_env
* 功能：写入备份ENV参数
* 参数：env 环境参数存储指针
* 返回：无
*/
void bootloader_write_backup_env(bootloader_env_t *env);

/*名称：bootloader_copy_update_to_user_app
* 功能：根据ENV把更新程序复制到用户APP区域
* 参数：env  环境参数
* 返回：无
*/
void bootloader_copy_update_to_user_app(bootloader_env_t *env);

/*名称：bootloader_copy_user_app_to_backup
* 功能：根据ENV把用户APP程序复制到备份区域
* 参数：env  环境参数
* 返回：无
*/
void bootloader_copy_user_app_to_backup(bootloader_env_t *env);

/*名称：bootloader_copy_user_app_to_backup
* 功能：根据ENV把用户APP程序复制到备份区域
* 参数：env  环境参数
* 返回：无
*/
void bootloader_copy_user_app_to_backup(bootloader_env_t *env);

/*名称：bootloader_copy_backup_to_user_app
* 功能：根据ENV把备份程序复制到用户APP区域
* 参数：env  环境参数
* 返回：无
*/
void bootloader_copy_backup_to_user_app(bootloader_env_t *env);

#endif