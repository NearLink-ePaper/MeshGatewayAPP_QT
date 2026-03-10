/**
 * @file  wifi_socket_server.c
 * @brief TCP Socket 服务器 — 接收 PC 端图片并触发 ePaper 显示
 */

#include "wifi_socket_server.h"
#include "image_receiver.h"
#include "epaper.h"
#include "watchdog.h"
#include "soc_osal.h"
#include "cmsis_os2.h"
#include "securec.h"
#include "lwip/sockets.h"
#include <stdio.h>
#include <string.h>

#define SOCK_LOG    "[wifi sock]"
#define SERVER_TASK_STACK   0x1800  /* 6KB: waterline=0x498(1.2KB), lwip recv/send 需要额外栈空间。释放 2KB 堆给 WiFi+BT OAL */
#define SERVER_TASK_PRIO    (osPriority_t)(15)

/**
 * @brief 从 socket 精确读取 len 字节
 * @return 0=成功, -1=失败/断开
 */
static int recv_exact(int fd, uint8_t *buf, uint32_t len)
{
    uint32_t got = 0;
    while (got < len) {
        int n = lwip_recv(fd, buf + got, (int)(len - got), 0);
        if (n <= 0) return -1;
        got += (uint32_t)n;
        uapi_watchdog_kick();
    }
    return 0;
}

/**
 * @brief 处理一个客户端连接
 */
static void handle_client(int client_fd)
{
    uint8_t header[WIFI_IMG_HEADER_SIZE];
    uint8_t resp;
    uint16_t width, height;
    uint8_t  mode;
    uint32_t data_size;
    uint8_t *img_buf;

    /* 接收协议头 */
    if (recv_exact(client_fd, header, WIFI_IMG_HEADER_SIZE) != 0) {
        printf("%s header recv failed\r\n", SOCK_LOG);
        return;
    }

    /* 校验 magic */
    if (header[0] != WIFI_IMG_MAGIC_0 || header[1] != WIFI_IMG_MAGIC_1) {
        printf("%s bad magic: %02X %02X\r\n", SOCK_LOG, header[0], header[1]);
        resp = WIFI_IMG_RESP_FAIL;
        lwip_send(client_fd, &resp, 1, 0);
        return;
    }

    /* 解析头 */
    width     = ((uint16_t)header[2] << 8) | header[3];
    height    = ((uint16_t)header[4] << 8) | header[5];
    mode      = header[6];
    /* header[7] reserved */
    data_size = ((uint32_t)header[8]  << 24) | ((uint32_t)header[9]  << 16) |
                ((uint32_t)header[10] << 8)  | header[11];

    printf("%s header: %ux%u mode=%u size=%lu\r\n",
           SOCK_LOG, width, height, mode, (unsigned long)data_size);

    /* 检查缓冲区大小 */
    if (data_size > IMG_RX_BUF_SIZE) {
        printf("%s data_size %lu > buf %d, OOM\r\n",
               SOCK_LOG, (unsigned long)data_size, IMG_RX_BUF_SIZE);
        resp = WIFI_IMG_RESP_OOM;
        lwip_send(client_fd, &resp, 1, 0);
        return;
    }

    /* 等待 image_receiver 空闲 */
    {
        const img_rx_info_t *info = image_receiver_get_info();
        if (info->state == IMG_STATE_RECEIVING) {
            printf("%s busy (BLE receiving), reject\r\n", SOCK_LOG);
            resp = WIFI_IMG_RESP_FAIL;
            lwip_send(client_fd, &resp, 1, 0);
            return;
        }
    }

    /* 直接接收到 image_receiver 缓冲区 */
    img_buf = image_receiver_get_buffer_writable();
    if (recv_exact(client_fd, img_buf, data_size) != 0) {
        printf("%s data recv failed\r\n", SOCK_LOG);
        resp = WIFI_IMG_RESP_FAIL;
        lwip_send(client_fd, &resp, 1, 0);
        return;
    }

    printf("%s received %lu bytes OK\r\n", SOCK_LOG, (unsigned long)data_size);

    /* 触发 ePaper 显示 */
    epaper_trigger_mesh_image(img_buf, width, height, mode, (uint16_t)data_size);

    /* 回复成功 */
    resp = WIFI_IMG_RESP_OK;
    lwip_send(client_fd, &resp, 1, 0);

    printf("%s triggered ePaper\r\n", SOCK_LOG);
}

/**
 * @brief TCP 服务器主循环
 */
static void *server_task(const char *arg)
{
    (void)arg;
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    int opt = 1;

    /* 创建 socket */
    server_fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        printf("%s socket() failed\r\n", SOCK_LOG);
        return NULL;
    }

    lwip_setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 绑定地址 */
    (void)memset_s(&server_addr, sizeof(server_addr), 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(WIFI_IMG_SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (lwip_bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("%s bind() failed\r\n", SOCK_LOG);
        lwip_close(server_fd);
        return NULL;
    }

    if (lwip_listen(server_fd, 1) < 0) {
        printf("%s listen() failed\r\n", SOCK_LOG);
        lwip_close(server_fd);
        return NULL;
    }

    printf("%s listening on port %d\r\n", SOCK_LOG, WIFI_IMG_SERVER_PORT);

    /* 主循环: 接受连接并处理 */
    while (1) {
        addr_len = sizeof(client_addr);
        client_fd = lwip_accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            osal_msleep(100);
            continue;
        }
        printf("%s client connected\r\n", SOCK_LOG);
        handle_client(client_fd);
        lwip_close(client_fd);
        printf("%s client disconnected\r\n", SOCK_LOG);
    }

    lwip_close(server_fd);
    return NULL;
}

void wifi_socket_server_start(void)
{
    osThreadAttr_t attr = {0};
    attr.name       = "wifi_img_server";
    attr.stack_size = SERVER_TASK_STACK;
    attr.priority   = SERVER_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)server_task, NULL, &attr) == NULL) {
        printf("%s create task failed\r\n", SOCK_LOG);
    } else {
        printf("%s task created\r\n", SOCK_LOG);
    }
}
