/*******************************************************************************
 *
 * Copyright (c) 2016 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Simon Bernard - xiny_initial API and implementation
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
#include "xiny_internals.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// the maximum payload transferred by block1 we accumulate per server
#define MAX_BLOCK1_SIZE 4096

uint8_t xiny_coap_block1_handler(xiny_lwm2m_block1_data_t ** pBlock1Data,
                            uint16_t mid,
                            uint8_t * buffer,
                            size_t length,
                            uint16_t blockSize,
                            uint32_t blockNum,
                            bool blockMore,
                            uint8_t ** outputBuffer,
                            size_t * outputLength)
{
    xiny_lwm2m_block1_data_t * block1Data = *pBlock1Data;;

    // manage new block1 transfer
    if (blockNum == 0)
    {
       // we already have block1 data for this server, clear it
       if (block1Data != NULL)
       {
           xiny_lwm2m_free(block1Data->block1buffer);
       }
       else
       {
           block1Data = xiny_lwm2m_malloc(sizeof(xiny_lwm2m_block1_data_t));
           *pBlock1Data = block1Data;
           if (NULL == block1Data) return xiny_COAP_500_INTERNAL_SERVER_ERROR;
       }

       block1Data->block1buffer = xiny_lwm2m_malloc(length);
       block1Data->block1bufferSize = length;

       // write new block in buffer
       memcpy(block1Data->block1buffer, buffer, length);
       block1Data->lastmid = mid;
    }
    // manage already xiny_started block1 transfer
    else
    {
       if (block1Data == NULL)
       {
           // we never receive the first block
           // TODO should we clean block1 data for this server ?
           return xiny_COAP_408_REQ_ENTITY_INCOMPLETE;
       }

       // If this is a retransmission, we already did that.
       if (block1Data->lastmid != mid)
       {
          uint8_t * oldBuffer = block1Data->block1buffer;
          size_t oldSize = block1Data->block1bufferSize;

          if (block1Data->block1bufferSize != blockSize * blockNum)
          {
              // we don't receive block in right order
              // TODO should we clean block1 data for this server ?
              return xiny_COAP_408_REQ_ENTITY_INCOMPLETE;
          }

          // is it too large?
          if (block1Data->block1bufferSize + length >= MAX_BLOCK1_SIZE) {
              return xiny_COAP_413_ENTITY_TOO_LARGE;
          }
          // re-alloc new buffer
          block1Data->block1bufferSize = oldSize+length;
          block1Data->block1buffer = xiny_lwm2m_malloc(block1Data->block1bufferSize);
          if (NULL == block1Data->block1buffer) return xiny_COAP_500_INTERNAL_SERVER_ERROR;
          memcpy(block1Data->block1buffer, oldBuffer, oldSize);
          xiny_lwm2m_free(oldBuffer);

          // write new block in buffer
          memcpy(block1Data->block1buffer + oldSize, buffer, length);
          block1Data->lastmid = mid;
       }
    }

    if (blockMore)
    {
        *outputLength = -1;
        return xiny_COAP_231_CONTINUE;
    }
    else
    {
        // buffer is full, set output parameter
        // we don't free it to be able to send retransmission
        *outputLength = block1Data->block1bufferSize;
        *outputBuffer = block1Data->block1buffer;

        return xiny_NO_ERROR;
    }
}

void xiny_free_block1_buffer(xiny_lwm2m_block1_data_t * block1Data)
{
    if (block1Data != NULL)
    {
        // free block1 buffer
        xiny_lwm2m_free(block1Data->block1buffer);
        block1Data->block1bufferSize = 0 ;

        // free current element
        xiny_lwm2m_free(block1Data);
    }
}
