#if XY_DTLS

#include "mbedtls/aes.h"
#include "xy_utils.h"
#include "xy_aes.h"
#include "hw_types.h"

#define AES_BLOCK_SIZE 16

// AES CBC 256, PKCS7PADDING
int xy_encrypt(char* in_buf, int in_len, char* out_buf, int *out_len)
{
	unsigned char key[32];
	unsigned char iv[16];
	int nLen;
	int nTimes;
	int nTotal;
	unsigned char nNumber;
	unsigned char *enc_s = NULL;

	if (*out_len <= in_len || *out_len % AES_BLOCK_SIZE != 0) {
		return -1;
	}
	if(in_buf == NULL || out_buf == NULL) {
		return -1;
	}

	memset(key, 0, sizeof(key));
	memset(iv, 0, sizeof(iv));

	memcpy(key,(void *)HWREG(PUBLIC_KEY), sizeof(unsigned long));

	nLen = in_len;

	nTimes = nLen / AES_BLOCK_SIZE + 1;
	nTotal = nTimes * AES_BLOCK_SIZE;

	enc_s = ( unsigned char*)xy_zalloc(nTotal);
	if (enc_s == NULL)
	{
		softap_printf(USER_LOG, WARN_LOG, "enc_s mem err\n");
		return -1;
	}

	if (nLen % AES_BLOCK_SIZE > 0)
		nNumber = nTotal - nLen;
	else
		nNumber = AES_BLOCK_SIZE;

	memcpy(enc_s, in_buf, nLen);
	memset(enc_s + nLen, nNumber, nTotal - nLen);

	mbedtls_aes_context ctx;
	mbedtls_aes_init( &ctx );
	mbedtls_aes_setkey_enc( &ctx, (const unsigned char *)key, 256);
	mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_ENCRYPT, nTimes * AES_BLOCK_SIZE, iv, enc_s, (unsigned char *)out_buf);
	mbedtls_aes_free( &ctx );

	*out_len = nTimes * AES_BLOCK_SIZE;
	if(enc_s != NULL)
		xy_free(enc_s);
	enc_s = NULL;

	return 0;
}

int xy_decrypt(char* in_buf, int in_len, char* out_buf, int *out_len)
{
	unsigned char key[32];
	unsigned char iv[16];

	//int nTotal;
	unsigned char nNumber;
	char *temp_buf = NULL;
	int malloced = 0;

	if (in_buf == NULL || out_buf == NULL) {
		return -1;
	}
	if (in_len % AES_BLOCK_SIZE != 0) {
		return -1;
	}
	if (*out_len < in_len) {
		temp_buf = xy_zalloc(in_len);
		if (temp_buf == NULL) {
			return -1;
		}
		malloced = 1;
	}
	else {
		temp_buf = out_buf;
	}
	memset(key, 0, sizeof(key));
	memset(iv, 0, sizeof(iv));
	memcpy(key,(void *)HWREG(PUBLIC_KEY), sizeof(unsigned long));

	mbedtls_aes_context ctx;
	mbedtls_aes_init( &ctx );
	mbedtls_aes_setkey_dec( &ctx, (const unsigned char *)key, 256);
	mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_DECRYPT, in_len, iv, (const unsigned char *)in_buf, (unsigned char *)temp_buf);
	mbedtls_aes_free( &ctx );

	nNumber = out_buf[in_len - 1];
	*out_len = in_len - nNumber;
	if (malloced) {
		memcpy(out_buf, temp_buf, *out_len);
		xy_free(temp_buf);
	}
	return 0;
}

#endif
