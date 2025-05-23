/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
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
 *    David Navarro, Intel Corporation - xiny_initial API and implementation
 *    Julien Vermillard, Sierra Wireless
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    
 *******************************************************************************/

/*
 *  Resources:
 *
 *          Name         | ID | Operations | Instances | Mandatory |  Type   |  Range  | Units |
 *  Short ID             |  0 |     R      |  Single   |    Yes    | Integer | 1-65535 |       |
 *  Lifetime             |  1 |    R/W     |  Single   |    Yes    | Integer |         |   s   |
 *  Default Min Period   |  2 |    R/W     |  Single   |    No     | Integer |         |   s   |
 *  Default Max Period   |  3 |    R/W     |  Single   |    No     | Integer |         |   s   |
 *  Disable              |  4 |     E      |  Single   |    No     |         |         |       |
 *  Disable Timeout      |  5 |    R/W     |  Single   |    No     | Integer |         |   s   |
 *  Notification Storing |  6 |    R/W     |  Single   |    Yes    | Boolean |         |       |
 *  Binding              |  7 |    R/W     |  Single   |    Yes    | String  |         |       |
 *  Registration Update  |  8 |     E      |  Single   |    Yes    |         |         |       |
 *
 */

#include "xiny_liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _server_instance_
{
    struct _server_instance_ * next;   // matches xiny_lwm2m_list_t::next
    uint16_t    instanceId;            // matches xiny_lwm2m_list_t::id
    uint16_t    shortServerId;
    uint32_t    lifetime;
    bool        storing;
    char        binding[4];
} server_instance_t;

static uint8_t prv_get_value(xiny_lwm2m_data_t * dataP,
                             server_instance_t * targetP)
{
    switch (dataP->id)
    {
    case xiny_LWM2M_SERVER_SHORT_ID_ID:
        xiny_lwm2m_data_encode_int(targetP->shortServerId, dataP);
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SERVER_LIFETIME_ID:
        xiny_lwm2m_data_encode_int(targetP->lifetime, dataP);
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SERVER_DISABLE_ID:
        return xiny_COAP_405_METHOD_NOT_ALLOWED;

    case xiny_LWM2M_SERVER_STORING_ID:
        xiny_lwm2m_data_encode_bool(targetP->storing, dataP);
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SERVER_BINDING_ID:
        xiny_lwm2m_data_encode_string(targetP->binding, dataP);
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SERVER_UPDATE_ID:
        return xiny_COAP_405_METHOD_NOT_ALLOWED;

    default:
        return xiny_COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_server_read(uint16_t instanceId,
                               int * numDataP,
                               xiny_lwm2m_data_t ** dataArrayP,
							   xiny_lwm2m_object_t * objectP)
{
    server_instance_t * targetP;
    uint8_t result;
    int i;

    targetP = (server_instance_t *)xiny_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return xiny_COAP_404_NOT_FOUND;

    // is the server asking for the full instance ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {
                xiny_LWM2M_SERVER_SHORT_ID_ID,
                xiny_LWM2M_SERVER_LIFETIME_ID,
                xiny_LWM2M_SERVER_STORING_ID,
                xiny_LWM2M_SERVER_BINDING_ID
        };
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = xiny_lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL) return xiny_COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do
    {
        result = prv_get_value((*dataArrayP) + i, targetP);
        i++;
    } while (i < *numDataP && result == xiny_COAP_205_CONTENT);

    return result;
}

static uint8_t prv_server_discover(uint16_t instanceId,
                                   int * numDataP,
                                   xiny_lwm2m_data_t ** dataArrayP,
								   xiny_lwm2m_object_t * objectP)
{
    uint8_t result;
    int i;

    (void) instanceId;

    (void) objectP;

    result = xiny_COAP_205_CONTENT;

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {
            xiny_LWM2M_SERVER_SHORT_ID_ID,
            xiny_LWM2M_SERVER_LIFETIME_ID,
            xiny_LWM2M_SERVER_MIN_PERIOD_ID,
            xiny_LWM2M_SERVER_MAX_PERIOD_ID,
            xiny_LWM2M_SERVER_DISABLE_ID,
            xiny_LWM2M_SERVER_TIMEOUT_ID,
            xiny_LWM2M_SERVER_STORING_ID,
            xiny_LWM2M_SERVER_BINDING_ID,
            xiny_LWM2M_SERVER_UPDATE_ID
        };
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = xiny_lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL) return xiny_COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }
    else
    {
        for (i = 0; i < *numDataP && result == xiny_COAP_205_CONTENT; i++)
        {
            switch ((*dataArrayP)[i].id)
            {
            case xiny_LWM2M_SERVER_SHORT_ID_ID:
            case xiny_LWM2M_SERVER_LIFETIME_ID:
            case xiny_LWM2M_SERVER_MIN_PERIOD_ID:
            case xiny_LWM2M_SERVER_MAX_PERIOD_ID:
            case xiny_LWM2M_SERVER_DISABLE_ID:
            case xiny_LWM2M_SERVER_TIMEOUT_ID:
            case xiny_LWM2M_SERVER_STORING_ID:
            case xiny_LWM2M_SERVER_BINDING_ID:
            case xiny_LWM2M_SERVER_UPDATE_ID:
                break;
            default:
                result = xiny_COAP_404_NOT_FOUND;
            }
        }
    }

    return result;
}

static uint8_t prv_set_int_value(xiny_lwm2m_data_t * dataArray,
                                 uint32_t * data)
{
    uint8_t result;
    int64_t value;

    if (1 == xiny_lwm2m_data_decode_int(dataArray, &value))
    {
        if (value >= 0 && value <= 0xFFFFFFFF)
        {
            *data = value;
            result = xiny_COAP_204_CHANGED;
        }
        else
        {
            result = xiny_COAP_406_NOT_ACCEPTABLE;
        }
    }
    else
    {
        result = xiny_COAP_400_BAD_REQUEST;
    }
    return result;
}

static uint8_t prv_server_write(uint16_t instanceId,
                                int numData,
                                xiny_lwm2m_data_t * dataArray,
								xiny_lwm2m_object_t * objectP)
{
    server_instance_t * targetP;
    int i;
    uint8_t result;

    targetP = (server_instance_t *)xiny_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP)
    {
        return xiny_COAP_404_NOT_FOUND;
    }

    i = 0;
    do
    {
        switch (dataArray[i].id)
        {
        case xiny_LWM2M_SERVER_SHORT_ID_ID:
        {
            uint32_t value;

            result = prv_set_int_value(dataArray + i, &value);
            if (xiny_COAP_204_CHANGED == result)
            {
                if (0 < value && value <= 0xFFFF)
                {
                    targetP->shortServerId = value;
                }
                else
                {
                    result = xiny_COAP_406_NOT_ACCEPTABLE;
                }
            }
        }
        break;

        case xiny_LWM2M_SERVER_LIFETIME_ID:
            result = prv_set_int_value(dataArray + i, (uint32_t *)&(targetP->lifetime));
            break;

        case xiny_LWM2M_SERVER_DISABLE_ID:
            result = xiny_COAP_405_METHOD_NOT_ALLOWED;
            break;

        case xiny_LWM2M_SERVER_STORING_ID:
        {
            bool value;

            if (1 == xiny_lwm2m_data_decode_bool(dataArray + i, &value))
            {
                targetP->storing = value;
                result = xiny_COAP_204_CHANGED;
            }
            else
            {
                result = xiny_COAP_400_BAD_REQUEST;
            }
        }
        break;

        case xiny_LWM2M_SERVER_BINDING_ID:
            if ((dataArray[i].type == xiny_LWM2M_TYPE_STRING || dataArray[i].type == xiny_LWM2M_TYPE_OPAQUE)
             && dataArray[i].value.asBuffer.length > 0 && dataArray[i].value.asBuffer.length <= 3
             && (strncmp((char*)dataArray[i].value.asBuffer.buffer, "U",   dataArray[i].value.asBuffer.length) == 0
              || strncmp((char*)dataArray[i].value.asBuffer.buffer, "UQ",  dataArray[i].value.asBuffer.length) == 0
              || strncmp((char*)dataArray[i].value.asBuffer.buffer, "S",   dataArray[i].value.asBuffer.length) == 0
              || strncmp((char*)dataArray[i].value.asBuffer.buffer, "SQ",  dataArray[i].value.asBuffer.length) == 0
              || strncmp((char*)dataArray[i].value.asBuffer.buffer, "US",  dataArray[i].value.asBuffer.length) == 0
              || strncmp((char*)dataArray[i].value.asBuffer.buffer, "UQS", dataArray[i].value.asBuffer.length) == 0))
            {
                strncpy(targetP->binding, (char*)dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length);
                result = xiny_COAP_204_CHANGED;
            }
            else
            {
                result = xiny_COAP_400_BAD_REQUEST;
            }
            break;

        case xiny_LWM2M_SERVER_UPDATE_ID:
            result = xiny_COAP_405_METHOD_NOT_ALLOWED;
            break;

        default:
            return xiny_COAP_404_NOT_FOUND;
        }
        i++;
    } while (i < numData && result == xiny_COAP_204_CHANGED);

    return result;
}

static uint8_t prv_server_execute(uint16_t instanceId,
                                  uint16_t resourceId,
                                  uint8_t * buffer,
                                  int length,
								  xiny_lwm2m_object_t * objectP)

{
	(void) buffer;

	(void) length;

    server_instance_t * targetP;

    targetP = (server_instance_t *)xiny_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return xiny_COAP_404_NOT_FOUND;

    switch (resourceId)
    {
    case xiny_LWM2M_SERVER_DISABLE_ID:
        // executed in core, if xiny_COAP_204_CHANGED is returned
        return xiny_COAP_204_CHANGED;

    case xiny_LWM2M_SERVER_UPDATE_ID:
        // executed in core, if xiny_COAP_204_CHANGED is returned
        return xiny_COAP_204_CHANGED;

    default:
        return xiny_COAP_405_METHOD_NOT_ALLOWED;
    }
}

static uint8_t prv_server_delete(uint16_t id,
							xiny_lwm2m_object_t * objectP)
{
    server_instance_t * serverInstance;

    objectP->instanceList = xiny_lwm2m_list_remove(objectP->instanceList, id, (xiny_lwm2m_list_t **)&serverInstance);
    if (NULL == serverInstance) return xiny_COAP_404_NOT_FOUND;

    xiny_lwm2m_free(serverInstance);

    return xiny_COAP_202_DELETED;
}

static uint8_t prv_server_create(uint16_t instanceId,
                                 int numData,
								 xiny_lwm2m_data_t * dataArray,
								 xiny_lwm2m_object_t * objectP)
{
    server_instance_t * serverInstance;
    uint8_t result;

    serverInstance = (server_instance_t *)xiny_lwm2m_malloc(sizeof(server_instance_t));
    if (NULL == serverInstance) return xiny_COAP_500_INTERNAL_SERVER_ERROR;
    memset(serverInstance, 0, sizeof(server_instance_t));

    serverInstance->instanceId = instanceId;
    objectP->instanceList = xiny_LWM2M_LIST_ADD(objectP->instanceList, serverInstance);

    result = prv_server_write(instanceId, numData, dataArray, objectP);

    if (result != xiny_COAP_204_CHANGED)
    {
        (void)prv_server_delete(instanceId, objectP);
    }
    else
    {
        result = xiny_COAP_201_CREATED;
    }

    return result;
}

xiny_lwm2m_object_t * xiny_get_server_object()
{
	xiny_lwm2m_object_t * serverObj;

    serverObj = (xiny_lwm2m_object_t *)xiny_lwm2m_malloc(sizeof(xiny_lwm2m_object_t));

    if (NULL != serverObj)
    {
        server_instance_t * serverInstance;

        memset(serverObj, 0, sizeof(xiny_lwm2m_object_t));

        serverObj->objID = 1;

        // Manually create an hardcoded server
        serverInstance = (server_instance_t *)xiny_lwm2m_malloc(sizeof(server_instance_t));
        if (NULL == serverInstance)
        {
            xiny_lwm2m_free(serverObj);
            return NULL;
        }

        memset(serverInstance, 0, sizeof(server_instance_t));
        serverInstance->instanceId = 0;
        serverInstance->shortServerId = 123;
        serverInstance->lifetime = 300;
        serverInstance->storing = false;
        serverInstance->binding[0] = 'U';
        serverObj->instanceList = xiny_LWM2M_LIST_ADD(serverObj->instanceList, serverInstance);

        serverObj->readFunc = prv_server_read;
        serverObj->writeFunc = prv_server_write;
        serverObj->createFunc = prv_server_create;
        serverObj->deleteFunc = prv_server_delete;
        serverObj->executeFunc = prv_server_execute;
        serverObj->discoverFunc = prv_server_discover;
    }

    return serverObj;
}

void free_server_object(xiny_lwm2m_object_t * object)
{
    while (object->instanceList != NULL)
    {
        server_instance_t * serverInstance = (server_instance_t *)object->instanceList;
        object->instanceList = object->instanceList->next;
        xiny_lwm2m_free(serverInstance);
    }
    xiny_lwm2m_free(object);
}
