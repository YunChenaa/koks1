#include "app_flash_otp.h"
#include "sfc_read.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include <sys/mman.h>

// 假设每个区域对应一个 Security 寄存器
#define REGION_TO_INDEX(otpid) (otpid)

const int FACTORY_STATION_SIZE = OTP_STATION_SIZE;

void *my_memmem(const void *haystack, size_t haystack_len, const void *needle, size_t needle_len)
{
    const unsigned char *begin = (const unsigned char *)haystack;
    const unsigned char *end = begin + haystack_len - needle_len;
    const unsigned char *p = (const unsigned char*)needle;

    if (needle_len == 0)
    {
        return (void *)haystack;
    }

    while (begin <= end)
    {
        if (*begin == *p)
        {
            if (memcmp(begin, p, needle_len) == 0)
            {
                return (void *)begin;
            }
        }
        begin++;
    }

    return NULL;
}

bool app_flash_write_otp(FLASH_OTP_REGION otpid, uint32_t addr, uint32_t len, uint8_t *buf)
{
    if (otpid >= FLASH_OTP_REGIONMAX || addr + len > OTP_REGION_MAX_SIZE)
    {
        return false;
    }

    int sfc_fd = open(SFC_DEV_PATH, O_RDONLY);
    if (sfc_fd < 0)
    {
        printf("open /dev/ingenic_sfc failed \n");
        return false;
    }

    int memfd = open(MEM_DEV_PATH, O_RDWR | O_SYNC);
    if (memfd < 0)
    {
        printf("open /dev/mem failed!\n");
        close(sfc_fd);
        return false;
    }

    struct sfc_transfer trans_config;
    trans_config.status = BUF_WRITE;
    trans_config.len = len;
    trans_config.addr = ((uint32_t)REGION_TO_INDEX(otpid) << SEC_REG_IND_OFFSET) + addr;
    memcpy(trans_config.data, buf, len);

    int ret = ioctl(sfc_fd, SFC_WRITE_OTP, &trans_config);
    if (ret < 0)
    {
        printf("sfc ioctl write buf error ,ret = %d\n", ret);
        goto err_exit;
    }

    close(memfd);
    close(sfc_fd);
    return true;

err_exit:
    close(memfd);
    close(sfc_fd);
    return false;
}

bool app_flash_read_otp(FLASH_OTP_REGION otpid, uint32_t addr, uint32_t len, uint8_t *buf)
{
    if (otpid >= FLASH_OTP_REGIONMAX || addr + len > OTP_REGION_MAX_SIZE)
    {
        return false;
    }

    int sfc_fd = open(SFC_DEV_PATH, O_RDONLY);
    if (sfc_fd < 0)
    {
        printf("open /dev/ingenic_sfc failed \n");
        return false;
    }

    int memfd = open(MEM_DEV_PATH, O_RDWR | O_SYNC);
    if (memfd < 0)
    {
        printf("open /dev/mem failed!\n");
        close(sfc_fd);
        return false;
    }

    struct sfc_transfer trans_config;
    int ret = 0;

    // 步骤1: MALLOC_READ
    trans_config.status = MALLOC_READ;
    trans_config.addr = ((uint32_t)REGION_TO_INDEX(otpid) << SEC_REG_IND_OFFSET) + addr;
    trans_config.len = OTP_REGION_MAX_SIZE - len;
    ret = ioctl(sfc_fd, SFC_READ_OTP, &trans_config);
    if (ret < 0)
    {
        printf("sfc ioctl MALLOC_READ error ,ret = %d\n", ret);
        goto err_exit;
    }

    // 步骤2: BUF_READ
    trans_config.status = BUF_READ;
    ret = ioctl(sfc_fd, SFC_READ_OTP, &trans_config);
    if (ret < 0)
    {
        printf("sfc ioctl BUF_READ error ,ret = %d\n", ret);
        goto err_exit;
    }

    trans_config.p_vir_buf_addr = (uint32_t)mmap(NULL, trans_config.len, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, trans_config.d_phy_buf_addr);
    if (trans_config.p_vir_buf_addr == (uint32_t)MAP_FAILED)
    {
        printf("mmap failed!\n");
        goto err_exit;
    }

    memcpy(buf, (void *)trans_config.p_vir_buf_addr, len);

    if (munmap((void *)trans_config.p_vir_buf_addr, trans_config.len) < 0)
    {
        printf("munmap buf failed!\n");
    }

    // 步骤3: FREE_BUF
    trans_config.status = FREE_BUF;
    ret = ioctl(sfc_fd, SFC_READ_OTP, &trans_config);
    if (ret < 0)
    {
        printf("sfc ioctl FREE_BUF error ,ret = %d\n", ret);
        goto err_exit;
    }

    close(memfd);
    close(sfc_fd);
    return true;

err_exit:
    close(memfd);
    close(sfc_fd);
    return false;
}

bool app_flash_erase_otp(FLASH_OTP_REGION otpid)
{
    if (otpid >= FLASH_OTP_REGIONMAX)
    {
        return false;
    }

    int sfc_fd = open(SFC_DEV_PATH, O_RDONLY);
    if (sfc_fd < 0)
    {
        printf("open /dev/ingenic_sfc failed \n");
        return false;
    }

    int memfd = open(MEM_DEV_PATH, O_RDWR | O_SYNC);
    if (memfd < 0)
    {
        printf("open /dev/mem failed!\n");
        close(sfc_fd);
        return false;
    }
    struct sfc_transfer trans_config;
    trans_config.status = BUF_ERASE;
    trans_config.addr = ((uint32_t)REGION_TO_INDEX(otpid) << SEC_REG_IND_OFFSET) + 0x00;

    int ret = ioctl(sfc_fd, SFC_ERASE_OTP, &trans_config);
    if (ret < 0)
    {
        printf("sfc ioctl erase buf error ,ret = %d\n", ret);
        goto err_exit;
    }

    close(memfd);
    close(sfc_fd);
    return true;

err_exit:
    close(memfd);
    close(sfc_fd);
    return false;
}

bool app_flash_write_sn(uint8_t *sn, uint32_t len)
{
    uint8_t read_buf[OTP_SN_SIZE] = {0};
    uint8_t sn_buf[OTP_SN_STATION_SIZE] = {0};
    uint8_t end_buf[6] = {'E', 'N', 'D', 'O', 'T', 'P'}; // 结束标志
    if (len > (OTP_SN_SIZE - 6) || len == 0 || sn == NULL)
    {
        return false;
    }

    if (!app_flash_read_otp(OTP_SN_STATION_ID0, OTP_SN_OFFSET, OTP_SN_STATION_SIZE, sn_buf))
    {
        printf("app_flash_read_otp failed\n");
        return false;
    }

    // 先擦除 OTP 区域
    if (!app_flash_erase_otp(OTP_SN_STATION_ID0))
    {
        printf("app_flash_erase_otp failed\n");
        return false;
    }

    // 拼接结束标志
    memcpy(sn_buf, sn, len);
    memcpy(sn_buf + len, end_buf, sizeof(end_buf));
    // 计算长度
    len += sizeof(end_buf);
    // 然后写入 SN
    if (!app_flash_write_otp(OTP_SN_STATION_ID0, OTP_SN_OFFSET, OTP_SN_STATION_SIZE, sn_buf))
    {
        printf("app_flash_write_otp failed\n");
        return false;
    }
    // 最后读取 SN 进行验证
    // 读取 SN 的时候，长度要加上结束标志的长度
    if (!app_flash_read_otp(OTP_SN_STATION_ID0, OTP_SN_OFFSET, len, read_buf))
    {
        printf("app_flash_read_otp failed\n");
        return false;
    }
    // 比较 SN 和读取到的 SN
    if (memcmp(sn_buf, read_buf, len) != 0)
    {
        printf("SN verification failed\n");
        return false;
    }
    // printf("SN verification succeeded\n");
    // 如果验证成功，返回 true
    return true;
}

bool app_flash_read_sn(uint8_t *sn)
{
    uint8_t read_buf[OTP_SN_SIZE] = {0};
    uint8_t end_buf[6] = {'E', 'N', 'D', 'O', 'T', 'P'}; // 结束标志
    if (sn == NULL)
    {
        return false;
    }
    // 读取 SN
    if (!app_flash_read_otp(OTP_SN_STATION_ID0, OTP_SN_OFFSET, OTP_SN_SIZE, read_buf))
    {
        printf("app_flash_read_otp failed\n");
        return false;
    }
    // 查找结束标志
    uint8_t *end_ptr = (uint8_t *)my_memmem(read_buf, OTP_SN_SIZE, end_buf, sizeof(end_buf));
    if (end_ptr == NULL)
    {
        printf("SN not found\n");
        return false;
    }
    // 计算 SN 的长度
    uint32_t sn_len = end_ptr - read_buf;
    if (sn_len > OTP_SN_SIZE)
    {
        printf("SN length is too long\n");
        return false;
    }
    // 复制 SN
    memcpy(sn, read_buf, sn_len);
    // 添加结束符
    sn[sn_len] = '\0';
    // 打印 SN
    // printf("SN length: %d sn: %s\n", sn_len, sn);
    return true;
}

bool app_flash_write_licence(uint8_t *licence, uint32_t len)
{
    uint8_t read_buf[OTP_LICENCE_SIZE] = {0};
    uint8_t licence_buf[OTP_LICENCE_SIZE] = {0};
    uint8_t end_buf[6] = {'E', 'N', 'D', 'O', 'T', 'P'}; // 结束标志
    if (len > OTP_LICENCE_SIZE || len == 0 || licence == NULL)
    {
        return false;
    }
    // 先擦除 OTP 区域
    if (!app_flash_erase_otp(OTP_LICENCE_ID1))
    {
        printf("app_flash_erase_otp failed\n");
        return false;
    }
    // 拼接结束标志
    memcpy(licence_buf, licence, len);
    memcpy(licence_buf + len, end_buf, sizeof(end_buf));
    // 计算长度
    len += sizeof(end_buf);
    // 然后写入 LICENCE
    if (!app_flash_write_otp(OTP_LICENCE_ID1, OTP_LICENCE_OFFSET, len, licence_buf))
    {
        printf("app_flash_write_otp failed\n");
        return false;
    }

    // 最后读取 LICENCE 进行验证
    // 读取 LICENCE 的时候，长度要加上结束标志的长度
    if (!app_flash_read_otp(OTP_LICENCE_ID1, OTP_LICENCE_OFFSET, len, read_buf))
    {
        printf("app_flash_read_otp failed\n");
        return false;
    }
    // 比较 LICENCE 和读取到的 LICENCE
    if (memcmp(licence_buf, read_buf, len) != 0)
    {
        printf("LICENCE verification failed\n");
        return false;
    }
    // printf("LICENCE verification succeeded\n");
    // 如果验证成功，返回 true
    return true;
}

bool app_flash_read_licence(uint8_t *licence)
{
    uint8_t read_buf[OTP_LICENCE_SIZE] = {0};
    uint8_t end_buf[6] = {'E', 'N', 'D', 'O', 'T', 'P'}; // 结束标志
    if (licence == NULL)
    {
        return false;
    }
    // 读取 LICENCE
    if (!app_flash_read_otp(OTP_LICENCE_ID1, OTP_LICENCE_OFFSET, OTP_LICENCE_SIZE, read_buf))
    {
        printf("app_flash_read_otp failed\n");
        return false;
    }
    // 查找结束标志
    uint8_t *end_ptr = (uint8_t *)my_memmem(read_buf, OTP_LICENCE_SIZE, end_buf, sizeof(end_buf));
    if (end_ptr == NULL)
    {
        printf("LICENCE not found\n");
        return false;
    }
    // 计算 LICENCE 的长度
    uint32_t licence_len = end_ptr - read_buf;
    if (licence_len > OTP_LICENCE_SIZE)
    {
        printf("LICENCE length is too long\n");
        return false;
    }
    // 复制 LICENCE
    memcpy(licence, read_buf, licence_len);
    // 添加结束符
    licence[licence_len] = '\0';
    // 打印 LICENCE
    // printf("LICENCE length: %d licence: %s\n", licence_len, licence);
    return true;
}

bool app_flash_write_station(uint8_t *station, uint32_t len)
{
    uint8_t read_buf[OTP_STATION_SIZE] = {0};
    uint8_t station_buf[OTP_SN_STATION_SIZE] = {0};
    uint8_t end_buf[6] = {'E', 'N', 'D', 'O', 'T', 'P'}; // 结束标志

    if (len > (OTP_STATION_SIZE - 6) || len == 0 || station == NULL)
    {
        return false;
    }

    if (!app_flash_read_otp(OTP_SN_STATION_ID0, OTP_SN_OFFSET, OTP_SN_STATION_SIZE, station_buf))
    {
        printf("app_flash_read_otp failed\n");
        return false;
    }

    // 先擦除 OTP 区域
    if (!app_flash_erase_otp(OTP_SN_STATION_ID0))
    {
        printf("app_flash_erase_otp failed\n");
        return false;
    }

    // 拼接结束标志
    memcpy(station_buf + OTP_STATION_OFFSET, station, len);
    memcpy(station_buf + OTP_STATION_OFFSET + len, end_buf, sizeof(end_buf));
    // 计算长度
    len += sizeof(end_buf);

    // 然后写入 STATION
    if (!app_flash_write_otp(OTP_SN_STATION_ID0, OTP_SN_OFFSET, OTP_SN_STATION_SIZE, station_buf))
    {
        printf("app_flash_write_otp failed\n");
        return false;
    }

    // 最后读取 STATION 进行验证
    // 读取 STATION 的时候，长度要加上结束标志的长度
    if (!app_flash_read_otp(OTP_SN_STATION_ID0, OTP_SN_OFFSET, len, read_buf))
    {
        printf("app_flash_read_otp failed\n");
        return false;
    }

    // 比较 STATION 和读取到的 STATION
    if (memcmp(station_buf, read_buf, len) != 0)
    {
        printf("STATION verification failed\n");
        return false;
    }
    // printf("STATION verification succeeded\n");
    // 如果验证成功，返回 true
    return true;
}

bool app_flash_read_station(uint8_t *station)
{
    uint8_t read_buf[OTP_STATION_SIZE] = {0};
    uint8_t end_buf[6] = {'E', 'N', 'D', 'O', 'T', 'P'}; // 结束标志
    if (station == NULL)
    {
        return false;
    }
    // 读取 STATION
    if (!app_flash_read_otp(OTP_SN_STATION_ID0, OTP_STATION_OFFSET, OTP_STATION_SIZE, read_buf))
    {
        printf("app_flash_read_otp failed\n");
        return false;
    }
    // 查找结束标志
    uint8_t *end_ptr = (uint8_t *)my_memmem(read_buf, OTP_STATION_SIZE, end_buf, sizeof(end_buf));
    if (end_ptr == NULL)
    {
        printf("STATION not found\n");
        return false;
    }
    // 计算 STATION 的长度
    uint32_t station_len = end_ptr - read_buf;
    if (station_len > (OTP_STATION_SIZE - 6))
    {
        printf("CAM length is too long\n");
        return false;
    }
    // 复制 STATION
    memcpy(station, read_buf, station_len);
    // 添加结束符
    station[station_len] = '\0';
    // 打印 STATION
    // printf("STATION length: %d station: %s\n", station_len, station);
    return true;
}

bool app_flash_enter_factory_mode(void)
{
    if (!app_flash_erase_otp(OTP_CAM_ID3))
    {
        printf("app_flash_erase_otp failed\n");
        return false;
    }
    return true;
}

bool app_flash_exit_factory_mode(void)
{
    char station[OTP_CAM_SIZE] = {0x03, 'P', 'P', 'N'}; // 恢复出厂状态
    app_flash_write_station((uint8_t *)station, 4); // 写入恢复出厂状态
    return true;
}


// 反转字节序
uint32_t reverse_bytes(uint32_t value)
{
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8) & 0x0000FF00) |
           ((value << 8) & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

// 封装的接口函数，返回 chipid
int app_flash_get_chipid(uint32_t chipid[3])
{
    int fd;
    void *mapped_base;
    volatile uint32_t *reg32;

    fd = open(MEM_DEV, O_RDONLY);
    if (fd < 0)
    {
        perror("open /dev/mem failed");
        return -1;
    }

    mapped_base = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, fd, BASE_ADDR & ~(PAGE_SIZE - 1));
    if (mapped_base == MAP_FAILED)
    {
        perror("mmap failed");
        close(fd);
        return -1;
    }

    reg32 = (volatile uint32_t *)((uintptr_t)mapped_base + (BASE_ADDR & (PAGE_SIZE - 1)));

    // 读取 96 位 ID
    chipid[0] = reg32[0];
    chipid[1] = reg32[1];
    chipid[2] = reg32[2];

    munmap(mapped_base, PAGE_SIZE);
    close(fd);

    chipid[0] = reverse_bytes(chipid[0]);
    chipid[1] = reverse_bytes(chipid[1]);
    chipid[2] = reverse_bytes(chipid[2]);

    return 0;
}

bool app_flash_write_cam(p_cam_calibration_param_t cam)
{
    uint8_t read_buf[OTP_CAM_SIZE] = {0};
    uint8_t cam_buf[OTP_CAM_SIZE] = {0};
    uint8_t end_buf[6] = {'E', 'N', 'D', 'O', 'T', 'P'}; // 结束标志
    uint32_t len = sizeof(cam_calibration_param_t);
    if (len > (OTP_CAM_SIZE - 6) || cam == NULL)
    {
        printf("invalid len or pcam is null\n");
        return false;
    }

    // 先擦除 OTP 区域
    if (!app_flash_erase_otp(OTP_CAM_ID3))
    {
        printf("app_flash_erase_otp failed\n");
        return false;
    }
    printf("app_flash_erase_otp OTP_CAM_ID3 ok\n");

    // 拼接结束标志
    memcpy(cam_buf, cam, len);
    memcpy(cam_buf + len, end_buf, sizeof(end_buf));
    // 计算长度
    len += sizeof(end_buf);
    // 然后写入 CAM
    printf("app_flash_write_cam  write otp start\n");
    if (!app_flash_write_otp(OTP_CAM_ID3, OTP_CAM_OFFSET, len, cam_buf))
    {
        printf("app_flash_write_otp failed\n");
        return false;
    }
    // 最后读取 CAM 进行验证
    // 读取 CAM 的时候，长度要加上结束标志的长度
    printf("app_flash_read_otp cam start\n");
    if (!app_flash_read_otp(OTP_CAM_ID3, OTP_CAM_OFFSET, len, read_buf))
    {
        printf("app_flash_read_otp failed\n");
        return false;
    }
    // 比较 CAM 和读取到的 CAM
    printf("read %d Byte\n", len);
    if (memcmp(cam_buf, read_buf, len) != 0)
    {
        printf("CAM verification failed\n");
        return false;
    }
    printf("CAM verification succeeded\n");
    // 如果验证成功，返回 true
    return true;
}

bool app_flash_read_cam(p_cam_calibration_param_t cam)
{
    uint8_t read_buf[OTP_CAM_SIZE] = {0};
    uint8_t end_buf[6] = {'E', 'N', 'D', 'O', 'T', 'P'}; // 结束标志
    if (cam == NULL)
    {
        return false;
    }
    // 读取 CAM
    if (!app_flash_read_otp(OTP_CAM_ID3, OTP_CAM_OFFSET, OTP_CAM_SIZE, read_buf))
    {
        printf("app_flash_read_otp failed\n");
        return false;
    }
    // 查找结束标志
    uint8_t *end_ptr = (uint8_t *)my_memmem(read_buf, OTP_CAM_SIZE, end_buf, sizeof(end_buf));
    if (end_ptr == NULL)
    {
        printf("CAM not found\n");
        return false;
    }
    // 计算 CAM 的长度
    uint32_t cam_len = end_ptr - read_buf;
    if (cam_len != sizeof(cam_calibration_param_t))
    {
        printf("CAM length is invalid length=%d and need length is %d\n", cam_len, sizeof(cam_calibration_param_t));
        return false;
    }
    // 复制 CAM
    memcpy(cam, read_buf, cam_len);

    return true;
}

