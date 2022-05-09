
/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "socket_proxy.h"
#include "osal.h"
#include "xy_system.h"
#include "ipcmsg.h"
#include "xy_mem.h"
#include "inter_core_msg.h"
#if XY_SOCKET_PROXY
#include "ps_netif_api.h"
#endif

/*******************************************************************************

 *		   IPV6ADDLEN_MAX=28 for temporary use ,not correct											   *			
 ******************************************************************************/

/*******************************************************************************
 *							   Macro definitions							   *
 ******************************************************************************/


#define SOL_IPV6 1
#define SOCKPROXY_RECV_TIMEOUT 5

/*******************************************************************************
 *							   Type definitions 							   *
 ******************************************************************************/

/*******************************************************************************
 *						  Local function declarations						   *
 ******************************************************************************/


/*******************************************************************************
 *						   Local variable definitions						   *
 ******************************************************************************/

osMessageQueueId_t proxy_sock_msg_q = NULL;
unsigned int proxy_sock_task_handler;


/*
 * Get the address part
 */
void* pj_sockaddr_get_addr(const struct sockaddr *addr)
{
	const pj_sockaddr *a = (const pj_sockaddr*)addr;

	if (a->addr.sa_family == AF_INET6)
		return (void*) &a->ipv6.sin6_addr;
	else
		return (void*) &a->ipv4.sin_addr;
}


/*
 * Get the length of the address part.
 */
unsigned pj_sockaddr_get_addr_len(const struct sockaddr *addr)
{
	const pj_sockaddr *a = (const pj_sockaddr*) addr;
	return a->addr.sa_family == AF_INET6 ?
	       sizeof(struct in6_addr) : sizeof(struct in_addr);
}


int write_to_cp(struct response_msg * p_response_msg, int msglen)
{
	void *res_pkg = NULL;
	int ret = 0;
	if(DSP_IS_NOT_LOADED())
	{
		softap_printf(USER_LOG, WARN_LOG, "dsp not runned and packet dropped!!!");
		return 1;
	}
	
	res_pkg = xy_zalloc_Align(msglen);
	if(res_pkg == NULL)
		xy_assert(0);
	memcpy(res_pkg,p_response_msg,msglen);
	ret = shm_zero_copy(res_pkg,msglen,ICM_SOCKPROXY);
	
	if(ret == -1)
		softap_printf(USER_LOG, WARN_LOG, "dsp not runned and socket response dropped");
	return 1;
}


void proc_set_opt_msg( proxy_request_set_opt_msg	*set_opt_msg)
{

	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *sock, *level, *option_name, *option_len;
	char *option_val;
	int *ret, *err;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	sock = &(set_opt_msg->sock);
	level = &(set_opt_msg->level);
	option_val = set_opt_msg->option_val;
	option_name =  &(set_opt_msg->option_name);
	option_len =  &(set_opt_msg->option_len);
	ret = &(p_response_msg->response_msg_list.response_set_opt_msg.ret);
	err = &(p_response_msg->response_msg_list.response_set_opt_msg.err);
	//PrintMsg((const unsigned char * )set_opt_msg, sizeof(proxy_request_set_opt_msg));
	//tos SO_REUSEADDR=2 linux SO_REUSEADD=2
	if (4 == *option_name) {
		*option_name = 2;
	}
	//SOL_SOCKET = 0xffff ,, linux SOL_SOCKET = 1
	if (0xffff == *level) {
		*level = 1;
	}

	//Log_PROXY(INF, "proc_set_opt_msg sock= %d, level = %d, option_name = %d \n", *sock,*level,*option_name);

	*ret = setsockopt(*sock, *level, *option_name, option_val, *option_len);
	if (*ret < 0) {
		*err = errno;
		softap_printf(USER_LOG, WARN_LOG,  "setsockopt: sock= %d, level = %d, option_name = %d ,ret = %d, errno = %d, errmsg = %s\n", *sock, *level, *option_name, *ret, errno, strerror(errno));
	} else
		softap_printf(USER_LOG, WARN_LOG,  "setsockopt: sock= %d, level = %d, option_name = %d \n", *sock, *level, *option_name);

	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}


void proc_get_opt_msg( proxy_request_get_opt_msg   *get_opt_msg)
{
	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *sock, *level, *option_name;
	int *ret, *err, *option_len;
	char *option_val;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	sock = &(get_opt_msg->sock);
	level = &(get_opt_msg->level);
	option_name =  &(get_opt_msg->option_name);
	option_len  =  &(get_opt_msg->option_len);
	ret = &(p_response_msg->response_msg_list.response_get_opt_msg.ret);
	err = &(p_response_msg->response_msg_list.response_get_opt_msg.err);
	option_val = p_response_msg->response_msg_list.response_get_opt_msg.option_value;

	//PrintMsg((unsigned char *) option_name, *option_len);
//	Log_PROXY(LOG_ALERT,"proc_get_opt_msg sock = %d, level = %d, option_name = %d, option_len = %d\n", *sock, *level, *option_name, *option_len);

	//PrintMsg((unsigned char *) option_name, *option_len);
	*ret = getsockopt(*sock, *level, *option_name, (void *)option_val, (socklen_t *)option_len);
	p_response_msg->response_msg_list.response_get_opt_msg.option_len = *option_len;
	if (*ret < 0) {
		*err = errno;
		softap_printf(USER_LOG, WARN_LOG,  "getsockopt: sock = %d, level = %d, option_name = %d ,ret = %d, errno = %d, errmsg = %s\n", *sock, *level, *option_name, *ret, errno, strerror(errno));
	} else
		softap_printf(USER_LOG, WARN_LOG,  "getsockopt: sock = %d, level = %d, option_name = %d \n", *sock, *level, *option_name);
	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}

void proc_get_sockname_msg( proxy_request_get_sock_name_msg *get_sock_name_msg)
{
	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	volatile int writesize = 0;
	char ipstring[64] = {0};
	unsigned  short port = 0;
	int *sock;
	int *ret, *err, *name_len;
	struct sockaddr *name;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	sock = &(get_sock_name_msg->sock);
	ret = &(p_response_msg->response_msg_list.response_get_sock_name_msg.ret);
	err = &(p_response_msg->response_msg_list.response_get_sock_name_msg.err);
	name_len = &(p_response_msg->response_msg_list.response_get_sock_name_msg.name_len);
	name = (struct sockaddr *)p_response_msg->response_msg_list.response_get_sock_name_msg.name;
	*name_len = 28;
	*ret = getsockname(*sock, (struct sockaddr *)name, (socklen_t*)name_len);

	if (*ret < 0) {
		*err = errno;
		softap_printf(USER_LOG, WARN_LOG,  "getsockname: sock = %d, ret = %d, errno = %d, errmsg = %s\n", *sock, *ret, errno, strerror(errno));

	} else {
		if (NULL == inet_ntop(((addr_hdr *)name)->sa_family, pj_sockaddr_get_addr((struct sockaddr *)name), ipstring, sizeof(ipstring))) {
			softap_printf(USER_LOG, WARN_LOG,  "getsockname: sock = %d, ip addr is not legal!!!\n", *sock);

		} else {
			port = ((struct sockaddr_in *)name)->sin_port;
			softap_printf(USER_LOG, WARN_LOG,  "getsockname: sock = %d, sockname is %s:%d\n", *sock, ipstring, ntohs(port));
		}
	}
	//PrintMsg((unsigned char *)name, (int)sizeof(struct sockaddr));
	//Log_PROXY(LOG_ALERT,"proc_get_sockname_msg, sock = %d, name_len = %d\n", *sock,*name_len);
	writesize = write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
	(void) writesize;
}

void proc_get_peername_msg( proxy_request_get_peer_name_msg *get_peer_name_msg)
{
	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *sock;
	int *ret, *err, *name_len;
	struct sockaddr *name;
	char ipstring[64] = {0};
	unsigned short port = 0;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	sock = &(get_peer_name_msg->sock);
	ret = &(p_response_msg->response_msg_list.response_get_peer_name_msg.ret);
	err = &(p_response_msg->response_msg_list.response_get_peer_name_msg.err);
	name_len = &(p_response_msg->response_msg_list.response_get_peer_name_msg.name_len);
	name = (struct sockaddr *)(p_response_msg->response_msg_list.response_get_peer_name_msg.name);
	*name_len = sizeof(struct sockaddr);
	*ret = getpeername(*sock, name, (socklen_t *)name_len);
	if (*ret < 0) {
		*err = errno;
		softap_printf(USER_LOG, WARN_LOG,  "getpeername: sock = %d, ret = %d, errno = %d, errmsg = %s\n", *sock, *ret, errno, strerror(errno));
	} else {
		if (NULL == inet_ntop(((addr_hdr *)name)->sa_family, pj_sockaddr_get_addr((struct sockaddr *)name), ipstring, sizeof(ipstring))) {
			softap_printf(USER_LOG, WARN_LOG,  "getpeername: sock = %d, ip addr is not legal!!!\n", *sock);

		} else {
			port = ((struct sockaddr_in *)name)->sin_port;
			softap_printf(USER_LOG, WARN_LOG,  "getpeername: sock = %d, peername is %s:%d\n", *sock, ipstring, ntohs(port));
		}
	}

	//PrintMsg((unsigned char *)name, (int)sizeof(struct sockaddr));
	//Log_PROXY(LOG_ALERT,"proc_get_peername_msg, sock = %d, name_len = %d\n", *sock,*name_len);
	//Log_PROXY(INF, "proc_get_peername_msg  ret = %d, errno = %d, errmsg = %s\n", *ret, errno, strerror(errno));
	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}

void proc_ioctl_msg(proxy_request_ioctrl_msg *ioctrl_msg)
{
	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *sock;
	long *cmd;
	char *argp;
	int *ret, *err;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	sock = &(ioctrl_msg->sock);
	cmd = &(ioctrl_msg->cmd);
	argp = ioctrl_msg->argp;
	ret = &(p_response_msg->response_msg_list.response_ioctrl_msg.ret);
	err = &(p_response_msg->response_msg_list.response_ioctrl_msg.err);

	*ret = ioctl(*sock, *cmd, (void *)argp);

	if (*ret < 0) {
		*err = errno;
		softap_printf(USER_LOG, WARN_LOG,  "ioctl:  *sock = %d, cmd = %ld, ret = %d, errno = %d, errmsg = %s\n", *sock, *cmd, *ret, errno, strerror(errno));
	} else
		softap_printf(USER_LOG, WARN_LOG,  "ioctl:  *sock = %d, cmd = %ld\n", *sock, *cmd);

	write_to_cp(p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}

void proc_ioctl_socket_msg( proxy_request_ioctrl_sock_msg *ioctrl_sock_msg)
{
	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int * sock;
	long *cmd;
	char *argp;
	int *ret, *err;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	sock = &(ioctrl_sock_msg->sock);
	cmd = &(ioctrl_sock_msg->cmd);
	argp = ioctrl_sock_msg->argp;
	ret = &(p_response_msg->response_msg_list.response_ioctrl_sock_msg.ret);
	err = &(p_response_msg->response_msg_list.response_ioctrl_sock_msg.err);

	*ret = ioctl(*sock, *cmd, (void *)argp);

	if (*ret < 0) {
		*err = errno;
		softap_printf(USER_LOG, WARN_LOG, "ioctlsocket:  sock = %d, *cmd=%ld, ret = %d, errno = %d, errmsg = %s\n", *sock, *cmd, *ret, errno, strerror(errno));
	} else
		softap_printf(USER_LOG, WARN_LOG,  "ioctlsocket: sock = %d, *cmd=%ld\n", *sock, *cmd);

	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}

//set recv timeout = 5 seconds when socket created
void proc_open_sock_msg(proxy_request_socket_msg *ioctrl_sock_msg)
{

	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *tmp_domain, *tmp_type, *tmp_protocol;
	int *ret, *err;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	tmp_domain = &(ioctrl_sock_msg->domain);
	tmp_type = &(ioctrl_sock_msg->type);
	tmp_protocol = &(ioctrl_sock_msg->protocol);
	ret = &(p_response_msg->response_msg_list.response_sock_msg.ret);
	err = &(p_response_msg->response_msg_list.response_sock_msg.err);

	*ret = socket(*tmp_domain, *tmp_type, *tmp_protocol);
	if (*ret < 0) {
		*err = errno;
		//softap_printf(USER_LOG, WARN_LOG,  "socket: domain = %d, type = %d, ret = %d, errno = %d, errmsg = %s\n", *tmp_domain, *tmp_type, *ret, errno, strerror(errno));
	} else{
		softap_printf(USER_LOG, WARN_LOG,  "socket: domain = %d, type = %d, protocol = %d\n", *tmp_domain, *tmp_type, *tmp_protocol);
	}
		
	struct timeval tv;
    tv.tv_sec = SOCKPROXY_RECV_TIMEOUT;
    tv.tv_usec = 0;

    setsockopt(*ret, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));

	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}

void proc_close_msg( proxy_request_close_msg *close_msg)
{
	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int ret;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	ret = close(close_msg->sock);
	p_response_msg->response_msg_list.response_close_msg.ret = ret;
	if (ret < 0){
		//softap_printf(USER_LOG, WARN_LOG,  "close: close socket  %d ,ret = %d, errno = %d, errmsg = %s!!!\n", close_msg->sock, ret, errno, strerror(errno));
	}
	else{
		//softap_printf(USER_LOG, WARN_LOG,  "close: close socket  %d !!!\n", close_msg->sock);
	}
	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}

void proc_select_msg( proxy_request_select_msg *select_msg)
{
	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *tmp_maxfdp1;
	int *ret, *err;
	
	fd_set fdr,*pfdr = NULL;
	fd_set fdw,*pfdw = NULL;
	fd_set fde,*pfde = NULL;
	struct timeval *p_timeout = NULL;
	struct timeval timeout = {0, 0};

	tmp_maxfdp1 = &(select_msg->maxfdp1);
	
	fdr = select_msg->readset;
	fdw = select_msg->writeset;
	fde = select_msg->exceptset;
	
	timeout.tv_sec = select_msg->timeout.tv_sec;
	timeout.tv_usec = select_msg->timeout.tv_usec;
	
	if(select_msg->para_bitmap & 0x01)
	{
		pfdr = &fdr;
	}
	if(select_msg->para_bitmap & 0x02)
	{
		pfdw = &fdw;
	}
	if(select_msg->para_bitmap & 0x04)
	{
		pfde = &fde;
	}
	if(select_msg->para_bitmap & 0x08)
	{
		p_timeout = &timeout;
	}
	
	ret = &(p_response_msg->response_msg_list.response_select_msg.ret);
	err = &(p_response_msg->response_msg_list.response_select_msg.err);
	
	*ret = select(*tmp_maxfdp1, pfdr, pfdw, pfde, p_timeout);
	
	if (*ret < 0) {
		*err = errno;
		
	} else {
		if(pfdr)
			p_response_msg->response_msg_list.response_select_msg.readset = fdr;
		if(pfdw)
			p_response_msg->response_msg_list.response_select_msg.writeset = fdw;
		if(pfde)
			p_response_msg->response_msg_list.response_select_msg.exceptset = fde;
	}

	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}




void proc_recv_from_msg( proxy_request_recv_from_msg *recv_from_msg)
{

	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *sock, *size, *flags, *sock_from_len;
	int *err, *ret, *sock_recv_len, *len;
	volatile int length = 0;
	unsigned short peer_port = 0;
	char ipstring[64] = {0};
	struct sockaddr *peer_addr;
	char *resopose_data;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	sock = &(recv_from_msg->sock);
	size = &(recv_from_msg->size);
	flags = &(recv_from_msg->flags);
	sock_from_len = &(recv_from_msg->sock_len);

	err = &(p_response_msg->response_msg_list.response_recv_from_msg.err);
	ret = &(p_response_msg->response_msg_list.response_recv_from_msg.ret);
	sock_recv_len = &(p_response_msg->response_msg_list.response_recv_from_msg.socklen);
	len = &(p_response_msg->response_msg_list.response_recv_from_msg.len);
	peer_addr = (struct sockaddr *)p_response_msg->response_msg_list.response_recv_from_msg.addr;
	resopose_data = p_response_msg->response_msg_list.response_recv_from_msg.data;



	*ret = recvfrom(*sock, resopose_data, *size, * flags, peer_addr, (socklen_t *)sock_from_len);

	if (*ret < 0) {
		*err = errno;
		*len = 0;
		softap_printf(USER_LOG, WARN_LOG,  "recvfrom: *sock = %d ,ret = %d , errno = %d, errmsg = %s\n", *sock, *ret, errno, strerror(errno));
	} else {
		*len = *ret;
		length = *sock_from_len;
		peer_port = ((struct sockaddr_in *)peer_addr)->sin_port;
		if (NULL == inet_ntop(((addr_hdr *)peer_addr)->sa_family, pj_sockaddr_get_addr((struct sockaddr *)peer_addr), ipstring, sizeof(ipstring))) {
			softap_printf(USER_LOG, WARN_LOG,  "recvfrom: sock = %d, ip addr is not legal!!!\n", *sock);
			return;
		}
		softap_printf(USER_LOG, WARN_LOG,  "recvfrom: *sock = %d ,recv %d bytes form %s:%d \n", *sock, *ret, ipstring, ntohs(peer_port));
	}
	*sock_recv_len = *sock_from_len;

	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
	(void) length;
}

void proc_recv_msg( proxy_request_recv_msg *recv_msg)
{

	int recv_real_len;
	int *sock, *size, *flag;
	char *data;
	
	sock = &(recv_msg->sock);
	//data = recv_msg->data;
	size = &(recv_msg->size);
	flag = &(recv_msg->flags);
	
	int *ret, *err;
	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) + *size);
	xy_assert(p_response_msg != NULL);
	
	//memset(g_response_msg,0,sizeof(struct  response_msg) + *size);
	
	ret = &(p_response_msg->response_msg_list.response_recv_msg.ret);
	err = &(p_response_msg->response_msg_list.response_recv_msg.err);
	data = p_response_msg->response_msg_list.response_recv_msg.data;

	
	*ret = recv(*sock, data, *size, *flag);

	if (*ret < 0) {
		*err = errno;
		recv_real_len = 0;
		//softap_printf(USER_LOG, WARN_LOG,  "recv: *sock = %d ,ret = %d ,errno = %d, errmsg = %s\n", *sock, *ret, errno, strerror(errno));
	} else{
		recv_real_len = *ret;
		//softap_printf(USER_LOG, WARN_LOG,  "recv: *sock = %d ,recv %d bytes!!!\n", *sock, *ret);
	}

	write_to_cp( p_response_msg, sizeof(struct response_msg)+ recv_real_len);
	xy_free(p_response_msg);
}

void proc_send_msg( proxy_request_send_msg *send_msg)
{

	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *socket, *size, *flags;
	char *data;
	int *ret, *err;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	socket = &(send_msg->sock);
	flags = &(send_msg->flags);
	size = &(send_msg->size);
	data = send_msg->data;
	ret = &(p_response_msg->response_msg_list.response_send_msg.ret);
	err = &(p_response_msg->response_msg_list.response_send_msg.err);

	*ret = send(*socket, data, (size_t) * size, *flags);
	if (*ret <= 0) {
		*err = errno;
		//softap_printf(USER_LOG, WARN_LOG,  "send: socket =%d, datalen=%d ,ret = %d, errno = %d, errmsg = %s\n", *socket, *size, *ret, errno, strerror(errno));
	} else{
		//softap_printf(USER_LOG, WARN_LOG,  "send: socket =%d, datalen=%d ,ret = %d,\n", *socket, *size, *ret);
	}

	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}



void proc_connect_msg( proxy_request_connect_msg *connect_msg)
{

	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *socket, *socket_len;
	struct sockaddr *peer_addr;
	unsigned short peer_port = 0;
	char ipstring[64] = {0};
	int *ret, *err;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	socket = &(connect_msg->sock);
	socket_len = &(connect_msg->socket_len);
	peer_addr = (struct sockaddr *)connect_msg->peer_addr;

	ret = &(p_response_msg->response_msg_list.response_connect_msg.ret);
	err = &(p_response_msg->response_msg_list.response_connect_msg.err);

	peer_port = ((struct sockaddr_in *)peer_addr)->sin_port;

	if (NULL == inet_ntop(((addr_hdr *)peer_addr)->sa_family, pj_sockaddr_get_addr((struct sockaddr *)peer_addr), ipstring, sizeof(ipstring))) {
		softap_printf(USER_LOG, WARN_LOG,  "connect: sock = %d, ip addr is not legal!!!\n", *socket);
		return;
	}

	*ret = connect(*socket, peer_addr, *socket_len);
	if (*ret < 0) {
		*err = errno;
		//Log_PROXY(INF, "proc_connect_msg  ret = %d, errno = %d, errmsg = %s", *ret, errno, strerror(errno));
	}

	softap_printf(USER_LOG, WARN_LOG,  "connect: sock = %d, connecting to %s:%d, ret = %d, errno = %d, errmsg = %s\n", *socket, ipstring, ntohs(peer_port), *ret, errno, strerror(errno));

	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}


void proc_send_to_msg( proxy_request_send_to_msg *send_to_msg)
{

	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *socket, *size, *socket_len, *flags;
	struct sockaddr *peer_addr;
	unsigned short peer_port = 0;
	char ipstring[64] = {0};
	char *data;
	int *ret, *err;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	socket = &(send_to_msg->sock);
	size = &(send_to_msg->size);
	flags = &(send_to_msg->flags);
	peer_addr = (struct sockaddr *)send_to_msg->peer_addr;
	socket_len = &(send_to_msg->socketlen);
	data = send_to_msg->data;
	ret = &(p_response_msg->response_msg_list.response_send_to_msg.ret);
	err = &(p_response_msg->response_msg_list.response_send_to_msg.err);
	//Log_PROXY(LOG_ALERT,"sock=%d, data_size= %d, sock_flag = %d, socklen=%d\n", *socket, *size, *flags, *socket_len);
	//PrintMsg((unsigned char *)&peer_addr, (int)sizeof(struct sockaddr));
	//PrintMsg((unsigned char *)data, *size);
	peer_port = ((struct sockaddr_in *)peer_addr)->sin_port;

	if (NULL == inet_ntop(((addr_hdr *)peer_addr)->sa_family, pj_sockaddr_get_addr((struct sockaddr *)peer_addr), ipstring, sizeof(ipstring))) {
		softap_printf(USER_LOG, WARN_LOG,  "sendto: sock = %d, ip addr is not legal!!!\n", *socket);
		return;
	}

	*ret = sendto(*socket, data, *size, *flags, peer_addr, (socklen_t) * socket_len);

	if (*ret < 0) {
		*err = errno;
		softap_printf(USER_LOG, WARN_LOG,  "sendto: sock = %d, sendo %s:%d ,datalen = %d, ret = %d, errno = %d, errmsg = %s\n", *socket, ipstring, peer_port, *size, *ret, errno, strerror(errno));
	} else
		softap_printf(USER_LOG, WARN_LOG,  "sendto: sock = %d, sendo %s:%d ,datalen = %d, ret = %d\n", *socket, ipstring, ntohs(peer_port), *size, *ret);
	//PrintMsg((unsigned char *)peer_addr, *socket_len);

	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}

void proc_bind_msg( proxy_request_bind_msg *bind_msg)
{

	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *sock, *socket_len;
	struct sockaddr *local_addr;
	char ipstring[64] = {0};
	//char gw_ipstring[64] = {0};
	unsigned short port = 0;
	//struct sockaddr_in *local_addr_in;
	int *err, *ret;
	//volatile char *gw_addr;
	//gw_addr = gw_addr;
	//volatile char *cid;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	//cid = cid;
	sock = &(bind_msg->sock);
	socket_len = &(bind_msg->socklen);
	//gw_addr = bind_msg->gw;
	local_addr = (struct sockaddr *)bind_msg->local_addr;
	//cid =  &(bind_msg->cid);
	//local_addr_in = (struct sockaddr_in *)local_addr;
	//syslog(LOG_ALERT,"socket = %d, addr ======= %d  port = === %d tmp_socket_len = %d\n", *sock, local_addr_in->sin_addr.s_addr, local_addr_in->sin_port, *socket_len);
	ret =  &(p_response_msg->response_msg_list.response_bind_msg.ret);
	err =  &(p_response_msg->response_msg_list.response_bind_msg.err);

	if (NULL == inet_ntop(((addr_hdr *)local_addr)->sa_family, pj_sockaddr_get_addr((struct sockaddr *)local_addr), ipstring, sizeof(ipstring))) {
		softap_printf(USER_LOG, WARN_LOG,  "bind:sock = %d, ip addr is not legal!!!\n", *sock);
		//return;
	}
	port = ((struct sockaddr_in *)local_addr)->sin_port;

	//check  net is OK?????????

	if (((addr_hdr *)local_addr)->sa_family == AF_INET6) {
		int val = 1;
		*ret = setsockopt(*sock, SOL_IPV6, IPV6_V6ONLY, (char *)&val, sizeof(val));

		if (*ret < 0)
			softap_printf(USER_LOG, WARN_LOG,  "setsockopt: IPV6_V6ONLY  failed, ret = %d, errno = %d, errmsg = %s!!!!!!!!\n", *ret, errno, strerror(errno));

	}
	*ret = bind(*sock, local_addr, *socket_len);

	if (*ret < 0) {
		*err = errno;
		softap_printf(USER_LOG, WARN_LOG,  "bind:  socket = %d, bind on %s:%d, ret = %d, errno = %d, errmsg = %s\n", *sock, ipstring, ntohs(port), *ret, errno, strerror(errno));
	} else
		softap_printf(USER_LOG, WARN_LOG,  "bind:  socket = %d, bind on %s:%d\n", *sock, ipstring, ntohs(port));
	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}

void proc_listen_msg( proxy_request_listen_msg *listen_msg)
{

	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *sock, *backlog;
	int *err, *ret;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	sock = &(listen_msg->sock);
	backlog = &(listen_msg->backlog);
	ret =  &(p_response_msg->response_msg_list.response_listen_msg.ret);
	err =  &(p_response_msg->response_msg_list.response_listen_msg.err);

	*ret = listen(*sock, *backlog);
	if (*ret < 0) {
		*err = errno;
		softap_printf(USER_LOG, WARN_LOG,  "listen: sock = %d, backlog = %d, ret = %d, errno = %d, errmsg = %s\n", *sock, backlog, *ret, errno, strerror(errno));
	} else
		softap_printf(USER_LOG, WARN_LOG,  "listen: sock = %d, backlog = %d\n", *sock, backlog);
	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}

void proc_accept_msg( proxy_request_accept_msg *accept_msg)
{


	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *sock, *socklen;
	int *err, *ret;
	struct sockaddr *addr;
	//char ipstring[64] = {0};
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	sock = &(accept_msg->sock);
	socklen = &(p_response_msg->response_msg_list.response_accept_msg.socklen);
	*socklen = accept_msg->socklen;

	addr = (struct sockaddr *)p_response_msg->response_msg_list.response_accept_msg.addr;
	err = &(p_response_msg->response_msg_list.response_accept_msg.err);
	ret = &(p_response_msg->response_msg_list.response_accept_msg.ret);

	*ret = accept(*sock, addr, (socklen_t *)socklen);
	if (*ret < 0) {
		*err = errno;
	}

	softap_printf(USER_LOG, WARN_LOG,  "accept: sock = %d, ret = %d, errno = %d, errmsg = %s\n", *sock, *ret, errno, strerror(errno));

	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}

void proc_shutdown_msg( proxy_request_shutdown_msg *shutdown_msg)
{

	struct response_msg *p_response_msg =  xy_zalloc(sizeof(struct  response_msg) );
	int *sock, *how;
	int *ret, *err;
	//memset(p_response_msg,0,sizeof(struct  response_msg));
	sock = &(shutdown_msg->sock);
	how = &(shutdown_msg->how);
	ret = &(p_response_msg->response_msg_list.response_shutdown_msg.ret);
	err = &(p_response_msg->response_msg_list.response_shutdown_msg.err);

	*ret = shutdown(*sock, *how);
	if (*ret < 0) {
		*err = errno;
	}

	//softap_printf(USER_LOG, WARN_LOG,  "shutdown: sock = %d, how = %d, ret = %d, errno = %d, errmsg = %s\n", *sock, *how, *ret, errno, strerror(errno));
	write_to_cp( p_response_msg, sizeof(struct response_msg));
	xy_free(p_response_msg);
}


int set_proxy_fds(fd_set *fdr, fd_set *fdw, fd_set *fde, int channel_fd)
{
	int max_file = 0;


	if (fdr != NULL) {
		FD_ZERO(fdr);
		FD_SET(channel_fd, fdr);
	}

	if (fdw != NULL) {
		FD_ZERO(fdw);
		FD_SET(channel_fd, fdw);
	}

	if (fde != NULL) {
		FD_ZERO(fde);
		FD_SET(channel_fd, fde);
	}

	max_file = max_file > channel_fd ? max_file : channel_fd;

	return max_file;
}

#if XY_SOCKET_PROXY
void proxy_sock_task(void)
{
	struct request_msg *msg = NULL;

	proxy_request_recv_msg *temp_recv_msg;
	proxy_request_send_msg *temp_send_msg;
	proxy_request_connect_msg *temp_connect_msg;
	proxy_request_send_to_msg *temp_send_to_msg;
	proxy_request_bind_msg	*temp_bind_msg;
	proxy_request_recv_from_msg *temp_recv_from_msg;
	proxy_request_listen_msg *temp_listen_msg;
	proxy_request_accept_msg *temp_accept_msg;
	proxy_request_shutdown_msg *temp_shutdown_msg;
	proxy_request_close_msg *temp_close_msg;


	proxy_request_set_opt_msg	*temp_set_opt_msg;
	proxy_request_get_opt_msg	*temp_get_opt_msg;
	proxy_request_get_sock_name_msg *temp_get_sock_name_msg;
	proxy_request_get_peer_name_msg *temp_get_peer_name_msg;
	proxy_request_ioctrl_msg		*temp_ioctrl_msg;
	proxy_request_ioctrl_sock_msg	*temp_ioctrl_sock_msg;
	proxy_request_socket_msg	*temp_sock_msg;
	proxy_request_select_msg	*temp_select_msg;

	while (1) 
	{
		osMessageQueueGet(proxy_sock_msg_q, &msg, NULL, osWaitForever);

		if(msg == NULL)
		{
            xy_assert(0);
            continue;
		}
		
		switch (msg->request_msg_id) 
		{
			case PROXY_SOCK:
		
				temp_sock_msg = &(msg->request_msg_list.request_sock_msg);
				proc_open_sock_msg(temp_sock_msg);
				break;
			case PROXY_SEND:
				temp_send_msg = &(msg->request_msg_list.request_send_msg);
				proc_send_msg(temp_send_msg);
				break;
		
			case PROXY_RECV:
				temp_recv_msg = &(msg->request_msg_list.request_recv_msg);
				proc_recv_msg( temp_recv_msg);
				break;
		
			case PROXY_CONNECT:
				temp_connect_msg = &(msg->request_msg_list.request_connect_msg);
				proc_connect_msg(temp_connect_msg);
				break;
		
			case PROXY_SEND_TO:
				temp_send_to_msg = &(msg->request_msg_list.request_send_to_msg);
				proc_send_to_msg( temp_send_to_msg);
				break;
		
			case PROXY_RECV_FROM:
				temp_recv_from_msg = &(msg->request_msg_list.request_recv_from_msg);
				proc_recv_from_msg(temp_recv_from_msg);
				break;
			case PROXY_BIND:
				temp_bind_msg = &(msg->request_msg_list.request_bind_msg);
				proc_bind_msg(temp_bind_msg);
				break;
		
			case PROXY_LISTEN:
				temp_listen_msg = &(msg->request_msg_list.request_listen_msg);
				proc_listen_msg(temp_listen_msg);
				break;
		
			case PROXY_ACCEPT:
				temp_accept_msg = &(msg->request_msg_list.request_accept_msg);
				proc_accept_msg(temp_accept_msg);
				break;
		
			case PROXY_SHUTDOWN:
				temp_shutdown_msg = &(msg->request_msg_list.request_shutdown_msg);
				proc_shutdown_msg(temp_shutdown_msg);
				break;
		
			case PROXY_SETOPT:
				temp_set_opt_msg =	&(msg->request_msg_list.request_set_opt_msg);
				proc_set_opt_msg(temp_set_opt_msg);
				break;
			case PROXY_GETOPT:
				temp_get_opt_msg =	&(msg->request_msg_list.request_get_opt_msg);
				proc_get_opt_msg(temp_get_opt_msg);
				break;
			case PROXY_GETSOCKNAME:
				temp_get_sock_name_msg = &(msg->request_msg_list.request_get_sock_name_msg);
				proc_get_sockname_msg(temp_get_sock_name_msg);
				break;
			case PROXY_GETPEERNAME:
				temp_get_peer_name_msg = &(msg->request_msg_list.request_get_peer_name_msg);
				proc_get_peername_msg(temp_get_peer_name_msg);
				break;
			case PROXY_IOCTRL:
				temp_ioctrl_msg = &(msg->request_msg_list.request_ioctrl_msg);
				proc_ioctl_msg(temp_ioctrl_msg);
				break;
			case PROXY_IOCTRLSOCK:
				temp_ioctrl_sock_msg = &(msg->request_msg_list.request_ioctrl_sock_msg);
				proc_ioctl_socket_msg(temp_ioctrl_sock_msg);
				break;
			case PROXY_SELECT:
				temp_select_msg = &(msg->request_msg_list.request_select_msg);
				proc_select_msg(temp_select_msg);
				break;
			case PROXY_CLOSE:
				temp_close_msg = &(msg->request_msg_list.request_close_msg);
				proc_close_msg(temp_close_msg);

				break;
			default:
				//Log_PROXY(ERR, "messeg id is %d, ext_thread_id = %d\n", channel_info->channel_request_msg->request_msg_id, channel_info->ext_thread_id);
				softap_printf(USER_LOG, WARN_LOG, "messeg id is %d\n", msg->request_msg_id);
				break;;
		}
		
		xy_free(msg);
	}

}
volatile unsigned int g_debug_struct_size;
int send_msg_to_sockproxy(void *buf,unsigned int len)
{
	softap_printf(USER_LOG, WARN_LOG, "recv sockproxy from dsp!!");
	is_SRAM_addr((unsigned int)buf);

	g_debug_struct_size = sizeof(struct request_msg);
	if (len < sizeof(struct request_msg))
		xy_assert(0);

	struct request_msg *msg = xy_zalloc(len);

	proxy_sock_init();
	ps_netif_is_ok();

	if (buf != NULL)
	{
		memcpy(msg, buf, len);
	}
	else
	{
		xy_assert(0);
	}
	osMessageQueuePut(proxy_sock_msg_q, &msg, 0, osWaitForever);
	xy_free((void *)buf);
	return 1;
}
#endif

void proxy_sock_init(void)
{
	static int g_sockproxy_working = 0;
	osThreadAttr_t thread_attr = {0};
	if(g_sockproxy_working == 1)
		return;
	g_sockproxy_working = 1;
	proxy_sock_msg_q = osMessageQueueNew(20, sizeof(void *), NULL);
	thread_attr.name	   = "proxy_sock";
	thread_attr.priority   = osPriorityNormal1;
	thread_attr.stack_size = 0x1000;
	osThreadNew((osThreadFunc_t)(proxy_sock_task), NULL, &thread_attr);
}


