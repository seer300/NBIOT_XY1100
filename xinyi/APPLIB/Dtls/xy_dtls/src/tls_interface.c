/*
 *  TLS client interface, only used for https now
 */

#include "softap_macro.h"
#if 1

#include <string.h>
#include "tls_interface.h"
#include "mbedtls/net_sockets.h"
#include "xy_mem.h"
//#include "xy_printf.h"
#include "rtc_tmr.h"
#include "xy_mbedtls_net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_internal.h"
//#define XY_TLS_DEBUG

#ifdef XY_TLS_DEBUG
#define XY_TLS_LOG(fmt, ...) \
    do \
    { \
        (void)xy_printf("[XY_TLS][%s:%d] " fmt "\r\n", \
        __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#else
#define XY_TLS_LOG(fmt, ...) ((void)0)
#endif

#define XY_TLS_LOG_BUF_SIZE 64

static int xy_tls_printf(const char *format, ...)
{
	int ret;
    char str_buf[XY_TLS_LOG_BUF_SIZE] = {0};
    va_list list;

    memset(str_buf, 0, XY_TLS_LOG_BUF_SIZE);
    va_start(list, format);
    ret = vsnprintf(str_buf, XY_TLS_LOG_BUF_SIZE, format, list);
    va_end(list);

    ret = xy_printf("%s", str_buf);

    return ret;
}

static void* xy_tls_calloc(uint32_t n, uint32_t size)
{
	return xy_zalloc(n *size);
}

static void xy_tls_free(void *ptr)
{
	if(ptr == NULL)
		return;
	xy_free(ptr);
}

mbedtls_ssl_context *xy_tls_ssl_new(tls_establish_info_s* info)
{
    int ret;
    mbedtls_ssl_context *ssl = NULL;
    mbedtls_ssl_config *conf = NULL;
    mbedtls_entropy_context *entropy = NULL;
    mbedtls_ctr_drbg_context *ctr_drbg = NULL;
    mbedtls_timing_delay_context *timer = NULL;
    mbedtls_x509_crt *cacert = NULL;
    mbedtls_x509_crt *client_cert = NULL;
    mbedtls_pk_context *client_pk = NULL;

    const char *pers = "ssl_client";

    xy_tls_init();

    ssl       = mbedtls_calloc(1, sizeof(mbedtls_ssl_context));
    conf      = mbedtls_calloc(1, sizeof(mbedtls_ssl_config));
    entropy   = mbedtls_calloc(1, sizeof(mbedtls_entropy_context));
    ctr_drbg  = mbedtls_calloc(1, sizeof(mbedtls_ctr_drbg_context));
    cacert    = mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
    if (info->client_cert != NULL && info->client_pk != NULL)
    {
    	client_cert = mbedtls_calloc(1, sizeof(mbedtls_x509_crt));
    	if (client_cert == NULL)
    	{
			XY_TLS_LOG("client_cert calloc failed in xy_tls_ssl_new");
			goto exit_fail;
    	}
    	client_pk = mbedtls_calloc(1, sizeof(mbedtls_pk_context));
    	if (client_pk == NULL)
    	{
			XY_TLS_LOG("client_pk calloc failed in xy_tls_ssl_new");
			goto exit_fail;
    	}
    }

    if (NULL == ssl
        || NULL == conf || NULL == entropy
        || NULL == ctr_drbg
        || NULL == cacert
        )
	{
		XY_TLS_LOG("mbedtls_calloc failed in xy_tls_ssl_new");
		goto exit_fail;
	}

    mbedtls_ssl_init(ssl);
    mbedtls_ssl_config_init(conf);
    mbedtls_ctr_drbg_init(ctr_drbg);
    mbedtls_entropy_init(entropy);

    mbedtls_x509_crt_init(cacert);
    if (client_cert != NULL)
    	mbedtls_x509_crt_init(client_cert);
    if (client_pk != NULL)
    	mbedtls_pk_init(client_pk);

    if ((ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy,
                                     (const unsigned char *) pers,
                                     strlen(pers))) != 0)
    {
        XY_TLS_LOG("mbedtls_ctr_drbg_seed failed: -0x%x", -ret);
    	{
    		goto exit_fail;
    	}
    }

    XY_TLS_LOG("setting up the SSL structure");

    ret = mbedtls_ssl_config_defaults(conf,
    		MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_STREAM,
			MBEDTLS_SSL_PRESET_DEFAULT);

    if (ret != 0)
    {
        XY_TLS_LOG("mbedtls_ssl_config_defaults failed: -0x%x", -ret);
    	{
    		goto exit_fail;
    	}
    }

    mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_rng(conf, mbedtls_ctr_drbg_random, ctr_drbg);

    mbedtls_ssl_conf_read_timeout(conf, TLS_SHAKEHAND_TIMEOUT);

    ret = mbedtls_x509_crt_parse(cacert, info->ca_cert, info->ca_cert_len);
	if(ret < 0)
	{
		XY_TLS_LOG("mbedtls_x509_crt_parse failed -0x%x", -ret);
		{
			goto exit_fail;
		}
	}
	mbedtls_ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_ca_chain(conf, cacert, NULL);

	if (client_cert != NULL && client_pk != NULL)
	{
		ret = mbedtls_x509_crt_parse(client_cert, info->client_cert, info->client_cert_len);
		if(ret < 0)
		{
			XY_TLS_LOG("mbedtls_x509_crt_parse failed -0x%x", -ret);
			{
				goto exit_fail;
			}
		}

		ret = mbedtls_pk_parse_key(client_pk, info->client_pk, info->client_pk_len, "", 0);
		if(ret < 0)
		{
			XY_TLS_LOG("mbedtls_pk_parse_key failed -0x%x", -ret);
			{
				goto exit_fail;
			}
		}

		ret = mbedtls_ssl_conf_own_cert(conf, client_cert, client_pk);
		if(ret < 0)
		{
			XY_TLS_LOG("mbedtls_ssl_conf_own_cert failed -0x%x", -ret);
			{
				goto exit_fail;
			}
		}
	}

    if ((ret = mbedtls_ssl_setup(ssl, conf)) != 0)
    {
        XY_TLS_LOG("mbedtls_ssl_setup failed: -0x%x", -ret);
    	{
    		goto exit_fail;
    	}
    }

    XY_TLS_LOG("set SSL structure succeed");

    return ssl;

exit_fail:

    if (conf)
    {
        mbedtls_ssl_config_free(conf);
        mbedtls_free(conf);
    }

    if (ctr_drbg)
    {
        mbedtls_ctr_drbg_free(ctr_drbg);
        mbedtls_free(ctr_drbg);
    }

    if (entropy)
    {
        mbedtls_entropy_free(entropy);
        mbedtls_free(entropy);
    }

    if (timer)
    {
        mbedtls_free(timer);
    }

    if (cacert)
    {
        mbedtls_x509_crt_free(cacert);
        mbedtls_free(cacert);
    }
    if (client_cert)
	{
		mbedtls_x509_crt_free(client_cert);
		mbedtls_free(client_cert);
	}
    if (client_pk)
	{
    	mbedtls_pk_free(client_pk);
		mbedtls_free(client_pk);
	}

    if (ssl)
    {
        mbedtls_ssl_free(ssl);
        mbedtls_free(ssl);
    }
    return NULL;
}

static inline uint32_t xy_tls_gettime()
{
    return (uint32_t)(osKernelGetTickCount() / 1000);
}

int xy_tls_shakehand(mbedtls_ssl_context *ssl, const tls_shakehand_info_s *info)
{
    int ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
    uint32_t change_value = 0;
    mbedtls_net_context *server_fd = NULL;
    uint32_t max_value;
    unsigned int flags;

    XY_TLS_LOG("connecting to server");

    server_fd = xy_mbedtls_net_connect(info->host, info->port, MBEDTLS_NET_PROTO_TCP);

    if (server_fd == NULL)
    {
        XY_TLS_LOG("connect failed! mode %d", info->client_or_server);
        ret = MBEDTLS_ERR_NET_CONNECT_FAILED;
        goto exit_fail;
    }

    mbedtls_ssl_set_bio(ssl, server_fd,
    		xy_mbedtls_net_send, xy_mbedtls_net_recv, xy_mbedtls_net_recv_timeout);

    XY_TLS_LOG("performing the SSL/TLS handshake");

//    max_value = ((MBEDTLS_SSL_IS_SERVER == info->client_or_server || info->udp_or_tcp == MBEDTLS_NET_PROTO_UDP) ?
//                (tls_gettime() + info->timeout) :  50);
    max_value = 50;

    do
    {
        ret = mbedtls_ssl_handshake(ssl);
        XY_TLS_LOG("mbedtls_ssl_handshake %d %d", change_value, max_value);

        change_value++;

        if (info->step_notify)
        {
            info->step_notify(info->param);
        }
    }
    while ((ret == MBEDTLS_ERR_SSL_WANT_READ ||
            ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
            (ret == MBEDTLS_ERR_SSL_TIMEOUT)) &&
            (change_value < max_value));

    if (info->finish_notify)
    {
        info->finish_notify(info->param);
    }

    if (ret != 0)
    {
        XY_TLS_LOG("mbedtls_ssl_handshake failed: -0x%x", -ret);
        goto exit_fail;
    }

    if((flags = mbedtls_ssl_get_verify_result(ssl)) != 0)
	{
		char vrfy_buf[512];
		mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);
		XY_TLS_LOG("cert verify failed: %s", vrfy_buf);
		goto exit_fail;
	}
	else
		XY_TLS_LOG("cert verify succeed");


    XY_TLS_LOG("handshake succeed");

    return 0;

exit_fail:

    if (server_fd)
    {
        xy_mbedtls_net_free(server_fd);
        ssl->p_bio = NULL;
    }

    return ret;

}
void xy_tls_ssl_destroy(mbedtls_ssl_context *ssl)
{
    mbedtls_ssl_config           *conf = NULL;
    mbedtls_ctr_drbg_context     *ctr_drbg = NULL;
    mbedtls_entropy_context      *entropy = NULL;
    mbedtls_net_context          *server_fd = NULL;
    mbedtls_timing_delay_context *timer = NULL;
    mbedtls_x509_crt             *cacert = NULL;
    mbedtls_x509_crt *client_cert = NULL;
    mbedtls_pk_context *client_pk = NULL;

    if (ssl == NULL)
    {
        return;
    }

    conf       = ssl->conf;
    server_fd  = (mbedtls_net_context *)ssl->p_bio;
    timer      = (mbedtls_timing_delay_context *)ssl->p_timer;
    cacert     = (mbedtls_x509_crt *)conf->ca_chain;
    if(conf->key_cert != NULL)
    {
        client_cert = conf->key_cert->cert;
        client_pk  =  conf->key_cert->key;
    }

    if (conf)
    {
        ctr_drbg   = conf->p_rng;

        if (ctr_drbg)
        {
            entropy =  ctr_drbg->p_entropy;
        }
    }

    if (server_fd)
    {
        xy_mbedtls_net_free(server_fd);
    }

    if (conf)
    {
        mbedtls_ssl_config_free(conf);
        mbedtls_free(conf);
        ssl->conf = NULL; //  need by mbedtls_debug_print_msg(), see mbedtls_ssl_free(ssl)
    }

    if (ctr_drbg)
    {
        mbedtls_ctr_drbg_free(ctr_drbg);
        mbedtls_free(ctr_drbg);
    }

    if (entropy)
    {
        mbedtls_entropy_free(entropy);
        mbedtls_free(entropy);
    }

    if (timer)
    {
        mbedtls_free(timer);
    }

    if (cacert)
    {
        mbedtls_x509_crt_free(cacert);
        mbedtls_free(cacert);
    }

    if (client_cert)
	{
		mbedtls_x509_crt_free(client_cert);
		mbedtls_free(client_cert);
	}
    if (client_pk)
	{
    	mbedtls_pk_free(client_pk);
		mbedtls_free(client_pk);
	}

    mbedtls_ssl_free(ssl);
    mbedtls_free(ssl);
}

int xy_tls_write(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len)
{
    int ret = mbedtls_ssl_write(ssl, (unsigned char *) buf, len);

    if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
    {
        return -3;
    }
    else if (ret < 0)
    {
        return -1;
    }

    return ret;
}

int xy_tls_read(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len, uint32_t timeout)
{
    int ret;

    mbedtls_ssl_conf_read_timeout(ssl->conf, timeout);

    ret = mbedtls_ssl_read(ssl, buf, len);

    if (ret == MBEDTLS_ERR_SSL_WANT_READ)
    {
        return -3;
    }
    else if (ret == MBEDTLS_ERR_SSL_TIMEOUT)
    {
        return -2;
    }

    return ret;
}

void xy_tls_init(void)
{
    (void)mbedtls_platform_set_calloc_free(xy_tls_calloc, xy_tls_free);
//    (void)mbedtls_platform_set_snprintf(snprintf);
    (void)mbedtls_platform_set_printf(xy_tls_printf);
    (void)mbedtls_platform_set_time(xy_tls_gettime);
}

#endif
