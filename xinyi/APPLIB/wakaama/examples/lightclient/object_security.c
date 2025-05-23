/*******************************************************************************
 *
 * Copyright (c) 2013, 2014, 2015 Intel Corporation and others.
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
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    
 *******************************************************************************/

/*
 *  Resources:
 *
 *          Name            | ID | Operations | Instances | Mandatory |  Type   |  Range  | Units |
 *  Server URI              |  0 |            |  Single   |    Yes    | String  |         |       |
 *  Bootstrap Server        |  1 |            |  Single   |    Yes    | Boolean |         |       |
 *  Security Mode           |  2 |            |  Single   |    Yes    | Integer |   0-3   |       |
 *  Public Key or ID        |  3 |            |  Single   |    Yes    | Opaque  |         |       |
 *  Server Public Key or ID |  4 |            |  Single   |    Yes    | Opaque  |         |       |
 *  Secret Key              |  5 |            |  Single   |    Yes    | Opaque  |         |       |
 *  SMS Security Mode       |  6 |            |  Single   |    Yes    | Integer |  0-255  |       |
 *  SMS Binding Key Param.  |  7 |            |  Single   |    Yes    | Opaque  |   6 B   |       |
 *  SMS Binding Secret Keys |  8 |            |  Single   |    Yes    | Opaque  | 32-48 B |       |
 *  Server SMS Number       |  9 |            |  Single   |    Yes    | Integer |         |       |
 *  Short Server ID         | 10 |            |  Single   |    No     | Integer | 1-65535 |       |
 *  Client Hold Off Time    | 11 |            |  Single   |    Yes    | Integer |         |   s   |
 *
 */

/*
 * Here we implement a very basic LWM2M Security Object which only knows NoSec security mode.
 */

#include "xiny_liblwm2m.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct _security_instance_
{
    struct _security_instance_ * next;        // matches xiny_lwm2m_list_t::next
    uint16_t                     instanceId;  // matches xiny_lwm2m_list_t::id
    char *                       uri;
    bool                         isBootstrap;
    uint16_t                     shortID;
    uint32_t                     clientHoldOffTime;
} security_instance_t;

static uint8_t prv_get_value(xiny_lwm2m_data_t * dataP,
                             security_instance_t * targetP)
{

    switch (dataP->id)
    {
    case xiny_LWM2M_SECURITY_URI_ID:
        xiny_lwm2m_data_encode_string(targetP->uri, dataP);
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SECURITY_BOOTSTRAP_ID:
        xiny_lwm2m_data_encode_bool(targetP->isBootstrap, dataP);
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SECURITY_SECURITY_ID:
        xiny_lwm2m_data_encode_int(xiny_LWM2M_SECURITY_MODE_NONE, dataP);
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SECURITY_PUBLIC_KEY_ID:
        // Here we return an opaque of 1 byte containing 0
        {
            uint8_t value = 0;

            xiny_lwm2m_data_encode_opaque(&value, 1, dataP);
        }
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SECURITY_SERVER_PUBLIC_KEY_ID:
        // Here we return an opaque of 1 byte containing 0
        {
            uint8_t value = 0;

            xiny_lwm2m_data_encode_opaque(&value, 1, dataP);
        }
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SECURITY_SECRET_KEY_ID:
        // Here we return an opaque of 1 byte containing 0
        {
            uint8_t value = 0;

            xiny_lwm2m_data_encode_opaque(&value, 1, dataP);
        }
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SECURITY_SMS_SECURITY_ID:
        xiny_lwm2m_data_encode_int(xiny_LWM2M_SECURITY_MODE_NONE, dataP);
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SECURITY_SMS_KEY_PARAM_ID:
        // Here we return an opaque of 6 bytes containing a buggy value
        {
            char * value = "12345";
            xiny_lwm2m_data_encode_opaque((uint8_t *)value, 6, dataP);
        }
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SECURITY_SMS_SECRET_KEY_ID:
        // Here we return an opaque of 32 bytes containing a buggy value
        {
            char * value = "1234567890abcdefghijklmnopqrstu";
            xiny_lwm2m_data_encode_opaque((uint8_t *)value, 32, dataP);
        }
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SECURITY_SMS_SERVER_NUMBER_ID:
        xiny_lwm2m_data_encode_int(0, dataP);
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SECURITY_SHORT_SERVER_ID:
        xiny_lwm2m_data_encode_int(targetP->shortID, dataP);
        return xiny_COAP_205_CONTENT;

    case xiny_LWM2M_SECURITY_HOLD_OFF_ID:
        xiny_lwm2m_data_encode_int(targetP->clientHoldOffTime, dataP);
        return xiny_COAP_205_CONTENT;

    default:
        return xiny_COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_security_read(uint16_t instanceId,
                                 int * numDataP,
								 xiny_lwm2m_data_t ** dataArrayP,
								 xiny_lwm2m_object_t * objectP)
{
    security_instance_t * targetP;
    uint8_t result;
    int i;

    targetP = (security_instance_t *)xiny_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return xiny_COAP_404_NOT_FOUND;

    // is the server asking for the full instance ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {xiny_LWM2M_SECURITY_URI_ID,
                              xiny_LWM2M_SECURITY_BOOTSTRAP_ID,
                              xiny_LWM2M_SECURITY_SECURITY_ID,
                              xiny_LWM2M_SECURITY_PUBLIC_KEY_ID,
                              xiny_LWM2M_SECURITY_SERVER_PUBLIC_KEY_ID,
                              xiny_LWM2M_SECURITY_SECRET_KEY_ID,
                              xiny_LWM2M_SECURITY_SMS_SECURITY_ID,
                              xiny_LWM2M_SECURITY_SMS_KEY_PARAM_ID,
                              xiny_LWM2M_SECURITY_SMS_SECRET_KEY_ID,
                              xiny_LWM2M_SECURITY_SMS_SERVER_NUMBER_ID,
                              xiny_LWM2M_SECURITY_SHORT_SERVER_ID,
                              xiny_LWM2M_SECURITY_HOLD_OFF_ID};
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

xiny_lwm2m_object_t * xiny_get_security_object()
{
	xiny_lwm2m_object_t * securityObj;

    securityObj = (xiny_lwm2m_object_t *)xiny_lwm2m_malloc(sizeof(xiny_lwm2m_object_t));

    if (NULL != securityObj)
    {
        security_instance_t * targetP;

        memset(securityObj, 0, sizeof(xiny_lwm2m_object_t));

        securityObj->objID = 0;

        // Manually create an hardcoded instance
        targetP = (security_instance_t *)xiny_lwm2m_malloc(sizeof(security_instance_t));
        if (NULL == targetP)
        {
            xiny_lwm2m_free(securityObj);
            return NULL;
        }

        memset(targetP, 0, sizeof(security_instance_t));
        targetP->instanceId = 0;
        targetP->uri = strdup("coap://139.224.131.190:5683");
        targetP->isBootstrap = false;
        targetP->shortID = 123;
        targetP->clientHoldOffTime = 10;

        securityObj->instanceList = xiny_LWM2M_LIST_ADD(securityObj->instanceList, targetP);

        securityObj->readFunc = prv_security_read;
    }

    return securityObj;
}

void free_security_object(xiny_lwm2m_object_t * objectP)
{
    while (objectP->instanceList != NULL)
    {
        security_instance_t * securityInstance = (security_instance_t *)objectP->instanceList;
        objectP->instanceList = objectP->instanceList->next;
        if (NULL != securityInstance->uri)
        {
            xiny_lwm2m_free(securityInstance->uri);
        }
        xiny_lwm2m_free(securityInstance);
    }
    xiny_lwm2m_free(objectP);
}

char * get_server_uri(xiny_lwm2m_object_t * objectP,
                      uint16_t secObjInstID)
{
    security_instance_t * targetP = (security_instance_t *)xiny_LWM2M_LIST_FIND(objectP->instanceList, secObjInstID);

    if (NULL != targetP)
    {
        return xiny_lwm2m_strdup(targetP->uri);
    }

    return NULL;
}
