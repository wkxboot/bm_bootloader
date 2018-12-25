#ifndef  __BOOTLOADER_ENV_H__
#define  __BOOTLOADER_ENV_H__



#define  BOOTLOADER_FLASH_BASE_ADDR                      (0x8000000)
#define  BOOTLOADER_FLASH_PAGE_SIZE                      (2048)

#define  BOOTLOADER_FLASH_ENV_BANK1_ADDR_OFFSET          (0x6C00)
#define  BOOTLOADER_FLASH_ENV_BANK1_SIZE                 (0x800) /*环境参数区1 2k*/

#define  BOOTLOADER_FLASH_ENV_BANK2_ADDR_OFFSET          (0x7400)
#define  BOOTLOADER_FLASH_ENV_BANK2_SIZE                 (0x800) /*环境参数区2 2k*/

#define  BOOTLOADER_FLASH_USER_APPLICATION_ADDR_OFFSET   (0x7C00)
#define  BOOTLOADER_FLASH_USER_APPLICATION_SIZE          (0x19000)/*用户的应用程序区 100k*/

#define  BOOTLOADER_FLASH_UPDATE_APPLICATION_ADDR_OFFSET (0x20C00)/*更新的应用程序区 100k*/
#define  BOOTLOADER_FLASH_UPDATE_APPLICATION_SIZE        (0x19000)

#define  BOOTLOADER_FLASH_SWAP_BLOCK_ADDR_OFFSET         (0x39C00)/*数据交换区 20k*/
#define  BOOTLOADER_FLASH_SWAP_BLOCK_SIZE                (0x5000)

#define  BOOTLOADER_FLASH_SIZE                           (0x3FFFF)



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
char value[36];
}bootloader_fw_md5_t;

typedef struct
{
char value[4];
}bootloader_fw_version_t;



typedef struct
{
uint32_t update_offset;/*更新区的偏移*/
uint32_t origin_offset;/*用户区的偏移*/
uint32_t size;         /*交换区的数据大小*/
}bootloader_swap_ctrl_t;

typedef struct
{
bootloader_fw_version_t version;
bootloader_fw_md5_t     md5;
uint32_t                size;
}bootloader_fw_t;

typedef struct
{
bootloader_flag_t       boot_flag;    /*启动标志*/
uint32_t                reserved[32]; /*保留32个参数，供应用程序使用*/
bootloader_fw_t         fw_update;    /*更新的固件*/
bootloader_fw_t         fw_origin;    /*原有的固件*/
bootloader_swap_ctrl_t  swap_ctrl;    /*数据交换控制*/
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



#endif