/*******************************************************************************
/*
 * This object is single instance only, and provide dm info functionality.
 * Object ID is 666.
 * Object instance ID is 0.
 */
#include "softap_macro.h"
#if MOBILE_VER
#include "xy_utils.h"

#include "../cis_api.h"
#include "../cis_internals.h"
#include "std_object.h"
#include "factory_nv.h"
#if CIS_ENABLE_DM
#include "dm_endpoint.h"
#include "cmcc_dm.h"

#define RES_DM_DEV_INFO                  6601
#define RES_DM_APP_INFO                  6602
#define RES_DM_MAC                       6603
#define RES_DM_ROM                       6604
#define RES_DM_RAM                       6605
#define RES_DM_CPU                       6606
#define RES_DM_SYS_VERSION               6607
#define RES_DM_FIRM_VERSION              6608
#define RES_DM_FIRM_NAME                 6609
#define RES_DM_VOLTE                     6610
#define RES_DM_NET_TYPE                  6611
#define RES_DM_NET_ACCT                  6612
#define RES_DM_PHONE                     6613
#define RES_DM_LOCATION                  6614

#define RES_DM_ROUTEMAC                  6617
#define RES_DM_BRAND	                 6619
#define RES_DM_GPU                  	 6620
#define RES_DM_BOARD	                 6621
#define RES_DM_MODEL                     6622
#define RES_DM_RESOLUTION                6623



#define RES_VALUE_BUFF_LEN  64

typedef struct _dmobj_data_
{
	struct _dmobj_data_ * next;        	// matches st_list_t::next
	uint16_t             instanceId;  	// matches st_list_t::id
    char* 	dev_info;
    char* 	app_info;
	char* 	mac;
    char* 	rom;
    char* 	ram;
	char* 	cpu;
    char* 	sys_ver;
    char* 	firm_ver;
    char* 	firm_name;
	char* 	volte;
    char* 	net_type;
    char* 	net_acct;
	char* 	phone;
    char* 	location;
    char* 	routeMac;
    char* 	brand;
    char* 	gpu;
    char* 	board;
    char* 	model;
    char* 	resolution;	
} dmobj_data_t;

extern int my_aes_encrypt(char* enckey,char* encbuf, char* decbuf,int inlen,int* outlen);

static dmobj_data_t * prv_dm_find(st_context_t * contextP,cis_iid_t instanceId)
{
	dmobj_data_t * targetP;
	targetP = (dmobj_data_t *)(std_object_get_dm(contextP, instanceId));

	if (NULL != targetP)
	{
		return targetP;
	}

	return NULL;
}
#define DM_EP_MEM_SIZE  (264)

int prv_dm_encode(char *szin,char **szout)
{
	int ret = -1;
    char *name = szin;
	char *base64tmp=NULL;
	unsigned char *encData=0;
	//unsigned char *decData=0;
	int encDataLen=0;
	//int decDataLen=0;
	int ciphertext_len=0;
   	char *testbase64="123456789";//MTIzNDU2Nzg5
    //AES CBC
	/* A 256 bit key */  
	char *passwd = "00000000000000000000000000000000";

	if(strlen(g_softap_fac_nv->dm_app_pwd)>0)
	{
		passwd = g_softap_fac_nv->dm_app_pwd;
	}
	else
	{
		xy_printf("pwd is null,use default pwd is:%s~", passwd);
	}
	//
	unsigned char key[64]={0};
	memset(key,0,sizeof(key));
	
	my_sha256(passwd,strlen(passwd), key);
	
	//printf("pwd=%s\n",passwd);
	//printf("shaout=%s\n",shaout);
	//HexStrToByte(shaout,strlen(shaout),key);
	//hex_print("16sha:",key,32);
	   
	/* Buffer for the decrypted text */  
	//unsigned char decryptedtext[264]={0}; 

	//int decryptedtext_len=0,

	char *plaintext =  "plaintext";
	plaintext = name;

    unsigned char *ciphertext = cis_malloc(DM_EP_MEM_SIZE);
  	if(ciphertext==NULL)
  	{
		xy_printf("mem err r1\n");
		ret=-1;
		goto fail;
	}
	cis_memset(ciphertext, 0, DM_EP_MEM_SIZE);
	
	my_aes_encrypt((char *)key,(char *)plaintext, ciphertext,strlen((const char *)plaintext),&ciphertext_len );  
 
	//printf("Ciphertext is %d bytes long:\n", ciphertext_len);
	//hex_print("Ciphertext:",ciphertext,ciphertext_len);
	
	//name = ciphertext;

	name = ciphertext; //???????

	testbase64=name;
	base64tmp=( char *)cis_malloc(DM_EP_MEM_SIZE);//szEpname is free now,use again;	
	if(base64tmp==NULL)
	{
		xy_printf("mem err r2\n");
		ret = -1;
		goto fail;
	}	
	cis_memset(base64tmp,0,DM_EP_MEM_SIZE);
	
	j_base64_encode((unsigned char *)testbase64, ciphertext_len, &encData, (unsigned int *)&encDataLen);
	cis_memcpy(base64tmp, encData, encDataLen);
	j_base64_free(encData, encDataLen);

	*szout = base64tmp;
	ret = encDataLen;
	
    if(ciphertext != NULL)
	{
		free(ciphertext);		
		ciphertext = NULL;
	}
	
	return ret;
	
fail:	
	
    //j_base64_free(encData, encDataLen);
    if(ciphertext != NULL)
	{
		free(ciphertext);		
		ciphertext = NULL;
	}

    if(base64tmp != NULL)
	{
		free(base64tmp);		
		base64tmp = NULL;
	}

    return ret;
}
#define EP_MEM_SIZE  (264)

static uint8_t prv_dm_get_value(st_context_t * contextP,
									st_data_t * dataArrayP,
									int number,
									dmobj_data_t * dmDataP)
{
	uint8_t result;
	uint16_t resId;
	st_data_t *dataP;
	int i;

	(void) contextP;

	for (i = 0; i<number;i++)
	{
		if(number == 1)
		{
			resId = dataArrayP->id;
			dataP = dataArrayP;
		}
		else
		{
			resId = dataArrayP->value.asChildren.array[i].id;
			dataP = dataArrayP->value.asChildren.array+i;
		}
		switch (resId)//need do error handle
		{
		case RES_DM_DEV_INFO:
	        {
	        	char *outbuff = NULL;
				if(dmDataP->dev_info == NULL) 
					return 0;
				
				cmccdm_getDevinfo(dmDataP->dev_info, RES_VALUE_BUFF_LEN);

				prv_dm_encode(dmDataP->dev_info, &outbuff);
			
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}
				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_APP_INFO:
			{
				char *outbuff = NULL;
				if(dmDataP->app_info == NULL) 
					return 0;
				
				cmccdm_getAppinfo(dmDataP->app_info,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->app_info, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_MAC:
			{
				char *outbuff = NULL;
				if(dmDataP->mac == NULL) 
					return 0;
				
				cmccdm_getMacinfo(dmDataP->mac,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->mac, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_ROM:
			{
				char *outbuff = NULL;
				if(dmDataP->rom == NULL) 
					return 0;
				
				cmccdm_getRominfo(dmDataP->rom, RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->rom, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_RAM:
			{
				char *outbuff = NULL;
				if(dmDataP->ram == NULL) 
					return 0;
				
				cmccdm_getRaminfo(dmDataP->ram, RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->ram, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_CPU:
			{
				char *outbuff = NULL;
				if(dmDataP->cpu == NULL) 
					return 0;
				
				cmccdm_getCpuinfo(dmDataP->cpu,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->cpu, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_SYS_VERSION:
			{
				char *outbuff = NULL;
				if(dmDataP->sys_ver == NULL) 
					return 0;
				
				cmccdm_getSysinfo(dmDataP->sys_ver,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->sys_ver, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_FIRM_VERSION:
			{
				char *outbuff = NULL;
				if(dmDataP->firm_ver == NULL) 
					return 0;
				
				cmccdm_getSoftVer(dmDataP->firm_ver,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->firm_ver, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_FIRM_NAME:
			{
				char *outbuff = NULL;
				if(dmDataP->firm_name == NULL) 
					return 0;
				
				cmccdm_getSoftName(dmDataP->firm_name,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->firm_name, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_VOLTE:
			{
				char *outbuff = NULL;
				if(dmDataP->volte == NULL) 
					return 0;
				
				cmccdm_getVolteinfo(dmDataP->volte,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->volte, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_NET_TYPE:
			{
				char *outbuff = NULL;
				if(dmDataP->net_type == NULL) 
					return 0;
				
				cmccdm_getNetType(dmDataP->net_type,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->net_type, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_NET_ACCT:
			{
				char *outbuff = NULL;
				if(dmDataP->net_acct == NULL) 
					return 0;
				
				cmccdm_getNetAccount(dmDataP->net_acct,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->net_acct, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_PHONE:
			{
				char *outbuff = NULL;
				if(dmDataP->phone == NULL) 
					return 0;
				
				cmccdm_getPNumber(dmDataP->phone,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->phone, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}

		case RES_DM_LOCATION:
			{
				char *outbuff = NULL;
				if(dmDataP->location == NULL) 
					return 0;
				
				cmccdm_getLocinfo(dmDataP->location,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->location, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_ROUTEMAC:
			{
				char *outbuff = NULL;
				if(dmDataP->routeMac == NULL) 
					return 0;
				
				cmccdm_getRouteMac(dmDataP->routeMac,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->routeMac, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_BRAND:
			{
				char *outbuff = NULL;
				if(dmDataP->brand == NULL) 
					return 0;
				
				cmccdm_getBrandinfo(dmDataP->brand,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->brand, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_GPU:
			{
				char *outbuff = NULL;
				if(dmDataP->gpu == NULL) 
					return 0;
				
				cmccdm_getGPUinfo(dmDataP->gpu,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->gpu, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_BOARD:
			{
				char *outbuff = NULL;
				if(dmDataP->board == NULL) 
					return 0;
				
				cmccdm_getBoardinfo(dmDataP->board,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->board, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_MODEL:
			{
				char *outbuff = NULL;
				if(dmDataP->model == NULL) 
					return 0;
				
				cmccdm_getModelinfo(dmDataP->model,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->model, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		case RES_DM_RESOLUTION:
			{
				char *outbuff = NULL;
				if(dmDataP->resolution == NULL) 
					return 0;
				
				cmccdm_getResinfo(dmDataP->resolution,  RES_VALUE_BUFF_LEN);
				
				prv_dm_encode(dmDataP->resolution, &outbuff);
				
				data_encode_string(outbuff, dataP);
				if(outbuff != NULL)
				{
					cis_free(outbuff);
					outbuff = NULL;
				}

				result = COAP_205_CONTENT;
				break;
			}
		default:
			return COAP_404_NOT_FOUND;
		}
	}  
	return result;
}



bool std_dm_create(st_context_t * contextP,
					   int instanceId,
                       st_object_t * dmObj)
{
    dmobj_data_t * instDM=NULL;
    dmobj_data_t * targetP = NULL;
    uint8_t instBytes = 0;
    uint8_t instCount = 0;
    cis_iid_t instIndex;
    if (NULL == dmObj)
    {
        return false;   
    }

    // Manually create a hard-code instance
    targetP = (dmobj_data_t *)cis_malloc(sizeof(dmobj_data_t));
    if (NULL == targetP)
    {
        return false;
    }

    cis_memset(targetP, 0, sizeof(dmobj_data_t));
    
    targetP->instanceId = (uint16_t)instanceId;
    targetP->dev_info = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->dev_info,0,RES_VALUE_BUFF_LEN);
	
    targetP->app_info = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->app_info,0,RES_VALUE_BUFF_LEN);
	
    targetP->mac = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->mac,0,RES_VALUE_BUFF_LEN);
	
    targetP->rom = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->rom,0,RES_VALUE_BUFF_LEN);
	
    targetP->ram = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->ram,0,RES_VALUE_BUFF_LEN);
	
    targetP->cpu = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->cpu,0,RES_VALUE_BUFF_LEN);
	
    targetP->sys_ver = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->sys_ver,0,RES_VALUE_BUFF_LEN);
	
    targetP->firm_ver = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->firm_ver,0,RES_VALUE_BUFF_LEN);
	
    targetP->firm_name = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->firm_name,0,RES_VALUE_BUFF_LEN);
	
    targetP->volte = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->volte,0,RES_VALUE_BUFF_LEN);
	
    targetP->net_type = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->net_type,0,RES_VALUE_BUFF_LEN);

    targetP->net_acct = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->net_acct,0,RES_VALUE_BUFF_LEN);

    targetP->phone = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->phone,0,RES_VALUE_BUFF_LEN);

    targetP->location = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->location,0,RES_VALUE_BUFF_LEN);

	targetP->routeMac = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->routeMac,0,RES_VALUE_BUFF_LEN);

	targetP->brand = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->brand,0,RES_VALUE_BUFF_LEN);

	targetP->gpu = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->gpu,0,RES_VALUE_BUFF_LEN);

	targetP->board = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->board,0,RES_VALUE_BUFF_LEN);

	targetP->model = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->model,0,RES_VALUE_BUFF_LEN);

    targetP->resolution = (char *)cis_malloc(RES_VALUE_BUFF_LEN); 
    cis_memset(targetP->resolution,0,RES_VALUE_BUFF_LEN);

    instDM = (dmobj_data_t * )std_object_put_dm(contextP,(cis_list_t*)targetP);
    
    instCount = CIS_LIST_COUNT(instDM);
    if(instCount == 0)
    {
        cis_free(targetP->dev_info);
		cis_free(targetP->app_info);
        cis_free(targetP->rom);
		cis_free(targetP->ram);
		cis_free(targetP->cpu);
		cis_free(targetP->sys_ver);
		cis_free(targetP->firm_ver);
		cis_free(targetP->firm_name);
		cis_free(targetP->volte);
		cis_free(targetP->net_type);
		cis_free(targetP->net_acct);
		cis_free(targetP->phone);
		cis_free(targetP->location);
		cis_free(targetP->resolution);
		cis_free(targetP->brand);
		cis_free(targetP->gpu);
		cis_free(targetP->board);
		cis_free(targetP->model);
		cis_free(targetP->resolution);
        cis_free(targetP);
        return false;
    }

    /*first security object instance
     *don't malloc instance bitmap ptr*/
    if(instCount == 1)
    {
        return true;
    }

    dmObj->instBitmapCount = instCount;
    instBytes = (instCount - 1) / 8 + 1;
    if(dmObj->instBitmapBytes < instBytes){
        if(dmObj->instBitmapBytes != 0 && dmObj->instBitmapPtr != NULL)
        {
            cis_free(dmObj->instBitmapPtr);
        }
        dmObj->instBitmapPtr = (uint8_t*)cis_malloc(instBytes);
        dmObj->instBitmapBytes = instBytes;
    }
    cissys_memset(dmObj->instBitmapPtr,0,instBytes);
    targetP = instDM;
    for (instIndex = 0;instIndex < instCount;instIndex++)
    {
        uint8_t instBytePos = targetP->instanceId / 8;
        uint8_t instByteOffset = 7 - (targetP->instanceId % 8);
        dmObj->instBitmapPtr[instBytePos] += 0x01 << instByteOffset;

        targetP = targetP->next;
    }
    return true;
}



uint8_t std_dm_read(st_context_t * contextP,
						uint16_t instanceId,
                        int * numDataP,
                        st_data_t ** dataArrayP,
                        st_object_t * objectP)
{
    uint8_t result=0;
    int i;
	dmobj_data_t * targetP = NULL;
	targetP = prv_dm_find(contextP,instanceId);

	(void) objectP;

    // this is a single instance object
    if (instanceId != 0||targetP==NULL)
    {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {
			RES_DM_DEV_INFO,
 			RES_DM_APP_INFO,
 			RES_DM_MAC,
 			RES_DM_ROM,
 			RES_DM_RAM,
 			RES_DM_CPU,
 			RES_DM_SYS_VERSION,
 			RES_DM_FIRM_VERSION,
 			RES_DM_FIRM_NAME,
 			RES_DM_VOLTE,
 			RES_DM_NET_TYPE,
 			RES_DM_NET_ACCT,
 			RES_DM_PHONE,
 			RES_DM_LOCATION,
 			RES_DM_ROUTEMAC,
 			RES_DM_BRAND,
 			RES_DM_GPU,
 			RES_DM_BOARD,
 			RES_DM_MODEL,
 			RES_DM_RESOLUTION
        };
        int nbRes = sizeof(resList)/sizeof(uint16_t);
		(*dataArrayP)->id = 0;
		(*dataArrayP)->type = DATA_TYPE_OBJECT_INSTANCE;
		(*dataArrayP)->value.asChildren.count = nbRes;
		(*dataArrayP)->value.asChildren.array =  data_new(nbRes);
		cis_memset((*dataArrayP)->value.asChildren.array,0,(nbRes)*sizeof(cis_data_t));

        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
			(*dataArrayP)->value.asChildren.array[i].id = resList[i];
        }
		
		if (prv_dm_get_value(contextP,(*dataArrayP),nbRes,targetP)!=COAP_205_CONTENT)
		{
			return COAP_500_INTERNAL_SERVER_ERROR;
		}
    }
	else
	{
		result = prv_dm_get_value(contextP,(*dataArrayP),1,targetP);
	}

    return result;
}


#endif
#endif


