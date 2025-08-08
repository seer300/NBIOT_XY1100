/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "xy_api.h"
#include "xy_utils.h"
#include "lwip/tcp.h"
#include "lwip/netdb.h"
#include "lwip/api.h"
#include "lwip/err.h"

/* TCP服务器数据处理服务器回调函数 */
static err_t recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *tcp_recv_pbuf, err_t err)
{
  struct pbuf *tcp_send_pbuf;
  char echoString[]="This is the client content echo:\r\n";

  if (tcp_recv_pbuf != NULL)
  {
    /* 更新接收窗口 */
    tcp_recved(pcb, tcp_recv_pbuf->tot_len);

    /* 将接收的数据拷贝给发送结构体 */
    tcp_send_pbuf = tcp_recv_pbuf;
    tcp_write(pcb,echoString, strlen(echoString), 1);
    /* 将接收到的数据再转发出去 */
    tcp_write(pcb, tcp_send_pbuf->payload, tcp_send_pbuf->len, 1);

    pbuf_free(tcp_recv_pbuf);
    tcp_close(pcb);
  }
  else if (err == ERR_OK)
  {
    return tcp_close(pcb);
  }

  return ERR_OK;
}

// 定义数据发送完成的回调函数
static err_t sent_callback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(len);

    // 在这里可以添加额外的逻辑，比如发送更多数据

    return ERR_OK;
}

// 定义错误处理的回调函数
static void err_callback(void *arg, err_t err) {
    LWIP_UNUSED_ARG(arg);

    if (err != ERR_ABRT) {
        // 错误处理逻辑
    }
}

/* TCP服务器接收回调函数，当客户端建立连接后本函数被调用 */
static err_t TCPServerAccept(void *arg, struct tcp_pcb *pcb, err_t err)
{
  /* 注册接收回调函数 */
  tcp_recv(pcb, TCPServerCallback);

  return ERR_OK;
}

// 初始化TCP服务器监听
void init_tcp_server() {
    

    struct tcp_pcb *tcp_server_pcb;
    ip4_addr_t ipaddr;
    IP4_ADDR(&ipaddr, 0, 0, 0, 0);

    /* 为tcp服务器分配一个tcp_pcb结构体 */
    tcp_server_pcb = tcp_new();

    /* 绑定本地端号和IP地址 */
    tcp_bind(tcp_server_pcb, &ipaddr, 3300);

    /* 监听之前创建的结构体tcp_server_pcb */
    tcp_server_pcb = tcp_listen(tcp_server_pcb);

    /* 初始化结构体接收回调函数 */
    tcp_accept(tcp_server_pcb, TCPServerAccept);

    xy_printf("PPP: init_tcp_server run");
}