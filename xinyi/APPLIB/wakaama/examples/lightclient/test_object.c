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
 *    domedambrosio - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Achim Kraus, Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Ville Skyttä - Please refer to git log
 *    
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

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

 David Navarro <david.navarro@intel.com>

*/

/*
 * Implements an object for testing purpose
 *
 *                  Multiple
 * Object |  ID   | Instances | Mandatory |
 *  Test  | 31024 |    Yes    |    No     |
 *
 *  Resources:
 *              Supported    Multiple
 *  Name | ID | Operations | Instances | Mandatory |  Type   | Range | Units |      Description      |
 *  test |  1 |    R/W     |    No     |    Yes    | Integer | 0-255 |       |                       |
 *  exec |  2 |     E      |    No     |    Yes    |         |       |       |                       |
 *  dec  |  3 |    R/W     |    No     |    Yes    |  Float  |       |       |                       |
 *  sig  |  4 |    R/W     |    No     |    Yes    | Integer |       |       | 16-bit signed integer |
 *
 */

#include "xiny_liblwm2m.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

static void prv_output_buffer(uint8_t * buffer,
                              int length)
{
    int i;
    uint8_t array[16];

    i = 0;
    while (i < length)
    {
        int j;
        softap_printf(USER_LOG, WARN_LOG, "  ");

        memcpy(array, buffer+i, 16);

        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            softap_printf(USER_LOG, WARN_LOG, "%02X ", array[j]);
        }
        while (j < 16)
        {
            softap_printf(USER_LOG, WARN_LOG, "   ");
            j++;
        }
        softap_printf(USER_LOG, WARN_LOG, "  ");
        for (j = 0 ; j < 16 && i+j < length; j++)
        {
            if (isprint(array[j]))
                softap_printf(USER_LOG, WARN_LOG, "%c ", array[j]);
            else
                softap_printf(USER_LOG, WARN_LOG, ". ");
        }
        softap_printf(USER_LOG, WARN_LOG, "\n");

        i += 16;
    }
}

/*
 * Multiple instance xiny_objects can use userdata to store data that will be shared between the different instances.
 * The xiny_lwm2m_object_t object structure - which represent every object of the liblwm2m as seen in the single instance
 * object - contain a chained list called instanceList with the object specific structure prv_instance_t:
 */
typedef struct _prv_instance_
{
    /*
     * The first two are mandatories and represent the pointer to the next instance and the ID of this one. The rest
     * is the instance scope user data (uint8_t test in this case)
     */
    struct _prv_instance_ * next;   // matches xiny_lwm2m_list_t::next
    uint16_t shortID;               // matches xiny_lwm2m_list_t::id
    uint8_t  test;
    double   dec;
    int16_t  sig;
} prv_instance_t;

static uint8_t prv_read(uint16_t instanceId,
                        int * numDataP,
                        xiny_lwm2m_data_t ** dataArrayP,
                        xiny_lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    int i;

    targetP = (prv_instance_t *)xiny_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return xiny_COAP_404_NOT_FOUND;

    if (*numDataP == 0)
    {
        *dataArrayP = xiny_lwm2m_data_new(3);
        if (*dataArrayP == NULL) return xiny_COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 3;
        (*dataArrayP)[0].id = 1;
        (*dataArrayP)[1].id = 3;
        (*dataArrayP)[2].id = 4;
    }

    for (i = 0 ; i < *numDataP ; i++)
    {
        switch ((*dataArrayP)[i].id)
        {
        case 1:
            xiny_lwm2m_data_encode_int(targetP->test, *dataArrayP + i);
            break;
        case 2:
            return xiny_COAP_405_METHOD_NOT_ALLOWED;
        case 3:
            xiny_lwm2m_data_encode_float(targetP->dec, *dataArrayP + i);
            break;
        case 4:
            xiny_lwm2m_data_encode_int(targetP->sig, *dataArrayP + i);
            break;
        default:
            return xiny_COAP_404_NOT_FOUND;
        }
    }

    return xiny_COAP_205_CONTENT;
}

static uint8_t prv_discover(uint16_t instanceId,
                            int * numDataP,
                            xiny_lwm2m_data_t ** dataArrayP,
                            xiny_lwm2m_object_t * objectP)
{
    int i;

    (void) instanceId;

    (void) objectP;

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        *dataArrayP = xiny_lwm2m_data_new(4);
        if (*dataArrayP == NULL) return xiny_COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 4;
        (*dataArrayP)[0].id = 1;
        (*dataArrayP)[1].id = 2;
        (*dataArrayP)[2].id = 3;
        (*dataArrayP)[3].id = 4;
    }
    else
    {
        for (i = 0; i < *numDataP; i++)
        {
            switch ((*dataArrayP)[i].id)
            {
            case 1:
            case 2:
            case 3:
            case 4:
                break;
            default:
                return xiny_COAP_404_NOT_FOUND;
            }
        }
    }
    return xiny_COAP_205_CONTENT;
}

static uint8_t prv_write(uint16_t instanceId,
                         int numData,
                         xiny_lwm2m_data_t * dataArray,
                         xiny_lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    int i;

    targetP = (prv_instance_t *)xiny_lwm2m_list_find(objectP->instanceList, instanceId);
    if (NULL == targetP) return xiny_COAP_404_NOT_FOUND;

    for (i = 0 ; i < numData ; i++)
    {
        switch (dataArray[i].id)
        {
        case 1:
        {
            int64_t value;

            if (1 != xiny_lwm2m_data_decode_int(dataArray + i, &value) || value < 0 || value > 0xFF)
            {
                return xiny_COAP_400_BAD_REQUEST;
            }
            targetP->test = (uint8_t)value;
        }
        break;
        case 2:
            return xiny_COAP_405_METHOD_NOT_ALLOWED;
        case 3:
            if (1 != xiny_lwm2m_data_decode_float(dataArray + i, &(targetP->dec)))
            {
                return xiny_COAP_400_BAD_REQUEST;
            }
            break;
        case 4:
        {
            int64_t value;

            if (1 != xiny_lwm2m_data_decode_int(dataArray + i, &value) || value < INT16_MIN || value > INT16_MAX)
            {
                return xiny_COAP_400_BAD_REQUEST;
            }
            targetP->sig = (int16_t)value;
        }
        break;
        default:
            return xiny_COAP_404_NOT_FOUND;
        }
    }

    return xiny_COAP_204_CHANGED;
}

static uint8_t prv_delete(uint16_t id,
                          xiny_lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;

    objectP->instanceList = xiny_lwm2m_list_remove(objectP->instanceList, id, (xiny_lwm2m_list_t **)&targetP);
    if (NULL == targetP) return xiny_COAP_404_NOT_FOUND;

    xiny_lwm2m_free(targetP);

    return xiny_COAP_202_DELETED;
}

static uint8_t prv_create(uint16_t instanceId,
                          int numData,
                          xiny_lwm2m_data_t * dataArray,
                          xiny_lwm2m_object_t * objectP)
{
    prv_instance_t * targetP;
    uint8_t result;


    targetP = (prv_instance_t *)xiny_lwm2m_malloc(sizeof(prv_instance_t));
    if (NULL == targetP) return xiny_COAP_500_INTERNAL_SERVER_ERROR;
    memset(targetP, 0, sizeof(prv_instance_t));

    targetP->shortID = instanceId;
    objectP->instanceList = xiny_LWM2M_LIST_ADD(objectP->instanceList, targetP);

    result = prv_write(instanceId, numData, dataArray, objectP);

    if (result != xiny_COAP_204_CHANGED)
    {
        (void)prv_delete(instanceId, objectP);
    }
    else
    {
        result = xiny_COAP_201_CREATED;
    }

    return result;
}

static uint8_t prv_exec(uint16_t instanceId,
                        uint16_t resourceId,
                        uint8_t * buffer,
                        int length,
                        xiny_lwm2m_object_t * objectP)
{

    if (NULL == xiny_lwm2m_list_find(objectP->instanceList, instanceId)) return xiny_COAP_404_NOT_FOUND;

    switch (resourceId)
    {
    case 1:
        return xiny_COAP_405_METHOD_NOT_ALLOWED;
    case 2:
        softap_printf(USER_LOG, WARN_LOG, "\r\n-----------------\r\n"
                        "Execute on %hu/%d/%d\r\n"
                        " Parameter (%d bytes):\r\n",
                        objectP->objID, instanceId, resourceId, length);
        prv_output_buffer((uint8_t*)buffer, length);
        softap_printf(USER_LOG, WARN_LOG, "-----------------\r\n\r\n");
        return xiny_COAP_204_CHANGED;
    case 3:
        return xiny_COAP_405_METHOD_NOT_ALLOWED;
    default:
        return xiny_COAP_404_NOT_FOUND;
    }
}

xiny_lwm2m_object_t * get_test_object(void)
{
    xiny_lwm2m_object_t * testObj;

    testObj = (xiny_lwm2m_object_t *)xiny_lwm2m_malloc(sizeof(xiny_lwm2m_object_t));

    if (NULL != testObj)
    {
        int i;
        prv_instance_t * targetP;

        memset(testObj, 0, sizeof(xiny_lwm2m_object_t));

        testObj->objID = 31024;
        for (i=0 ; i < 3 ; i++)
        {
            targetP = (prv_instance_t *)xiny_lwm2m_malloc(sizeof(prv_instance_t));
            if (NULL == targetP) return NULL;
            memset(targetP, 0, sizeof(prv_instance_t));
            targetP->shortID = 10 + i;
            targetP->test    = 20 + i;
            targetP->dec     = -30 + i + (double)i/100.0;
            targetP->sig     = 0 - i;
            testObj->instanceList = xiny_LWM2M_LIST_ADD(testObj->instanceList, targetP);
        }
        /*
         * From a single instance object, two more functions are available.
         * - The first one (createFunc) create a new instance and filled it with the provided informations. If an ID is
         *   provided a check is done for verifying his disponibility, or a new one is generated.
         * - The other one (deleteFunc) delete an instance by removing it from the instance list (and freeing the memory
         *   allocated to it)
         */
        testObj->readFunc = prv_read;
        testObj->writeFunc = prv_write;
        testObj->executeFunc = prv_exec;
        testObj->createFunc = prv_create;
        testObj->deleteFunc = prv_delete;
        testObj->discoverFunc = prv_discover;
    }

    return testObj;
}

void free_test_object(xiny_lwm2m_object_t * object)
{
    xiny_LWM2M_LIST_FREE(object->instanceList);
    if (object->userData != NULL)
    {
        xiny_lwm2m_free(object->userData);
        object->userData = NULL;
    }
    xiny_lwm2m_free(object);
}