/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "xy_api.h"
#include "xy_utils.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "lwip/err.h"

#define SERVER_PORT 8080
#define MAX_BUF     256

// 初始化TCP服务器监听
void init_tcp_server() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[MAX_BUF];
    int recv_len;

    xy_printf("PPP: init_tcp_server run");

    // 1. 创建套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        xy_printf("Failed to create socket\n");
        return;
    }

    // 2. 配置服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网卡
    server_addr.sin_port = htons(SERVER_PORT);

    // 3. 绑定
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        xy_printf("Bind failed\n");
        closesocket(server_fd);
        return;
    }

    // 4. 监听
    if (listen(server_fd, 5) < 0) {  // 最多5个等待连接
        xy_printf("Listen failed\n");
        closesocket(server_fd);
        return;
    }

    xy_printf("TCP Server listening on port %d...\n", SERVER_PORT);

    while (1) {
        // 5. 接受客户端连接
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            xy_printf("Accept failed\n");
            continue;
        }

        xy_printf("Client connected: %s:%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_addr.s_addr));

        // 6. 与客户端通信
        while (1) {
            recv_len = recv(client_fd, buffer, MAX_BUF - 1, 0);
            if (recv_len > 0) {
                buffer[recv_len] = '\0';
                xy_printf("Received: %s", buffer);

                // 回显数据
                send(client_fd, buffer, recv_len, 0);
            } else {
                // 客户端断开或出错
                xy_printf("Client disconnected\n");
                break;
            }
        }

        // 7. 关闭客户端套接字
        closesocket(client_fd);
    }

    // 理论上不会执行到这里
    closesocket(server_fd);
}


/**
 * @brief 任务创建
 * 
 */
void init_tcp_server_task_init(void)
{
	osThreadAttr_t thread_attr = {0};
		
	thread_attr.name	   = "init_tcp_server_task";
	thread_attr.priority   = osPriorityNormal;
	thread_attr.stack_size = 1024;
	hal_uart_IT_demo_TskHandle = osThreadNew((osThreadFunc_t)init_tcp_server, NULL, &thread_attr);
}