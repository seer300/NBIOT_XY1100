#pragma once

#include <stdint.h>
#include <string.h>

#define UTILS_SHA256_DIGEST_LENGTH            (32)
#define UTILS_SHA256_BLOCK_LENGTH             (64)
#define UTILS_SHA256_SHORT_BLOCK_LENGTH       (UTILS_SHA256_BLOCK_LENGTH - 8)
#define UTILS_SHA256_DIGEST_STRING_LENGTH     (UTILS_SHA256_DIGEST_LENGTH * 2 + 1)

/**
 * \brief          SHA-256 context structure
 */
typedef struct {
    uint32_t total[2];          /*!< number of bytes processed  */
    uint32_t state[8];          /*!< intermediate digest state  */
    unsigned char buffer[64];   /*!< data block being processed */
    uint8_t is224;                  /*!< 0 => SHA-256, else SHA-224 */
} utils_sha256_context_t;

/**
 * \brief          Initialize SHA-256 context
 *
 * \param ctx      SHA-256 context to be initialized
 */
void utils_sha256_init(utils_sha256_context_t *ctx);

/**
 * \brief          Clear SHA-256 context
 *
 * \param ctx      SHA-256 context to be cleared
 */
void utils_sha256_free(utils_sha256_context_t *ctx);


/**
 * \brief          SHA-256 context setup
 *
 * \param ctx      context to be initialized
 */
void utils_sha256_starts(utils_sha256_context_t *ctx);

/**
 * \brief          SHA-256 process buffer
 *
 * \param ctx      SHA-256 context
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 */
void utils_sha256_update(utils_sha256_context_t *ctx, const unsigned char *input, uint32_t ilen);

/**
 * \brief          SHA-256 final digest
 *
 * \param ctx      SHA-256 context
 * \param output   SHA-256 checksum result
 */
void utils_sha256_finish(utils_sha256_context_t *ctx, uint8_t output[32]);

/* Internal use */
void utils_sha256_process(utils_sha256_context_t *ctx, const unsigned char data[64]);

/**
 * \brief          Output = SHA-256( input buffer )
 *
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 * \param output   SHA-256 checksum result
 */
void utils_sha256(const uint8_t *input, uint32_t ilen, uint8_t output[32]);

void utils_hmac_sha256(const uint8_t *msg, uint32_t msg_len, const uint8_t *key, uint32_t key_len, uint8_t output[32]);

void utils_hex2str(uint8_t *input, uint32_t input_len, char *output, uint8_t lowercase);

