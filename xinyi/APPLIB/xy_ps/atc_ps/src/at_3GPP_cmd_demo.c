#include "atc_ps.h"

typedef struct
{
    unsigned char  cid;
    unsigned char* apn;
} ST_DEMO_3_RESULT;

static int atc_interface_call_demo_03_parse(void* pData, int dataLen, void* pResult)
{
    ATC_MSG_CGCONTRDP_CNF_STRU* pCgcongrdpCnf = (ATC_MSG_CGCONTRDP_CNF_STRU*)pData;
    ST_DEMO_3_RESULT*           pDemo3Result = (ST_DEMO_3_RESULT*)pResult;

    if(pResult == NULL)
    {
        return XY_ERR;
    }

    //以下解析开始
    if(pCgcongrdpCnf->stPara.ucValidNum != 1)
    {
        return XY_ERR;
    }

    pDemo3Result->cid = pCgcongrdpCnf->stPara.aucPdpDynamicInfo[0].ucCid;
    pDemo3Result->apn = (unsigned char*)xy_zalloc(pCgcongrdpCnf->stPara.aucPdpDynamicInfo[0].ucApnLen + 1);
    strcpy((char *)pDemo3Result->apn, (const char *)pCgcongrdpCnf->stPara.aucPdpDynamicInfo[0].aucApn);

    return XY_OK;
}

/**
 * 设置命令，只返回OK/ERROR
 */
void atc_interface_call_demo_01()
{
    int ret;
    
    ret = xy_atc_interface_call("AT+CGDCONT=1,IP\r\n", NULL, (void*)NULL);
    
    xy_printf("[atc_interface_call_demo_01] ret=%d", ret); 
}

/**
 * 查询命令，解析回调函数为空
 */
void atc_interface_call_demo_02()
{
    ATC_MSG_CGCONTRDP_CNF_STRU tCgcongrdpCnf = { 0 };
    char*                      pLogData;
    
    if(XY_OK != xy_atc_interface_call("AT+CGCONTRDP=0\r\n", NULL, (void*)&tCgcongrdpCnf))
    {
        return;
    }

    //解析参数
    pLogData = (char*)xy_zalloc(150);
    if(tCgcongrdpCnf.stPara.ucValidNum != 0)
    {  
        sprintf(pLogData, "cid=%d, apn=%s", tCgcongrdpCnf.stPara.aucPdpDynamicInfo[0].ucCid, tCgcongrdpCnf.stPara.aucPdpDynamicInfo[0].aucApn);
    }
    
    xy_printf("[atc_interface_call_demo_02] %s", pLogData);
    xy_free(pLogData);
}


/**
 * 查询命令，解析回调函数不为空
 */
void atc_interface_call_demo_03()
{   
    ST_DEMO_3_RESULT           tResult = { 0 };
    
    if(XY_OK != xy_atc_interface_call("AT+CGCONTRDP=0\r\n", atc_interface_call_demo_03_parse, (void*)&tResult))
    {
        return;
    }

    xy_printf("[atc_interface_call_demo_03] cid=%d, apn=%s", tResult.cid, tResult.apn);
    xy_free(tResult.apn);
}

