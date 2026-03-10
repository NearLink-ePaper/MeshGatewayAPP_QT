/**
 * @file  wifi_socket_server.h
 * @brief TCP Socket 服务器 — 接收 PC 端图片数据并触发 ePaper 显示
 *
 * @details
 *   协议格式 (PC → 设备):
 *     [Header 12B] magic(2) + width(2) + height(2) + mode(1) + reserved(1) + data_size(4)
 *     [Data N bytes] 原始图片数据 (JPEG 或 1bpp)
 *   设备回复: 1 byte (0=OK, 1=OOM, 2=decode_fail)
 */
#ifndef WIFI_SOCKET_SERVER_H
#define WIFI_SOCKET_SERVER_H

#include <stdint.h>

/** TCP 监听端口 */
#define WIFI_IMG_SERVER_PORT    8080

/** 协议 magic: 0xAA55 */
#define WIFI_IMG_MAGIC_0        0xAA
#define WIFI_IMG_MAGIC_1        0x55

/** 协议头大小 */
#define WIFI_IMG_HEADER_SIZE    12

/** 回复状态码 */
#define WIFI_IMG_RESP_OK        0
#define WIFI_IMG_RESP_OOM       1
#define WIFI_IMG_RESP_FAIL      2
#define WIFI_IMG_RESP_BUSY      3   /* BLE 已连接，互斥拒绝 */

/** 探测魔数: 客户端发此字节时服务器回复网关名称 */
#define WIFI_PROBE_MAGIC        0xFE

/**
 * @brief  设置 WiFi 服务器广播的网关名称
 * @param  name  名称字符串，如 "sle_gw_006E"
 */
void wifi_socket_server_set_name(const char *name);

/**
 * @brief  启动 WiFi 图片接收服务器任务
 * @note   需在 wifi_softap_start() 成功后调用
 */
void wifi_socket_server_start(void);

#endif /* WIFI_SOCKET_SERVER_H */
