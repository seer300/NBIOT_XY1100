/**
 * Copyright (c) 2017 China Mobile IOT.
 * All rights reserved.
 * Reference:
 *  tinydtls - https://sourceforge.net/projects/tinydtls/?source=navbar
**/
#include "softap_macro.h"
#if MOBILE_VER
#include "xy_utils.h"
#include "ccm.h"
#include "global.h"
#include "numeric.h"
#include "cis_internals.h"

#define CCM_FLAGS(A,M,L) (((A > 0) << 6) | (((M - 2)/2) << 3) | (L - 1))

#define MASK_L(_L)      ((1 << 8 * _L) - 1)

#define SET_COUNTER(A,L,cnt,C) \
{ \
    size_t i; \
    cis_memset( (A)+DTLS_CCM_BLOCKSIZE - (L),0, (L) ); \
    (C) = (cnt)& MASK_L( L ); \
    for ( i = DTLS_CCM_BLOCKSIZE - 1; (C) && (i > (L)); --i, (C) >>= 8 ) \
        (A)[i] |= (C)& 0xFF; \
}

static inline void block0( size_t         M,       /* number of auth bytes */
                           size_t         L,       /* number of bytes to encode message length */
                           size_t         la,      /* l(a) octets additional authenticated data */
                           size_t         lm,      /* l(m) message length */
                           unsigned char  nonce[DTLS_CCM_BLOCKSIZE],
                           unsigned char *result )
{
    size_t i;

    result[0] = CCM_FLAGS( la, M, L );

    /* copy the nonce */
    cis_memmove( result + 1, nonce, DTLS_CCM_BLOCKSIZE - L );

    for ( i = 0; i < L; i++ )
    {
        result[15 - i] = lm & 0xff;
        lm >>= 8;
    }
}

/**
 * Creates the CBC-MAC for the additional authentication data that
 * is sent in cleartext.
 *
 * \param ctx  The crypto context for the AES encryption.
 * \param msg  The message starting with the additional authentication data.
 * \param la   The number of additional authentication bytes in \p msg.
 * \param B    The input buffer for crypto operations. When this function
 *             is called, \p B must be initialized with \c B0 (the first
 *             authentication block.
 * \param X    The output buffer where the result of the CBC calculation
 *             is placed.
 * \return     The result is written to \p X.
*/
static void add_auth_data( rijndael_ctx        *ctx,
                           const unsigned char *msg,
                           size_t               la,
                           unsigned char        B[DTLS_CCM_BLOCKSIZE],
                           unsigned char        X[DTLS_CCM_BLOCKSIZE] )
{
    size_t i, j;

    rijndael_encrypt( ctx, B, X );

    cis_memset( B,0, DTLS_CCM_BLOCKSIZE );

    if ( !la )
        return;

    if ( la < 0xFF00 )
    { /* 2^16 - 2^8 */
        j = 2;
        dtls_int_to_uint16( B, la );
    }
    else if ( la <= UINT32_MAX )
    {
        j = 6;
        dtls_int_to_uint16( B, 0xFFFE );
        dtls_int_to_uint32( B + 2, la );
    }
    else
    {
        j = 10;
        dtls_int_to_uint16( B, 0xFFFF );
        dtls_int_to_uint64( B + 2, la );
    }

    i = min( DTLS_CCM_BLOCKSIZE - j, la );
    cis_memmove( B + j, msg, i );
    la -= i;
    msg += i;

    memxor( B, X, DTLS_CCM_BLOCKSIZE );

    rijndael_encrypt( ctx, B, X );

    while ( la > DTLS_CCM_BLOCKSIZE )
    {
        for ( i = 0; i < DTLS_CCM_BLOCKSIZE; ++i )
            B[i] = X[i] ^ *msg++;
        la -= DTLS_CCM_BLOCKSIZE;

        rijndael_encrypt( ctx, B, X );
    }

    if ( la )
    {
        cis_memset( B,0, DTLS_CCM_BLOCKSIZE );
        cis_memmove( B, msg, la );
        memxor( B, X, DTLS_CCM_BLOCKSIZE );

        rijndael_encrypt( ctx, B, X );
    }
}

static inline void encrypt( rijndael_ctx  *ctx,
                            size_t         L,
                            unsigned long  counter,
                            unsigned char *msg,
                            size_t         len,
                            unsigned char  A[DTLS_CCM_BLOCKSIZE],
                            unsigned char  S[DTLS_CCM_BLOCKSIZE] )
{

    static unsigned long counter_tmp;

    SET_COUNTER( A, L, counter, counter_tmp );
    rijndael_encrypt( ctx, A, S );
    memxor( msg, S, len );
}

static inline void mac( rijndael_ctx  *ctx,
                        unsigned char *msg,
                        size_t         len,
                        unsigned char  B[DTLS_CCM_BLOCKSIZE],
                        unsigned char  X[DTLS_CCM_BLOCKSIZE] )
{
    size_t i;

    for ( i = 0; i < len; ++i )
        B[i] = X[i] ^ msg[i];

    rijndael_encrypt( ctx, B, X );

}

long int dtls_ccm_encrypt_message( rijndael_ctx        *ctx,
                                   size_t               M,
                                   size_t               L,
                                   unsigned char        nonce[DTLS_CCM_BLOCKSIZE],
                                   unsigned char       *msg,
                                   size_t               lm,
                                   const unsigned char *aad,
                                   size_t               la )
{
    size_t i, len;
    unsigned long counter_tmp;
    unsigned long counter = 1; /* \bug does not work correctly on ia32 when
                               lm >= 2^16 */
    unsigned char A[DTLS_CCM_BLOCKSIZE]; /* A_i blocks for encryption input */
    unsigned char B[DTLS_CCM_BLOCKSIZE]; /* B_i blocks for CBC-MAC input */
    unsigned char S[DTLS_CCM_BLOCKSIZE]; /* S_i = encrypted A_i blocks */
    unsigned char X[DTLS_CCM_BLOCKSIZE]; /* X_i = encrypted B_i blocks */

    len = lm;			/* save original length */
    /* create the initial authentication block B0 */
    block0( M, L, la, lm, nonce, B );
    add_auth_data( ctx, aad, la, B, X );

    /* initialize block template */
    A[0] = L - 1;

    /* copy the nonce */
    cis_memmove( A + 1, nonce, DTLS_CCM_BLOCKSIZE - L );

    while ( lm >= DTLS_CCM_BLOCKSIZE )
    {
        /* calculate MAC */
        mac( ctx, msg, DTLS_CCM_BLOCKSIZE, B, X );

        /* encrypt */
        encrypt( ctx, L, counter, msg, DTLS_CCM_BLOCKSIZE, A, S );

        /* update local pointers */
        lm -= DTLS_CCM_BLOCKSIZE;
        msg += DTLS_CCM_BLOCKSIZE;
        counter++;
    }

    if ( lm )
    {
        /* Calculate MAC. The remainder of B must be padded with zeroes, so
        * B is constructed to contain X ^ msg for the first lm bytes (done in
        * mac() and X ^ 0 for the remaining DTLS_CCM_BLOCKSIZE - lm bytes
        * (i.e., we can use cissys_memmove() here).
        */
        cis_memmove( B + lm, X + lm, DTLS_CCM_BLOCKSIZE - lm );
        mac( ctx, msg, lm, B, X );

        /* encrypt */
        encrypt( ctx, L, counter, msg, lm, A, S );

        /* update local pointers */
        msg += lm;
    }

    /* calculate S_0 */
    SET_COUNTER( A, L, 0, counter_tmp );
    rijndael_encrypt( ctx, A, S );

    for ( i = 0; i < M; ++i )
        *msg++ = X[i] ^ S[i];

    return len + M;
}

long int dtls_ccm_decrypt_message( rijndael_ctx        *ctx,
                                   size_t               M,
                                   size_t               L,
                                   unsigned char        nonce[DTLS_CCM_BLOCKSIZE],
                                   unsigned char       *msg,
                                   size_t               lm,
                                   const unsigned char *aad,
                                   size_t               la )
{

    size_t len;
    unsigned long counter_tmp;
    unsigned long counter = 1; /* \bug does not work correctly on ia32 when
                               lm >= 2^16 */
    unsigned char A[DTLS_CCM_BLOCKSIZE]; /* A_i blocks for encryption input */
    unsigned char B[DTLS_CCM_BLOCKSIZE]; /* B_i blocks for CBC-MAC input */
    unsigned char S[DTLS_CCM_BLOCKSIZE]; /* S_i = encrypted A_i blocks */
    unsigned char X[DTLS_CCM_BLOCKSIZE]; /* X_i = encrypted B_i blocks */

    if ( lm < M )
        goto error;

    len = lm;	      /* save original length */
    lm -= M;	      /* detract MAC size*/

    /* create the initial authentication block B0 */
    block0( M, L, la, lm, nonce, B );
    add_auth_data( ctx, aad, la, B, X );

    /* initialize block template */
    A[0] = L - 1;

    /* copy the nonce */
    cis_memmove( A + 1, nonce, DTLS_CCM_BLOCKSIZE - L );

    while ( lm >= DTLS_CCM_BLOCKSIZE )
    {
        /* decrypt */
        encrypt( ctx, L, counter, msg, DTLS_CCM_BLOCKSIZE, A, S );

        /* calculate MAC */
        mac( ctx, msg, DTLS_CCM_BLOCKSIZE, B, X );

        /* update local pointers */
        lm -= DTLS_CCM_BLOCKSIZE;
        msg += DTLS_CCM_BLOCKSIZE;
        counter++;
    }

    if ( lm )
    {
        /* decrypt */
        encrypt( ctx, L, counter, msg, lm, A, S );

        /* Calculate MAC. Note that msg ends in the MAC so we must
        * construct B to contain X ^ msg for the first lm bytes (done in
        * mac() and X ^ 0 for the remaining DTLS_CCM_BLOCKSIZE - lm bytes
        * (i.e., we can use cissys_memmove() here).
        */
        cis_memmove( B + lm, X + lm, DTLS_CCM_BLOCKSIZE - lm );
        mac( ctx, msg, lm, B, X );

        /* update local pointers */
        msg += lm;
    }

    /* calculate S_0 */
    SET_COUNTER( A, L, 0, counter_tmp );
    rijndael_encrypt( ctx, A, S );

    memxor( msg, S, M );

    /* return length if MAC is valid, otherwise continue with error handling */
    if ( equals( X, msg, M ) )
        return len - M;

error:
    return -1;
}
#endif