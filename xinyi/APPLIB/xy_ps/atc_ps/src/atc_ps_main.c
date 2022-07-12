#include "atc_ps.h"
#include "xy_atc_interface.h"
#include "xy_memmap.h"
#include "xy_system.h"

#if ATC_DSP
unsigned long           g_ulRegEventId;
volatile unsigned long* g_pRegEventId = &g_ulRegEventId;
#else
volatile unsigned long* g_pRegEventId = (volatile unsigned long*)PS_AT_FILTER;
factory_nv_t*           g_factory_nv = (factory_nv_t*)RAM_FACTORY_NV_BASE;
#endif
osMutexId_t             g_pRegEventId_Mutex;
unsigned char           g_CSQ_ReqFlg;

void xy_atc_data_req(unsigned short usDataLen, unsigned char*pucData)
{
    if (NULL == pucData || 0 == usDataLen)
    {
        return;
    }
    
    ATC_SendApDataReq(ATC_AP_TRUE, 0, usDataLen, pucData);
}

int xy_atc_interface_call(char* pCmd, func_AppInterfaceCallback callback_func, void* pResult)
{
    osSemaphoreId_t               semaphore;
    ST_ATC_AP_APP_INTERFACE_NODE* pNode;
    unsigned char                 ucRtnVal   = XY_OK;

    if(NULL == pCmd || 0 == strlen(pCmd))
    {
        return XY_ERR;
    }

#if !IS_DSP_CORE
	if(DSP_IS_NOT_LOADED())
		return XY_ERR;
#endif

    semaphore = osSemaphoreNew(0xFFFF, 0, NULL);

    ATC_SendApDataReq(ATC_AP_TRUE, (unsigned long)semaphore, strlen(pCmd), (unsigned char*)pCmd);
    osSemaphoreAcquire(semaphore, osWaitForever);

    osMutexAcquire(g_AtcApInfo.stAppInterfaceInfo.mutex, osWaitForever);
    pNode = AtcAp_GetAppInterfaceInfo_BySema((unsigned long)semaphore);
    osSemaphoreDelete(semaphore);
    if(pNode == NULL)
    {
        osMutexRelease(g_AtcApInfo.stAppInterfaceInfo.mutex);
        return XY_ERR;
    }

    if(D_APP_INTERFACE_RESULT_SUCC != pNode->ucResult)
    {
        AtcAp_DelAppInterfaceResult(pNode);
        osMutexRelease(g_AtcApInfo.stAppInterfaceInfo.mutex);
        return XY_ERR;
    }
    
    if(callback_func != NULL)
    {
        if(pNode->pucData != NULL)
        {
            ucRtnVal = callback_func(pNode->pucData, pNode->usDataLen, pResult);        
        }
        else
        {
            ucRtnVal = XY_ERR;
        }
    }
    else if(pResult != NULL)
    {
        if(pNode->pucData != NULL)
        {
            AtcAp_MemCpy(pResult, pNode->pucData, pNode->usDataLen);   
            ucRtnVal = XY_OK;
        }
        else
        {
     //       ucRtnVal = XY_ERR;
        }
    }
    AtcAp_DelAppInterfaceResult(pNode);
    osMutexRelease(g_AtcApInfo.stAppInterfaceInfo.mutex);
    
    return ucRtnVal;
}

void xy_atc_registerPSEventCallback(unsigned long eventId, xy_psEventCallback_t callback)
{
    if(osKernelGetState() != osKernelInactive)
    {
        osMutexAcquire(g_pRegEventId_Mutex, osWaitForever);
    }
    
    AtcAp_registerPSEventCallback(eventId, callback);

    if(osKernelGetState() != osKernelInactive)
    {
        osMutexRelease(g_pRegEventId_Mutex);
    }
}


void AtcAp_LinkList_AddNode(ST_ATC_AP_LINK_NODE** ppHead, ST_ATC_AP_LINK_NODE** ppLast, ST_ATC_AP_LINK_NODE* pNode)
{
    if(NULL == *ppHead)
    {
        *ppHead = pNode;
        if(ppLast != NULL)
        {
            *ppLast = pNode;
        }
    }
    else
    {
        if(ppLast != NULL)
        {
            (*ppLast)->next = pNode;
            pNode->prev = *ppLast;
            *ppLast = pNode;
        }
        else
        {
            (*ppHead)->prev = pNode;
            pNode->next = *ppHead;
            *ppHead = pNode;
        }
    }
}

void AtcAp_LinkList_RemoveNode(ST_ATC_AP_LINK_NODE** ppHead, ST_ATC_AP_LINK_NODE** ppLast, ST_ATC_AP_LINK_NODE* pNode)
{
    if(pNode->next != NULL)
    {
        pNode->next->prev = pNode->prev;
    }

    if(pNode->prev != NULL)
    {
        pNode->prev->next = pNode->next;
    }

    if(*ppHead == pNode)
    {
        *ppHead = pNode->next;
    }

    if(*ppLast == pNode)
    {
        *ppLast = pNode->prev;
    }
}

void AtcAp_DelAppInterfaceResult(ST_ATC_AP_APP_INTERFACE_NODE* pNode)
{
    if(pNode == NULL)
    {
        return;
    }

    AtcAp_LinkList_RemoveNode((ST_ATC_AP_LINK_NODE**)&g_AtcApInfo.stAppInterfaceInfo.pHead,
                              (ST_ATC_AP_LINK_NODE**)&g_AtcApInfo.stAppInterfaceInfo.pTail, (ST_ATC_AP_LINK_NODE*)pNode);
    if(NULL == g_AtcApInfo.stAppInterfaceInfo.pHead)
    {
        g_AtcApInfo.stAppInterfaceInfo.ucSeqNum = 0;
    }
    
    if(NULL != pNode->pucData)
    {
        AtcAp_Free(pNode->pucData);
    }
    AtcAp_Free(pNode);
}

ST_ATC_AP_APP_INTERFACE_NODE* AtcAp_AddAppInterfaceInfo(unsigned long ulSemaphoreId)
{
    ST_ATC_AP_APP_INTERFACE_NODE* pNode;

    pNode = AtcAp_Malloc(sizeof(ST_ATC_AP_APP_INTERFACE_NODE));
    pNode->ulSemaphore = ulSemaphoreId;

    osMutexAcquire(g_AtcApInfo.stAppInterfaceInfo.mutex, osWaitForever);
    pNode->ucSeqNum = ++g_AtcApInfo.stAppInterfaceInfo.ucSeqNum;
    if(0 == pNode->ucSeqNum)
    {
        pNode->ucSeqNum = ++g_AtcApInfo.stAppInterfaceInfo.ucSeqNum;
    }
    
    AtcAp_LinkList_AddNode((ST_ATC_AP_LINK_NODE**)&g_AtcApInfo.stAppInterfaceInfo.pHead,
                           (ST_ATC_AP_LINK_NODE**)&g_AtcApInfo.stAppInterfaceInfo.pTail, (ST_ATC_AP_LINK_NODE*)pNode);

    osMutexRelease(g_AtcApInfo.stAppInterfaceInfo.mutex);
    
    g_AtcApInfo.ucTempSeqNum = pNode->ucSeqNum;
    return pNode;
}

ST_ATC_AP_APP_INTERFACE_NODE* AtcAp_GetAppInterfaceInfo_BySeqNum(unsigned char ucSeqNum)
{
    ST_ATC_AP_APP_INTERFACE_NODE* pNode;

    pNode = g_AtcApInfo.stAppInterfaceInfo.pHead;
    while(pNode != NULL)
    {
        if(pNode->ucSeqNum == ucSeqNum)
        {
            return pNode;
        }
        pNode = (ST_ATC_AP_APP_INTERFACE_NODE*)(pNode->node.next);
    }

    return NULL;
}

ST_ATC_AP_APP_INTERFACE_NODE* AtcAp_GetAppInterfaceInfo_BySema(unsigned long ulSemaphore)
{
    ST_ATC_AP_APP_INTERFACE_NODE* pNode;

    pNode = g_AtcApInfo.stAppInterfaceInfo.pHead;
    while(pNode != NULL)
    {
        if(pNode->ulSemaphore == ulSemaphore)
        {
            return pNode;
        }
        pNode = (ST_ATC_AP_APP_INTERFACE_NODE*)(pNode->node.next);
    }

    return NULL;
}

void AtcAp_AppInterfaceInfo_CmdRstProc(unsigned char ucSeqNum, unsigned ucResult)
{
    ST_ATC_AP_APP_INTERFACE_NODE*     pAppInterfaceInfo;

    osMutexAcquire(g_AtcApInfo.stAppInterfaceInfo.mutex, osWaitForever);
    pAppInterfaceInfo = AtcAp_GetAppInterfaceInfo_BySeqNum(ucSeqNum);
    if(pAppInterfaceInfo == NULL)
    {
        osMutexRelease(g_AtcApInfo.stAppInterfaceInfo.mutex);
        return;
    }

    if(pAppInterfaceInfo->ulSemaphore != 0xffffffff)
    {
        pAppInterfaceInfo->ucResult = ucResult;
        osMutexRelease(g_AtcApInfo.stAppInterfaceInfo.mutex);
        osSemaphoreRelease((osSemaphoreId_t)pAppInterfaceInfo->ulSemaphore);
    }
    else
    {
        AtcAp_DelAppInterfaceResult(pAppInterfaceInfo);
        osMutexRelease(g_AtcApInfo.stAppInterfaceInfo.mutex);    
    } 
}

static signed long AtcAp_RecvMsg(unsigned long* pRecvMsg, ST_ATC_AP_MSG_INFO* pMsgInfo)
{
    ST_ATC_AP_MSG_NODE* pCurrNode;
  
    osSemaphoreAcquire(pMsgInfo->msgSemaPhore, osWaitForever);
    osMutexAcquire(pMsgInfo->msgMutex, osWaitForever);
    
    if(NULL == pMsgInfo->head)
    {
        osMutexRelease(pMsgInfo->msgMutex);
        return -1;
    }

    pCurrNode = pMsgInfo->head; 
    *pRecvMsg = pCurrNode->ulMsg;

    pMsgInfo->head = pCurrNode->next;
    if(NULL == pMsgInfo->head)
    {
        pMsgInfo->last = NULL;
    }
    pMsgInfo->ucCnt--;
    osMutexRelease(pMsgInfo->msgMutex);

    AtcAp_Free(pCurrNode);
    
    return 0;
}

void AtcAp_SendMsg2AtcAp(void* pMsg, ST_ATC_AP_MSG_INFO* pMsgInfo)
{
    ST_ATC_AP_MSG_NODE* pAddNode;

    pAddNode = (ST_ATC_AP_MSG_NODE*)AtcAp_Malloc(sizeof(ST_ATC_AP_MSG_NODE));
    pAddNode->ulMsg = (unsigned long)pMsg;
    pAddNode->next = NULL;

    osMutexAcquire(pMsgInfo->msgMutex, osWaitForever);
    pMsgInfo->ucCnt++;
    
    if(NULL == pMsgInfo->last)
    {
        pMsgInfo->head = pAddNode;
        pMsgInfo->last = pAddNode;
    }
    else
    {
        pMsgInfo->last->next = pAddNode;
        pMsgInfo->last = pAddNode;
    }
    osMutexRelease(pMsgInfo->msgMutex);

    osSemaphoreRelease(pMsgInfo->msgSemaPhore);
}

void AtcAp_TaskEntry(void* pArgs)
{
    unsigned long RecvMsg;

    if(g_CSQ_ReqFlg == ATC_AP_TRUE)
    {
        ATC_SendApDataReq(ATC_AP_TRUE, 0xffffffff, strlen("AT+CESQ=10\r\n"), (unsigned char*)"AT+CESQ=10\r\n");
    }

    for (;;)
    {
#ifdef PS_INTEGRATION_TEST
        if(Test_AtcAp_JumpOut() == PS_TRUE)
            break;
#endif 
        if (-1 != AtcAp_RecvMsg(&RecvMsg, &g_AtcApInfo.msgInfo))
        {
            if(ATC_AP_TRUE == AtcAp_NormalMsgDistribute((unsigned char*)RecvMsg))
            {
                AtcAp_Free(RecvMsg);
            }
        }
    }
}

static void AtcAp_TaskEntry_EventCallback(void* pArgs)
{
    unsigned long           RecvMsg;
    ATC_MSG_DATA_IND_STRU*  pAtcDataInd;
    ST_ATC_CMD_COM_EVENT*   pCmdEvent;
    unsigned short          i;

    for (;;)
    {
        if (-1 != AtcAp_RecvMsg(&RecvMsg, &g_AtcApInfo.MsgInfo_EventCb))
        {
            pAtcDataInd = (ATC_MSG_DATA_IND_STRU*)RecvMsg;
            AtcAp_DataIndProc_RegEventInd(pAtcDataInd->aucMsgData, pAtcDataInd->usMsgLen);

            if(0 == pAtcDataInd->ucSeqNum)
            {        
                osMutexAcquire(g_AtcApInfo.stAtRspInfo.mutex, osWaitForever);
                if(NULL == g_AtcApInfo.stAtRspInfo.aucAtcRspBuf)
                {
                    g_AtcApInfo.stAtRspInfo.aucAtcRspBuf = (unsigned char*)AtcAp_Malloc(D_ATC_RSP_MAX_BUF_SIZE);
                }
                
                pCmdEvent = (ST_ATC_CMD_COM_EVENT*)pAtcDataInd->aucMsgData;
                for(i = 0; i < D_ATC_DATAIND_PROC_TBL_SIZE; i++)
                {
                    if(AtcAp_DataIndProcTable[i].usEvent == pCmdEvent->usEvent)
                    {
                        AtcAp_DataIndProcTable[i].CommandProc((unsigned char *)pCmdEvent);
                        break;
                    }
                }
                
                if(g_AtcApInfo.MsgInfo_EventCb.ucCnt == 0)
                {
                    AtcAp_Free(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
                }
                osMutexRelease(g_AtcApInfo.stAtRspInfo.mutex);
            }
            AtcAp_Free(pAtcDataInd);
        }
    }
}

void AtcAp_registerPSEventCallback(unsigned long eventId, xy_psEventCallback_t callback)
{
    ST_ATC_AP_PS_EVENT_REGISTER_INFO* pEventInfo;

    pEventInfo = (ST_ATC_AP_PS_EVENT_REGISTER_INFO*)AtcAp_Malloc(sizeof(ST_ATC_AP_PS_EVENT_REGISTER_INFO));
    pEventInfo->eventId = eventId;
    pEventInfo->callback = callback;

    AtcAp_LinkList_AddNode((ST_ATC_AP_LINK_NODE**)&g_AtcApInfo.pEventRegList, (ST_ATC_AP_LINK_NODE**)NULL, (ST_ATC_AP_LINK_NODE*)pEventInfo);
    (*g_pRegEventId) |= eventId;

    if(0 != (eventId & D_XY_PS_REG_EVENT_CESQ_Ind))
    {
        if(osKernelGetState() == osKernelInactive)
        {
            g_CSQ_ReqFlg = ATC_AP_TRUE;
        }
        else
        {      
            ATC_SendApDataReq(ATC_AP_TRUE, 0xffffffff, strlen("AT+CESQ=10\r\n"), (unsigned char*)"AT+CESQ=10\r\n");
        }
    }
}

void atc_ap_task_init()
{
	osThreadAttr_t thread_attr = {0};

    AtcAp_MemSet(&g_AtcApInfo, 0, sizeof(ST_ATC_AP_INFO));
    
    g_AtcApInfo.ucAtHeaderSpaceFlg = 255;

    g_AtcApInfo.stAtRspInfo.mutex = osMutexNew(NULL);

    g_AtcApInfo.msgInfo.msgSemaPhore = osSemaphoreNew(0xFFFF, 0, NULL);
    g_AtcApInfo.msgInfo.msgMutex = osMutexNew(NULL);
    
    g_AtcApInfo.MsgInfo_EventCb.msgSemaPhore = osSemaphoreNew(0xFFFF, 0, NULL);
    g_AtcApInfo.MsgInfo_EventCb.msgMutex = osMutexNew(NULL);
    
    g_AtcApInfo.stAppInterfaceInfo.mutex = osMutexNew(NULL);
    g_pRegEventId_Mutex =  osMutexNew(NULL);
	
	thread_attr.name	   = "ATC_AP";
	thread_attr.priority   = D_THEARD_ATC_AP_PRIO;
	thread_attr.stack_size = 2*1024;
    osThreadNew((osThreadFunc_t)(AtcAp_TaskEntry), NULL, &thread_attr);

    thread_attr.name       = "ATC_AP_CB";
    thread_attr.priority   = D_THEARD_ATC_AP_PRIO;
    thread_attr.stack_size = 1536;
    osThreadNew((osThreadFunc_t)(AtcAp_TaskEntry_EventCallback), NULL, &thread_attr);
    
    *g_pRegEventId = 0;
    g_CSQ_ReqFlg = ATC_AP_FALSE;
}

