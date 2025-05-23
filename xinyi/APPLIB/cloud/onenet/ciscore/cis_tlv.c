/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
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
 *    David Navarro, Intel Corporation - initial API and implementation
 *    Fabien Fleutot - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Baijie & Longrong, China Mobile - Please refer to git log
 *    
 *******************************************************************************/
#include "softap_macro.h"
#if MOBILE_VER
#include "xy_utils.h"
#include "cis_internals.h"
#include "cis_log.h" 
#include <float.h>
 

#if !defined LWM2M_BIG_ENDIAN && !defined LWM2M_LITTLE_ENDIAN
#error Please define LWM2M_BIG_ENDIAN or LWM2M_LITTLE_ENDIAN
#endif

#define _PRV_TLV_TYPE_MASK 0xC0
#define _PRV_TLV_HEADER_MAX_LENGTH 6

#define _PRV_TLV_TYPE_UNKNOWN           (uint8_t)0xFF
#define _PRV_TLV_TYPE_OBJECT            (uint8_t)0x10
#define _PRV_TLV_TYPE_INSTANCE          (uint8_t)0x00
#define _PRV_TLV_TYPE_RESOURCE          (uint8_t)0xC0
#define _PRV_TLV_TYPE_MULTIPLE_RESOURCE (uint8_t)0x80
#define _PRV_TLV_TYPE_RESOURCE_INSTANCE (uint8_t)0x40




static uint8_t prv_getHeaderType(et_data_type_t type)
{
    switch (type)
    {
    case DATA_TYPE_OBJECT:
        return _PRV_TLV_TYPE_OBJECT;

    case DATA_TYPE_OBJECT_INSTANCE:
        return _PRV_TLV_TYPE_INSTANCE;
    
    /*
    //multiple resource disabled
    case cis_data_type_multiple_resource:
        return _PRV_TLV_TYPE_MULTIPLE_RESOURCE;
    */
    case DATA_TYPE_STRING:
    case DATA_TYPE_INTEGER:
    case DATA_TYPE_FLOAT:
    case DATA_TYPE_BOOL:
    case DATA_TYPE_OPAQUE:
    case DATA_TYPE_UNDEFINE:
    default:
        return _PRV_TLV_TYPE_UNKNOWN;
    }
}

static et_data_type_t prv_getDataType(uint8_t type)
{
    switch (type)
    {
    case _PRV_TLV_TYPE_OBJECT:
        return DATA_TYPE_OBJECT;

    case _PRV_TLV_TYPE_INSTANCE:
        return DATA_TYPE_OBJECT_INSTANCE;

    case _PRV_TLV_TYPE_RESOURCE:
    case _PRV_TLV_TYPE_RESOURCE_INSTANCE:
        return DATA_TYPE_OPAQUE;

    default:
        return DATA_TYPE_UNDEFINE;
    }
}

static int prv_getHeaderLength(uint16_t id,
                               size_t dataLen)
{
    int length;

    length = 2;

    if (id > 0xFF)
    {
        length += 1;
    }

    if (dataLen > 0xFFFF)
    {
        length += 3;
    }
    else if (dataLen > 0xFF)
    {
        length += 2;
    }
    else if (dataLen > 7)
    {
        length += 1;
    }

    return length;
}

static int prv_createHeader(uint8_t * header,
                            bool isInstance,
                            bool isResourceInstance,
                            et_data_type_t type,
                            uint16_t id,
                            size_t data_len)
{
    int header_len;
    int offset;
    uint8_t hdrType;

    header_len = prv_getHeaderLength(id, data_len);
    if (isInstance == true)
    {
        hdrType =  _PRV_TLV_TYPE_INSTANCE;
    }
    else if (isResourceInstance == true)
    {
        hdrType =  _PRV_TLV_TYPE_RESOURCE_INSTANCE;
    }
    else
    {
        hdrType = prv_getHeaderType(type);
    }

    header[0] = 0;
    header[0] |= hdrType&_PRV_TLV_TYPE_MASK;

    if (id > 0xFF)
    {
        header[0] |= 0x20;
        header[1] = (id >> 8) & 0XFF;
        header[2] = id & 0XFF;
        offset = 3;
    }
    else
    {
        header[1] = (uint8_t)id;
        offset = 2;
    }
    if (data_len <= 7)
    {
        header[0] += (uint8_t)data_len;
    }
    else if (data_len <= 0xFF)
    {
        header[0] |= 0x08;
        header[offset] = (uint8_t)data_len;
    }
    else if (data_len <= 0xFFFF)
    {
        header[0] |= 0x10;
        header[offset] = (data_len >> 8) & 0XFF;
        header[offset + 1] = data_len & 0XFF;
    }
    else if (data_len <= 0xFFFFFF)
    {
        header[0] |= 0x18;
        header[offset] = (data_len >> 16) & 0XFF;
        header[offset + 1] = (data_len >> 8) & 0XFF;
        header[offset + 2] = data_len & 0XFF;
    }

    return header_len;
}

int tlv_decode_TLV(const uint8_t * buffer,
                    size_t buffer_len,
                    et_data_type_t * oType,
                    uint16_t * oID,
                    size_t * oDataIndex,
                    size_t * oDataLen)
{
    if (buffer_len < 2) 
    {
        return 0;
    }

    *oDataIndex = 2;

    *oType = prv_getDataType(buffer[0]&_PRV_TLV_TYPE_MASK);

    if ((buffer[0]&0x20) == 0x20)
    {
        // id is 16 bits long
        if (buffer_len < 3) 
        {
            return 0;
        }
        *oDataIndex += 1;
        *oID = (buffer[1]<<8) + buffer[2];
    }
    else
    {
        // id is 8 bits long
        *oID = buffer[1];
    }

    switch (buffer[0]&0x18)
    {
    case 0x00:
        {
            // no length field
            *oDataLen = buffer[0]&0x07;
        }
        break;
    case 0x08:
        {
            // length field is 8 bits long
            if (buffer_len < *oDataIndex + 1) return 0;
            *oDataLen = buffer[*oDataIndex];
            *oDataIndex += 1;
        }
        break;
    case 0x10:
        {
             // length field is 16 bits long
            if (buffer_len < *oDataIndex + 2) return 0;
            *oDataLen = (buffer[*oDataIndex]<<8) + buffer[*oDataIndex+1];
            *oDataIndex += 2;
        }
        break;
    case 0x18:
        {
            // length field is 24 bits long
            if (buffer_len < *oDataIndex + 3) return 0;
            *oDataLen = (buffer[*oDataIndex]<<16) + (buffer[*oDataIndex+1]<<8) + buffer[*oDataIndex+2];
            *oDataIndex += 3;
        }
        break;
    default:
        // can't happen
        return 0;
    }

    if (*oDataIndex + *oDataLen > buffer_len) 
    {
        return 0;
    }

    return *oDataIndex + *oDataLen;
}


int tlv_parse(uint8_t * buffer,
              size_t bufferLen,
              st_data_t ** dataP)
{
    et_data_type_t type;
    uint16_t id;
    size_t dataIndex;
    size_t dataLen;
    int index = 0;
    int result;
    int size = 0;
    

    *dataP = NULL;
    while (0 != (result = tlv_decode_TLV((uint8_t*)buffer + index, bufferLen - index, &type, &id, &dataIndex, &dataLen)))
    {
        st_data_t * newTlvP;

        newTlvP = data_new(size + 1);
        if (size >= 1)
        {
            if (newTlvP == NULL)
            {
                data_free(size, *dataP);
                return 0;
            }
            else
            {
                cis_memcpy(newTlvP, *dataP, size * sizeof(st_data_t));
                cis_free(*dataP);
            }
        }
        *dataP = newTlvP;

        (*dataP)[size].type = type;
        (*dataP)[size].id = id;
        if (type == DATA_TYPE_OBJECT_INSTANCE || type == DATA_TYPE_MULTIPLE_RESOURCE)
        {
            (*dataP)[size].value.asChildren.count = tlv_parse(buffer + index + dataIndex,
                                                          dataLen,
                                                          &((*dataP)[size].value.asChildren.array));
            if ((*dataP)[size].value.asChildren.count == 0)
            {
                data_free(size + 1, *dataP);
                return 0;
            }
        }
        else
        {
            data_encode_opaque(buffer + index + dataIndex, dataLen, (*dataP) + size);
            
        }
        size++;
        index += result;
    }

    return size;
}


static size_t prv_getLength(int size,
                            st_data_t * dataP)
{
    int length;
    int i;

    length = 0;

    for (i = 0 ; i < size && length != -1 ; i++)
    {
        switch (dataP[i].type)
        {
        case DATA_TYPE_OBJECT_INSTANCE:
        //case cis_data_type_multiple_resource:
            {
                int subLength;

                subLength = prv_getLength(dataP[i].value.asChildren.count, dataP[i].value.asChildren.array);
                if (subLength == -1)
                {
                    length = -1;
                }
                else
                {
                    length += prv_getHeaderLength(dataP[i].id, subLength) + subLength;
                }
            }
            break;

        case DATA_TYPE_STRING:
        case DATA_TYPE_OPAQUE:
            {
               length += prv_getHeaderLength(dataP[i].id, dataP[i].asBuffer.length) + dataP[i].asBuffer.length;
               
            }
            break;

        case DATA_TYPE_INTEGER:
            {
                size_t data_len;
                uint8_t unused_buffer[_PRV_64BIT_BUFFER_SIZE];

                data_len = utils_encodeInt(dataP[i].value.asInteger, unused_buffer);
                length += prv_getHeaderLength(dataP[i].id, data_len) + data_len;
            }
            break;

        case DATA_TYPE_FLOAT:
            {
                size_t data_len;

                if ((dataP[i].value.asFloat < 0.0 - (double)FLT_MAX)
                    || (dataP[i].value.asFloat >(double)FLT_MAX))
                {
                    data_len = 8;
                }
                else
                {
                    data_len = 8;
                }

                length += prv_getHeaderLength(dataP[i].id, data_len) + data_len;
            }
            break;

        case DATA_TYPE_BOOL:
            {
                // Booleans are always encoded on one byte
                length += prv_getHeaderLength(dataP[i].id, 1) + 1;
            }
            break;
        default:
            {
                //length = -1;
            }
            break;
        }
    }

    if (length < 0)
    {
        return 0;
    }
    else
    {
        return (size_t)length;
    }
}


size_t tlv_serialize(bool isResourceInstance, 
                     int size,
                     st_data_t * dataP,
                     uint8_t ** bufferP)
{
    size_t length;
    int index;
    int i;

    LOGD("tlv_serialize isResourceInstance: %s, size: %d", isResourceInstance?"TRUE":"FALSE", size);

    *bufferP = NULL;
    length = prv_getLength(size, dataP);
    if (length <= 0) 
    {
        return length;
    }

    *bufferP = (uint8_t *)cis_malloc(length);
    if (*bufferP == NULL) 
    {
        return 0;
    }

    index = 0;
    for (i = 0 ; i < size && length != 0 ; i++)
    {
        int headerLen;
        switch (dataP[i].type)
        {
        /*
        case cis_data_type_multiple_resource:
            isResourceInstance = true;
        */
        case DATA_TYPE_OBJECT_INSTANCE:
            {
                uint8_t * tmpBuffer;
                size_t tmpLength;

                tmpLength = tlv_serialize(isResourceInstance, dataP[i].value.asChildren.count, dataP[i].value.asChildren.array, &tmpBuffer);
                if (tmpLength == 0)
                {
                    length = 0;
                }
                else
                {
                    headerLen = prv_createHeader(*bufferP + index, false,false,dataP[i].type, dataP[i].id, tmpLength);
                    index += headerLen;
                    cis_memcpy(*bufferP + index, tmpBuffer, tmpLength);
                    index += tmpLength;
                    cis_free(tmpBuffer);
                }
            }
            break;

        case DATA_TYPE_STRING:
        case DATA_TYPE_OPAQUE:
            {
                headerLen = prv_createHeader(*bufferP + index, false,isResourceInstance, dataP[i].type, dataP[i].id, dataP[i].asBuffer.length);
                if (headerLen == 0)
                {
                    length = 0;
                }
                else
                {
                    index += headerLen;
                    cis_memcpy(*bufferP + index, dataP[i].asBuffer.buffer, dataP[i].asBuffer.length);
                    index += dataP[i].asBuffer.length;
                }
            }
            break;

        case DATA_TYPE_INTEGER:
            {
                size_t data_len;
                uint8_t data_buffer[_PRV_64BIT_BUFFER_SIZE];

                data_len = utils_encodeInt(dataP[i].value.asInteger, data_buffer);
                headerLen = prv_createHeader(*bufferP + index, false,isResourceInstance, dataP[i].type, dataP[i].id, data_len);
                if (headerLen == 0)
                {
                    length = 0;
                }
                else
                {
                    index += headerLen;
                    cis_memcpy(*bufferP + index, data_buffer, data_len);
                    index += data_len;
                }
            }
            break;

        case DATA_TYPE_FLOAT:
            {
                size_t data_len;
                uint8_t data_buffer[_PRV_64BIT_BUFFER_SIZE];

                data_len = utils_encodeFloat(dataP[i].value.asFloat, data_buffer);
                headerLen = prv_createHeader(*bufferP + index, false,isResourceInstance,dataP[i].type, dataP[i].id, data_len);
                if (headerLen == 0)
                {
                    length = 0;
                }
                else
                {
                    index += headerLen;
                    cis_memcpy(*bufferP + index, data_buffer, data_len);
                    index += data_len;
                }
            }
            break;

        case DATA_TYPE_BOOL:
            {
                headerLen = prv_createHeader(*bufferP + index, false,isResourceInstance, dataP[i].type, dataP[i].id, 1);
                if (headerLen == 0)
                {
                    length = 0;
                }
                else
                {
                    index += headerLen;
                    (*bufferP)[index] = dataP[i].value.asBoolean ? 1 : 0;
                    index += 1;
                }
            }
            break;

        default:
            {
                //length = 0;
            }
            break;
        }
    }

    if (length == 0)
    {
        cis_free(*bufferP);
    }

    return length;
}

#endif