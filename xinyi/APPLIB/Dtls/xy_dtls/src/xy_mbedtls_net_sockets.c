#include "softap_macro.h"
#if 1

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if !defined(MBEDTLS_NET_C)


#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#endif

#include "mbedtls/net_sockets.h"

#include "lwip/sockets.h"
#include "lwip/opt.h"
#include "lwip/netdb.h"
#include "lwip/errno.h"

void xy_mbedtls_net_init(mbedtls_net_context *ctx)
{
    ctx->fd = -1;
}

void *xy_mbedtls_net_connect(const char *host, const char *port, int proto)
{
	int ret;
	struct addrinfo hints = {0}, *addr_list, *cur;
	mbedtls_net_context *ctx = mbedtls_calloc(1, sizeof(mbedtls_net_context));

	ctx->fd = -1;

	/* Do name resolution with both IPv6 and IPv4 */
	memset( &hints, 0, sizeof( hints ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = proto == MBEDTLS_NET_PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;
	hints.ai_protocol = proto == MBEDTLS_NET_PROTO_UDP ? IPPROTO_UDP : IPPROTO_TCP;

	ret = getaddrinfo( host, port, &hints, &addr_list );

	/* Try the sockaddrs until a connection succeeds */
	for( cur = addr_list; cur != NULL && ret == 0; cur = cur->ai_next )
	{
		ctx->fd = (int) socket( cur->ai_family, cur->ai_socktype,
							cur->ai_protocol );
		if( ctx->fd < 0 )
		{
			ret = MBEDTLS_ERR_NET_SOCKET_FAILED;
			continue;
		}

		if( connect( ctx->fd, cur->ai_addr, cur->ai_addrlen ) == 0 )
		{
			ret = 0;
			break;
		}

		close( ctx->fd );
		ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
	}

	freeaddrinfo( addr_list );

	if (ret != 0)
	{
		mbedtls_free(ctx);
		ctx = NULL;
	}

	return ( ctx );
}

void xy_mbedtls_net_usleep(unsigned long usec)
{

}

int xy_mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len)
{
	int ret;
	int fd = ((mbedtls_net_context *) ctx)->fd;

	if( fd < 0 )
		return( MBEDTLS_ERR_NET_INVALID_CONTEXT );

	ret = (int) read( fd, buf, len );

	if( ret < 0 )
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
		{
			return( MBEDTLS_ERR_SSL_WANT_READ );
		}
		else if (ret == 0)
		{
			return( MBEDTLS_ERR_NET_CONN_RESET );
		}

		return( MBEDTLS_ERR_NET_RECV_FAILED );
	}

	return( ret );
}

int xy_mbedtls_net_recv_timeout(void *ctx, unsigned char *buf, size_t len,
                             uint32_t timeout)
{
	int ret;
	struct timeval tv;
	fd_set read_fds;
	int fd = ((mbedtls_net_context *) ctx)->fd;

	if( fd < 0 )
		return( MBEDTLS_ERR_NET_INVALID_CONTEXT );

	FD_ZERO( &read_fds );
	FD_SET( fd, &read_fds );

	tv.tv_sec  = timeout / 1000;
	tv.tv_usec = ( timeout % 1000 ) * 1000;

	ret = select( fd + 1, &read_fds, NULL, NULL, timeout == 0 ? NULL : &tv );

	/* Zero fds ready means we timed out */
	if( ret == 0 )
		return( MBEDTLS_ERR_SSL_TIMEOUT );

	if( ret < 0 )
	{
		if( errno == EINTR )
			return( MBEDTLS_ERR_SSL_WANT_READ );

		return( MBEDTLS_ERR_NET_RECV_FAILED );
	}

	/* This call will not block */
	return( xy_mbedtls_net_recv( ctx, buf, len ) );
}

int xy_mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len)
{
	int ret;
	int fd = ((mbedtls_net_context *) ctx)->fd;

	if( fd < 0 )
		return( MBEDTLS_ERR_NET_INVALID_CONTEXT );

	ret = (int) write( fd, buf, len );

	if( ret < 0 )
	{
		/* no data available for now */
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
		{
			return( MBEDTLS_ERR_SSL_WANT_WRITE );
		}
		else
		{
			return( MBEDTLS_ERR_NET_CONN_RESET );
		}

		return( MBEDTLS_ERR_NET_SEND_FAILED );
	}

	return( ret );
}

void xy_mbedtls_net_free(mbedtls_net_context *ctx)
{
	if( ctx->fd == -1 )
		return;

	shutdown( ctx->fd, 2 );
	close( ctx->fd );

	ctx->fd = -1;

	xy_free(ctx);
}
#endif /* MBEDTLS_NET_C */

#endif

