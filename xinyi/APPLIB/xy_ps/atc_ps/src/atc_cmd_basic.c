/*******************************************************************************
@(#)Copyright(C)2016,HuaChang Technology (Dalian) Co,. LTD.
File name   : atc_cmd_basic.c
Description : 
Function List:
History:
1. Dep2_066    2016.12.20  Create
*******************************************************************************/
#include "atc_ps.h"

#ifndef HWREG(x)
#define HWREG(x)	(*((volatile unsigned long *)(x)))
#endif
#if 0
/*lint -e438*/
/*******************************************************************************
  MODULE    : ATC_CGMI_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGMI_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 0;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    usCmdStrLen = 5;                                                                    /* +CGMI length set                     */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);

    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_TEST:
        ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_CGMI_T;
        ucResult = D_ATC_COMMAND_OK;
        break;
    case D_ATC_CMD_FUNC_READ:                                                           /* no break                             */
    case D_ATC_CMD_FUNC_SET:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        if (D_ATC_N_CR == *(pCommandBuffer + usCmdStrLen))
        {
            ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_CGMI;
            ucResult = D_ATC_COMMAND_OK;
        }
        else
        {
            ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
        }
        break;
    default:
        break;
    }

    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CGMR_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGMR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 0;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    usCmdStrLen = 5;                                                                    /* +CGMR length set                     */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);

    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_TEST:
        ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_CGMR_T;
        ucResult = D_ATC_COMMAND_OK;
        break;
    case D_ATC_CMD_FUNC_READ:                                                          /* no break                              */
    case D_ATC_CMD_FUNC_SET:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        if (D_ATC_N_CR == *(pCommandBuffer + usCmdStrLen))
        {
            ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_CGMR;
            ucResult = D_ATC_COMMAND_OK;
        }
        else
        {
            ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
        }
        break;
    default:
        break;
    }
    return ucResult;
}
#endif
/*******************************************************************************
  MODULE    : ATC_CGSN_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGSN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* +CGSN length set                     */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pucValue;
                                                               
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGSN,pEventBuffer,&ucCmdFunc);

    if(ucCmdFunc == D_ATC_CMD_FUNC_SET)
    {
        usCmdStrLen += 1;        
        pucValue     = &(((ST_ATC_CGSN_PARAMETER *)pEventBuffer)->ucSnt);
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 3, 0, 1);    
    }
    else if(D_ATC_CMD_FUNC_NOEQUAL_SET == ucCmdFunc)
    {
        if (D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
        {
            return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
        }
    }     
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CEREG_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CEREG_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* +CEREG length                        */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;
                                                                
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CEREG,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /*  '=' set                             */
        pucValue     = &(((ST_ATC_CEREG_PARAMETER *)pEventBuffer)->ucN);
        pucValueFlag = &(((ST_ATC_CEREG_PARAMETER *)pEventBuffer)->ucNFlag);
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 5, 1, 1);     
    }
    return ucResult;  
}

/*******************************************************************************
  MODULE    : ATC_CGATT_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGATT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* +CGATT length                        */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGATT,pEventBuffer,&ucCmdFunc);
    
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' length                           */
        pValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
        ucResult   = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pValue, NULL, 0, 1, 0, 1);        
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CIMI_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CIMI_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* +CIMI length set                     */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CIMI,pEventBuffer,&ucCmdFunc);

    if(ucCmdFunc == D_ATC_CMD_FUNC_NOEQUAL_SET && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
    {
        return  D_ATC_COMMAND_TOO_MANY_PARAMETERS;
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CGDCONT_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGDCONT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 8;                                                              /* "+CGDCONT" length                    */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    unsigned char aucPDPtypeData[7] ={0};                                                       /* PDP type                             */
    unsigned char aucApnValue[101] ={0};
    unsigned char ucAnlParLength;
    unsigned char i;
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;
    unsigned char ucMinValue;
    unsigned char ucMaxValue;
    ST_ATC_CGDCONT_PARAMETER* pCgdcontParam = (ST_ATC_CGDCONT_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGDCONT,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }    
    usCmdStrLen += 1;                                                               /* '=' set                              */
   
    for (; iParamNum < D_ATC_PARAM_MAX_P_CGDCONT && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucCid);
            pucValueFlag = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucCidFlag);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 0); 
            break; 

        case 1:
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen),
                 6, &ucParLen, &ucAnlParLength, aucPDPtypeData, &ucStopStrInf);

            if(D_ATC_PARAM_OK == ucParResult)     
            {
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                          /* parameter length set                 */
                
                for (i = 0; i < D_ATC_PDPTYPE_LIST_NUM; i++)
                {
                    if (0 == AtcAp_Strncmp(aucPDPtypeData, (unsigned char*)ATC_PdpType_Table[i].aucStr))
                    {
                        ((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucPdpTypeValue = ATC_PdpType_Table[i].ucStrVal;
                         ((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucPdpTypeFlg = D_ATC_FLAG_TRUE; /* IP Existence                 */
                        ucResult = D_ATC_COMMAND_OK;
                        break;
                    }
                } 
                if(i == D_ATC_PDPTYPE_LIST_NUM)
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
            }
            else if(D_ATC_PARAM_ERROR == ucParResult)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;                               /* parameter error set                  */
            }
            else
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }
            
            if(ucStopStrInf == D_ATC_STOP_CHAR_KANMA)
            {
                usCmdStrLen += 1;                                                       /* ',' set                              */
            }

            break;
            
        case 2:
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                       D_ATC_P_CGDCONT_APN_SIZE_MAX - 1, &ucParLen, 
                       &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucApnLen),
                       aucApnValue, &ucStopStrInf);

            if(D_ATC_PARAM_OK == ucParResult)     
            {
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                          /* parameter length set                 */
                if (0 != pCgdcontParam->ucApnLen)
                {
                    if(pCgdcontParam->ucApnLen <= sizeof(pCgdcontParam->aucApnValue))
                    {
                        AtcAp_MemCpy(pCgdcontParam->aucApnValue, aucApnValue, pCgdcontParam->ucApnLen);
                    }
                    else
                    {
                        pCgdcontParam->pucApnValue = (unsigned char*)AtcAp_Malloc(pCgdcontParam->ucApnLen);
                        AtcAp_MemCpy(pCgdcontParam->pucApnValue, aucApnValue, pCgdcontParam->ucApnLen);
                    }
                }
            }
            else if(D_ATC_PARAM_EMPTY == ucParResult)
            {
                if ((D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    ||(D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
                    ||(D_ATC_STOP_CHAR_SEMICOLON == ucStopStrInf))
                {
                    usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                      /* parameter length set                 */
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }                    
            }
            else if(D_ATC_PARAM_ERROR == ucParResult)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            else
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }

            if(ucStopStrInf == D_ATC_STOP_CHAR_KANMA)
            {
                usCmdStrLen += 1;                                                       /* ',' set                              */
            }
            break;
            
        case 3:
            for (; D_ATC_N_COMMA != *(pCommandBuffer + usCmdStrLen); usCmdStrLen++)
            {
                if (D_ATC_N_CR == *(pCommandBuffer + usCmdStrLen))
                {
                    return D_ATC_COMMAND_OK;
                }
            }
            ucStopStrInf = D_ATC_STOP_CHAR_KANMA;
            usCmdStrLen += 1;                                                       /* ',' set                              */
            break;
            
        case 4:            
        case 5:
        case 10: 
        case 11:
            switch(iParamNum)
            {
                case 4:            /* <d_comp> value get */
                    pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucD_comp);
                    pucValueFlag = NULL;
                    ucMinValue   = 0;
                    ucMaxValue   = 0;
                    break;
                case 5:            /* <h_comp> value get */
                    pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucH_comp);
                    pucValueFlag = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucH_compFlag);
                    ucMinValue   = 0;
                    ucMaxValue   = 1;
                    break;
                case 10:           /* <NSLPI> value get                    */
                    pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucNSLPI);
                    pucValueFlag = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucNSLPIFlag);
                    ucMinValue   = 0;
                    ucMaxValue   = 1;                    
                    break;
                default:           /* <SecurePco> value get                    */
                    pucValue     = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucSecurePco);
                    pucValueFlag = &(((ST_ATC_CGDCONT_PARAMETER *)pEventBuffer)->ucSecurePcoFlag);
                    ucMinValue   = 0;
                    ucMaxValue   = 1;
                    break;
            } 
            if(iParamNum == 11)
            {
                ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, ucMinValue, ucMaxValue, 1, 1); 
            }
            else
            {
                ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, ucMinValue, ucMaxValue, 1, 0); 
            }
            
            if(pucValueFlag != NULL) /* default value get                    */
            {
                *pucValueFlag = D_ATC_FLAG_TRUE;
            }          
            break;
        case 6:            /* <IPv4AddrAlloc> value get            */
        case 7:            /* <request_type> value get *//*lint !e685 !e568*/                   
        case 8:            /* <P-CSCF_discovery> value get*/
        case 9:            /* <IM_CN_Signalling_Flag_Ind> value get */
            if(D_ATC_N_COMMA != *(pCommandBuffer + usCmdStrLen) && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            usCmdStrLen += 1;
            break;
        default:
            break;
        }

        if(ucResult != D_ATC_COMMAND_OK)
        {
            return ucResult;
        }
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CFUN_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CFUN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pucValue;
    unsigned char iParamNum=0;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CFUN,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    for (; iParamNum < D_ATC_PARAM_MAX_P_CFUN && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:  /* <Fun> value get */
            pucValue  = &(((ST_ATC_CFUN_PARAMETER *)pEventBuffer)->ucFun);
            ucResult  = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 5, 0, 0);  
            if(0 != *pucValue && 1 != *pucValue && 5 != *pucValue)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            break;
        case 1: /* <Rst> value get */
            pucValue  = &(((ST_ATC_CFUN_PARAMETER *)pEventBuffer)->ucRst);
            ucResult  = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 1, 1, 1);    
            break;
        default:
            break;
        }          
    }
#if 0

    pucValue  = &(((ST_ATC_CFUN_PARAMETER *)pEventBuffer)->ucFun);
    ucResult  = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 5, 1, 1);
    if(0 != *pucValue && 1 != *pucValue && 5 != *pucValue)
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
#endif

    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CMEE_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CMEE_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pucValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CMEE,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;  
        pucValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 2, 1, 1);  
    }      
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CLAC_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CLAC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                       /* +CLAC length set                         */
    unsigned char ucResult;                                                              /* command analysis result                  */
    unsigned char ucCmdFunc;                                                             /* command form                             */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CLAC,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_NOEQUAL_SET == ucCmdFunc && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
    {
        ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CESQ_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CESQ_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* +CESQ length set                     */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned long ulBinaryData;
    unsigned char ucValueFlag;
    ST_ATC_CESQ_PARAMETER* pCesqParam = (ST_ATC_CESQ_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CESQ,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_COMMAND_OK != ATC_GetDecimalParameterLong(pCommandBuffer, &usCmdStrLen, 5, (unsigned long*)&ulBinaryData, &ucValueFlag, 0, 65535, 1, 1, 0))
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }

    pCesqParam->usTimerVal = (unsigned short)ulBinaryData;
    return ucResult;
}

unsigned char ATC_CSQ_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 4;                                                              /* command length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CSQ,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_NOEQUAL_SET == ucCmdFunc && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
    {
        ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CGPADDR_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGPADDR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 8;                                                              /* +CGPADDR length                       */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucCidNum = 0;
    unsigned char ucValue;
                                                               
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGPADDR,pEventBuffer,&ucCmdFunc);

    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;

        while (D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
        {
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, &ucValue, NULL, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, ucCidNum == D_MAX_CNT_CID - 1 ? 1 : 0);            
            if(ucResult != D_ATC_COMMAND_OK)
            {
                return ucResult;
            }

            ((ST_ATC_CGPADDR_PARAMETER *)pEventBuffer)->aucCid[ucCidNum] = ucValue;
            ucCidNum++;
        }
        
        ((ST_ATC_CGPADDR_PARAMETER *)pEventBuffer)->ucAllCidFlg = D_ATC_FLAG_FALSE;
        ((ST_ATC_CGPADDR_PARAMETER *)pEventBuffer)->ucCidNum = ucCidNum;
        break;     
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        if (D_ATC_N_CR == *(pCommandBuffer + usCmdStrLen))
        {
            ((ST_ATC_CGPADDR_PARAMETER *)pEventBuffer)->ucAllCidFlg = D_ATC_FLAG_TRUE;
        }
        else
        {
            return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
        }
        break;
    case D_ATC_CMD_FUNC_READ:
        ((ST_ATC_CGPADDR_PARAMETER *)pEventBuffer)->ucAllCidFlg = D_ATC_FLAG_TRUE;
        break;
    default:
        break;
    }

    return ucResult;
}
#if 0
/*******************************************************************************
  MODULE    : ATC_CGMM_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGMM_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 0;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    usCmdStrLen = 5;                                                                    /* +CGMM length set                     */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);
    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_TEST:
        ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_CGMM_T;
        ucResult = D_ATC_COMMAND_OK;
        break;
    case D_ATC_CMD_FUNC_READ:                                                           /* no break                             */
    case D_ATC_CMD_FUNC_SET:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        if (D_ATC_N_CR == *(pCommandBuffer + usCmdStrLen))
        {
            ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_CGMM;
            ucResult = D_ATC_COMMAND_OK;
        }
        else
        {
            ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
        }
        break;
    default:
        break;
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CGDATA_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGDATA_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 0;                                                              /* command length                       */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucBinaryData;                                                                 /* binary data                          */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    unsigned char aucL2ptypeData[4] ={0};                                                       /* PDP type                             */
    unsigned char ucAnlParLength;
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */

    usCmdStrLen += 7;                                                                   /* +CGDATA length                       */
    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_TEST:
        ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_CGDATA_T;         /* 'TEST' form set                      */
        ucResult = D_ATC_COMMAND_OK;
        break;
    case D_ATC_CMD_FUNC_READ:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;
        ((ST_ATC_CGDATA_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_CGDATA;
        ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen),
            3, &ucParLen, &ucAnlParLength, aucL2ptypeData, &ucStopStrInf);
        switch(ucParResult)
        {
        case D_ATC_PARAM_OK:
            usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                              /* parameter length set                 */
            if ((0 == AtcAp_Strncmp((unsigned char *)aucL2ptypeData, (unsigned char *)"PPP", 4))
                || (0 == AtcAp_Strncmp((unsigned char *)aucL2ptypeData, (unsigned char *)"ppp", 4)))
            {
                ((ST_ATC_CGDATA_PARAMETER *)pEventBuffer)->ucL2p = D_L2P_TYPE_PPP;      /* 0:PPP                                */
                ((ST_ATC_CGDATA_PARAMETER *)pEventBuffer)->ucL2pFlag = D_ATC_FLAG_TRUE; /* PPP Existence                        */
                ucResult = D_ATC_COMMAND_OK;
            }
            else
            {
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;                               /* parameter error set                  */
            }
            break;
        case D_ATC_PARAM_EMPTY:
        case D_ATC_PARAM_ERROR:
            ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
            break;
        case D_ATC_PARAM_SYNTAX_ERROR:
            ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
            break;
        default:
            break;
        }
        if (D_ATC_COMMAND_OK == ucResult)
        {
            if (D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
            {
                usCmdStrLen += 1;
                ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3, 
                    &ucParLen, &ucBinaryData, &ucStopStrInf);
                switch(ucParResult)
                {
                case D_ATC_PARAM_OK:
                    usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
                    if ((ucBinaryData >= D_ATC_MIN_CID) && (ucBinaryData < D_ATC_MAX_CID))
                    {
                        ((ST_ATC_CGDATA_PARAMETER *)pEventBuffer)->ucCid = ucBinaryData;
                        ((ST_ATC_CGDATA_PARAMETER *)pEventBuffer)->ucCidFlag = D_ATC_FLAG_TRUE;
                        ucResult = D_ATC_COMMAND_OK;
                    }
                    else
                    {
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                    break;
                case D_ATC_PARAM_EMPTY:
                    ((ST_ATC_CGDATA_PARAMETER *)pEventBuffer)->ucCidFlag = D_ATC_FLAG_FALSE;
                    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                    break;
                case D_ATC_PARAM_SYNTAX_ERROR:
                    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                    break;
                default:
                    break;
                }
                if (D_ATC_COMMAND_OK == ucResult)
                {
                    if (D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
                    {
                        ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                    }
                }
            }
            else
            {
                ((ST_ATC_CGDATA_PARAMETER *)pEventBuffer)->ucCidFlag = D_ATC_FLAG_FALSE;
                ucResult = D_ATC_COMMAND_OK;
            }
        }
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ((ST_ATC_CGDATA_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_CGDATA;
        ucResult = D_ATC_COMMAND_OK;
        break;
    default:
        break;
    }

    return ucResult;
}
#endif

/*******************************************************************************
  MODULE    : ATC_CGACT_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGACT_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* +CGACT length                        */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGACT,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;                                                               /*  '=' set                             */

    pucValue     = &(((ST_ATC_CGACT_PARAMETER *)pEventBuffer)->ucState);
    pucValueFlag = &(((ST_ATC_CGACT_PARAMETER *)pEventBuffer)->ucStateFlag);
    ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 1, 1, 0);   
    if(ucResult != D_ATC_COMMAND_OK)
    {
        return ucResult;
    } 

    if(D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
    {
        pucValue     = &(((ST_ATC_CGACT_PARAMETER *)pEventBuffer)->ucCid );
        pucValueFlag = &(((ST_ATC_CGACT_PARAMETER *)pEventBuffer)->ucCidFlag);;
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 1);   
    }  
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CSODCP_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CSODCP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                             /* "+CSODCP" length                     */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    unsigned short usBinaryData;

    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;
    unsigned char  ucParamFlg = 0;
    unsigned char  ucValueFlag;
    ST_ATC_CSODCP_PARAMETER*   pCsodcpParam;
    unsigned short usCpdataLength;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CSODCP,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    } 
    usCmdStrLen += 1;                                                               /* '=' set                              */

    pCsodcpParam = (ST_ATC_CSODCP_PARAMETER*)pEventBuffer;
    for (; iParamNum < D_ATC_PARAM_MAX_P_CSODCP && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            pCsodcpParam->pucCpdata = NULL;

            pucValue     = &(pCsodcpParam->ucCid);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 0);
            break;
            
        case 1:
            ucParResult = ATC_CheckUSNumParameter((pCommandBuffer + usCmdStrLen), 5,
                &ucParLen, &usBinaryData, &ucStopStrInf);
            if(D_ATC_PARAM_OK == ucParResult)
            {
                usCmdStrLen += ucParLen;                                                /* parameter length set                 */
                if (EPS_CSODCP_CPDATA_LENGTH_MAX >= usBinaryData)
                {
                    pCsodcpParam->usCpdataLength = usBinaryData;
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;                           /* not normal command set               */
                }
            }
            else
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;                               /* not normal command set               */
            }
  
            if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }
            usCmdStrLen++;                                                          /* ',' set                              */
            break;

        case 2:
            if(0 != pCsodcpParam->usCpdataLength)
            {
                pCsodcpParam->pucCpdata = (unsigned char*)AtcAp_Malloc(pCsodcpParam->usCpdataLength);
            }
            if(D_ATC_PARAM_OK != ATC_GetHexStrParameter(pCommandBuffer, &usCmdStrLen, pCsodcpParam->usCpdataLength * 2, &usCpdataLength, pCsodcpParam->pucCpdata, 1, 0)
                || usCpdataLength != pCsodcpParam->usCpdataLength)
            {
                if(NULL != pCsodcpParam->pucCpdata)
                {
                    AtcAp_Free(pCsodcpParam->pucCpdata);
                }
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            break;      
        case 3:
            pucValue     = &(pCsodcpParam->ucRAI);
            pucValueFlag = &(pCsodcpParam->ucRAIFlag);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 2, 1, 0);  

            if(ucResult != D_ATC_COMMAND_OK && NULL != pCsodcpParam->pucCpdata)
            {
                AtcAp_Free(pCsodcpParam->pucCpdata);
            }        
            break;
            
        case 4:
            pucValue     = &(pCsodcpParam->ucTUserData);
            pucValueFlag = &(pCsodcpParam->ucTUserDataFlag);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 1, 1, 0);  

            if(ucResult != D_ATC_COMMAND_OK && NULL != pCsodcpParam->pucCpdata)
            {
                AtcAp_Free(pCsodcpParam->pucCpdata);
            }        
            break;        
        case 5:
            pucValue     = &(pCsodcpParam->ucSequence);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, &ucValueFlag, 1, 255, 1, 1);  

            if(ucResult != D_ATC_COMMAND_OK && NULL != pCsodcpParam->pucCpdata)
            {
                AtcAp_Free(pCsodcpParam->pucCpdata);
            }
            break;
        default:
            break;
        }

        if(ucResult != D_ATC_COMMAND_OK)
        {
            return ucResult;
        }
        ucParamFlg |= (1 << iParamNum);
    }

    if((ucParamFlg & 0x07) != 0x07)
    {
        return D_ATC_COMMAND_PARAMETER_ERROR;
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CRTDCP_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CRTDCP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                              /* +CRTDCP length                       */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CRTDCP,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /*  '=' set                             */
        pucValue     = &(((ST_ATC_CRTDCP_PARAMETER *)pEventBuffer)->ucReporting);
        pucValueFlag = &(((ST_ATC_CRTDCP_PARAMETER *)pEventBuffer)->ucReportingFlag);        
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 1, 1, 1);  
    } 
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CGAPNRC_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CGAPNRC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 8;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGAPNRC,pEventBuffer,&ucCmdFunc);
    if(ucCmdFunc == D_ATC_CMD_FUNC_SET)
    {
        usCmdStrLen += 1;        
        pucValue     = &(((ST_ATC_CGAPNRC_PARAMETER *)pEventBuffer)->ucCid);
        pucValueFlag = &(((ST_ATC_CGAPNRC_PARAMETER *)pEventBuffer)->ucCidFlag);        
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, D_ATC_MIN_CID, D_ATC_MAX_CID-1, 0, 1);          
    }
    else if(ucCmdFunc == D_ATC_CMD_FUNC_NOEQUAL_SET)
    {
        if (D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
        {
            return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
        }
    }     
    return ucResult;
}
#if 0
/*******************************************************************************
  MODULE    : ATC_CRC_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CRC_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 0;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucBinaryData;
    unsigned char ucStopStrInf;
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    usCmdStrLen = 4;                                                                    /* +CRC command length set              */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);
    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_TEST:
        ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_CRC_T;            /* TEST event                           */
        ucResult = D_ATC_COMMAND_OK;
        break;
    case D_ATC_CMD_FUNC_READ:
        ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_CRC_R;            /* READ event                           */
        ucResult = D_ATC_COMMAND_OK;
        break;
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;                                                               /* '=' set                              */
        ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3, 
            &ucParLen, &ucBinaryData, &ucStopStrInf);
        switch(ucParResult)
        {
        case D_ATC_PARAM_OK:
            usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_CRC;
            if (ucBinaryData <= 1)
            {
                ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue = ucBinaryData;
                ucResult = D_ATC_COMMAND_OK;
            }
            else
            {
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
            }
            break;
        case D_ATC_PARAM_EMPTY:
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_CRC;
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue = D_ATC_OMIT_VAL_P_CRC_MODE;
            ucResult = D_ATC_COMMAND_OK;
            break;
        case D_ATC_PARAM_SYNTAX_ERROR:
            ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
            break;
        default:
            break;
        }
        if (D_ATC_COMMAND_OK == ucResult)
        {
            if (D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
            {
                ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
        }
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    default:
        break;
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CMUX_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CMUX_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen;                                                                  /* command length                       */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucBinaryData;                                                                 /* binary data                          */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    signed int iParamNum;                                                                      /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned short usMaxFraSize;

    usCmdStrLen = 5;                                                                    /* +CMUX  length                        */
    iParamNum = D_ATC_PARAM_MAX_P_CMUX;
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);
    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_TEST:
        ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_CMUX_T;           /* 'TEST' set                           */
        ucResult = D_ATC_COMMAND_OK;
        break;
    case D_ATC_CMD_FUNC_READ:
        ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_CMUX_R;           /* 'READ' set                           */
        ucResult = D_ATC_COMMAND_OK;
        break;
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;                                                               /* '=' length                           */
        iParamNum = 0;
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    default:
        break;
    }

    for (; iParamNum < D_ATC_PARAM_MAX_P_CMUX; iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:                                                                         /* <mode> analysis                      */
            ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_CMUX;
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen += ucParLen;
                if ( 1 >= ucBinaryData)
                {
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMode = ucBinaryData;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum++;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucSubSetFlg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucPortSpeedFlg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxFraSizeFlg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer1Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxNumFlg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer2Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer3Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucWinSizeFlg = D_ATC_FLAG_FALSE;
                    ucResult = D_ATC_COMMAND_OK;
                    break;
                }
                else
                {
                    usCmdStrLen += 1;
                    continue;
                }
            }
            break;
        case 1:                                                                         /* <subset> analysis                    */
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen += ucParLen;
                if (2 >= ucBinaryData)
                {
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucSubSet = ucBinaryData;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucSubSetFlg = D_ATC_FLAG_TRUE;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                usCmdStrLen += ucParLen;
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucSubSet = 0;
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucSubSetFlg = D_ATC_FLAG_TRUE;
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum++;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucPortSpeedFlg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxFraSizeFlg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer1Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxNumFlg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer2Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer3Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucWinSizeFlg = D_ATC_FLAG_FALSE;
                    ucResult = D_ATC_COMMAND_OK;
                    break;
                }
                else
                {
                    usCmdStrLen += 1;
                    continue;
                }
            }
            break;
        case 2:                                                                         /* <port_speed> analysis                */
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen += ucParLen;
                if ((1 <= ucBinaryData) && (6 >= ucBinaryData))
                {
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucPortSpeed = ucBinaryData;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucPortSpeedFlg = D_ATC_FLAG_TRUE;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum++;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxFraSizeFlg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer1Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxNumFlg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer2Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer3Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucWinSizeFlg = D_ATC_FLAG_FALSE;
                    ucResult = D_ATC_COMMAND_OK;
                    break;
                }
                else
                {
                    usCmdStrLen += 1;
                    continue;
                }
            }
            break;
        case 3:                                                                         /* <N1> analysis                         */
            ucParResult = ATC_CheckUSNumParameter((pCommandBuffer + usCmdStrLen), 5,
                &ucParLen, &usMaxFraSize, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen += ucParLen;
                if ((usMaxFraSize >= 1) && (usMaxFraSize <= 32768))
                {
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->usMaxFraSize = usMaxFraSize;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxFraSizeFlg = D_ATC_FLAG_TRUE;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                usCmdStrLen += ucParLen;
                if (0 == ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMode)
                {
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->usMaxFraSize = 31;
                }
                else
                {
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->usMaxFraSize = 64;
                }
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxFraSizeFlg = D_ATC_FLAG_TRUE;
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum++;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer1Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxNumFlg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer2Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer3Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucWinSizeFlg = D_ATC_FLAG_FALSE;
                    ucResult = D_ATC_COMMAND_OK;
                    break;
                }
                else
                {
                    usCmdStrLen += 1;
                    continue;
                }
            }
            break;
        case 4:                                                                         /* <T1> analysis                        */
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen += ucParLen;
                if ((ucBinaryData >= 1) && ucBinaryData <= 255)                         /*lint !e685                            */
                {
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer1 = ucBinaryData;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer1Flg = D_ATC_FLAG_TRUE;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                usCmdStrLen += ucParLen;
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer1 = 10;
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer1Flg = D_ATC_FLAG_TRUE;
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum++;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxNumFlg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer2Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer3Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucWinSizeFlg = D_ATC_FLAG_FALSE;
                    ucResult = D_ATC_COMMAND_OK;
                    break;
                }
                else
                {
                    usCmdStrLen += 1;
                    continue;
                }
            }
            break;
        case 5:                                                                         /* <N2> analysis                        */
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen += ucParLen;
                if (ucBinaryData <= 100)
                {
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxNum = ucBinaryData;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxNumFlg = D_ATC_FLAG_TRUE;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                usCmdStrLen += ucParLen;
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxNum = 3;
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucMaxNumFlg = D_ATC_FLAG_TRUE;
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum++;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer2Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer3Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucWinSizeFlg = D_ATC_FLAG_FALSE;
                    ucResult = D_ATC_COMMAND_OK;
                    break;
                }
                else
                {
                    usCmdStrLen += 1;
                    continue;
                }
            }
            break;
        case 6:                                                                         /* <T2> analysis                        */
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen += ucParLen;
                if ((ucBinaryData >= 2) && (ucBinaryData <= 255))                       /*lint !e685                            */
                {
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer2 = ucBinaryData;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer2Flg = D_ATC_FLAG_TRUE;
                    if (((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer2 > 
                        ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer1)
                    {
                        ucResult = D_ATC_COMMAND_OK;
                    }
                    else
                    {
                        iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                usCmdStrLen += ucParLen;
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer2 = 30;
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer2Flg = D_ATC_FLAG_TRUE;
                if (((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer2 > 
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer1)
                {
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum++;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer3Flg = D_ATC_FLAG_FALSE;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucWinSizeFlg = D_ATC_FLAG_FALSE;
                    ucResult = D_ATC_COMMAND_OK;
                    break;
                }
                else
                {
                    usCmdStrLen += 1;
                    continue;
                }
            }
            break;
        case 7:                                                                         /* <T3> analysis                        */
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen += ucParLen;
                if ((ucBinaryData >= 1) && (ucBinaryData <= 255))                       /*lint !e685                            */
                {
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer3 = ucBinaryData;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer3Flg = D_ATC_FLAG_TRUE;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                usCmdStrLen += ucParLen;
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer3 = 10;
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucTimer3Flg = D_ATC_FLAG_TRUE;
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf != D_ATC_STOP_CHAR_KANMA)
                {
                    iParamNum++;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucWinSizeFlg = D_ATC_FLAG_FALSE;
                    ucResult = D_ATC_COMMAND_OK;
                    break;
                }
                else
                {
                    usCmdStrLen += 1;
                    continue;
                }
            }
            break;
        case 8:                                                                         /* <k> analysis                         */
            ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3,
                &ucParLen, &ucBinaryData, &ucStopStrInf);
            switch(ucParResult)
            {
            case D_ATC_PARAM_OK:
                usCmdStrLen += ucParLen;
                if ((ucBinaryData >= 1) && (ucBinaryData <= 7))
                {
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucWinSize = ucBinaryData;
                    ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucWinSizeFlg = D_ATC_FLAG_TRUE;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case D_ATC_PARAM_EMPTY:
                usCmdStrLen += ucParLen;
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucWinSize = 2;
                ((ST_ATC_CMUX_PARAMETER *)pEventBuffer)->ucWinSizeFlg = D_ATC_FLAG_TRUE;
                ucResult = D_ATC_COMMAND_OK;
                break;
            case D_ATC_PARAM_SYNTAX_ERROR:
                iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                break;
            default:
                break;
            }
            if (D_ATC_COMMAND_OK == ucResult)
            {
                if (ucStopStrInf == D_ATC_STOP_CHAR_KANMA)
                {
                    usCmdStrLen += 1;
                    iParamNum = D_ATC_PARAM_MAX_P_CMUX;
                    ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                }
                else if (D_ATC_STOP_CHAR_CR == ucStopStrInf)
                {
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
                }
            }
            break;
        default:
            break;
        }
        break;
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_S3_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_S3_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 0;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucBinaryData;
    unsigned char ucStopStrInf;
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    usCmdStrLen += 2;                                                                   /* command length set                   */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);
    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_TEST:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;                                          /* no 'TEST' form                       */
        break;
    case D_ATC_CMD_FUNC_READ:
        ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_S3_R;             /* command event set                    */
        ucResult = D_ATC_COMMAND_OK;                                                    /* normal command                       */
        break;
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;                                                               /* '=' set                              */
        ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3, 
            &ucParLen, &ucBinaryData, &ucStopStrInf);
        switch(ucParResult)
        {
        case D_ATC_PARAM_OK:
            usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_S3;           /* event set                            */
            if ((ucBinaryData < 32) && (g_AtcMng.ucBs != ucBinaryData) && (g_AtcMng.ucLf != ucBinaryData))
            {
                ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue = ucBinaryData;         /* parameter set                        */
                ucResult = D_ATC_COMMAND_OK;
            }
            else
            {
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
            }
            break;
        case D_ATC_PARAM_EMPTY:
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_S3;           /* event set                            */
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue = 13;                       /* parameter set                        */
            ucResult = D_ATC_COMMAND_OK;
            break;
        case D_ATC_PARAM_SYNTAX_ERROR:
            ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
            break;
        default:
            break;
        }
        if (D_ATC_COMMAND_OK == ucResult)
        {
            if (D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
            {
                ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
            else if (D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                ucResult = D_ATC_COMMAND_OK;
            }
            else
            {
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
            }
        }
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    default:
        break;
    }

    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_S4_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_S4_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 0;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucBinaryData;
    unsigned char ucStopStrInf;
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    usCmdStrLen += 2;                                                                   /* command length set                   */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);
    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_TEST:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;                                          /* no 'TEST' form                       */
        break;
    case D_ATC_CMD_FUNC_READ:
        ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_S4_R;             /* command event set                    */
        ucResult = D_ATC_COMMAND_OK;                                                    /* normal command                       */
        break;
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;                                                               /* '=' set                              */
        ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3, 
            &ucParLen, &ucBinaryData, &ucStopStrInf);
        switch(ucParResult)
        {
        case D_ATC_PARAM_OK:
            usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_S4;           /* event set                            */
            if ((ucBinaryData < 32) && (g_AtcMng.ucBs != ucBinaryData) && (D_ATC_DEFAULT_CR != ucBinaryData))
            {
                ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue = ucBinaryData;         /* parameter set                        */
                ucResult = D_ATC_COMMAND_OK;
            }
            else
            {
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
            }
            break;
        case D_ATC_PARAM_EMPTY:
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_S4;           /* event set                            */
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue = 10;                       /* parameter set                        */
            ucResult = D_ATC_COMMAND_OK;
            break;
        case D_ATC_PARAM_SYNTAX_ERROR:
            ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
            break;
        default:
            break;
        }
        if (D_ATC_COMMAND_OK == ucResult)
        {
            if (D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
            {
                ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
            else if (D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                ucResult = D_ATC_COMMAND_OK;
            }
            else
            {
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
            }
        }
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    default:
        break;
    }

    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_S5_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_S5_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 0;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucBinaryData;
    unsigned char ucStopStrInf;
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    usCmdStrLen += 2;                                                                   /* command length set                   */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);
    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_TEST:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;                                          /* no 'TEST' form                       */
        break;
    case D_ATC_CMD_FUNC_READ:
        ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = D_ATC_EVENT_S5_R;             /* command event set                    */
        ucResult = D_ATC_COMMAND_OK;                                                    /* normal command                       */
        break;
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;                                                               /* '=' set                              */
        ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3, 
            &ucParLen, &ucBinaryData, &ucStopStrInf);
        switch(ucParResult)
        {
        case D_ATC_PARAM_OK:
            usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_S5;           /* event set                            */
            if ((ucBinaryData < 32) && (D_ATC_DEFAULT_LF != ucBinaryData) && (D_ATC_DEFAULT_CR != ucBinaryData))
            {
                ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue = ucBinaryData;         /* parameter set                        */
                ucResult = D_ATC_COMMAND_OK;
            }
            else
            {
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
            }
            break;
        case D_ATC_PARAM_EMPTY:
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_S5;           /* event set                            */
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue = 8;                        /* parameter set                        */
            ucResult = D_ATC_COMMAND_OK;
            break;
        case D_ATC_PARAM_SYNTAX_ERROR:
            ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
            break;
        default:
            break;
        }
        if (D_ATC_COMMAND_OK == ucResult)
        {
            if (D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
            {
                ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
            else if (D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                ucResult = D_ATC_COMMAND_OK;
            }
            else
            {
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
            }
        }
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    default:
        break;
    }

    return ucResult;
}
#endif
/*******************************************************************************
  MODULE    : ATC_E_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_E_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 1;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char *pucValue;
    ((ST_ATC_CMD_COM_EVENT *)pEventBuffer)->usEvent = ATC_Event_Table[EVENT_E].ucFucNoQualSet;
    pucValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);  
    ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 1, 1, 1);         

    return ucResult;
}
#if 0
/*******************************************************************************
  MODULE    : ATC_V_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_V_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 0;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucBinaryData;
    unsigned char ucStopStrInf;
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    usCmdStrLen = 1;                                                                    /* V set                                */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);
    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_TEST:                                                           /* no 'TEST' form                       */
    case D_ATC_CMD_FUNC_READ:                                                           /* no 'READ' form                       */
    case D_ATC_CMD_FUNC_SET:                                                            /* no '=' form                          */
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;                                          /* syntax error                         */
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3, 
            &ucParLen, &ucBinaryData, &ucStopStrInf);
        switch(ucParResult)
        {
        case D_ATC_PARAM_OK:
            usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_V;            /* get 'SET' form                       */
            if (ucBinaryData <= 1)
            {
                ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue = ucBinaryData;
                ucResult = D_ATC_COMMAND_OK;
            }
            else
            {
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
            }
            break;
        case D_ATC_PARAM_EMPTY:
            usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_V;
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue = D_ATC_OMIT_VAL_P_V_VALUE;
            ucResult = D_ATC_COMMAND_OK;
            break;
        case D_ATC_PARAM_SYNTAX_ERROR:
            ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
            break;
        default:
            break;
        }
        if (D_ATC_COMMAND_OK == ucResult)
        {
            if (D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
            {
                ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
            else if (D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                ucResult = D_ATC_COMMAND_OK;
            }
            else
            {
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
            }
        }
        break;
    default:
        break;
    }
    return ucResult;
}


/*******************************************************************************
  MODULE    : ATC_F_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_F_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 0;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucBinaryData;
    unsigned char ucStopStrInf;
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    usCmdStrLen = 2;                                                                    /* &F set                               */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);
    switch(ucCmdFunc)
    {
    case D_ATC_CMD_FUNC_TEST:                                                          /* no 'TEST' form                        */
    case D_ATC_CMD_FUNC_READ:                                                          /* no 'READ' form                        */
    case D_ATC_CMD_FUNC_SET:                                                            /* no '=' form                          */
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;                                          /* syntax error                         */
        break;
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3, 
            &ucParLen, &ucBinaryData, &ucStopStrInf);
        switch(ucParResult)
        {
        case D_ATC_PARAM_OK:
            usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_F;            /* get 'SET' form                       */
            if (0 == ucBinaryData)
            {
                ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue = ucBinaryData;
                ucResult = D_ATC_COMMAND_OK;
            }
            else
            {
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
            }
            break;
        case D_ATC_PARAM_EMPTY:
            usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_F;
            ((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue = D_ATC_OMIT_VAL_P_F_VALUE;
            ucResult = D_ATC_COMMAND_OK;
            break;
        case D_ATC_PARAM_SYNTAX_ERROR:
            ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
            break;
        default:
            break;
        }
        if (D_ATC_COMMAND_OK == ucResult)
        {
            if (D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
            {
                ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
            }
            else if (D_ATC_STOP_CHAR_CR == ucStopStrInf)
            {
                ucResult = D_ATC_COMMAND_OK;
            }
            else
            {
                ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
            }
        }
        break;
    default:
        break;
    }
    return ucResult;
}
#endif
/*******************************************************************************
  MODULE    : ATC_SIMST_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_SIMST_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* ^SIMST command length set            */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_SIMST,pEventBuffer,&ucCmdFunc);

    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_CEDRXS_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/

unsigned char ATC_CEDRXS_PARAMETER_Analysis(unsigned short usCmdStrLen, unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    unsigned char auceDRXData[5];                                                               /* eDRX data                            */
    unsigned char ucAnlParLength;
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char ucCtr;                                                                        /* count                                */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;
    
    for (; iParamNum < D_ATC_PARAM_MAX_P_CEDRXS && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            pucValue     = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucMode);
            pucValueFlag = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucModeFlag);
            *pucValue     = 0;/*default value == 0  */
            *pucValueFlag = D_ATC_FLAG_TRUE;
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 3, 1, 0); 
            break;
            
        case 1:
            pucValue     = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucActType);
            pucValueFlag = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucActTypeFlg);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 5, 5, 0, 0);             
            break;
            
        case 2:
        case 3:
            if(2 == iParamNum)
            {
                pucValue     = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucEDRXValue);
                pucValueFlag = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucEDRXValueFlag);
            }
            else 
            {
                if( ((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->usEvent == D_ATC_EVENT_CEDRXS)
                {
                    return D_ATC_COMMAND_SYNTAX_ERROR;
                }
                pucValue     = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucPtwValue);
                pucValueFlag = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucPtwValueFlag);
                
            }
            
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                                                    4,
                                                    &ucParLen, &ucAnlParLength,
                                                    auceDRXData, &ucStopStrInf);
            if(D_ATC_PARAM_OK == ucParResult)
            {
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);
                if (ucAnlParLength != 4)
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                else
                {
                    *pucValueFlag = D_ATC_FLAG_TRUE;
                    for (ucCtr = 0; ucCtr < 4; ucCtr++)
                    {
                        if ((auceDRXData[ucCtr] == '0') || (auceDRXData[ucCtr] == '1'))
                        {
                            auceDRXData[ucCtr] = (unsigned char)(auceDRXData[ucCtr] - '0');
                        }
                        else
                        {
                            return D_ATC_COMMAND_PARAMETER_ERROR;
                        }
                        (*pucValue) |= auceDRXData[ucCtr] << (3 - ucCtr);
                    }                    
                }
            }
            else if(D_ATC_PARAM_ERROR == ucParResult)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            else if(D_ATC_PARAM_SYNTAX_ERROR == ucParResult)
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }       
       
            if (ucStopStrInf == D_ATC_STOP_CHAR_KANMA)
            {
                if(3 == iParamNum)
                {
                    return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                }                
            }
            else
            {
                return ucResult;
            }
            usCmdStrLen++;
            break;         

        default:
            break;
        }
        
        if(ucResult != D_ATC_COMMAND_OK)
        {
            return ucResult;
        }        
    }
    return ucResult;
}

unsigned char ATC_CEDRXS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                              /* "+CEDRXS" length                     */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CEDRXS,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    } 
    usCmdStrLen += 1;                                                               /* '=' set                              */
    
    return ATC_CEDRXS_PARAMETER_Analysis(usCmdStrLen, pCommandBuffer,pEventBuffer);
}

unsigned char ATC_QEDRXCFG_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 9;                                                              /* "+CEDRXCFG" length                     */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_QEDRXCFG,pEventBuffer,&ucCmdFunc);

    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    } 
    usCmdStrLen += 1;                                                               /* '=' set                              */
    
    return ATC_CEDRXS_PARAMETER_Analysis(usCmdStrLen, pCommandBuffer,pEventBuffer);
}

/*******************************************************************************
  MODULE    : ATC_CPSMS_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_CPSMS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* "+CPSMS" length                      */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    unsigned char aucReqPeriTAUData[9];                                                         /* Requested_Periodic-TAU data          */
    unsigned char ucAnlParLength;
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    unsigned char ucCtr;                                                                        /* count                                */
    unsigned char *pucValue;
    unsigned char *pucValueFlag;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CPSMS,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    } 
    usCmdStrLen += 1;                                                               /* '=' set                              */
    
    for (; iParamNum < D_ATC_PARAM_MAX_P_CPSMS && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); iParamNum++)
    {
        switch(iParamNum)
        {
        case 0:
            pucValue     = &(((ST_ATC_CPSMS_PARAMETER *)pEventBuffer)->ucMode);
            pucValueFlag = &(((ST_ATC_CEDRXS_PARAMETER *)pEventBuffer)->ucModeFlag);
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, pucValueFlag, 0, 2, 1, 0);              
            break;
            
        case 1:
        case 2:
            if(D_ATC_N_COMMA != *(pCommandBuffer + usCmdStrLen) && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            usCmdStrLen++;
            break;
        case 3:
        case 4:
            if(3 == iParamNum)
            {
                pucValue     = &(((ST_ATC_CPSMS_PARAMETER *)pEventBuffer)->ucReqPeriTAU);
                pucValueFlag = &(((ST_ATC_CPSMS_PARAMETER *)pEventBuffer)->ucReqPeriTAUFlg);
            }
            else 
            {
                pucValue     = &(((ST_ATC_CPSMS_PARAMETER *)pEventBuffer)->ucReqActTime);
                pucValueFlag = &(((ST_ATC_CPSMS_PARAMETER *)pEventBuffer)->ucReqActTimeFlag);
            }            
            ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 
                8,
                &ucParLen, &ucAnlParLength,
                aucReqPeriTAUData, &ucStopStrInf);
            if(D_ATC_PARAM_OK == ucParResult)
            {
                usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                
                if (ucAnlParLength != 8)
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                else
                {
                    *pucValueFlag = D_ATC_FLAG_TRUE;
                    for (ucCtr = 0; ucCtr < 8; ucCtr++)
                    {
                        if ((aucReqPeriTAUData[ucCtr] == '0') || (aucReqPeriTAUData[ucCtr] == '1'))
                        {
                            aucReqPeriTAUData[ucCtr] = (unsigned char)(aucReqPeriTAUData[ucCtr] - '0');
                        }
                        else
                        {
                            return D_ATC_COMMAND_PARAMETER_ERROR;
                        }
                        (*pucValue) |= aucReqPeriTAUData[ucCtr] << (7-ucCtr);
                    }
                }
            }
            else if(D_ATC_PARAM_ERROR == ucParResult)
            {
                return D_ATC_COMMAND_PARAMETER_ERROR;
            }
            else if(D_ATC_PARAM_SYNTAX_ERROR == ucParResult)
            {
                return D_ATC_COMMAND_SYNTAX_ERROR;
            }           

            if (ucStopStrInf == D_ATC_STOP_CHAR_KANMA)
            {
                if(4 == iParamNum)
                {
                    return D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                }                
            }
            else
            {
                return ucResult;
            }
            usCmdStrLen++;
            break;          
        default:
            break;
        }
        
        if(ucResult != D_ATC_COMMAND_OK)
        {
            return ucResult;
        }
    }
    return ucResult;
}
/*lint +e438*/

/*******************************************************************************
  MODULE    : ATC_CSCON_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  jiangna   2018.03.07   create
*******************************************************************************/
unsigned char ATC_CSCON_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* command length                       */
    unsigned char ucResult;                                                                     /* command analysis result              */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char *pucValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CSCON,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                              */
        pucValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 1, 1, 1);           
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_NL2THP_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
      1.  Dep2_066   2016.12.20   create
*******************************************************************************/
unsigned char ATC_NL2THP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 7;                                                              /* command length                       */
    unsigned char ucCmdFunc;                                                                   /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pucValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NL2THP,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' length                           */
        pucValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
        ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 1, 0, 1);          
    }
    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_NSET_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
*******************************************************************************/
unsigned char ATC_NSET_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 5;                                                              /* +NSET length                        */
    unsigned char ucParLen = 0;                                                                 /* parameter length                     */
    unsigned char ucStopStrInf;                                                                 /* stop string                          */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                         */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char ucParResult;                                                                  /* parameter result                     */
    ST_ATC_NSET_PARAMETER * pNsetParam = NULL;
    unsigned char  ucLastParamFlg;
    unsigned char  ucAllowZeroStartFlg = ATC_FALSE;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NSET,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    } 
    usCmdStrLen += 1;  
   
    pNsetParam = (ST_ATC_NSET_PARAMETER *)pEventBuffer;

    for (; iParamNum < D_ATC_PARAM_MAX_P_NSET; iParamNum++)
    {
        switch(iParamNum)
        {
            case 0:
                ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen),
                                 D_ATC_P_NSET_INS_SIZE_MAX - 1, &ucParLen, &(pNsetParam->ucInsLen), pNsetParam->aucInsValue, &ucStopStrInf);
                if(D_ATC_PARAM_OK == ucParResult)
                {
                    usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen);                          /* parameter length set                 */    
                    if(pNsetParam->ucInsLen != 0 && D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                        pNsetParam->usEvent = D_ATC_EVENT_NSET_R;
                    }                    
                }
                else if(D_ATC_PARAM_SYNTAX_ERROR == ucParResult)
                {
                    return D_ATC_COMMAND_SYNTAX_ERROR;
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }         

                usCmdStrLen += 1;
                if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                {
                    return ucParResult;
                }
                                                                      /* ',' set                              */                
                if(0 == AtcAp_Strncmp((unsigned char *)(pNsetParam->aucInsValue), (unsigned char *)"SETIMEI"))
                {
                    ucAllowZeroStartFlg = ATC_TRUE;
                }
                break;
                
            case 1: 
                if (0 == AtcAp_Strncmp((unsigned char *)(pNsetParam->aucInsValue), (unsigned char *)"SETSN")
                    || 0 == AtcAp_Strncmp((unsigned char *)(pNsetParam->aucInsValue), (unsigned char *)"SETSVN"))
                {
                    ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), NVM_MAX_SN_LEN, &ucParLen, &pNsetParam->ucLen, pNsetParam->aucData, &ucStopStrInf);
                    if(D_ATC_PARAM_OK == ucParResult && D_ATC_DEFAULT_CR == *(pCommandBuffer + usCmdStrLen + ucParLen))
                    {
                                         
                    }
                    else if(D_ATC_PARAM_SYNTAX_ERROR == ucParResult)
                    {
                        return D_ATC_COMMAND_SYNTAX_ERROR;
                    }
                    else
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                    return ucResult;                  
                }
                else
                {
                    if(0 == AtcAp_Strncmp((unsigned char *)(pNsetParam->aucInsValue), (unsigned char *)"SETIMEI")
                        || 0 == AtcAp_Strncmp((unsigned char *)(pNsetParam->aucInsValue), (unsigned char *)"ULGB"))
                    {
                        ucLastParamFlg = ATC_FALSE;
                    }
                    else
                    {
                        ucLastParamFlg = ATC_TRUE;
                    }
                    ucResult     = ATC_GetDecimalParameterLong(pCommandBuffer, &usCmdStrLen, 8, &(pNsetParam->ulParam1), &(pNsetParam->ucParam1Flg), 0, 0xFFFFFFFF, 0, ucLastParamFlg, ucAllowZeroStartFlg);
                    if(D_ATC_PARAM_OK != ucResult || ATC_TRUE == ucLastParamFlg)
                    {
                        return ucResult;
                    }
                }
                break;
            case 2:
                ucResult     = ATC_GetDecimalParameterLong(pCommandBuffer, &usCmdStrLen, 8, &(pNsetParam->ulParam2), &(pNsetParam->ucParam2Flg), 0, 0xFFFFFFFF, 0, 1, ucAllowZeroStartFlg);          
                if(ucResult != D_ATC_COMMAND_OK)
                {
                    return ucResult;
                }
                break;
            default:
                break;
        }
    }    
    return ucResult;
}



unsigned char ATC_NUESTATS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 9;                                                              /* command length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned short usParLen = 0;                                                                 /* parameter length                         */
    unsigned char ucParResult;                                                                  /* parameter result                         */
    unsigned char aucType[9] = {0};
    unsigned char i;
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NUESTATS,pEventBuffer,&ucCmdFunc);

    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ((ST_ATC_NUESTATS_PARAMETER*)pEventBuffer)->mu8Type = ATC_NUESTATS_TYPE_RADIO;
        break;
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;                                                               /* '=' set                                   */
        ucParResult = ATC_ChkStrPara((pCommandBuffer + usCmdStrLen), 8, &usParLen, aucType, &ucStopStrInf);
        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
        if((D_ATC_PARAM_OK == ucParResult) && (D_ATC_STOP_CHAR_CR == ucStopStrInf))
        {
            for (i = 0; i < ATC_NUESTATS_MAX; i++)
            {
                if (0 == AtcAp_Strncmp(aucType, (unsigned char*)ATC_NUESTATS_Table[i].aucStr))
                {
                    ((ST_ATC_NUESTATS_PARAMETER*)pEventBuffer)->mu8Type = ATC_NUESTATS_Table[i].ucStrVal;
                    ucResult = D_ATC_COMMAND_OK;
                    break;
                }
            }          
        }  
        break;
    default :
        break;
    }
    return ucResult;
}

unsigned char ATC_NEARFCN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 8;                                                              /* "+NEARFCN" length                        */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucParLen = 0;                                                                 /* parameter length                         */
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned char ucParResult;                                                                  /* parameter result                         */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                             */

    unsigned char u8Mode;
    unsigned long u32Earfcn;
    unsigned short u16Pci;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NEARFCN,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                                   */        
        for (; iParamNum < D_ATC_MAX_PARAM_NUM_3; iParamNum++)
        {
            switch(iParamNum)
            {
            case 0:
                ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 1, &ucParLen, &u8Mode, &ucStopStrInf);
                usCmdStrLen = usCmdStrLen + ucParLen;
                if((D_ATC_PARAM_OK == ucParResult) && (0 == u8Mode) && (D_ATC_STOP_CHAR_KANMA == ucStopStrInf))
                {
                    ((ST_ATC_NEARFCN_PARAMETER*)pEventBuffer)->mu8Mode = u8Mode;                    
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                usCmdStrLen+=1;
                break;
                
            case 1:
                ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 10, &ucParLen, &u32Earfcn, &ucStopStrInf, 0);
                usCmdStrLen = usCmdStrLen + ucParLen;
                if(D_ATC_PARAM_OK == ucParResult)
                {
                    ((ST_ATC_NEARFCN_PARAMETER*)pEventBuffer)->mu32Earfcn = u32Earfcn;

                    if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                        ((ST_ATC_NEARFCN_PARAMETER*)pEventBuffer)->mu16Pci = 0xFFFF;
                        return ucResult;
                    }
                    else if(D_ATC_STOP_CHAR_KANMA != ucStopStrInf)
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                usCmdStrLen+=1;
                break;
                
            case 2:
                ucParResult = ATC_CheckHexNumParameter((pCommandBuffer + usCmdStrLen), 3, &ucParLen, &u16Pci, &ucStopStrInf);

                if((D_ATC_PARAM_OK == ucParResult) && (u16Pci <= 0x1F7))
                {
                    if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                       ((ST_ATC_NEARFCN_PARAMETER*)pEventBuffer)->mu16Pci = u16Pci;
                    }
                    else
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            default:
                return D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            }
        }
    }
    return ucResult;
}

unsigned char ATC_NBAND_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* "+NBAND" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char i = 0;
    unsigned char *pucValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NBAND,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                                   */
        for(; i < 14 && D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen); i++)
        {
            pucValue     = &(((ST_ATC_NBAND_PARAMETER*)pEventBuffer)->mau8Band[i]);  
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pucValue, NULL, 0, 0xFF, 0, i == 13 ? 1 : 0);    
            if(ucResult != D_ATC_COMMAND_OK)
            {
                return ucResult;
            }            
        }    
        ((ST_ATC_NBAND_PARAMETER*)pEventBuffer)->mu8Num = i;    
    }
    return ucResult;
}

unsigned char ATC_QBAND_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* "+NBAND" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char i = 0;
    unsigned char *pucValue;
    unsigned char ucBandNum;
    ST_ATC_NBAND_PARAMETER*  stQbandparam =(ST_ATC_NBAND_PARAMETER*)pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NBAND,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                                   */
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, &ucBandNum, NULL, 0, 3, 0, 0);
        if(ucResult != D_ATC_COMMAND_OK)
        {
            return ucResult;
        }
        if(ucBandNum == 0 && *(pCommandBuffer + 8) != D_ATC_DEFAULT_CR)
        {
            return D_ATC_COMMAND_SYNTAX_ERROR;
        }
        
        for(; i < ucBandNum; i++)
        {
            pucValue     = &(stQbandparam->mau8Band[i]);  
            ucResult     = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pucValue, NULL, 0, 0xFF, 0, i == (ucBandNum-1) ? 1 : 0);    
            if(ucResult != D_ATC_COMMAND_OK)
            {
                return ucResult;
            }            
        }
        stQbandparam->mu8Num = i;
    }
    stQbandparam->ucQBandFlg = 1;
    return ucResult;
}

unsigned char ATC_NCONFIG_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 8;                                                              /* "+NCONFIG" length                        */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned char ucParLen = 0;                                                                 /* parameter length                         */
    unsigned char ucStrLen = 0; 
    unsigned char ucParResult;                                                                  /* parameter result                         */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                             */
    unsigned char aucFunc[40]  = { 0 };
    unsigned long ulVal;
    unsigned char index;
#if VER_QUECTEL
    unsigned char aucValue[6] = { 0 };
    unsigned char i;
#endif

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NCONFIG,pEventBuffer,&ucCmdFunc);
    
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' set                                  */
        for (; iParamNum < D_ATC_MAX_PARAM_NUM_2; iParamNum++)
        {
            switch(iParamNum)
            {
            case 0:
                ucParResult = ATC_CheckStrParameter((pCommandBuffer + usCmdStrLen), 39, &ucParLen, &ucStrLen, aucFunc, &ucStopStrInf);
                if((D_ATC_PARAM_OK == ucParResult) && (D_ATC_STOP_CHAR_KANMA == ucStopStrInf))
                {
                    for(index = 0; index < D_ATC_NCONFIG_MAX; index++)
                    {
                        if(0 == strcmp((char*)aucFunc, (char*)ATC_NConfig_Table[index].aucStr))
                        {
                            ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu8Func = ATC_NConfig_Table[index].ucStrVal;
                            break;
                        }
                    }
                
                    if(D_ATC_NCONFIG_MAX == index)
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                    
                    usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen + 1);
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;               
            case 1:
#if VER_QUECTEL
                if(ATC_AP_TRUE == ATC_NCONFIG_SET_IsStrChk(ATC_NConfig_Table[index].ucStrVal))
                {
                    if(D_ATC_COMMAND_OK != ATC_GetStrParameter(pCommandBuffer, &usCmdStrLen, 5, &ucParLen, aucValue, 0, 1))
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }

                    for(i = 0; i < ucParLen; i++)
                    {
                        aucValue[i] = toupper(aucValue[i]);
                    }

                    if(ATC_NConfig_Table[index].ucStrVal == D_ATC_NCONFIG_PCO_IE_TYPE)
                    {
                        if(0 == strcmp(aucValue, "EPCO"))
                        {
                            ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu16Val = 1;
                        }
                        else if(0 == strcmp(aucValue, "PCO"))
                        {
                            ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu16Val = 0;
                        }
                         else
                        {
                            return D_ATC_COMMAND_PARAMETER_ERROR;
                        }
                    }
                    else
                    {
                        if(0 == strcmp(aucValue, "TRUE"))
                        {
                            ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu16Val = 1;
                        }
                        else if(0 == strcmp(aucValue, "FALSE"))
                        {
                            ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu16Val = 0;
                        }
                        else
                        {
                            return D_ATC_COMMAND_PARAMETER_ERROR;
                        }
                    }
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 5, &ucParLen, &ulVal, &ucStopStrInf, 0);
                    if((D_ATC_PARAM_OK == ucParResult) && (D_ATC_STOP_CHAR_CR== ucStopStrInf) && ulVal <= 65535)
                    {
                        ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu16Val = (unsigned short)ulVal;
                    }
                    else
                    {
                        return D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                break;
#else
                ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 5, &ucParLen, &ulVal, &ucStopStrInf, 0);
                if((D_ATC_PARAM_OK == ucParResult) && (D_ATC_STOP_CHAR_CR== ucStopStrInf) && ulVal <= 65535)
                {
                    ((ST_ATC_NCONFIG_PARAMETER*)pEventBuffer)->mu16Val = (unsigned short)ulVal;
                }
                else
                {
                    return D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
#endif
            default:
                return D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            }
        }            
    }
    return ucResult;
}

unsigned char ATC_NCCID_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* "+NCCID" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NCCID,pEventBuffer,&ucCmdFunc);

    return ucResult;
}

unsigned char ATC_QCCID_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 6;                                                              /* "+QCCID" length                          */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NCCID,pEventBuffer,&ucCmdFunc);

    if((ucCmdFunc == D_ATC_CMD_FUNC_NOEQUAL_SET) && *(pCommandBuffer + 6) == D_ATC_DEFAULT_CR)
    {
        g_AtcApInfo.ucQccidFlg = 1;
        return ucResult;
    }
    
    return D_ATC_COMMAND_PARAMETER_ERROR;
}

#if 0

unsigned char ATC_NPSMR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen	= 0;                                                                  /* command length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
	unsigned char ucParLen;
	unsigned char ucStopStrInf;
	unsigned long ulPSMRTimerLength;
	unsigned char ucParResult;
    
    usCmdStrLen = 6;                                                                    /* "+NPSMR" length                            */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);

    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ((ST_ATC_CMD_COM_EVENT*)pEventBuffer)->usEvent = D_ATC_EVENT_NPSMR;
        ucResult = D_ATC_COMMAND_OK;
        break;
	case D_ATC_CMD_FUNC_SET:
		usCmdStrLen += 1;
		((ST_ATC_PSMR_PARAMETER *)pEventBuffer)->usEvent = D_ATC_EVENT_NPSMR;
		ucParResult = ATC_CheckLongNumParameter((pCommandBuffer + usCmdStrLen), 10, &ucParLen, &ulPSMRTimerLength, &ucStopStrInf);
		if( ATC_OK != ucParResult)
		{
			ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
		}
		else
		{
			ucResult = D_ATC_COMMAND_OK;
		}

		((ST_ATC_PSMR_PARAMETER *)pEventBuffer)->ulPSMRTimer	= ulPSMRTimerLength;

		// here can invoke the API to enter deep sleep

//		OutputTraceMessage((7 << 8 | 0x00000001),"Enter Into PSM: 0x%x", ulPSMRTimerLength);

		HWREG(0x50060000) = 0x55AA55AA;
		HWREG(0x50060004) = ulPSMRTimerLength;
		HWREG(0xA011001C) |= 0x01;
		__asm__ ("waiti 0");

		break;
				
			
    default :
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    }

    return ucResult;
}
unsigned char ATC_NMGS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen;                                                                  /* command length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    usCmdStrLen = 4;                                                                    /* "+NRB" length                            */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);

    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ((ST_ATC_CMD_COM_EVENT*)pEventBuffer)->usEvent = D_ATC_EVENT_NMGS;
        ucResult = D_ATC_COMMAND_OK;
        break;
    default :
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    }

    return ucResult;
}

unsigned char ATC_NMGR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen;                                                                  /* command length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    usCmdStrLen = 4;                                                                    /* "+NRB" length                            */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);

    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ((ST_ATC_CMD_COM_EVENT*)pEventBuffer)->usEvent = D_ATC_EVENT_NMGR;
        ucResult = D_ATC_COMMAND_OK;
        break;
    default :
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    }

    return ucResult;
}

unsigned char ATC_NQMGS_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen;                                                                  /* command length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    usCmdStrLen = 4;                                                                    /* "+NRB" length                            */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);

    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ((ST_ATC_CMD_COM_EVENT*)pEventBuffer)->usEvent = D_ATC_EVENT_NQMGS;
        ucResult = D_ATC_COMMAND_OK;
        break;
    default :
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    }

    return ucResult;
}

unsigned char ATC_NQMGR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen;                                                                  /* command length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    usCmdStrLen = 4;                                                                    /* "+NRB" length                            */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);

    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ((ST_ATC_CMD_COM_EVENT*)pEventBuffer)->usEvent = D_ATC_EVENT_NQMGR;
        ucResult = D_ATC_COMMAND_OK;
        break;
    default :
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    }

    return ucResult;
}

unsigned char ATC_QLWUDATAEX_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen;                                                                  /* command length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    usCmdStrLen = 4;                                                                    /* "+NRB" length                            */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);

    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ((ST_ATC_CMD_COM_EVENT*)pEventBuffer)->usEvent = D_ATC_EVENT_QLWUDATAEX;
        ucResult = D_ATC_COMMAND_OK;
        break;
    default :
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    }

    return ucResult;
}

unsigned char ATC_NCDP_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen;                                                                  /* command length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    usCmdStrLen = 4;                                                                    /* "+NRB" length                            */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);

    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ((ST_ATC_CMD_COM_EVENT*)pEventBuffer)->usEvent = D_ATC_EVENT_NCDP;
        ucResult = D_ATC_COMMAND_OK;
        break;
    default :
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    }

    return ucResult;
}
#endif

unsigned char ATC_NCSEARFCN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 10;                                                             /* "+NCSEARFCN" length                      */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NCSEARFCN,pEventBuffer,&ucCmdFunc);

    return ucResult;
}

unsigned char ATC_QCSEARFCN_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 10;                                                             /* "+QCSEARFCN" length                      */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_NCSEARFCN,pEventBuffer,&ucCmdFunc);

    return ucResult;
}

/*******************************************************************************
  MODULE    : ATC_RAI_LNB_Command
  FUNCTION  : 
  NOTE      :
  HISTORY   :
*******************************************************************************/
unsigned char ATC_RAI_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen = 4;                                                              /* +RAI length                        */
    unsigned char ucCmdFunc;                                                                    /* command form                         */
    unsigned char ucResult;                                                                     /* command result                       */
    unsigned char *pucValue;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_RAI,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET == ucCmdFunc)
    {
        usCmdStrLen += 1;                                                               /* '=' length                           */
        pucValue     = &(((ST_ATC_CMD_PARAMETER *)pEventBuffer)->ucValue);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 3, pucValue, NULL, 0, 0xFF, 0, 1);         
    }   
    return ucResult;
}

#if 0
unsigned char ATC_NATSPEED_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short usCmdStrLen;                                                                  /* command length                           */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    
    usCmdStrLen = 4;                                                                    /* "+NRB" length                            */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);

    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_NOEQUAL_SET:
        ((ST_ATC_CMD_COM_EVENT*)pEventBuffer)->usEvent = D_ATC_EVENT_NATSPEED;
        ucResult = D_ATC_COMMAND_OK;
        break;
    default :
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    }

    return ucResult;
}
#endif

#if 0
unsigned char ATC_NSOCR_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    ST_ATC_SOCKET_INFO_STRU* pstMsg = (ST_ATC_SOCKET_INFO_STRU*)pEventBuffer;
    unsigned short usCmdStrLen;                                                                  /* command length                           */
    unsigned char ucParLen = 0;                                                                 /* parameter length                         */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                             */
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucParResult;                                                                  /* parameter result                         */

    unsigned char aucSocType[10] = {0};     /* support type is DGRAM */
    unsigned char ucPrtcl = 0;
    unsigned char ucRxFlg = 0;
    unsigned short usPortNum = 0;
    
    usCmdStrLen = 6;                                                                    /* "+NSOCR" length                          */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);
    
    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;                                                               /* '=' set                                   */
        pstMsg->usEvent = D_ATC_EVENT_NSOCR;
        for (; iParamNum < D_ATC_PARAM_MAX_NSOCR; iParamNum++)
        {
            switch(iParamNum)
            {
            case 0:
                ucParResult = ATC_ChkStrPara((pCommandBuffer + usCmdStrLen), 6, &ucParLen, aucSocType, &ucStopStrInf);
                if((D_ATC_PARAM_OK == ucParResult) && (0 == AtcAp_Strncmp(aucSocType, (unsigned char*)"DGRAM", 5)) && (D_ATC_STOP_CHAR_KANMA == ucStopStrInf))
                {
                    usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen + 1);
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_NSOCR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case 1:
                ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3, &ucParLen, &ucPrtcl, &ucStopStrInf);
                if((D_ATC_PARAM_OK == ucParResult) && (17 == ucPrtcl) && (D_ATC_STOP_CHAR_KANMA == ucStopStrInf))
                {
                    pstMsg->mu8Protocol = ucPrtcl;
                    usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen + 1);
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_NSOCR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case 2:
                ucParResult = ATC_CheckUSNumParameter((pCommandBuffer + usCmdStrLen), 5, &ucParLen, &usPortNum, &ucStopStrInf);
                if(D_ATC_PARAM_OK == ucParResult)
                {
                    if(D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
                    {
                        pstMsg->mu16Port = usPortNum;
                        usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen + 1);
                    }
                    else if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                        pstMsg->mu16Port = usPortNum;
                        pstMsg->mu8RxFlg = 1;
                        iParamNum = D_ATC_PARAM_MAX_NSOCR;
                    }
                    else
                    {
                        iParamNum = D_ATC_PARAM_MAX_NSOCR;
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_NSOCR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case 3:
                ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3, &ucParLen, &ucRxFlg, &ucStopStrInf);
                if((D_ATC_PARAM_OK == ucParResult) && ((1 == ucRxFlg) || (0 == ucRxFlg)))
                {
                    if(D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
                    {
                        ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                    }
                    else if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                        pstMsg->mu8RxFlg = ucRxFlg;
                    }
                    else
                    {
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_NSOCR;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            default:
                iParamNum = D_ATC_PARAM_MAX_NSOCR;
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            }
        }
        break;
    default :
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    }

    return ucResult;

}

unsigned char ATC_NSOST_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    ST_ATC_UDP_DATA_STRU* pstMsg = (ST_ATC_UDP_DATA_STRU*)pEventBuffer;
    unsigned short usCmdStrLen;                                                                 /* command length                           */
    unsigned char ucParLen = 0;                                                                 /* parameter length                         */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                             */
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucParResult;                                                                  /* parameter result                         */

    unsigned char ucSocNum = 0;     /* the socket number AT+NSOCR returned */
    unsigned char aucIpAddrChar[16] = {0};
    unsigned short usLen = 0;
    unsigned short usDstPort = 0;
    unsigned short usParLen = 0;
    unsigned short usTmpLen = 0;
    unsigned char aucTmpData[3000] = {0};
    unsigned char i;
    
    usCmdStrLen = 6;                                                                    /* "+NSOST" length                          */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);
    
    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;                                                               /* '=' set                                   */
        pstMsg->usEvent = D_ATC_EVENT_NSOST;
        for (; iParamNum < D_ATC_PARAM_MAX_NSOST; iParamNum++)
        {
            switch(iParamNum)
            {
            case 0:
                ucParResult = ATC_CheckNumParameter((pCommandBuffer + usCmdStrLen), 3, &ucParLen, &ucSocNum, &ucStopStrInf);
                if((D_ATC_PARAM_OK == ucParResult) && (D_ATC_STOP_CHAR_KANMA == ucStopStrInf))
                {
                    usCmdStrLen = usCmdStrLen + ucParLen + 1;
                    ucResult = D_ATC_COMMAND_OK;
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_NSOST;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case 1:
                ucParResult = ATC_ChkStrPara((pCommandBuffer + usCmdStrLen), 16, &ucParLen, aucIpAddrChar, &ucStopStrInf);

                if((D_ATC_PARAM_OK == ucParResult) && (D_ATC_STOP_CHAR_KANMA == ucStopStrInf))
                {
                    if(D_ATC_PARAM_OK != ATC_ChkIpv4Addr(ucParLen, aucIpAddrChar, pstMsg->mau8DstAddr))
                    {
                        iParamNum = D_ATC_PARAM_MAX_NSOST;
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;

                    }
                    else
                    {
                        usCmdStrLen = usCmdStrLen + ucParLen + 1;
                    }
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_NSOST;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case 2:
                ucParResult = ATC_CheckUSNumParameter((pCommandBuffer + usCmdStrLen), 5, &ucParLen, &usDstPort, &ucStopStrInf);
                if((D_ATC_PARAM_OK == ucParResult) && (D_ATC_STOP_CHAR_KANMA == ucStopStrInf))
                {
                    pstMsg->mu16DstPort = usDstPort;
                    usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen + 1);
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_NSOST;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case 3:
                ucParResult = ATC_CheckUSNumParameter((pCommandBuffer + usCmdStrLen), 4, &ucParLen, &usLen, &ucStopStrInf);
                if((D_ATC_PARAM_OK == ucParResult) && (D_ATC_STOP_CHAR_KANMA == ucStopStrInf))
                {
                    pstMsg->mu16Len = usLen;
                    usCmdStrLen = (unsigned char)(usCmdStrLen + ucParLen + 1);
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_NSOST;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case 4:
                pstMsg->mpu8Data = (unsigned char*)ATC_Malloc(usLen);
                ucParResult = ATC_PacketDataChkStrAndTrans((pCommandBuffer + usCmdStrLen), usLen, pstMsg->mpu8Data, &ucStopStrInf);

                if(D_ATC_PARAM_OK == ucParResult)
                {
                    if(D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
                    {
                        ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                    }
                    else if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {

                    }
                    else
                    {
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                else
                {
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            default:
                iParamNum = D_ATC_PARAM_MAX_NSOCR;
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            }
        }
        break;
    default :
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    }

    return ucResult;

}

unsigned char ATC_NPING_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    ST_ATC_PING_DATA_STRU* pstMsg = (ST_ATC_PING_DATA_STRU*)pEventBuffer;
    unsigned short usCmdStrLen;                                                                  /* command length                           */
    unsigned char ucParLen = 0;                                                                 /* parameter length                         */
    signed int iParamNum = 0;                                                                  /* PARAMETER NO                             */
    unsigned char ucStopStrInf;                                                                 /* stop string                              */
    unsigned char ucCmdFunc;                                                                    /* command form                             */
    unsigned char ucResult;                                                                     /* command result                           */
    unsigned char ucParResult;                                                                  /* parameter result                         */

    unsigned char aucIpAddrChar[16] = {0};
    unsigned short usLen = 0;
    unsigned short usTimeOut = 0;
    unsigned char i = 0;
    
    usCmdStrLen = 6;                                                                    /* "+NPING" length                          */
    ucResult = D_ATC_COMMAND_SYNTAX_ERROR;

    ucCmdFunc = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen);

    switch(ucCmdFunc) 
    {
    case D_ATC_CMD_FUNC_SET:
        usCmdStrLen += 1;
        pstMsg->usEvent = D_ATC_EVENT_NPING;
        for (; iParamNum < D_ATC_PARAM_MAX_NPING; iParamNum++)
        {
            switch(iParamNum)
            {
            case 0:
                ucParResult = ATC_ChkStrPara((pCommandBuffer + usCmdStrLen), 16, &ucParLen, aucIpAddrChar, &ucStopStrInf);

                if(D_ATC_PARAM_OK == ucParResult)
                {
                    ucResult = D_ATC_COMMAND_OK;

                    if(D_ATC_PARAM_OK == ATC_ChkIpv4Addr(ucParLen, aucIpAddrChar, pstMsg->mu8DstAddr))
                    {
                        if(D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
                        {
                            usCmdStrLen = usCmdStrLen + ucParLen + 1;
                        }
                        else if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                        {
                            iParamNum = D_ATC_PARAM_MAX_NPING;
                            pstMsg->mu16Len = 8;
                            pstMsg->mu16TimeOut = 10000;
                        }
                        else
                        {
                            iParamNum = D_ATC_PARAM_MAX_NPING;
                            ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                        }
                    }
                    else
                    {
                        iParamNum = D_ATC_PARAM_MAX_NPING;
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_NPING;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case 1:
                ucParResult = ATC_CheckUSNumParameter((pCommandBuffer + usCmdStrLen), 4, &ucParLen, &usLen, &ucStopStrInf);
                if(D_ATC_PARAM_OK == ucParResult)
                {
                    pstMsg->mu16Len = usLen;
                    if(D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
                    {
                        usCmdStrLen = usCmdStrLen + ucParLen + 1;
                    }
                    else if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                        iParamNum = D_ATC_PARAM_MAX_NPING;
                        pstMsg->mu16TimeOut = 10000;
                    }
                    else
                    {
                        iParamNum = D_ATC_PARAM_MAX_NPING;
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                else
                {
                    iParamNum = D_ATC_PARAM_MAX_NPING;
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            case 2:
                ucParResult = ATC_CheckUSNumParameter((pCommandBuffer + usCmdStrLen), 4, &ucParLen, &usTimeOut, &ucStopStrInf);
                if(D_ATC_PARAM_OK == ucParResult)
                {
                    pstMsg->mu16Len = usLen;
                    if(D_ATC_STOP_CHAR_KANMA == ucStopStrInf)
                    {
                        ucResult = D_ATC_COMMAND_TOO_MANY_PARAMETERS;
                    }
                    else if(D_ATC_STOP_CHAR_CR == ucStopStrInf)
                    {
                        pstMsg->mu16TimeOut = usTimeOut;
                    }
                    else
                    {
                        ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                    }
                }
                else
                {
                    ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                }
                break;
            default:
                iParamNum = D_ATC_PARAM_MAX_NPING;
                ucResult = D_ATC_COMMAND_PARAMETER_ERROR;
                break;
            }
        }
        break;
    default:
        ucResult = D_ATC_COMMAND_SYNTAX_ERROR;
        break;
    }

    return ucResult;
}
#endif

unsigned char ATC_CGPIAF_LNB_Command(unsigned char *pCommandBuffer, unsigned char *pEventBuffer)
{
    unsigned short   usCmdStrLen = 7;                                                               /* command length                       */
    unsigned char   ucResult;                                                                     /* command analysis result              */
    unsigned char   ucCmdFunc;                                                                    /* command form                         */
    unsigned char*   pucValue;
    unsigned char*   pucValueFlag;
    ST_ATC_CGPIAF_PARAMETER*   pCgpiafParam = (ST_ATC_CGPIAF_PARAMETER*) pEventBuffer;

    ucResult = ATC_CmdFuncInf(pCommandBuffer + usCmdStrLen,EVENT_CGPIAF,pEventBuffer,&ucCmdFunc);
    if(D_ATC_CMD_FUNC_SET != ucCmdFunc)
    {
        return ucResult;
    }
    usCmdStrLen += 1;

    if(D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen))
    {
        pucValue = &(pCgpiafParam->ucIpv6AddressFormat);
        pucValueFlag = &(pCgpiafParam->ucIpv6AddressFormatFlag);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pucValue, pucValueFlag, 0, 1, 0, 0);
    }
    if(D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen) && ucResult == D_ATC_COMMAND_OK)
    {
        pucValue = &(pCgpiafParam->ucIpv6SubnetNotation);
        pucValueFlag = &(pCgpiafParam->ucIpv6SubnetNotationFlag);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pucValue, pucValueFlag, 0, 1, 0, 0);
    }
    if(D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen) && ucResult == D_ATC_COMMAND_OK)
    {
        pucValue = &(pCgpiafParam->ucIpv6LeadingZeros);
        pucValueFlag = &(pCgpiafParam->ucIpv6LeadingZerosFlag);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pucValue, pucValueFlag, 0, 1, 0, 0);
    }
    if(D_ATC_N_CR != *(pCommandBuffer + usCmdStrLen) && ucResult == D_ATC_COMMAND_OK)
    {
        pucValue = &(pCgpiafParam->ucIpv6CompressZeros);
        pucValueFlag = &(pCgpiafParam->ucIpv6CompressZerosFlag);
        ucResult = ATC_GetDecimalParameterByte(pCommandBuffer, &usCmdStrLen, 2, pucValue, pucValueFlag, 0, 1, 0, 1);
    }
    
    return ucResult;
}

