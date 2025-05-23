/*******************************************************************************
 *
 * Copyright (c) 2016 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Simon Bernard - initial API and implementation
 *
 *******************************************************************************/
/*
 Copyright (c) 2016 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "softap_macro.h"
#if MOBILE_VER
#include "xy_utils.h"
#include "cis_internals.h"

#if CIS_ENABLE_BLOCK
coap_status_t coap_block1_handler(cis_block1_data_t ** pBlock1Data,
                                  uint16_t mid,
                                  uint8_t * buffer,
                                  size_t length,
                                  uint16_t blockSize,
                                  uint32_t blockNum,
                                  bool blockMore,
                                  uint8_t ** outputBuffer,
                                  uint32_t * outputLength)
{
    cis_block1_data_t * block1Data = *pBlock1Data;

    // manage new block1 transfer
    if (blockNum == 0)
    {
       // we already have block1 data for this server, clear it
       if (block1Data != NULL)
       {
           cis_free(block1Data->block1buffer);
       }
       else
       {
           block1Data = (cis_block1_data_t*)cis_malloc(sizeof(cis_block1_data_t));
           *pBlock1Data = block1Data;
           if (NULL == block1Data) return COAP_500_INTERNAL_SERVER_ERROR;
       }

       block1Data->block1buffer = (uint8_t *)cis_malloc(length);
       block1Data->block1bufferSize = length;

       // write new block in buffer
       cis_memcpy(block1Data->block1buffer, buffer, length);
       block1Data->lastmid = mid;
    }
    // manage already started block1 transfer
    else
    {
       if (block1Data == NULL)
       {
           // we never receive the first block
           // TODO should we clean block1 data for this server ?
           return COAP_408_REQ_ENTITY_INCOMPLETE;
       }

       // If this is a retransmission, we already did that.
       if (block1Data->lastmid != mid)
       {
           if (block1Data->block1bufferSize != blockSize * blockNum)
          {
              // we don't receive block in right order
              // TODO should we clean block1 data for this server ?
              return COAP_408_REQ_ENTITY_INCOMPLETE;
          }

          // is it too large?
          if (block1Data->block1bufferSize + length >= CIS_CONFIG_BLOCK1_SIZE_MAX) {
              return COAP_413_ENTITY_TOO_LARGE;
          }
          // re-alloc new buffer
          uint8_t * oldBuffer = block1Data->block1buffer;
          size_t oldSize = block1Data->block1bufferSize;
          block1Data->block1bufferSize = oldSize+length;
          block1Data->block1buffer = (uint8_t*)cis_malloc(block1Data->block1bufferSize);
          if (NULL == block1Data->block1buffer) return COAP_500_INTERNAL_SERVER_ERROR;
          cis_memcpy(block1Data->block1buffer, oldBuffer, oldSize);
          cis_free(oldBuffer);

          // write new block in buffer
          cis_memcpy(block1Data->block1buffer + oldSize, buffer, length);
          block1Data->lastmid = mid;
       }
    }

    if (blockMore)
    {
        *outputLength = 0;
        return COAP_231_CONTINUE;
    }
    else
    {
        // buffer is full, set output parameter
        // we don't free it to be able to send retransmission
        *outputLength = block1Data->block1bufferSize;
        *outputBuffer = block1Data->block1buffer;

        return COAP_NO_ERROR;
    }
}

void free_block1_buffer(cis_block1_data_t * block1Data)
{
    if (block1Data != NULL)
    {
        // free block1 buffer
        cis_free(block1Data->block1buffer);
        block1Data->block1bufferSize = 0 ;

        // free current element
        cis_free(block1Data);
    }
}

#endif//CIS_ENABLE_BLOCK
#endif