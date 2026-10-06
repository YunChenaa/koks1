#pragma once

#define SFC_IOTC_CHECK 'S'
#define SFC_READ_SR _IO(SFC_IOTC_CHECK, 0)
#define SFC_READ_SR1 _IO(SFC_IOTC_CHECK, 1)
#define SFC_READ_SR2 _IO(SFC_IOTC_CHECK, 2)

#define SFC_WRITE_SR _IO(SFC_IOTC_CHECK, 3)
#define SFC_WRITE_SR1 _IO(SFC_IOTC_CHECK, 4)
#define SFC_WRITE_SR2 _IO(SFC_IOTC_CHECK, 5)

#define SFC_DO_READ _IO(SFC_IOTC_CHECK, 6)

#define SFC_READ_OTP _IO(SFC_IOTC_CHECK, 9)
#define SFC_WRITE_OTP _IO(SFC_IOTC_CHECK, 10)
#define SFC_ERASE_OTP _IO(SFC_IOTC_CHECK, 11)

#define SFC_DEV_PATH "/dev/ingenic_sfc0"
#define MEM_DEV_PATH "/dev/rmem"

#define SFC_OTP_STATUS SFC_READ_SR1
#define SFC_OTP_LOCK SFC_WRITE_SR1

#define SFC_READ_ADDR 0x0
#define SFC_READ_LEN 0x1000000

#define SEC_REG_IND_OFFSET 12

struct sfc_transfer
{

	uint32_t addr;
	uint32_t len;
	uint32_t retlen;
	uint8_t status;
	uint8_t data[1024];
	uint32_t p_vir_buf_addr;
	uint32_t d_vir_buf_addr;
	uint32_t d_phy_buf_addr;
} __attribute__((packed));

enum
{
	MALLOC_READ,
	BUF_READ,
	FREE_BUF,
	BUF_WRITE,
	BUF_ERASE,
	READ_OTP_LOCK,
	LOCK_OTP_BIT,
};

// for OTP
// 定义 OTP 区域枚举
typedef enum
{
    FLASH_OTP_REGION_NONE = 0,
    FLASH_OTP_REGION1,
    FLASH_OTP_REGION2,
    FLASH_OTP_REGION3,
    FLASH_OTP_REGIONMAX,
    FLASH_OTP_REGION_INVALID,
} FLASH_OTP_REGION;

// 定义常量
#define OTP_REGION_MAX_SIZE 0x400 // 1K
#define OTP_REGION_MAX_NUM 3
#define OTP_MAX_SIZE 1024

#define OTP_SN_STATION_ID0 FLASH_OTP_REGION1
#define OTP_SN_OFFSET 0
#define OTP_SN_SIZE 64
#define OTP_STATION_OFFSET  OTP_SN_SIZE
#define OTP_STATION_SIZE  64
#define OTP_SN_STATION_SIZE (OTP_SN_SIZE + OTP_STATION_SIZE)

#define OTP_LICENCE_ID1 FLASH_OTP_REGION2
#define OTP_LICENCE_OFFSET 0
#define OTP_LICENCE_SIZE 256

#define OTP_CAM_ID3 FLASH_OTP_REGION3
#define OTP_CAM_OFFSET 0
#define OTP_CAM_SIZE 256

// // 函数声明
// bool app_flash_write_otp(FLASH_OTP_REGION otpid, uint32_t addr, uint32_t len, uint8_t *buf);
// bool app_flash_read_otp(FLASH_OTP_REGION otpid, uint32_t addr, uint32_t len, uint8_t *buf);
// bool app_flash_erase_otp(FLASH_OTP_REGION otpid);


// for chip id
#define BASE_ADDR       0x13540200
#define PAGE_SIZE       4096
#define MEM_DEV         "/dev/mem"
