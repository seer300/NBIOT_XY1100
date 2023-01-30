#include "atc_ps.h"
#include "oss_nv.h"

extern int g_NITZ_mode;
const ST_ATC_AP_STR_TABLE ATC_NConfig_Table[D_ATC_NCONFIG_MAX] = 
{
    {   D_ATC_NCONFIG_AUTOCONNECT,                      "AUTOCONNECT"                   },
    {   D_ATC_NCONFIG_COMBINE_ATTACH,                   "COMBINE_ATTACH"                },
    {   D_ATC_NCONFIG_CELL_RESELECTION,                 "CELL_RESELECTION"              },
    {   D_ATC_NCONFIG_ENABLE_BIP,                       "ENABLE_BIP"                    },
    {   D_ATC_NCONFIG_MULTITONE,                        "MULTITONE"                     },
    {   D_ATC_NCONFIG_BARRING_RELEASE_DELAY,            "BARRING_RELEASE_DELAY"         },
    {   D_ATC_NCONFIG_RELEASE_VERSION,                  "RELEASE_VERSION"               },
    {   D_ATC_NCONFIG_SYNC_TIME_PERIOD,                 "SYNC_TIME_PERIOD"              },
    {   D_ATC_NCONFIG_PCO_IE_TYPE,                      "PCO_IE_TYPE"                   },
    {   D_ATC_NCONFIG_NON_IP_NO_SMS_ENABLE,             "NON_IP_NO_SMS_ENABLE"          },
    {   D_ATC_NCONFIG_T3324_T3412_EXT_CHANGE_REPORT,    "T3324_T3412_EXT_CHANGE_REPORT" },
    {   D_ATC_NCONFIG_IPV6_GET_PREFIX_TIME,             "IPV6_GET_PREFIX_TIME"          },
#if VER_QUECTEL
    {   D_ATC_NCONFIG_NAS_SIM_POWER_SAVING_ENABLE,      "NAS_SIM_POWER_SAVING_ENABLE"   },
    {   D_ATC_NCONFIG_CR_0354_0338_SCRAMBLING,          "CR_0354_0338_SCRAMBLING"       },
    {   D_ATC_NCONFIG_CR_0859_SI_AVOID,                 "CR_0859_SI_AVOID"              },
#endif
};

const ST_ATC_AP_STR_TABLE ATC_ECURC_Table[D_ATC_ECURC_MAX] = 
{
    {D_ATC_ECURC_ALL,                       "ALL"       },
    {D_ATC_ECURC_CEREG,                     "CEREG"     },
    {D_ATC_ECURC_CEDRXP,                    "CEDRXP"    },
    {D_ATC_ECURC_CCIOTOPTI,                 "CCIOTOPTI" },
    {D_ATC_ECURC_CSCON,                     "CSCON"     },
    {D_ATC_ECURC_CTZEU,                     "CTZEU"     },
    {D_ATC_ECURC_CGEV,                      "CGEV"      },
};
/*******************************************************************************
  MODULE    : AtcAp_CGSN_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.  Du Zuchuang   2017.01.17   create
*******************************************************************************/
unsigned char AtcAp_CGSN_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CGSN:(0-3)\r\n");

    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CEREG_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   q   2017.01.04   create
*******************************************************************************/
unsigned char AtcAp_CEREG_T_LNB_Process(unsigned char *pEventBuffer)
{
    unsigned char ucPsmEnableFlg = ATC_AP_TRUE;

    if (ucPsmEnableFlg)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
            (const unsigned char *)"\r\n+CEREG: (0,1,2,3,4,5)\r\n");
    } 
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
            (const unsigned char *)"\r\n+CEREG: (0,1,2,3)\r\n");
    }

    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}


/*******************************************************************************
  MODULE    : ATC_CIMI_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Dep2_053   2008.05.21   create
*******************************************************************************/
unsigned char AtcAp_CIMI_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CGATT_T_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Dep2_066   2008.09.02   create
*******************************************************************************/
unsigned char AtcAp_CGATT_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
        (const unsigned char *)"\r\n+CGATT: (0,1)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CGDCONT_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   duzuchuang   2017.01.13   create
*******************************************************************************/
unsigned char AtcAp_CGDCONT_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CGDCONT: (0-10),(%cIP%c,%cIPV6%c,%cIPV4V6%c,%cNon-IP%c),,,(0),(0,1),,,,,(0,1),(0,1)\r\n",
        D_ATC_N_QUOTATION,D_ATC_N_QUOTATION,D_ATC_N_QUOTATION,D_ATC_N_QUOTATION,
        D_ATC_N_QUOTATION,D_ATC_N_QUOTATION,D_ATC_N_QUOTATION,D_ATC_N_QUOTATION);

    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CFUN_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Du Zuchuang   2016.12.21   create
*******************************************************************************/
unsigned char AtcAp_CFUN_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CFUN: (0,1,4),(0)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CMEE_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Dep2_053   2008.05.21   create
*******************************************************************************/
unsigned char AtcAp_CMEE_LNB_Process(unsigned char *pEventBuffer)
{
    ATC_MSG_CMEE_CNF_STRU* pCmeeCnf = (ATC_MSG_CMEE_CNF_STRU*)pEventBuffer;

    if(pCmeeCnf->ucErrMod != g_softap_fac_nv->cmee_mode)
    {
        g_softap_fac_nv->cmee_mode = pCmeeCnf->ucErrMod;
        SAVE_FAC_PARAM(cmee_mode);
    }

    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CMEE_R_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Dep2_053   2008.05.21   create
*******************************************************************************/
unsigned char AtcAp_CMEE_R_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf((const unsigned char *)"\r\n+CMEE:%d\r\n", g_softap_fac_nv->cmee_mode);
    AtcAp_SendDataInd();
    
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CMEE_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Dep2_053   2008.05.21   create
*******************************************************************************/
unsigned char AtcAp_CMEE_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CMEE:(0-2)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CLAC_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Dep2_053   2008.05.21   create
*******************************************************************************/
unsigned char AtcAp_CLAC_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\nAT+CGSN\r\nAT+CEREG\r\nAT+CGATT\r\nAT+CIMI\
\r\nAT+CGDCONT\r\nAT+CFUN\r\nAT+CMEE\r\nAT+CLAC\r\nAT+CESQ\r\nAT+CGPADDR\
\r\nAT+CSODCP\r\nAT+CRTDCP\r\nAT+CRC\r\nAT+CGACT\
\r\nAT+CEDRXS\r\nAT+CPSMS\r\nAT+CGAPNRC\
\r\nAT+CSCON\r\nAT^SIMST\r\nAT+NL2THP\
\r\nATE\r\nAT+CGMM\r\nAT+CCIOTOPT\r\nAT+CEDRXRDP\r\nAT+CGEQOSRDP\
\r\nAT+CTZR\r\nAT+CGCONTRDP\r\nAT+CSQ");

#ifdef ESM_DEDICATED_EPS_BEARER
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
        (const unsigned char *)"\r\nAT+CGDSCONT\r\nAT+CGTFT\r\nAT+CGEQOS");
#endif
#ifdef ESM_EPS_BEARER_MODIFY
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
        (const unsigned char *)"\r\nAT+CGCMOD");
#endif
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
        (const unsigned char *)"\r\nAT+CLCK\r\nAT+CPWD\r\nAT+CPIN");
#ifdef NBIOT_SMS_FEATURE
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
        (const unsigned char *)"\r\nAT+CSMS\r\nAT+CMGF\r\nAT+CSCA\r\nAT+CMGS");
#endif
#if SPECIAL_PLMN_SEARCH_FEATURE_FOR_IT_TEST
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
        (const unsigned char *)"\r\nAT+COPS");
#endif

    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
        (const unsigned char *)"\r\n");

    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CLAC_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Dep2_053   2008.05.21   create
*******************************************************************************/
unsigned char AtcAp_CLAC_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CESQ_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   GuanShihang   2017.01.17   create
*******************************************************************************/
unsigned char AtcAp_CESQ_T_LNB_Process(unsigned char *pEventBuffer)
{    
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CESQ: (99),(99),(255),(255),(0-34,255),(0-97,255)\r\n");

    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CGACT_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Du Zuchuang   2016.12.21   create
*******************************************************************************/
unsigned char AtcAp_CGACT_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CGACT: (0,1)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CSODCP_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   q    2017.01.06   create
*******************************************************************************/
unsigned char AtcAp_CSODCP_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CSODCP:(0-10),(0-%d),(0-2),(0,1),(1-255)\r\n", EPS_CSODCP_CPDATA_LENGTH_MAX);

    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CRTDCP_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   GuanShihang    2017.01.04   create
*******************************************************************************/
unsigned char AtcAp_CRTDCP_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CRTDCP:(0,1),(0-10),(65535)\r\n");

    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CEDRXS_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   q    2017.01.09   create
*******************************************************************************/
unsigned char AtcAp_CEDRXS_T_LNB_Process(unsigned char *pEventBuffer)
{
    ST_ATC_CEDRXS_PARAMETER* pCedrxsParam = (ST_ATC_CEDRXS_PARAMETER*)pEventBuffer;
 
    if(D_ATC_EVENT_CEDRXS_T == pCedrxsParam->usEvent)
    {
#if VER_QUCTL260
        AtcAp_StrPrintf_AtcRspBuf("\r\n+CEDRXS: (0-3),(5),(\"0010\",\"0011\",\"0101\",\"1001-1111\")\r\n");
#else
        AtcAp_StrPrintf_AtcRspBuf("\r\n+CEDRXS:(0-3),(5),(\"0010\",\"0011\",\"0101\",\"1001-1111\"),(\"0000-1111\")\r\n");
#endif
    }
    else if(D_ATC_EVENT_NPTWEDRXS_T == pCedrxsParam->usEvent)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+NPTWEDRXS:(0-3),(5),(\"0000-1111\"),(\"0010\",\"0011\",\"0101\",\"1001-1111\")\r\n");
    }
    else
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+QEDRXCFG: (0-3),(5),(\"0010\",\"0011\",\"0101\",\"1001-1111\"),(\"0000-1111\")\r\n");
    }

    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CPSMS_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   q    2017.01.10   create
*******************************************************************************/
unsigned char AtcAp_CPSMS_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_PrintLog(0, NAS_THREAD_ID, DEBUG_LOG,"[AtcAp_CPSMS_T_LNB_Process]");

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CPSMS: (0-2),,,(00000000-11111111),(00000000-11111111)\r\n");

    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

#ifdef EPS_BEARER_TFT_SUPPORT
/*******************************************************************************
  MODULE    : ATC_CGTFT_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.  Qiujingdong   2017.01.20   create
*******************************************************************************/
unsigned char AtcAp_CGTFT_T_LNB_Process (unsigned char *pEventBuffer)
{
    unsigned char aucPdpType[][7] = {"IP","IPV6","IPV4V6"};
    unsigned char *pInd = NULL;

    pInd = AtcAp_Malloc(D_ATC_TFT_TEST_MSG_MAX_LENGTH + 1);

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)pInd,
        (const unsigned char *)"\r\n+CGTFT:%c%s%c,(1-6),(0-255),((0-255).(0-255).(0-255).(0-255).(0-255).(0-255).\
(0-255).(0-255)),(0-255),((0-65535).(0-65535)),((0-65535).(0-65535)),(0-4294967295),\
((0-255).(0-255)),,(0-3),((0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255))\r\n\
+CGTFT:%c%s%c,(1-6),(0-255),((0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).\
(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).\
(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255)),\
(0-255),((0-65535).(0-65535)),((0-65535).(0-65535)),(0-4294967295),((0-255).(0-255)),(0-1048575),(0-3),\
((0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).\
(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).\
(0-255).(0-255).(0-255).(0-255).(0-255).(0-255))\r\n\
+CGTFT:%c%s%c,(1-6),(0-255),((0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).\
(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).\
(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255)),\
(0-255),((0-65535).(0-65535)),((0-65535).(0-65535)),(0-4294967295),((0-255).(0-255)),(0-1048575),(0-3),\
((0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).\
(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).(0-255).\
(0-255).(0-255).(0-255).(0-255).(0-255).(0-255))\r\n", 
        D_ATC_N_QUOTATION, &aucPdpType[0], D_ATC_N_QUOTATION, 
        D_ATC_N_QUOTATION, &aucPdpType[1], D_ATC_N_QUOTATION, 
        D_ATC_N_QUOTATION, &aucPdpType[2], D_ATC_N_QUOTATION);
    if (g_AtcApInfo.stAtRspInfo.usRspLen > D_ATC_TFT_TEST_MSG_MAX_LENGTH)
    {
        xy_assert(ATC_FALSE);
    }
    AtcAp_SendLongDataInd(&pInd, D_ATC_TFT_TEST_MSG_MAX_LENGTH);
    AtcAp_SendOkRsp();
    
    AtcAp_Free(pInd);
    return ATC_AP_TRUE;
}
#endif

/*******************************************************************************
  MODULE    : ATC_CGEQOS_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Du Zuchuang   2017.01.18   create
*******************************************************************************/
unsigned char AtcAp_CGEQOS_T_LNB_Process (unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CGEQOS:(0-%d),(0,1-4,75,5-9,79,128-254),(0-%d),(0-%d),(0-%d),(0-%d)\r\n", 
        (D_ATC_MAX_CID - 1), D_ATC_DL_GBR_MAX,D_ATC_UL_GBR_MAX,D_ATC_DL_MBR_MAX,D_ATC_DL_MBR_MAX);

    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;

}

#ifdef ESM_EPS_BEARER_MODIFY

#endif

#ifdef NBIOT_SMS_FEATURE
/*******************************************************************************
  MODULE    : ATC_CSMS_R_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Dep2_053   2008.07.11   create
*******************************************************************************/
unsigned char AtcAp_CSMS_T_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CSMS:(0,1)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CMGF_T_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Dep2_053   2008.07.11   create
*******************************************************************************/
unsigned char AtcAp_CMGF_T_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CMGF:(0)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CSCA_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Dep2_053   2008.07.12   create
*******************************************************************************/
unsigned char AtcAp_CSCA_T_Process(unsigned char *pEventBuffer)
{
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CMGS_T_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Dep2_053   2008.07.12   create
*******************************************************************************/
unsigned char AtcAp_CMGS_T_Process(unsigned char *pEventBuffer)
{
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CNMA_T_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   Dep2_053   2008.07.12   create
*******************************************************************************/
unsigned char AtcAp_CNMA_T_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CNMA:(0-2)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

unsigned char AtcAp_CMGC_T_Process(unsigned char *pEventBuffer)
{
    AtcAp_SendOkRsp();
	
	return ATC_AP_TRUE;
}

unsigned char AtcAp_CMMS_T_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf("\r\n+CMMS:(0-2)\r\n");
    AtcAp_SendDataInd();
    
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

#endif

/*******************************************************************************
  MODULE    : ATC_CSIM_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   shao   2018.01.03   create
*******************************************************************************/
unsigned char AtcAp_CSIM_T_LNB_Process(unsigned char *pEventBuffer)
{
    //do nothing
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CCHC_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   shao   2018.01.03   create
*******************************************************************************/
unsigned char AtcAp_CCHC_T_LNB_Process(unsigned char *pEventBuffer)
{
    //do nothing
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CCHO_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   shao   2018.01.03   create
*******************************************************************************/
unsigned char AtcAp_CCHO_T_LNB_Process(unsigned char *pEventBuffer)
{
    //do nothing
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CGLA_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   shao   2018.01.03   create
*******************************************************************************/
unsigned char AtcAp_CGLA_T_LNB_Process(unsigned char *pEventBuffer)
{
    //do nothing
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CRSM_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   shao   2018.01.03   create
*******************************************************************************/
unsigned char AtcAp_CRSM_T_LNB_Process(unsigned char *pEventBuffer)
{
    //do nothing
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CSCON_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   jiangna    2018.08.06   create
*******************************************************************************/
unsigned char AtcAp_CSCON_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CSCON: (0,1)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CGEREP_T_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   yuhao   2018.01.03   create
*******************************************************************************/
unsigned char AtcAp_CGEREP_T_Process(unsigned char*pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,(const unsigned char *)"\r\n+CGEREP:(0,1)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CCIOTOPT_T_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   jiangna   2018.11.06   create
*******************************************************************************/
unsigned char AtcAp_CCIOTOPT_T_Process(unsigned char*pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,(const unsigned char *)"\r\n+CCIOTOPT: (0,1,3),(1,3),(1,2)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CEDRXRDP_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   tianchengbin   2018.11.26   create
*******************************************************************************/
unsigned char AtcAp_CEDRXRDP_T_LNB_Process(unsigned char *pEventBuffer)
{
    //do nothing
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CTZR_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   JN   2018.11.26   create
*******************************************************************************/
unsigned char AtcAp_CTZR_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,(const unsigned char *)"\r\n+CTZR:(0,1,2,3)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CPIN_T_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   GCM   2018.12.18   create
*******************************************************************************/
unsigned char AtcAp_CPIN_T_Process(unsigned char *pEventBuffer)
{
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CLCK_T_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   GCM   2018.12.18   create
*******************************************************************************/
unsigned char AtcAp_CLCK_T_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CLCK:(%cSC%c)\r\n",
        D_ATC_N_QUOTATION, D_ATC_N_QUOTATION);
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_CPWD_T_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.   GCM   2018.12.18   create
*******************************************************************************/
unsigned char AtcAp_CPWD_T_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CPWD:(%cSC%c,4-8)\r\n",
        D_ATC_N_QUOTATION, D_ATC_N_QUOTATION);
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_NUESTATS_T_LNB_Process(unsigned char *pEventBuffer)
{
#if VER_QUECTEL
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
        (const unsigned char *)"\r\n%s\r\n", "NUESTATS:RADIO,CELL,BLER,THP,APPSMEM,ARMMEM,ARMSTACK,ALLMEM,SBAND,ALL");
#else
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
        (const unsigned char *)"\r\n%s\r\n", "+NUESTATS:RADIO,CELL,BLER,THP,APPSMEM,ARMMEM,ARMSTACK,ALLMEM,SBAND,ALL");
#endif
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_NEARFCN_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

unsigned char AtcAp_NCCID_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

/*******************************************************************************
  MODULE    : ATC_NL2THP_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
*******************************************************************************/
unsigned char AtcAp_NL2THP_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
        (const unsigned char *)"\r\n+NL2THP:(0,1)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_CSQ_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CSQ: (0-31,99),(0-7,99)\r\n");

    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

#ifdef LCS_MOLR_ENABLE
/*******************************************************************************
  MODULE    : ATC_CMOLR_T_LNB_Process
  FUNCTION  :
  NOTE      :
  HISTORY   :
      1.  jiangna   2018.04.02   create
*******************************************************************************/
unsigned char AtcAp_CMOLR_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
        (const unsigned char *)"\r\n+CMOLR:(0-3),(0-6),(0,1),(0-127),(0,1),\
                       (0,1),(0-127),(0-4),(0,1),(1-65535),(1-65535),\
                       (1,2,4,8,16,32,64),(0,1),(),()\r\n\r\nOK\r\n");

    AtcAp_SendDataInd();
    
    return ATC_AP_TRUE;
}
#endif

unsigned char AtcAp_CEER_T_LNB_Process(unsigned char *pEventBuffer)
{
     AtcAp_SendOkRsp();
     
     return ATC_AP_TRUE;
}

unsigned char AtcAp_CIPCA_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf("\r\n+CIPCA:(3),(0,1)\r\n");
    AtcAp_SendDataInd();

    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

unsigned char AtcAp_CGAUTH_T_LNB_Process(unsigned char *pEventBuffer)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = 0;
    AtcAp_StrPrintf_AtcRspBuf("\r\n+CGAUTH:(0-10),(0-2),(0-16),(0-16)\r\n");
    AtcAp_SendDataInd();
    
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

unsigned char AtcAp_CNMPSD_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_SendOkRsp();
    return ATC_AP_TRUE;
}

unsigned char AtcAp_CPINR_T_LNB_Process(unsigned char *pEventBuffer)
{
  //  AtcAp_StrPrintf_AtcRspBuf("\r\n+CPINR:(SIM PIN,SIM PUK,SIM PIN2,SIM PUK2)\r\n");   //wsl not use on document
  //  AtcAp_SendDataInd();
    
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

unsigned char AtcAp_NPIN_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

unsigned char AtcAp_NPTWEDRXS_T_LNB_Process(unsigned char *pEventBuffer)
{
    return AtcAp_CEDRXS_T_LNB_Process(pEventBuffer);
}

unsigned char AtcAp_NTSETID_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

unsigned char AtcAp_NCIDSTATUS_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf("\r\n+NCIDSTATUS:(0-10)\r\n");
    AtcAp_SendDataInd();

    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

unsigned char AtcAp_NGACTR_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf("\r\n+NGACTR:(0,1)\r\n");
    AtcAp_SendDataInd();
    
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

unsigned char AtcAp_NIPINFO_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf("\r\n+NIPINFO:(0,1)\r\n");
    AtcAp_SendDataInd();
    
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

unsigned char AtcAp_NQPODCP_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf("\r\n+NQPODCP:(0-10)\r\n");
    AtcAp_SendDataInd();
    
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

unsigned char AtcAp_NSNPD_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf("\r\n+NSNPD:(0-10),(1358),(0-2),(0,1),(1-255)\r\n");
    AtcAp_SendDataInd();
    
    AtcAp_SendOkRsp();
    
    return ATC_AP_TRUE;
}

unsigned char AtcAp_NQPNPD_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf("\r\n+NQPNPD:(0-10)\r\n");
    AtcAp_SendDataInd();
    
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_CNEC_T_LNB_Process(unsigned char* pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CNEC:(0,8,16,24)\r\n");
    AtcAp_SendDataInd();
	AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_NRNPDM_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf("\r\n+NRNPDM:(0,1),(0-10),(1358)\r\n");
    AtcAp_SendDataInd();
    
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_NCPCDPR_T_LNB_Process(unsigned char* pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+NCPCDPR:(0,1),(0,1)\r\n");
    AtcAp_SendDataInd();
	AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_CEID_T_LNB_Process(unsigned char *pEventBuffer)
{
	AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_NCONFIG_LNB_Process(unsigned char *pEventBuffer)
{
    ST_ATC_NCONFIG_PARAMETER* pNconfigParam = (ST_ATC_NCONFIG_PARAMETER*)pEventBuffer;

    if((pNconfigParam->mu8Func != D_ATC_NCONFIG_IPV6_GET_PREFIX_TIME)
#if VER_QUECTEL
        &&(pNconfigParam->mu8Func != D_ATC_NCONFIG_NAS_SIM_POWER_SAVING_ENABLE)
        &&(pNconfigParam->mu8Func != D_ATC_NCONFIG_CR_0354_0338_SCRAMBLING)
        &&(pNconfigParam->mu8Func != D_ATC_NCONFIG_CR_0859_SI_AVOID)
        &&(pNconfigParam->mu8Func != D_ATC_NCONFIG_COMBINE_ATTACH)
        &&(pNconfigParam->mu8Func != D_ATC_NCONFIG_T3324_T3412_EXT_CHANGE_REPORT)
#endif
        )
    {
        return ATC_AP_FALSE;
    }

#if VER_QUECTEL
    if(pNconfigParam->mu8Func == D_ATC_NCONFIG_COMBINE_ATTACH)
    {
        if(pNconfigParam->mu16Val == 0)
        {
            return ATC_AP_FALSE;
        }
    }
    if(pNconfigParam->mu8Func == D_ATC_NCONFIG_T3324_T3412_EXT_CHANGE_REPORT)
    {
        if(pNconfigParam->mu16Val == 1)
        {
            return ATC_AP_FALSE;
        }
    }
#endif

    if(pNconfigParam->mu8Func == D_ATC_NCONFIG_IPV6_GET_PREFIX_TIME)
    {
        if(pNconfigParam->mu16Val != g_softap_fac_nv->ra_timeout)
        {
            g_softap_fac_nv->ra_timeout = pNconfigParam->mu16Val;
            SAVE_FAC_PARAM(ra_timeout);
        }
    }

    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_MNBIOTEVENT_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+MNBIOTEVENT:(0,1),(1)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_CGPIAF_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CGPIAF:(0,1),(0,1),(0,1),(0,1)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

#if VER_QUCTL260
unsigned char AtcAp_QPLMNS_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_SendOkRsp();
    return ATC_AP_TRUE;
}
#endif

unsigned char AtcAp_QLOCKF_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+QLOCKF: (0-2)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_ECURC_T_LNB_Process(unsigned char *pEventBuffer)
{
    char *format = "%c%s%c:%s";

    AtcAp_StrPrintf_AtcRspBuf("\r\n+ECURC:");
    
    AtcAp_StrPrintf_AtcRspBuf(format, D_ATC_N_QUOTATION, (char*)ATC_ECURC_Table[D_ATC_ECURC_ALL].aucStr, D_ATC_N_QUOTATION, "(0-1)");
    AtcAp_StrPrintf_AtcRspBuf(format, D_ATC_N_QUOTATION, (char*)ATC_ECURC_Table[D_ATC_ECURC_CEREG].aucStr, D_ATC_N_QUOTATION, "(0-1)");
    AtcAp_StrPrintf_AtcRspBuf(format, D_ATC_N_QUOTATION, (char*)ATC_ECURC_Table[D_ATC_ECURC_CEDRXP].aucStr, D_ATC_N_QUOTATION, "(0-1)");
    AtcAp_StrPrintf_AtcRspBuf(format, D_ATC_N_QUOTATION, (char*)ATC_ECURC_Table[D_ATC_ECURC_CCIOTOPTI].aucStr, D_ATC_N_QUOTATION, "(0-1)");
    AtcAp_StrPrintf_AtcRspBuf(format, D_ATC_N_QUOTATION, (char*)ATC_ECURC_Table[D_ATC_ECURC_CSCON].aucStr, D_ATC_N_QUOTATION, "(0-1)");
    AtcAp_StrPrintf_AtcRspBuf(format, D_ATC_N_QUOTATION, (char*)ATC_ECURC_Table[D_ATC_ECURC_CTZEU].aucStr, D_ATC_N_QUOTATION, "(0-1)");
    AtcAp_StrPrintf_AtcRspBuf(format, D_ATC_N_QUOTATION, (char*)ATC_ECURC_Table[D_ATC_ECURC_CGEV].aucStr, D_ATC_N_QUOTATION, "(0-1)");
    AtcAp_StrPrintf_AtcRspBuf("\r\n");
    
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_QENG_T_LNB_Process(unsigned char *pEventBuffer)
{
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+QENG: (0,3)\r\n");
    AtcAp_SendDataInd();
    AtcAp_SendOkRsp();

    return ATC_AP_TRUE;
}

unsigned char AtcAp_QEDRXCFG_T_LNB_Process(unsigned char *pEventBuffer)
{
    return AtcAp_CEDRXS_T_LNB_Process(pEventBuffer);
}

void AtcAp_MsgProc_AT_CMD_RST(unsigned char* pRecvMsg)
{
    ST_ATC_AP_CMD_RST* pCmdRst;
    ST_ATC_AP_CMD_RST tCmsRst = { 0 };
    
    pCmdRst = &tCmsRst;
    AtcAp_MemCpy((unsigned char*)pCmdRst, pRecvMsg, sizeof(ST_ATC_AP_CMD_RST));

    if(ATC_OK == pCmdRst->ucResult)
    {
        AtcAp_SendOkRsp();
    }
    else
    {
        if(ATC_CMD_TYPE_CMEE == pCmdRst->ucCmdType)
        {
            AtcAp_SendCmeeErr(pCmdRst->usErrCode);
        }
        else
        {
            AtcAp_SendCmsErr(pCmdRst->usErrCode);
        }
    }
}

void AtcAp_MsgProc_CGSN_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CGSN_CNF_STRU* pCgsnCnf;
    unsigned char*         pucData;

    pCgsnCnf = (ATC_MSG_CGSN_CNF_STRU*)pRecvMsg;

    if(0 == pCgsnCnf->ucLen)
    {
        return;
    }

    pucData = AtcAp_Malloc(pCgsnCnf->ucLen + 1);
    AtcAp_MemCpy(pucData, pCgsnCnf->aucData, pCgsnCnf->ucLen);
    
    if (0 == pCgsnCnf->snt)
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n%s\r\n", pucData);
        AtcAp_SendDataInd();
    }
    else if (1 == pCgsnCnf->snt)
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CGSN: %s\r\n", pucData);
        AtcAp_SendDataInd();
    }
    else if (2 == pCgsnCnf->snt)
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CGSN: %s\r\n", pucData);
        AtcAp_SendDataInd();
    }
    else //ucSnt = 3
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CGSN: %s\r\n", pucData);
        AtcAp_SendDataInd();
    }

    AtcAp_Free(pucData);
}

void AtcAp_MsgProc_CEREG_R_Cnf(unsigned char* pRecvMsg)
{
    unsigned char             i;
    unsigned char             ucTemp    = 0;
    unsigned char             aucStr[9] = {0};
    ATC_MSG_CEREG_R_CNF_STRU* pCeregRCnf;

    pCeregRCnf = (ATC_MSG_CEREG_R_CNF_STRU*)pRecvMsg;

    //<n>,<stat>
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CEREG: %d,%d", pCeregRCnf->tRegisterState.ucIndPara, pCeregRCnf->tRegisterState.ucEPSRegStatus);
    if (1 < pCeregRCnf->tRegisterState.ucIndPara)
    {
        //[<tac>]two byte tracking area code in hexadecimal format 
        if (pCeregRCnf->tRegisterState.OP_TacLac)
        {
            g_AtcApInfo.stAtRspInfo.ucHexStrLen = 4;
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)",%h", pCeregRCnf->tRegisterState.usTacOrLac);
        }
        else
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)",");
        }
        //[<ci>] four byte E-UTRAN cell ID in hexadecimal format
        if (pCeregRCnf->tRegisterState.OP_CellId)
        {
            g_AtcApInfo.stAtRspInfo.ucHexStrLen = 8;
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)",%h", pCeregRCnf->tRegisterState.ulCellId);
        }
        else
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)",");
        }

        //[<Act>]
        AtcAp_WriteIntPara_M(pCeregRCnf->tRegisterState.OP_Act, pCeregRCnf->tRegisterState.ucAct, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
    }
    if ((3 == pCeregRCnf->tRegisterState.ucIndPara)
        &&pCeregRCnf->tRegisterState.OP_Act)
    {
        //[<cause_type>]
        AtcAp_WriteIntPara(pCeregRCnf->tRegisterState.OP_CauseType, pCeregRCnf->tRegisterState.ucCauseType );
        //[<reject_cause>]
        AtcAp_WriteIntPara(pCeregRCnf->tRegisterState.OP_RejectCause, pCeregRCnf->tRegisterState.ucRejectCause );
    }

    if (3 < pCeregRCnf->tRegisterState.ucIndPara)
    {    
        //[<cause_type>]
        AtcAp_WriteIntPara_M(pCeregRCnf->tRegisterState.OP_CauseType, pCeregRCnf->tRegisterState.ucCauseType, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
        //[<reject_cause>]
        AtcAp_WriteIntPara_M(pCeregRCnf->tRegisterState.OP_RejectCause, pCeregRCnf->tRegisterState.ucRejectCause, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);

        //<Active-Time>
        if (pCeregRCnf->tRegisterState.OP_ActiveTime)
        {
            for (i  = 0; i < 8; i++)
            {
                ucTemp = (0x80 >> i)& pCeregRCnf->tRegisterState.ucActiveTime;
                aucStr[i] = (ucTemp >> (7-i)) + 0x30;
            }
        }
        #if VER_QUECTEL
            AtcAp_WriteStrPara_M_NoQuotation(pCeregRCnf->tRegisterState.OP_ActiveTime, aucStr );
        #else
            AtcAp_WriteStrPara_M(pCeregRCnf->tRegisterState.OP_ActiveTime, aucStr );
        #endif

        //<Periodic-TAU>
        if (pCeregRCnf->tRegisterState.OP_PeriodicTAU)
        {
            for (i  = 0; i < 8; i++)
            {
                ucTemp = (0x80 >> i)& pCeregRCnf->tRegisterState.ucPeriodicTAU;
                aucStr[i] = (ucTemp >> (7-i)) + 0x30;
            }
        }
        #if VER_QUECTEL
            AtcAp_WriteStrPara_M_NoQuotation(pCeregRCnf->tRegisterState.OP_PeriodicTAU, aucStr );
        #else
            AtcAp_WriteStrPara_M(pCeregRCnf->tRegisterState.OP_PeriodicTAU, aucStr );
        #endif
    }
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)"\r\n");
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGATT_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CGATT_R_CNF_STRU* ptCgattRCnf;

    ptCgattRCnf = (ATC_MSG_CGATT_R_CNF_STRU*)pRecvMsg;
    
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
        (const unsigned char *)"\r\n+CGATT: %d\r\n", ptCgattRCnf->ucState);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CIMI_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CIMI_CNF_STRU*    ptCimiCnf;

    ptCimiCnf = (ATC_MSG_CIMI_CNF_STRU*)pRecvMsg;
#if VER_QUECTEL
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
         (const unsigned char *)"\r\n%s\r\n", ptCimiCnf->stImsi.aucImsi);
#else
   // g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
   //  (const unsigned char *)"\r\n+CIMI:%s\r\n", ptCimiCnf->stImsi.aucImsi);
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
     (const unsigned char *)"\r\n%s\r\n", ptCimiCnf->stImsi.aucImsi);
#endif
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGDCONT_R_Cnf(unsigned char* pRecvMsg)
{
    unsigned char                  i;
    unsigned char                  aucPdpType[][7] = {"RESERV","IP","IPV6","IPV4V6","RESERV","Non-IP"};
    ATC_MSG_CGDCONT_R_CNF_STRU*    ptCgdcontRCnf;

    ptCgdcontRCnf = (ATC_MSG_CGDCONT_R_CNF_STRU*)pRecvMsg;

    for (i = 0; i < ptCgdcontRCnf->ucNum; i++)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+CGDCONT: %d,\"%s\",\"%s\",,0,%d,,,,,%d,%d",
                                ptCgdcontRCnf->stPdpContext[i].ucAtCid, 
                                aucPdpType[ptCgdcontRCnf->stPdpContext[i].ucPdpType],
                                ptCgdcontRCnf->stPdpContext[i].aucApnValue,
                                ptCgdcontRCnf->stPdpContext[i].ucH_comp,
                                ptCgdcontRCnf->stPdpContext[i].ucNSLPI,
                                ptCgdcontRCnf->stPdpContext[i].ucSecurePco);
    }

    if(0 != g_AtcApInfo.stAtRspInfo.usRspLen)
    {   
        AtcAp_StrPrintf_AtcRspBuf("\r\n");
        AtcAp_SendDataInd();
    }
}

void AtcAp_MsgProc_CFUN_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CFUN_R_CNF_STRU*       ptCfunRCnf;

    ptCfunRCnf = (ATC_MSG_CFUN_R_CNF_STRU*)pRecvMsg;
    
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CFUN: %d\r\n", ptCfunRCnf->ucFunMode);

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CESQ_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CESQ_CNF_STRU*       pCesqCnf;

    pCesqCnf = (ATC_MSG_CESQ_CNF_STRU*)pRecvMsg;
    
     g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CESQ: 99,99,%d,%d,%d,%d\r\n", 
        pCesqCnf->ucRscp, pCesqCnf->ucEcno, pCesqCnf->ucRsrq, pCesqCnf->ucRsrp);

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CSQ_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CSQ_CNF_STRU*       pCsqCnf;

    pCsqCnf = (ATC_MSG_CSQ_CNF_STRU*)pRecvMsg;
#if VER_QUECTEL
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CSQ:%d,99\r\n", pCsqCnf->ucRxlev);
#else
    if(99 == pCsqCnf->ucRxlev)
		pCsqCnf->ucBer = 99;
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CSQ: %d,%d\r\n", pCsqCnf->ucRxlev, pCsqCnf->ucBer);
#endif
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGPADDR_Cnf(unsigned char* pRecvMsg)
{
    unsigned char                       i;
    ATC_MSG_CGPADDR_CNF_STRU*           pCgpaddrCnf;

    pCgpaddrCnf = (ATC_MSG_CGPADDR_CNF_STRU*)pRecvMsg;

    if(0 == pCgpaddrCnf->stPara.ucCidNum)
    {
        return;
    }

    for (i = 0; i < pCgpaddrCnf->stPara.ucCidNum; i++)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen,
            (const unsigned char *)"\r\n+CGPADDR: %d", (pCgpaddrCnf->stPara.aPdpAddr[i].ucCid));
        if (D_ATC_FLAG_TRUE == pCgpaddrCnf->stPara.aPdpAddr[i].ucPdpaddr1Flg)
        {
            AtcAp_OutputAddr(4, pCgpaddrCnf->stPara.aPdpAddr[i].Pdpaddr1.aucIpv4Addrs, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
        }
        
        if (D_ATC_FLAG_TRUE == pCgpaddrCnf->stPara.aPdpAddr[i].ucPdpaddr2Flg)
        {
            AtcAp_OutputAddr(16, pCgpaddrCnf->stPara.aPdpAddr[i].PdpAddr2.aucIpv6Addrs, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
        }
    }
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),(const unsigned char *)"\r\n");
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGPADDR_T_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CGPADDR_T_CNF_STRU*    pCgpaddrTCnf;
    unsigned char                  i;

    pCgpaddrTCnf = (ATC_MSG_CGPADDR_T_CNF_STRU*)pRecvMsg;

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGPADDR: (");
    for (i = 0; i < pCgpaddrTCnf->stPara.ucCidNum; i++)
    {
        if (i == (pCgpaddrTCnf->stPara.ucCidNum - 1))
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen,
                (const unsigned char *)"%d",
                (pCgpaddrTCnf->stPara.aucCid[i]));
        }
        else
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen,
                (const unsigned char *)"%d,",
                (pCgpaddrTCnf->stPara.aucCid[i]));
        }
    }
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
        (const unsigned char *)")\r\n");

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGACT_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CGACT_R_CNF_STRU*    pCgactRCnf;
    unsigned char                i;

    pCgactRCnf = (ATC_MSG_CGACT_R_CNF_STRU*)pRecvMsg;

    if (0 != pCgactRCnf->stState.ucValidNum)
    {
        for (i = 0; i < pCgactRCnf->stState.ucValidNum; i++)
        {
            AtcAp_StrPrintf_AtcRspBuf("\r\n+CGACT: %d,%d", pCgactRCnf->stState.aCidSta[i].ucCid, pCgactRCnf->stState.aCidSta[i].ucState);
        }

        AtcAp_StrPrintf_AtcRspBuf("\r\n");
        AtcAp_SendDataInd();
    }
}

void AtcAp_MsgProc_CRTDCP_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CRTDCP_R_CNF_STRU* pCrtdcpRCnf = (ATC_MSG_CRTDCP_R_CNF_STRU*)pRecvMsg;

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CRTDCP:%d\r\n", pCrtdcpRCnf->ucCrtdcpRepValue);
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_SIMST_R_Cnf(unsigned char* pRecvMsg)
{
    AtcAp_MsgProc_SIMST_Ind(pRecvMsg);
}

void AtcAp_MsgProc_CEDRXS_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CEDRXS_R_CNF_STRU* pEdrxsRCnf;
    unsigned char              aucStr[5]  = {0};
    unsigned char              aucStr1[5] = {0};
    unsigned char              i;
    
    pEdrxsRCnf = (ATC_MSG_CEDRXS_R_CNF_STRU*)pRecvMsg;
    for (i  = 0; i < 4; i++)
    {
        aucStr[i] = ((pEdrxsRCnf->stPara.ucReqDRXValue >> (3-i))&0x01) + 0x30;
    }
    for (i  = 0; i < 4; i++)
    {
        aucStr1[i] = ((pEdrxsRCnf->stPara.ucReqPagingTimeWin >> (3-i))&0x01) + 0x30;
    }

    if(D_ATC_EVENT_CEDRXS_R == pEdrxsRCnf->usEvent)
    {
#if VER_QUCTL260
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
           (const unsigned char *)"\r\n+CEDRXS:%d,%c%s%c\r\n", pEdrxsRCnf->stPara.ucActType,
           D_ATC_N_QUOTATION,aucStr,D_ATC_N_QUOTATION);
#else
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
            (const unsigned char *)"\r\n+CEDRXS:%d,%c%s%c,%c%s%c\r\n", pEdrxsRCnf->stPara.ucActType,
            D_ATC_N_QUOTATION,aucStr,D_ATC_N_QUOTATION, D_ATC_N_QUOTATION,aucStr1,D_ATC_N_QUOTATION);
#endif
    }
    else if(D_ATC_EVENT_NPTWEDRXS_R == pEdrxsRCnf->usEvent)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
            (const unsigned char *)"\r\n+NPTWEDRXS:%d,%c%s%c,%c%s%c", pEdrxsRCnf->stPara.ucActType,
            D_ATC_N_QUOTATION,aucStr1,D_ATC_N_QUOTATION, D_ATC_N_QUOTATION,aucStr,D_ATC_N_QUOTATION);
        
        if(pEdrxsRCnf->stPara.ucNWeDRXValueFlg)
        {
            AtcAp_ConvertByte2BitStr(pEdrxsRCnf->stPara.ucNWeDRXValue, 4, aucStr);
            AtcAp_StrPrintf_AtcRspBuf(",\"%s\"", aucStr);
        }
        else
        {
            if(pEdrxsRCnf->stPara.ucPagingTimeWinFlg)
            {
                AtcAp_StrPrintf_AtcRspBuf(",\"\"");
            }
        }

        if(pEdrxsRCnf->stPara.ucPagingTimeWinFlg)
        {
            AtcAp_ConvertByte2BitStr(pEdrxsRCnf->stPara.ucPagingTimeWin, 4, aucStr);
            AtcAp_StrPrintf_AtcRspBuf(",\"%s\"", aucStr);
        }
        
        AtcAp_StrPrintf_AtcRspBuf("\r\n");
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
            (const unsigned char *)"\r\n+QEDRXCFG: %d,%c%s%c,%c%s%c\r\n", pEdrxsRCnf->stPara.ucActType,
            D_ATC_N_QUOTATION,aucStr,D_ATC_N_QUOTATION, D_ATC_N_QUOTATION,aucStr1,D_ATC_N_QUOTATION);
    }
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CPSMS_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CPSMS_R_CNF_STRU*       pCpsmsCnf;
    unsigned char                   i = 0;
    unsigned char                   aucTau[9]     = {0};
    unsigned char                   aucActTime[9] = {0};

    pCpsmsCnf = (ATC_MSG_CPSMS_R_CNF_STRU*)pRecvMsg;
    for (i = 0;i < 8;i++)
    {
        aucActTime[i] = ((pCpsmsCnf->ucReqActTime >> (7 - i))&0x01) + 0x30;
        aucTau[i] = ((pCpsmsCnf->ucReqPeriTAU >> (7 - i))&0x01) + 0x30;
    }

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
        (const unsigned char *)"\r\n+CPSMS: %d,,,\"%s\",\"%s\"\r\n",
        pCpsmsCnf->ucMode, aucTau, aucActTime);

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGAPNRC_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CGAPNRC_CNF_STRU*      pCgapnrcCnf;
    unsigned char                  i;

    pCgapnrcCnf = (ATC_MSG_CGAPNRC_CNF_STRU*)pRecvMsg;
    
    for(i = 0; i < pCgapnrcCnf->ucNum; i++)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                        (const unsigned char *)"\r\n+CGAPNRC:%d,%d,%d,%d",
                        pCgapnrcCnf->stEpsApnRateCtl[i].ucCid, 
                        pCgapnrcCnf->stEpsApnRateCtl[i].ucAdditionExcepReportFlg,
                        pCgapnrcCnf->stEpsApnRateCtl[i].ulUplinkTimeUnit,
                        pCgapnrcCnf->stEpsApnRateCtl[i].ulMaxUplinkRate);
    }

    if(0 != g_AtcApInfo.stAtRspInfo.usRspLen)
    {
       AtcAp_StrPrintf_AtcRspBuf("\r\n");
       AtcAp_SendDataInd();
    } 
}

void AtcAp_MsgProc_CGAPNRC_T_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CGAPNRC_T_CNF_STRU*      pCgapnrcTCnf;
    unsigned char i;

    pCgapnrcTCnf = (ATC_MSG_CGAPNRC_T_CNF_STRU*)pRecvMsg;
    
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGAPNRC: (");
    for (i = 0; i < pCgapnrcTCnf->stPara.ucCidNum; i++)
    {
        if (i == ( pCgapnrcTCnf->stPara.ucCidNum - 1))
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen,
                (const unsigned char *)"%d",
                ( pCgapnrcTCnf->stPara.aucCid[i]));
        }
        else
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen,
                (const unsigned char *)"%d,",
                ( pCgapnrcTCnf->stPara.aucCid[i]));
        }
    }
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
        (const unsigned char *)")\r\n");

    AtcAp_SendDataInd();
    return;
}


void AtcAp_MsgProc_CSCON_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CSCON_R_CNF_STRU*    pCsconCnf;

    pCsconCnf = (ATC_MSG_CSCON_R_CNF_STRU*)pRecvMsg;
    
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CSCON: %d,%d",
        pCsconCnf->stPara.ucIndPara,pCsconCnf->stPara.ucMode);//TBD

    AtcAp_WriteIntPara(pCsconCnf->stPara.OP_State, pCsconCnf->stPara.ucState );//when n >= 2 and EMM_CONNECTED mode this param is exist

    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
        (const unsigned char *)"\r\n");
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NL2THP_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_NL2THP_R_CNF_STRU* pNl2thpRCnf;

    pNl2thpRCnf = (ATC_MSG_NL2THP_R_CNF_STRU*)pRecvMsg;

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
        (const unsigned char *)"\r\n+NL2THP:%d\r\n", pNl2thpRCnf->ucL2THPFlag);
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NUESTATS_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_NUESTATS_CNF_STRU* pNueStatsCnf;
    unsigned char              aucSelectPlmn[7] = { 0 };

    pNueStatsCnf = (ATC_MSG_NUESTATS_CNF_STRU*)pRecvMsg;
#if VER_QUECTEL
    if(ATC_NUESTATS_TYPE_RADIO == pNueStatsCnf->type || ATC_NUESTATS_TYPE_ALL == pNueStatsCnf->type)
    {
        if(0xFFFFFFFF != pNueStatsCnf->stRadio.current_plmn)
        {
            AtcAp_IntegerToPlmn(pNueStatsCnf->stRadio.current_plmn, aucSelectPlmn);
        }

        AtcAp_StrPrintf_AtcRspBuf("\r\n%s%d\r\n", "Signal power:", pNueStatsCnf->stRadio.rsrp);
        AtcAp_StrPrintf_AtcRspBuf("%s%d\r\n", "Total power:", pNueStatsCnf->stRadio.rssi);
        AtcAp_StrPrintf_AtcRspBuf("%s%d\r\n", "TX power:", pNueStatsCnf->stRadio.current_tx_power_level);
        AtcAp_StrPrintf_AtcRspBuf("%s%u\r\n", "TX time:", pNueStatsCnf->stRadio.total_tx_time);
        AtcAp_StrPrintf_AtcRspBuf("%s%u\r\n", "RX time:", pNueStatsCnf->stRadio.total_rx_time);
        AtcAp_StrPrintf_AtcRspBuf("%s%u\r\n", "Cell ID:", pNueStatsCnf->stRadio.last_cell_ID);
        AtcAp_StrPrintf_AtcRspBuf("%s%u\r\n", "ECL:", pNueStatsCnf->stRadio.last_ECL_value);
        AtcAp_StrPrintf_AtcRspBuf("%s%d\r\n", "SNR:", pNueStatsCnf->stRadio.last_snr_value);
        AtcAp_StrPrintf_AtcRspBuf("%s%u\r\n", "EARFCN:", pNueStatsCnf->stRadio.last_earfcn_value);
        AtcAp_StrPrintf_AtcRspBuf("%s%u\r\n", "PCI:", pNueStatsCnf->stRadio.last_pci_value);
        AtcAp_StrPrintf_AtcRspBuf("%s%d\r\n", "RSRQ:", pNueStatsCnf->stRadio.rsrq);
        AtcAp_StrPrintf_AtcRspBuf("%s%s\r\n", "PLMN:", aucSelectPlmn);
        AtcAp_StrPrintf_AtcRspBuf("%s%04X\r\n", "TAC:", pNueStatsCnf->stRadio.current_tac);
        if(pNueStatsCnf->stRadio.operation_mode == 255)
        {
            pNueStatsCnf->stRadio.operation_mode = 0;
        }
        else
        {
            pNueStatsCnf->stRadio.operation_mode++;
        }
        AtcAp_StrPrintf_AtcRspBuf("%s%d\r\n", "OPERATOR MODE:", pNueStatsCnf->stRadio.operation_mode);
        AtcAp_StrPrintf_AtcRspBuf("%s%d\r\n", "CURRENT BAND:", pNueStatsCnf->stRadio.band);
        AtcAp_SendDataInd();
    }

    if(ATC_NUESTATS_TYPE_CELL == pNueStatsCnf->type || ATC_NUESTATS_TYPE_ALL == pNueStatsCnf->type)
    {
        if(0 != pNueStatsCnf->stCell.stCellList.ucCellNum)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
            (const unsigned char *)"\r\n%s%d%c%d%s", "NUESTATS:CELL,", 
            pNueStatsCnf->stCell.stCellList.aNCell[0].ulDlEarfcn, ',', pNueStatsCnf->stCell.stCellList.aNCell[0].usPhyCellId, ",1,");
            AtcAp_StrPrintf_AtcRspBuf("%d,", pNueStatsCnf->stCell.stCellList.aNCell[0].sRsrp);
            AtcAp_StrPrintf_AtcRspBuf("%d,", pNueStatsCnf->stCell.stCellList.aNCell[0].sRsrq);
            AtcAp_StrPrintf_AtcRspBuf("%d,", pNueStatsCnf->stCell.stCellList.aNCell[0].sRssi);
            AtcAp_StrPrintf_AtcRspBuf("%d\r\n", pNueStatsCnf->stCell.stCellList.aNCell[0].sSinr);

            AtcAp_Build_NCell(&pNueStatsCnf->stCell.stCellList);
            
            AtcAp_SendDataInd();
        }
    }

    if(ATC_NUESTATS_TYPE_BLER == pNueStatsCnf->type || ATC_NUESTATS_TYPE_ALL == pNueStatsCnf->type)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
            (const unsigned char *)"\r\n%s%d\r\n%s%d\r\n%s%d\r\n%s%d\r\n%s%d\r\n%s%d", 
            "NUESTATS:BLER,RLC UL BLER,", pNueStatsCnf->stBler.rlc_ul_bler, 
            "NUESTATS:BLER,RLC DL BLER,", pNueStatsCnf->stBler.rlc_dl_bler, 
            "NUESTATS:BLER,MAC UL BLER,", pNueStatsCnf->stBler.mac_ul_bler, 
            "NUESTATS:BLER,MAC DL BLER,", pNueStatsCnf->stBler.mac_dl_bler, 
            "NUESTATS:BLER,Total TX bytes,", pNueStatsCnf->stBler.total_bytes_transmit, 
            "NUESTATS:BLER,Total RX bytes,", pNueStatsCnf->stBler.total_bytes_receive);
        AtcAp_SendDataInd();

        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
            (const unsigned char *)"\r\n%s%d\r\n%s%d\r\n%s%d\r\n%s%d\r\n", 
            "NUESTATS:BLER,Total TX blocks,", pNueStatsCnf->stBler.transport_blocks_send, 
            "NUESTATS:BLER,Total RX blocks,", pNueStatsCnf->stBler.transport_blocks_receive, 
            "NUESTATS:BLER,Total RTX blocks,", pNueStatsCnf->stBler.transport_blocks_retrans, 
            "NUESTATS:BLER,Total ACK/NACK RX,", pNueStatsCnf->stBler.total_ackOrNack_msg_receive);
        AtcAp_SendDataInd();
    }

    if(ATC_NUESTATS_TYPE_THP == pNueStatsCnf->type || ATC_NUESTATS_TYPE_ALL == pNueStatsCnf->type)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
            (const unsigned char *)"\r\n%s%d\r\n%s%d\r\n%s%d\r\n%s%d\r\n", 
            "NUESTATS:THP,RLC UL,", pNueStatsCnf->stThp.rlc_ul, 
            "NUESTATS:THP,RLC DL,", pNueStatsCnf->stThp.rlc_dl, 
            "NUESTATS:THP,MAC UL,", pNueStatsCnf->stThp.mac_ul, 
            "NUESTATS:THP,MAC DL,", pNueStatsCnf->stThp.mac_dl);

        AtcAp_SendDataInd();
    }

    if(ATC_NUESTATS_SBAND == pNueStatsCnf->type || ATC_NUESTATS_TYPE_ALL == pNueStatsCnf->type)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
            (const unsigned char *)"\r\nNUESTATS:SBAND,%d\r\n", pNueStatsCnf->stBand.band);
        AtcAp_SendDataInd();
    }
#else
    if(ATC_NUESTATS_TYPE_RADIO == pNueStatsCnf->type || ATC_NUESTATS_TYPE_ALL == pNueStatsCnf->type)
    {
        if(0xFFFFFFFF != pNueStatsCnf->stRadio.current_plmn)
        {
            AtcAp_IntegerToPlmn(pNueStatsCnf->stRadio.current_plmn, aucSelectPlmn);
        }
    
        AtcAp_StrPrintf_AtcRspBuf("\r\n%s%d\r\n", "+Signal power:", pNueStatsCnf->stRadio.rsrp);
        AtcAp_StrPrintf_AtcRspBuf("%s%d\r\n", "+Total power:", pNueStatsCnf->stRadio.rssi);
        AtcAp_StrPrintf_AtcRspBuf("%s%d\r\n", "+TX power:", pNueStatsCnf->stRadio.current_tx_power_level);
        AtcAp_StrPrintf_AtcRspBuf("%s%u\r\n", "+TX time:", pNueStatsCnf->stRadio.total_tx_time);
        AtcAp_StrPrintf_AtcRspBuf("%s%u\r\n", "+RX time:", pNueStatsCnf->stRadio.total_rx_time);
        AtcAp_StrPrintf_AtcRspBuf("%s%u\r\n", "+Cell ID:", pNueStatsCnf->stRadio.last_cell_ID);
        AtcAp_StrPrintf_AtcRspBuf("%s%u\r\n", "+ECL:", pNueStatsCnf->stRadio.last_ECL_value);
        AtcAp_StrPrintf_AtcRspBuf("%s%d\r\n", "+SNR:", pNueStatsCnf->stRadio.last_snr_value);
        AtcAp_StrPrintf_AtcRspBuf("%s%u\r\n", "+EARFCN:", pNueStatsCnf->stRadio.last_earfcn_value);
        AtcAp_StrPrintf_AtcRspBuf("%s%u\r\n", "+PCI:", pNueStatsCnf->stRadio.last_pci_value);
        AtcAp_StrPrintf_AtcRspBuf("%s%d\r\n", "+RSRQ:", pNueStatsCnf->stRadio.rsrq);
        AtcAp_StrPrintf_AtcRspBuf("%s%s\r\n", "+PLMN:", aucSelectPlmn);
        AtcAp_StrPrintf_AtcRspBuf("%s%04X\r\n", "+TAC:", pNueStatsCnf->stRadio.current_tac);
        AtcAp_StrPrintf_AtcRspBuf("%s%d\r\n", "+SBAND:", pNueStatsCnf->stRadio.band);
        AtcAp_StrPrintf_AtcRspBuf("%s%d\r\n", "+OPERATION MODE:", pNueStatsCnf->stRadio.operation_mode);
        
        AtcAp_SendDataInd();
    }

    if(ATC_NUESTATS_TYPE_CELL == pNueStatsCnf->type || ATC_NUESTATS_TYPE_ALL == pNueStatsCnf->type)
    {
        if(0 != pNueStatsCnf->stCell.stCellList.ucCellNum)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
            (const unsigned char *)"\r\n%s%d%c%d%s", "+NUESTATS:CELL,", 
            pNueStatsCnf->stCell.stCellList.aNCell[0].ulDlEarfcn, ',', pNueStatsCnf->stCell.stCellList.aNCell[0].usPhyCellId, ",1,");
            AtcAp_StrPrintf_AtcRspBuf("%d,", pNueStatsCnf->stCell.stCellList.aNCell[0].sRsrp);
            AtcAp_StrPrintf_AtcRspBuf("%d,", pNueStatsCnf->stCell.stCellList.aNCell[0].sRsrq);
            AtcAp_StrPrintf_AtcRspBuf("%d,", pNueStatsCnf->stCell.stCellList.aNCell[0].sRssi);
            AtcAp_StrPrintf_AtcRspBuf("%d\r\n", pNueStatsCnf->stCell.stCellList.aNCell[0].sSinr);

            AtcAp_Build_NCell(&pNueStatsCnf->stCell.stCellList);
            
            AtcAp_SendDataInd();
        }
    }

    if(ATC_NUESTATS_TYPE_BLER == pNueStatsCnf->type || ATC_NUESTATS_TYPE_ALL == pNueStatsCnf->type)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
            (const unsigned char *)"\r\n%s%d\r\n%s%d\r\n%s%d\r\n%s%d\r\n%s%d\r\n%s%d", 
            "+NUESTATS:BLER,RLC UL BLER,", pNueStatsCnf->stBler.rlc_ul_bler, 
            "+NUESTATS:BLER,RLC DL BLER,", pNueStatsCnf->stBler.rlc_dl_bler, 
            "+NUESTATS:BLER,MAC UL BLER,", pNueStatsCnf->stBler.mac_ul_bler, 
            "+NUESTATS:BLER,MAC DL BLER,", pNueStatsCnf->stBler.mac_dl_bler, 
            "+NUESTATS:BLER,Total TX bytes,", pNueStatsCnf->stBler.total_bytes_transmit, 
            "+NUESTATS:BLER,Total RX bytes,", pNueStatsCnf->stBler.total_bytes_receive);
        AtcAp_SendDataInd();

        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
            (const unsigned char *)"\r\n%s%d\r\n%s%d\r\n%s%d\r\n%s%d\r\n", 
            "+NUESTATS:BLER,Total TX blocks,", pNueStatsCnf->stBler.transport_blocks_send, 
            "+NUESTATS:BLER,Total RX blocks,", pNueStatsCnf->stBler.transport_blocks_receive, 
            "+NUESTATS:BLER,Total RTX blocks,", pNueStatsCnf->stBler.transport_blocks_retrans, 
            "+NUESTATS:BLER,Total ACK/NACK RX,", pNueStatsCnf->stBler.total_ackOrNack_msg_receive);
        AtcAp_SendDataInd();
    }

    if(ATC_NUESTATS_TYPE_THP == pNueStatsCnf->type || ATC_NUESTATS_TYPE_ALL == pNueStatsCnf->type)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
            (const unsigned char *)"\r\n%s%d\r\n%s%d\r\n%s%d\r\n%s%d\r\n", 
            "+NUESTATS:THP,RLC UL,", pNueStatsCnf->stThp.rlc_ul, 
            "+NUESTATS:THP,RLC DL,", pNueStatsCnf->stThp.rlc_dl, 
            "+NUESTATS:THP,MAC UL,", pNueStatsCnf->stThp.mac_ul, 
            "+NUESTATS:THP,MAC DL,", pNueStatsCnf->stThp.mac_dl);

        AtcAp_SendDataInd();
    }

    if(ATC_NUESTATS_SBAND == pNueStatsCnf->type || ATC_NUESTATS_TYPE_ALL == pNueStatsCnf->type)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
            (const unsigned char *)"\r\n+NUESTATS:SBAND,%d\r\n", pNueStatsCnf->stBand.band);
        AtcAp_SendDataInd();
    }
#endif
}

void AtcAp_MsgProc_NEARFCN_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_NEARFCN_R_CNF_STRU* pNearfcnRCnf;

    pNearfcnRCnf = (ATC_MSG_NEARFCN_R_CNF_STRU*)pRecvMsg;

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+NEARFCN:%d",pNearfcnRCnf->search_mode);
    if(1 == pNearfcnRCnf->search_mode)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen,
            (const unsigned char *)",%d", pNearfcnRCnf->earfcn);
    }
    else if(2 == pNearfcnRCnf->search_mode)
    {
        if (pNearfcnRCnf->pci > 255)
        {
            g_AtcApInfo.stAtRspInfo.ucHexStrLen = 3;
        }
        else
        {
            g_AtcApInfo.stAtRspInfo.ucHexStrLen = 2;
        }
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen,
            (const unsigned char *)",%d,%h", pNearfcnRCnf->earfcn, pNearfcnRCnf->pci);
    }
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen, (const unsigned char *)"\r\n");
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NBAND_R_Cnf(unsigned char* pRecvMsg)
{
    unsigned char             ucBandCount;
    unsigned char             i;
    unsigned char             aucBandInfo[G_LRRC_SUPPORT_BAND_MAX_NUM];
    unsigned short            usOffset;

    ATC_MSG_NBAND_R_CNF_STRU* pNbandRCnf;

    pNbandRCnf = (ATC_MSG_NBAND_R_CNF_STRU*)pRecvMsg;
    
    ucBandCount	= pNbandRCnf->stSupportBandList.ucSupportBandNum;
    for (i = 0; i < ucBandCount; i++ )
    {
        aucBandInfo[i] = pNbandRCnf->stSupportBandList.aucSuppBand[i];
    }

    if(pNbandRCnf->ucQBandFlg == 1)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n%s","+QBAND: ");
        
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n%s","+NBAND:");
    }
    usOffset    = g_AtcApInfo.stAtRspInfo.usRspLen;
    
    if (ucBandCount > 0)
    {
        for( i = 0; i < (ucBandCount-1); i++ )
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + usOffset, (const unsigned char *)"%d,", aucBandInfo[i]);
            usOffset    = g_AtcApInfo.stAtRspInfo.usRspLen;
        }
    
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + usOffset, (const unsigned char *)"%d\r\n", aucBandInfo[i]);
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + usOffset, (const unsigned char *)"\r\n");
    }

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NBAND_T_Cnf(unsigned char* pRecvMsg)
{
    unsigned char             ucBandCount;
    unsigned char             i;
    unsigned char             aucBandInfo[G_LRRC_SUPPORT_BAND_MAX_NUM];
    unsigned short            usOffset;
    
    ATC_MSG_NBAND_T_CNF_STRU* pNbandTCnf;

    pNbandTCnf = (ATC_MSG_NBAND_T_CNF_STRU*)pRecvMsg;
    
    ucBandCount = pNbandTCnf->stSupportBandList.ucSupportBandNum;
    for (i = 0; i < ucBandCount; i++ )
    {
        aucBandInfo[i] = pNbandTCnf->stSupportBandList.aucSuppBand[i];
    }

    if(pNbandTCnf->ucQBandFlg == 1)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n%s","+QBAND: (0-3),(");
        
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n%s","+NBAND:(");
    }
    usOffset    = g_AtcApInfo.stAtRspInfo.usRspLen;
    
    if (ucBandCount > 0)
    {
        for( i = 0; i < (ucBandCount-1); i++ )
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf+usOffset, (const unsigned char *)"%d,", aucBandInfo[i]);
            usOffset    = g_AtcApInfo.stAtRspInfo.usRspLen;
        }
    
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf+usOffset, (const unsigned char *)"%d)\r\n", aucBandInfo[i]);
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf+usOffset, (const unsigned char *)")\r\n");
    }
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NCONFIG_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_NCONFIG_R_CNF_STRU* pNconfigRCnf;
    char*                       format          = "+NCONFIG:%s,%d\r\n";

    pNconfigRCnf = (ATC_MSG_NCONFIG_R_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n");
    
    #if VER_QUECTEL
        char*                       format_str          = "+NCONFIG:%s,%s\r\n";
        AtcAp_StrPrintf_AtcRspBuf(format_str, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_AUTOCONNECT].aucStr, pNconfigRCnf->autoconnect == 1 ? "TRUE" : "FALSE");
        AtcAp_StrPrintf_AtcRspBuf(format_str, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_CR_0354_0338_SCRAMBLING].aucStr, "TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format_str, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_CR_0859_SI_AVOID].aucStr, "TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format_str, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_COMBINE_ATTACH].aucStr, pNconfigRCnf->combine_attach == 1 ? "TRUE" : "FALSE");
        AtcAp_StrPrintf_AtcRspBuf(format_str, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_CELL_RESELECTION].aucStr, pNconfigRCnf->cell_reselection == 1 ? "TRUE" : "FALSE");
        AtcAp_StrPrintf_AtcRspBuf(format_str, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_ENABLE_BIP].aucStr, pNconfigRCnf->enable_bip == 1 ? "TRUE" : "FALSE");
        AtcAp_StrPrintf_AtcRspBuf(format_str, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_MULTITONE].aucStr, pNconfigRCnf->multitone == 1 ? "TRUE" : "FALSE");
        AtcAp_StrPrintf_AtcRspBuf(format_str, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_NAS_SIM_POWER_SAVING_ENABLE].aucStr, "TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_BARRING_RELEASE_DELAY].aucStr, pNconfigRCnf->barring_release_delay);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_RELEASE_VERSION].aucStr, pNconfigRCnf->release_version);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_SYNC_TIME_PERIOD].aucStr, pNconfigRCnf->sync_time_period);
        AtcAp_StrPrintf_AtcRspBuf(format_str, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_PCO_IE_TYPE].aucStr, pNconfigRCnf->pco_ie_type == 1 ? "EPCO" : "PCO");
        AtcAp_StrPrintf_AtcRspBuf(format_str, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_NON_IP_NO_SMS_ENABLE].aucStr, pNconfigRCnf->non_ip_no_sms_enable == 1 ? "TRUE" : "FALSE");
        AtcAp_StrPrintf_AtcRspBuf(format_str, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_T3324_T3412_EXT_CHANGE_REPORT].aucStr, pNconfigRCnf->t3324_t3412_ext_chg_report == 1 ? "TRUE" : "FALSE");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_IPV6_GET_PREFIX_TIME].aucStr, g_softap_fac_nv->ra_timeout);
    #else
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_AUTOCONNECT].aucStr, pNconfigRCnf->autoconnect);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_COMBINE_ATTACH].aucStr, pNconfigRCnf->combine_attach);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_CELL_RESELECTION].aucStr, pNconfigRCnf->cell_reselection);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_ENABLE_BIP].aucStr, pNconfigRCnf->enable_bip);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_MULTITONE].aucStr, pNconfigRCnf->multitone);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_BARRING_RELEASE_DELAY].aucStr, pNconfigRCnf->barring_release_delay);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_RELEASE_VERSION].aucStr, pNconfigRCnf->release_version);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_SYNC_TIME_PERIOD].aucStr, pNconfigRCnf->sync_time_period);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_PCO_IE_TYPE].aucStr, pNconfigRCnf->pco_ie_type);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_NON_IP_NO_SMS_ENABLE].aucStr, pNconfigRCnf->non_ip_no_sms_enable);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_T3324_T3412_EXT_CHANGE_REPORT].aucStr, pNconfigRCnf->t3324_t3412_ext_chg_report);
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_IPV6_GET_PREFIX_TIME].aucStr, g_softap_fac_nv->ra_timeout);
    #endif
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NCONFIG_T_Cnf(unsigned char *pRecvMsg)
{
    ATC_MSG_NCONFIG_T_CNF_STRU* pNconfigTCnf = (ATC_MSG_NCONFIG_T_CNF_STRU*)pRecvMsg;
    char*                       format       = "+NCONFIG:(%s,(%s))\r\n";

    AtcAp_StrPrintf_AtcRspBuf("\r\n");
    
    #if VER_QUECTEL
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_AUTOCONNECT].aucStr, "FALSE,TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_CR_0354_0338_SCRAMBLING].aucStr, "FALSE,TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_CR_0859_SI_AVOID].aucStr, "FALSE,TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_COMBINE_ATTACH].aucStr, "FALSE,TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_CELL_RESELECTION].aucStr, "FALSE,TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_ENABLE_BIP].aucStr, "FALSE,TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_MULTITONE].aucStr, "FALSE,TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_NAS_SIM_POWER_SAVING_ENABLE].aucStr, "FALSE,TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_BARRING_RELEASE_DELAY].aucStr, "0-1800");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_RELEASE_VERSION].aucStr, "13,14");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_SYNC_TIME_PERIOD].aucStr, "0-65535");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_PCO_IE_TYPE].aucStr, "PCO,EPCO");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_NON_IP_NO_SMS_ENABLE].aucStr, "FALSE,TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_T3324_T3412_EXT_CHANGE_REPORT].aucStr, "FALSE,TRUE");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_IPV6_GET_PREFIX_TIME].aucStr, "0-65535");
    #else
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_AUTOCONNECT].aucStr, "0,1");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_COMBINE_ATTACH].aucStr, "0");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_CELL_RESELECTION].aucStr, "0,1");
        if(pNconfigTCnf->ucBipSupportType == D_ATC_NCONFIG_T_BIP_TYPE_SINGLE)
        {
            AtcAp_StrPrintf_AtcRspBuf("+NCONFIG:(%s,(%d))\r\n", (char*)ATC_NConfig_Table[D_ATC_NCONFIG_ENABLE_BIP].aucStr,  pNconfigTCnf->enable_bip);
        }
        else
        {
            AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_ENABLE_BIP].aucStr, "0,1");
        }
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_MULTITONE].aucStr, "0,1");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_BARRING_RELEASE_DELAY].aucStr, "0-1800");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_RELEASE_VERSION].aucStr, "13,14");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_SYNC_TIME_PERIOD].aucStr, "0-65535");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_PCO_IE_TYPE].aucStr, "0,1");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_NON_IP_NO_SMS_ENABLE].aucStr, "0,1");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_T3324_T3412_EXT_CHANGE_REPORT].aucStr, "1");
        AtcAp_StrPrintf_AtcRspBuf(format, (char*)ATC_NConfig_Table[D_ATC_NCONFIG_IPV6_GET_PREFIX_TIME].aucStr, "0-65535");
    #endif
    
    AtcAp_SendDataInd();

    return;
}

void AtcAp_MsgProc_NSET_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_NSET_R_CNF_STRU* pNsetRCnf;

    pNsetRCnf = (ATC_MSG_NSET_R_CNF_STRU*)pRecvMsg;

    if(0 == strcmp((char*)pNsetRCnf->aucInsValue, "SWVER"))
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+NSET:\"Software Version\",%s\r\n", (unsigned char*)pNsetRCnf->u.aucValue);
    }
    else
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+NSET:\"%s\",%d\r\n", (unsigned char*)pNsetRCnf->aucInsValue, pNsetRCnf->u.ulValue);
    }
    AtcAp_SendDataInd();
}

#ifdef LCS_MOLR_ENABLE
void AtcAp_MsgProc_CMOLR_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CMOLR_R_CNF_STRU* pCmolrRCnf;

    pCmolrRCnf = (ATC_MSG_CMOLR_R_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n+CMOLR:%d", pCmolrRCnf->enable);

    AtcAp_WriteIntPara_M(pCmolrRCnf->method_flag, pCmolrRCnf->method, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);

    AtcAp_StrPrintf_AtcRspBuf(",%d", pCmolrRCnf->hor_acc_set);

    AtcAp_WriteIntPara_M(pCmolrRCnf->hor_acc_flag, pCmolrRCnf->hor_acc, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);

    AtcAp_StrPrintf_AtcRspBuf(",%d,%d", pCmolrRCnf->ver_req, pCmolrRCnf->ver_acc_set);

    AtcAp_WriteIntPara_M(pCmolrRCnf->ver_acc_flag, pCmolrRCnf->ver_acc, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);

    AtcAp_StrPrintf_AtcRspBuf(",%d,%d", pCmolrRCnf->vel_req, pCmolrRCnf->rep_mode);

    AtcAp_WriteIntPara_M(pCmolrRCnf->timeout, pCmolrRCnf->timeout, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
    AtcAp_WriteIntPara_M(pCmolrRCnf->interval, pCmolrRCnf->interval, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
    AtcAp_WriteIntPara_M(pCmolrRCnf->shape_rep, pCmolrRCnf->shape_rep, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
    AtcAp_StrPrintf_AtcRspBuf(",%d", pCmolrRCnf->plane);
    
    AtcAp_SendDataInd();
}
#endif

#ifdef ESM_DEDICATED_EPS_BEARER
void AtcAp_MsgProc_CGDSCONT_R_Cnf(unsigned char* pRecvMsg)
{
    unsigned char                i;
    ATC_MSG_CGDSCONT_R_CNF_STRU* pCgdscontRCnf;

    pCgdscontRCnf = (ATC_MSG_CGDSCONT_R_CNF_STRU*)pRecvMsg;

    if(0 == pCgdscontRCnf->stPara.ucValidNum)
    {
        return;
    }
    
    for(i = 0; i < pCgdscontRCnf->stPara.ucValidNum; i++)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+CGDSCONT:%d,%d,0,0", 
            pCgdscontRCnf->stPara.aSPdpCtxtPara[i].ucCid, pCgdscontRCnf->stPara.aSPdpCtxtPara[i].ucP_cid);
    }
    AtcAp_StrPrintf_AtcRspBuf("\r\n");
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGDSCONT_T_Cnf(unsigned char *pRecvMsg)
{
    unsigned char i = 0;

    ATC_MSG_CGDSCONT_T_CNF_STRU* pCgdscontTCnf;

    pCgdscontTCnf = (ATC_MSG_CGDSCONT_T_CNF_STRU*)pRecvMsg;

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CGDSCONT:(");

    for (i = 0; i < pCgdscontTCnf->ucNum; i++)
    {
        if (pCgdscontTCnf->ucNum - 1 == i)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)"%d", pCgdscontTCnf->aucCid[i]);
        }
        else
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)"%d,",pCgdscontTCnf->aucCid[i]);
        }
    }

    if (0 == pCgdscontTCnf->ucP_CidNum)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)"),(");
    }
    else
    {
        for (i = 0;i < pCgdscontTCnf->ucP_CidNum; i++)
        {
            if (0 == i)
            {
                g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                    (const unsigned char *)"),(%d",pCgdscontTCnf->aucP_Cid[i]);
            }
            else
            {
                g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                    (const unsigned char *)",%d",pCgdscontTCnf->aucP_Cid[i]);
            }
        }
    }
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
        (const unsigned char *)"),(0),(0)\r\n");

    AtcAp_SendDataInd();

    return;
}
#endif


#ifdef EPS_BEARER_TFT_SUPPORT
void AtcAp_MsgProc_CGTFT_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CGTFT_R_CNF_STRU* pCgtftRCnf;
    unsigned char*            pRespBuff;
//    unsigned short            usLen;
    unsigned char             i,j;
    unsigned short            usMaxLen;

    pCgtftRCnf = (ATC_MSG_CGTFT_R_CNF_STRU*)pRecvMsg;

    usMaxLen = D_ATC_READ_TFT_MAX_VALUE * (pCgtftRCnf->atBearerTft[0].ucPacketFilterNum + pCgtftRCnf->atBearerTft[1].ucPacketFilterNum);
    pRespBuff = (unsigned char*)AtcAp_Malloc(usMaxLen + 1);
    if(NULL == pRespBuff)
    {
        return;
    }

    for(i = 0; i < pCgtftRCnf->ucNum; i++)
    {
        for(j = 0; j < pCgtftRCnf->atBearerTft[i].ucPacketFilterNum; j++)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pRespBuff + g_AtcApInfo.stAtRspInfo.usRspLen),
                                                                (const unsigned char *)"\r\n+CGTFT:%d,%d,%d",
                                                                pCgtftRCnf->atBearerTft[i].ucAtCid,
                                                                pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ucPacketFilterId, 
                                                                pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ucEvaluationPrecedenceIndex);

            AtcAp_OutputAddr(pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ucRemoteAddrAndSubMaskLen,
                             pCgtftRCnf->atBearerTft[i].atPacketFilter[j].aucRemoteAddrAndSubMask, pRespBuff);

            AtcAp_WriteIntPara_M(pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ucProtocolNum_NextHeaderFlg,
                                 pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ucProtocolNum_NextHeader, pRespBuff);

            AtcAp_OutputPortRange(pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ucLocalPortRangeLen, 
                                  pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ausLocalPortRange, pRespBuff);
            AtcAp_OutputPortRange(pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ucRemotePortRangeLen,
                                  pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ausRemotePortRange, pRespBuff);
            
            AtcAp_WriteHexPara_M(pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ucIpsecSPIFlg,
                                 pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ulIpsecSPI, pRespBuff, D_ATC_CGTFT_PARAM_SPI_LEN);

            if (0 != pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ucTypeOfServiceAndMaskLen)
            {
                g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pRespBuff + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)",\"%d.%d\"",
                                                                    pCgtftRCnf->atBearerTft[i].atPacketFilter[j].aucTypeOfServiceAndMask[0], 
                                                                    pCgtftRCnf->atBearerTft[i].atPacketFilter[j].aucTypeOfServiceAndMask[1]);
            }
            else
            {
                g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pRespBuff + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)",\"\"");
            }

            AtcAp_WriteHexPara_M(pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ucFlowLabelFlg, 
                                 pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ulFlowLabel, pRespBuff, D_ATC_CGTFT_PARAM_FLOW_LABEL_LEN);

            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pRespBuff + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)",%d", pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ucDirection);
            AtcAp_OutputAddr(pCgtftRCnf->atBearerTft[i].atPacketFilter[j].ucLocalAddrAndSubMaskLen, 
                             pCgtftRCnf->atBearerTft[i].atPacketFilter[j].aucLocalAddrAndSubMask, pRespBuff);
        }
    }

    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(pRespBuff + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)"\r\n");
    AtcAp_SendLongDataInd(&pRespBuff, usMaxLen);
    AtcAp_Free(pRespBuff);
}
#endif

void AtcAp_MsgProc_CGEQOS_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CGEQOS_R_CNF_STRU*  pCgeqosRCnf;
    unsigned char               i;

    pCgeqosRCnf = (ATC_MSG_CGEQOS_R_CNF_STRU*)pRecvMsg;

    for (i = 0; i < pCgeqosRCnf->stPara.ucValidNum; i++)
    {
       AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CGEQOS:%d,%d,%d,%d,%d,%d",
            pCgeqosRCnf->stPara.aQosInfo[i].ucCid, pCgeqosRCnf->stPara.aQosInfo[i].stQosInfo.ucQci,
            pCgeqosRCnf->stPara.aQosInfo[i].stQosInfo.ulDlGbr,pCgeqosRCnf->stPara.aQosInfo[i].stQosInfo.ulUlGbr,
            pCgeqosRCnf->stPara.aQosInfo[i].stQosInfo.ulDlMbr,pCgeqosRCnf->stPara.aQosInfo[i].stQosInfo.ulUlMbr);
    }
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n");

    AtcAp_SendDataInd();
}

#ifdef ESM_EPS_BEARER_MODIFY
void AtcAp_MsgProc_CGCMOD_T_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CGCMOD_T_CNF_STRU*  pCgcModTCnf;
    unsigned char               i;

    pCgcModTCnf = (ATC_MSG_CGCMOD_T_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CGCMOD:(");
    
    for (i = 0; i < pCgcModTCnf->stPara.ucCidNum; i++)
    {
        if (i == (pCgcModTCnf->stPara.ucCidNum - 1))
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)"%d", pCgcModTCnf->stPara.aucCid[i]);
        }
        else
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)"%d,", pCgcModTCnf->stPara.aucCid[i]);
        }
    }
    AtcAp_StrPrintf_AtcRspBuf((const char *)")\r\n");

    AtcAp_SendDataInd();
}
#endif

void AtcAp_MsgProc_COPS_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_COPS_R_CNF_STRU* pCopsRCnf;
    unsigned char            aucPlmnNum[7] = { 0 };

    pCopsRCnf = (ATC_MSG_COPS_R_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+COPS: %d", pCopsRCnf->stPara.ucMode);

    if(D_ATC_FLAG_TRUE == pCopsRCnf->stPara.ucPsAttachFlg || 1 == pCopsRCnf->stPara.ucMode)
    {
        AtcAp_WriteIntPara(pCopsRCnf->stPara.ucPlmnSelFlg, pCopsRCnf->stPara.ucFormat);

        if (pCopsRCnf->stPara.ucPlmnSelFlg)
        {
            AtcAp_IntegerToPlmn(pCopsRCnf->stPara.ulPlmnNum,aucPlmnNum);

            AtcAp_StrPrintf_AtcRspBuf((const char *)",%c%s%c",
                D_ATC_N_QUOTATION, aucPlmnNum, D_ATC_N_QUOTATION
                );
        }

        AtcAp_WriteIntPara(pCopsRCnf->stPara.ucPlmnSelFlg, pCopsRCnf->stPara.ucAct );
    }

    AtcAp_StrPrintf_AtcRspBuf("\r\n");
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_COPS_T_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_COPS_T_CNF_STRU* pCopsTCnf;
    unsigned char            aucPlmnNum[7] = { 0 };

    pCopsTCnf = (ATC_MSG_COPS_T_CNF_STRU*)pRecvMsg;
    
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+COPS: ");

    if (pCopsTCnf->stPara.ucPlmnSelFlg)
    {
        AtcAp_IntegerToPlmn(pCopsTCnf->stPara.ulPlmnNum, aucPlmnNum);
        g_AtcApInfo.stAtRspInfo.usRspLen +=AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)"(2,,,%c%s%c,9)",D_ATC_N_QUOTATION, aucPlmnNum, D_ATC_N_QUOTATION);
    }
    else
    {
        if (D_ATC_AP_COPS_MODE_MANUAL == pCopsTCnf->stPara.ucMode || D_ATC_AP_COPS_MODE_MANUAL_AUTO == pCopsTCnf->stPara.ucMode)
        {
            AtcAp_IntegerToPlmn(pCopsTCnf->stPara.ulPlmnNum, aucPlmnNum);

            g_AtcApInfo.stAtRspInfo.usRspLen +=AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)"(0,,,%c%s%c,9)",D_ATC_N_QUOTATION, aucPlmnNum, D_ATC_N_QUOTATION);
        }
        else
        {
            g_AtcApInfo.stAtRspInfo.usRspLen +=AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)"(0,,,,9)");
        }
    }

    g_AtcApInfo.stAtRspInfo.usRspLen +=AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
        (const unsigned char *)",,(0-4),(2)\r\n");
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGEREP_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CGEREP_R_CNF_STRU* pCgerepRCnf;

    pCgerepRCnf = (ATC_MSG_CGEREP_R_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CGEREP:%d\r\n", pCgerepRCnf->ucCgerepMode);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CCIOTOPT_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CCIOTOPT_R_CNF_STRU* pCciotoptRCnf = (ATC_MSG_CCIOTOPT_R_CNF_STRU*)pRecvMsg;
    
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CCIOTOPT: %d,%d,%d\r\n", 
        pCciotoptRCnf->stPara.ucCciotoptN, pCciotoptRCnf->stPara.ucSupptUeOpt, pCciotoptRCnf->stPara.ucPreferOpt);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CEDRXRDP_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CEDRXRDP_CNF_STRU* pCciotoptRCnf = (ATC_MSG_CEDRXRDP_CNF_STRU*)pRecvMsg;

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CEDRXRDP:%d", pCciotoptRCnf->stPara.ucActType);

    if(pCciotoptRCnf->stPara.ucActType != 0)
    {
        if (EMM_EDRX_INVALID_VALUE != pCciotoptRCnf->stPara.ucReqDRXValue)
        {
            AtcAp_Write4BitData(pCciotoptRCnf->stPara.ucReqDRXValue);

            if ((EMM_EDRX_INVALID_VALUE != pCciotoptRCnf->stPara.ucNWeDRXValue)
                && (EMM_PTW_INVALID_VALUE != pCciotoptRCnf->stPara.ucPagingTimeWin))
            {
                AtcAp_Write4BitData(pCciotoptRCnf->stPara.ucNWeDRXValue);
                AtcAp_Write4BitData(pCciotoptRCnf->stPara.ucPagingTimeWin);
            }
        }
    }
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)"\r\n");

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGEQOSRDP_Cnf(unsigned char* pRecvMsg)
{
    unsigned                    i;
    ATC_MSG_CGEQOSRDP_CNF_STRU* pCgeqosCnf = (ATC_MSG_CGEQOSRDP_CNF_STRU*)pRecvMsg;

    for (i = 0; i < pCgeqosCnf->stPara.ucValidNum; i++)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
            (const unsigned char *)"\r\n+CGEQOSRDP:%d,%d",
            pCgeqosCnf->stPara.aQosApnAmbrInfo[i].ucCid, 
            pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stQosInfo.ucQci);

        if ((((1 <= pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stQosInfo.ucQci) 
            && (4 >= pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stQosInfo.ucQci)) 
            || (65 == pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stQosInfo.ucQci)
            || (66 == pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stQosInfo.ucQci)
            || (75 == pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stQosInfo.ucQci)))
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf  + g_AtcApInfo.stAtRspInfo.usRspLen), 
                (const unsigned char *)",%d,%d,%d,%d",
                pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stQosInfo.ulDlGbr, 
                pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stQosInfo.ulUlGbr, 
                pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stQosInfo.ulDlMbr, 
                pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stQosInfo.ulUlMbr);

            if (ATC_AP_TRUE == pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stApnAmbrInfo.ucApnAmbrFlag)
            {
                g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf  + g_AtcApInfo.stAtRspInfo.usRspLen), 
                    (const unsigned char *)",%d,%d",
                    pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stApnAmbrInfo.ulDlApnAmbr, 
                    pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stApnAmbrInfo.ulUlApnAmbr);
            }
        }
        else
        {
            if (ATC_AP_TRUE == pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stApnAmbrInfo.ucApnAmbrFlag)
            {
                g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                    (const unsigned char *)",,,,,%d,%d",
                    pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stApnAmbrInfo.ulDlApnAmbr, 
                    pCgeqosCnf->stPara.aQosApnAmbrInfo[i].stApnAmbrInfo.ulUlApnAmbr);
            }
        }
    }

    if (0 != g_AtcApInfo.stAtRspInfo.usRspLen)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)"\r\n");
        AtcAp_SendDataInd();
    }
}

void AtcAp_MsgProc_CGEQOSRDP_T_Cnf(unsigned char* pRecvMsg)
{
    unsigned char                 i;
    ATC_MSG_CGEQOSRDP_T_CNF_STRU* pCgeqosrdpTCnf = (ATC_MSG_CGEQOSRDP_T_CNF_STRU*)pRecvMsg;

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CGEQOSRDP:(");
    for (i = 0; i < pCgeqosrdpTCnf->stPara.ucCidNum; i++)
    {
        if (i == (pCgeqosrdpTCnf->stPara.ucCidNum - 1))
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen,
                (const unsigned char *)"%d", (pCgeqosrdpTCnf->stPara.aucCid[i]));
        }
        else
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen,
                (const unsigned char *)"%d,",
                (pCgeqosrdpTCnf->stPara.aucCid[i]));
        }
    }
    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
        (const unsigned char *)")\r\n");

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CTZR_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CTZR_R_CNF_STRU*      pCtzrTCnf = (ATC_MSG_CTZR_R_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n+CTZR:%d\r\n", pCtzrTCnf->ucCtzrReport);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGCONTRDP_Cnf(unsigned char* pRecvMsg)
{
    unsigned char                    i;
    ATC_MSG_CGCONTRDP_CNF_STRU*      pCgcongrdpCnf = (ATC_MSG_CGCONTRDP_CNF_STRU*)pRecvMsg;
    unsigned char*                   pRespBuff;

    if(0 == pCgcongrdpCnf->stPara.ucValidNum)
    {
        return;
    }

    pRespBuff = (unsigned char*)AtcAp_Malloc(D_ATC_CGCONRRDP_MSG_MAX_LENGTH + 1);
    if(NULL == pRespBuff)
    {
        return;
    }
    
    for(i = 0; i < pCgcongrdpCnf->stPara.ucValidNum; i++)
    {
        switch(pCgcongrdpCnf->stPara.aucPdpDynamicInfo[i].ucPdpType)
        {
            case D_PDP_TYPE_IPV4:
            case D_PDP_TYPE_IPV6:
            case D_PDP_TYPE_NonIP:
                AtcAp_CGCONTRDP_Print(&pCgcongrdpCnf->stPara.aucPdpDynamicInfo[i], pCgcongrdpCnf->stPara.aucPdpDynamicInfo[i].ucPdpType, pRespBuff);
                break;
            default: //D_PDP_TYPE_IPV4V6   
                AtcAp_CGCONTRDP_Print(&pCgcongrdpCnf->stPara.aucPdpDynamicInfo[i], D_PDP_TYPE_IPV4, pRespBuff);
                AtcAp_CGCONTRDP_Print(&pCgcongrdpCnf->stPara.aucPdpDynamicInfo[i], D_PDP_TYPE_IPV6, pRespBuff);
                break;
        }
    }

    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf(pRespBuff + g_AtcApInfo.stAtRspInfo.usRspLen, (const unsigned char *)"\r\n");
    AtcAp_SendLongDataInd(&pRespBuff, D_ATC_CGCONRRDP_MSG_MAX_LENGTH);

    AtcAp_Free(pRespBuff);
}

void AtcAp_MsgProc_CGCONTRDP_T_Cnf(unsigned char* pRecvMsg)
{
    unsigned char                    i;
    ATC_MSG_CGCONTRDP_T_CNF_STRU*    pCgcongrdpTCnf = (ATC_MSG_CGCONTRDP_T_CNF_STRU*)pRecvMsg;

    for (i = 0; i < pCgcongrdpTCnf->stPara.ucCidNum ;i++)
    {
        if (0 == i)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
                (const unsigned char *)"\r\n+CGCONTRDP:(%d", pCgcongrdpTCnf->stPara.aucCid[i]);
        }
        else
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)",%d", pCgcongrdpTCnf->stPara.aucCid[i]);
        }
    }
    
    if(g_AtcApInfo.stAtRspInfo.usRspLen > 0)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
            (const unsigned char *)")\r\n");
        AtcAp_SendDataInd();
    }
}

void AtcAp_MsgProc_CEER_Cnf(unsigned char* pRecvMsg)
{
    const char*               errText;
    ATC_MSG_CEER_IND_STRU*    pCeerCnf = (ATC_MSG_CEER_IND_STRU*)pRecvMsg;
    
    if(D_FAIL_CAUSE_TYPE_NULL != pCeerCnf->ucType)
    {
        if(D_FAIL_CAUSE_TYPE_EMM == pCeerCnf->ucType)
        {
            errText = ATC_ConvertErrCode2Str(PEmmCauseTextTbl, D_ATC_EMM_CAUSE_TBL_SIZE, pCeerCnf->ucCause);
        }
        else
        {
            errText = ATC_ConvertErrCode2Str(PEsmCauseTextTbl, D_ATC_ESM_CAUSE_TBL_SIZE, pCeerCnf->ucCause);
        }
        AtcAp_StrPrintf_AtcRspBuf("\r\n+CEER:%s\r\n", errText);
        AtcAp_SendDataInd();
    }
}

void AtcAp_MsgProc_CIPCA_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CIPCA_R_CNF_STRU*    pCipcaRCnf = (ATC_MSG_CIPCA_R_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n+CIPCA:%d,%d\r\n", pCipcaRCnf->n, pCipcaRCnf->ucAttachWithOutPdn);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGAUTH_R_Cnf(unsigned char* pRecvMsg)
{
    unsigned char                 aucUserName[D_PCO_AUTH_MAX_LEN + 1];
    unsigned char                 aucPassword[D_PCO_AUTH_MAX_LEN + 1];
    unsigned char                 index;
    ATC_MSG_CGAUTH_R_CNF_STRU*    pCgauthRCnf = (ATC_MSG_CGAUTH_R_CNF_STRU*)pRecvMsg;

    for(index = 0; index < pCgauthRCnf->ucNum; index++)
    {
        AtcAp_MemSet(aucUserName, 0, D_PCO_AUTH_MAX_LEN + 1);
        AtcAp_MemSet(aucPassword, 0, D_PCO_AUTH_MAX_LEN + 1);
        AtcAp_MemCpy(aucUserName, pCgauthRCnf->stCgauth[index].aucUserName, D_PCO_AUTH_MAX_LEN);
        AtcAp_MemCpy(aucPassword, pCgauthRCnf->stCgauth[index].aucPassword, D_PCO_AUTH_MAX_LEN);

        AtcAp_StrPrintf_AtcRspBuf("\r\n+CGAUTH:%d,%d,%s,%s", pCgauthRCnf->stCgauth[index].ucAtCid, pCgauthRCnf->stCgauth[index].ucAuthProt, aucUserName, aucPassword);
    }

    if(0 != pCgauthRCnf->ucNum)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n");
        AtcAp_SendDataInd();
    }
}

void AtcAp_MsgProc_NPOWERCLASS_R_Cnf(unsigned char* pRecvMsg)
{
    unsigned char                      index;
    ATC_MSG_NPOWERCLASS_R_CNF_STRU*    pNpowerClassRCnf = (ATC_MSG_NPOWERCLASS_R_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n");
    for(index = 0; index < pNpowerClassRCnf->unNum; index++)
    {
        AtcAp_StrPrintf_AtcRspBuf("+NPOWERCLASS:%d,%d\r\n", pNpowerClassRCnf->stSupportBandInfo[index].ucBand, pNpowerClassRCnf->stSupportBandInfo[index].ucPowerClass);
    }

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NPOWERCLASS_T_Cnf(unsigned char* pRecvMsg)
{
    unsigned char                      index;
    ATC_MSG_NPOWERCLASS_T_CNF_STRU*    pNpowerClassTCnf = (ATC_MSG_NPOWERCLASS_T_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n+NPOWERCLASS:(");
    for(index = 0; index < pNpowerClassTCnf->unNum; index++)
    {
        if(index != 0)
        {
            AtcAp_StrPrintf_AtcRspBuf(",", pNpowerClassTCnf->aucBand[index]);
        }
        AtcAp_StrPrintf_AtcRspBuf("%d", pNpowerClassTCnf->aucBand[index]);
    }
    AtcAp_StrPrintf_AtcRspBuf("),(3,5,6)\r\n");
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NPTWEDRXS_R_Cnf(unsigned char* pRecvMsg)
{
    AtcAp_MsgProc_CEDRXS_R_Cnf(pRecvMsg);
}

void AtcAp_MsgProc_QEDRXCFG_R_Cnf(unsigned char* pRecvMsg)
{
    AtcAp_MsgProc_CEDRXS_R_Cnf(pRecvMsg);
}

void AtcAp_MsgProc_NCIDSTATUS_Cnf(unsigned char* pRecvMsg)
{
    unsigned char                 index;
    ATC_MSG_NCIDSTATUS_CNF_STRU*  pNcidStatusCnf = (ATC_MSG_NCIDSTATUS_CNF_STRU*)pRecvMsg;

    if(0 == pNcidStatusCnf->stCidStatus.ucNum)
    {
        return;
    }
    
    for(index = 0; index < pNcidStatusCnf->stCidStatus.ucNum; index++)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+NCIDSTATUS:%d,%d", pNcidStatusCnf->stCidStatus.aCidStaus[index].ucCid, pNcidStatusCnf->stCidStatus.aCidStaus[index].ucStatus);
        if(D_STATUS_BACKOFF == pNcidStatusCnf->stCidStatus.aCidStaus[index].ucStatus)
        {
            AtcAp_StrPrintf_AtcRspBuf(",%d", pNcidStatusCnf->stCidStatus.aCidStaus[index].ulBackOffVal);
        }
    }
    AtcAp_StrPrintf_AtcRspBuf("\r\n");
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NCIDSTATUS_R_Cnf(unsigned char* pRecvMsg)
{
    AtcAp_MsgProc_NCIDSTATUS_Cnf(pRecvMsg);
}

void AtcAp_MsgProc_NGACTR_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_NGACTR_R_CNF_STRU*    pNgactrRCnf = (ATC_MSG_NGACTR_R_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n+NGACTR:%d\r\n", pNgactrRCnf->ucNgactrN);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NPOPB_R_Cnf(unsigned char* pRecvMsg)
{
    unsigned char             ucIdx = 0, ucOperatorIndex = 0;
    unsigned char             aucPlmnNum[7] = {0};
    ATC_NPOB_PRE_BAND_STRU *  ptBand;
    ATC_MSG_NPOPB_R_CNF_STRU* pNpopbRCnf    = (ATC_MSG_NPOPB_R_CNF_STRU*)pRecvMsg;

    for (ucOperatorIndex = 0; ucOperatorIndex < NVM_MAX_OPERATOR_NUM; ucOperatorIndex++)
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+NPOPB:%d,(", ucOperatorIndex);
        
        for(ucIdx = 0; ucIdx < NVM_MAX_OPERATOR_PLMN_NUM; ucIdx++)
        {
            if(0 == pNpopbRCnf->stNpobList[ucOperatorIndex].aulPlmn[ucIdx])
            {
                break;
            }
            
            if (0 != ucIdx)
            {
                AtcAp_StrPrintf_AtcRspBuf((const char *)",");
            }
            
            AtcAp_IntegerToPlmn(pNpopbRCnf->stNpobList[ucOperatorIndex].aulPlmn[ucIdx], aucPlmnNum);
            AtcAp_StrPrintf_AtcRspBuf((const char *)"%s", aucPlmnNum);
        }
        
        AtcAp_StrPrintf_AtcRspBuf((const char *)"),(");
        
        for (ucIdx = 0; ucIdx < NVM_MAX_PRE_BAND_NUM; ucIdx++)
        {
            ptBand = &(pNpopbRCnf->stNpobList[ucOperatorIndex].aPreBand[ucIdx]);
            if(0 == ptBand->ucSuppBand)
            {
                break;
            }
 
            if (0 != ucIdx)
            {
               AtcAp_StrPrintf_AtcRspBuf((const char *)",");
            }
            
            AtcAp_StrPrintf_AtcRspBuf((const char *)"(%d, %d, %d)", ptBand->ucSuppBand, ptBand->ulStartFreq, ptBand->usOffset);
        }
        AtcAp_StrPrintf_AtcRspBuf((const char *)")");
    }

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n");
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NIPINFO_R_Cnf(unsigned char* pRecvMsg)
{
     ATC_MSG_NIPINFO_R_CNF_STRU* pNipInfoR  = (ATC_MSG_NIPINFO_R_CNF_STRU*)pRecvMsg;

     AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+NIPINFO:%d\r\n", pNipInfoR->ucN);
     AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NQPODCP_Cnf(unsigned char* pRecvMsg)
{
    unsigned char             i;
    ATC_MSG_NQPODCP_CNF_STRU* pNqpodcpCnf;

    pNqpodcpCnf      = (ATC_MSG_NQPODCP_CNF_STRU*)pRecvMsg;    
    if (pNqpodcpCnf->stSequence.ucSequenceNum == 0)
    {
        return;
    }

    if(pNqpodcpCnf->stSequence.ucSequenceNum > (D_ATC_RSP_MAX_BUF_SIZE - 4 - 9) / 4) //"\r\n+NQPODCP:xxx,xxx\r\n"
    {
        AtcAp_Free(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
        g_AtcApInfo.stAtRspInfo.aucAtcRspBuf = (unsigned char*)AtcAp_Malloc(D_ATC_NQPODCP_IND_MSG_MAX_LENGTH);
    }

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n%s","+NQPODCP:");
    for( i = 0; i < (pNqpodcpCnf->stSequence.ucSequenceNum); i++ )
    {  
        if(i != pNqpodcpCnf->stSequence.ucSequenceNum-1)
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)"%d,", pNqpodcpCnf->stSequence.Sequence[i]);
        }
        else
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)"%d\r\n", pNqpodcpCnf->stSequence.Sequence[i]);
        }
    }

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NQPNPD_Cnf(unsigned char* pRecvMsg)
{
    unsigned char             i;
    ATC_MSG_NQPNPD_CNF_STRU* pNqpnpdCnf;

    pNqpnpdCnf      = (ATC_MSG_NQPNPD_CNF_STRU*)pRecvMsg;    
    if (pNqpnpdCnf->stSequence.ucSequenceNum == 0)
    {
        return;
    }

    if(pNqpnpdCnf->stSequence.ucSequenceNum > (D_ATC_RSP_MAX_BUF_SIZE - 4 - 9) / 4) //"\r\n+NQPODCP:xxx,xxx\r\n"
    {
        AtcAp_Free(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
        g_AtcApInfo.stAtRspInfo.aucAtcRspBuf = (unsigned char*)AtcAp_Malloc(D_ATC_NQPODCP_IND_MSG_MAX_LENGTH);
    }

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n%s","+NQPNPD:");
    for( i = 0; i < (pNqpnpdCnf->stSequence.ucSequenceNum); i++ )
    {  
        if(i != pNqpnpdCnf->stSequence.ucSequenceNum-1)
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)"%d,", pNqpnpdCnf->stSequence.Sequence[i]);
        }
        else
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)"%d\r\n", pNqpnpdCnf->stSequence.Sequence[i]);
        }
    }

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CNEC_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CNEC_R_CNF_STRU* pCnecRCnf = (ATC_MSG_CNEC_R_CNF_STRU*)pRecvMsg;
    
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CNEC:%d\r\n", pCnecRCnf->ucN);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NCPCDPR_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_NCPCDPR_R_CNF_STRU* pNcpcdprRCnf = (ATC_MSG_NCPCDPR_R_CNF_STRU*)pRecvMsg;
    unsigned char               i;

    if(pNcpcdprRCnf->ucNum == 0)
    {
        return;
    }

    for(i = 0; i < pNcpcdprRCnf->ucNum; i++)
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+NCPCDPR:%d,%d", 
            pNcpcdprRCnf->stPdpCtxDnsSet[i].ucParam, pNcpcdprRCnf->stPdpCtxDnsSet[i].ucState);
    }
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n");
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CEID_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CEID_CNF_STRU* pCeidCnf  = (ATC_MSG_CEID_CNF_STRU*)pRecvMsg;
    unsigned char*         pDataBuff;
    
    if(0 == pCeidCnf->ucLen)
    {
        return;
    }

    pDataBuff = (unsigned char*)AtcAp_Malloc(pCeidCnf->ucLen * 2 + 1);
    AtcAp_HexToAsc(pCeidCnf->ucLen, pDataBuff, pCeidCnf->aucEid);
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n%s\r\n", pDataBuff);
    AtcAp_Free(pDataBuff);
    AtcAp_SendDataInd();
}

#ifdef NBIOT_SMS_FEATURE
void AtcAp_MsgProc_CSMS_Cnf(unsigned char* pRecvMsg)
{
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CSMS:1,1,0\r\n");
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CSMS_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CSMS_R_CNF_STRU*    pCsmsRCnf = (ATC_MSG_CSMS_R_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CSMS:%d,1,1,0\r\n", pCsmsRCnf->ucMsgService);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CMGC_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CMGC_CNF_STRU*    pCmgcInd = (ATC_MSG_CMGC_CNF_STRU*)pRecvMsg;
    
    if (0 == pCmgcInd->ucTpduLen)
    {
        AtcAp_StrPrintf_AtcRspBuf( (const char *)"\r\n+CMGC:%d\r\n", pCmgcInd->ucTpmr);
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.ucHexStrLen = pCmgcInd->ucTpduLen;
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CMGC:%d,%x\r\n", pCmgcInd->ucTpmr, pCmgcInd->aucTpdu);
    }
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CMT_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_CMT_IND_STRU*    pCmtInd = (ATC_MSG_CMT_IND_STRU*)pRecvMsg;

    g_AtcApInfo.stAtRspInfo.ucHexStrLen = pCmtInd->ucDataLen;
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CMT:\"HEX\",%d\r\n%x\r\n",pCmtInd->ucPduLen, pCmtInd->aucData);

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CMGF_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CMGF_R_CNF_STRU*    pCmgfRCnf = (ATC_MSG_CMGF_R_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CMGF:%d\r\n", pCmgfRCnf->ucValue);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CSCA_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CSCA_R_CNF_STRU*    pCscaRCnf      = (ATC_MSG_CSCA_R_CNF_STRU*)pRecvMsg;
    unsigned char               aucScaData[30] = { 0 };

    AtcAp_CSCA_ConvertScaByte2Str(pCscaRCnf->aucSca, pCscaRCnf->ucScaLen, aucScaData);
    
    if(0 == pCscaRCnf->ucToSca)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+CSCA:\"%s\"\r\n", aucScaData);
    }
    else
    {
#if 0
        if(0x90 == (pCscaRCnf->ucToSca & 0xF0))
        {
            AtcAp_StrPrintf_AtcRspBuf("\r\n+CSCA:\"+%s\",%d\r\n", aucScaData, pCscaRCnf->ucToSca);
        }
        else
        {
            AtcAp_StrPrintf_AtcRspBuf("\r\n+CSCA:\"%s\",%d\r\n", aucScaData, pCscaRCnf->ucToSca);
        }
#endif
        AtcAp_StrPrintf_AtcRspBuf("\r\n+CSCA:\"%s\",%d\r\n", aucScaData, pCscaRCnf->ucToSca);
    }
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CMMS_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CMMS_R_CNF_STRU*    pCmmsRCnf = (ATC_MSG_CMMS_R_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n+CMMS:%d\r\n", pCmmsRCnf->ucCmmsN);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CMGS_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CMGS_CNF_STRU*    pCmgsInd = (ATC_MSG_CMGS_CNF_STRU*)pRecvMsg;

    if (0 == pCmgsInd->ucTpduLen)
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CMGS:%d\r\n", pCmgsInd->ucTpmr);
    }
    else
    {
        g_AtcApInfo.stAtRspInfo.ucHexStrLen = pCmgsInd->ucTpduLen;
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CMGS:%d,%x\r\n", pCmgsInd->ucTpmr, pCmgsInd->aucTpdu);
    }
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_SMS_PDU_Ind(unsigned char* pRecvMsg)
{
    AtcAp_StrPrintf_AtcRspBuf("\r\n> ");
    AtcAp_SendDataInd();
}
#endif

void AtcAp_MsgProc_SIMST_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_SIMST_IND_STRU*    pSimstInd = (ATC_MSG_SIMST_IND_STRU*)pRecvMsg;

    //wjb 20230130: bc260y no sim state URC
#if (!VER_QUCTL260)
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n^SIMST:%d\r\n", pSimstInd->ucSimStatus);
#endif

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NPIN_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_NPIN_CNF_STRU* pNpingCnf = (ATC_MSG_NPIN_CNF_STRU*)pRecvMsg;

    if (0x9000 == pNpingCnf->usResult || 0x9100 == (pNpingCnf->usResult & 0xff00))
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+NPIN:OK\r\n");
    }
    else
    {
        if(0x63C0 == (pNpingCnf->usResult & 0xfff0))
        {
            AtcAp_StrPrintf_AtcRspBuf("\r\n+NPIN:ERROR wrong PIN %d\r\n", pNpingCnf->usResult & 0x000f);
        }
        else
        {
            AtcAp_StrPrintf_AtcRspBuf("\r\n+NPIN:ERROR\r\n");
        }
    }
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CPIN_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CPIN_R_CNF_STRU* pCpinRCnf = (ATC_MSG_CPIN_R_CNF_STRU*)pRecvMsg;

    if(pCpinRCnf->ucPinStatus >= (sizeof(ATC_PinCodeTbl) / sizeof(ATC_PinCodeTbl[0])))
    {
        return;
    }

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CPIN: %s\r\n", ATC_PinCodeTbl[pCpinRCnf->ucPinStatus]);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CLCK_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CLCK_CNF_STRU* pClckCnf = (ATC_MSG_CLCK_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CLCK:%d\r\n", pClckCnf->ucStatus);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_PINSTATUS_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_PIN_STATUS_IND_STRU* pPinStatusInd = (ATC_MSG_PIN_STATUS_IND_STRU*)pRecvMsg;
    const char*                  pPinStatusStr[3] = { "SIM PIN", "SIM PUK", "PUK BLOCKED" };

    if(pPinStatusInd->ucPinStatus > D_ATC_PIN_STATUS_PUK_BLOCK)
    {
        return;
    }

    if(pPinStatusInd->ucPinStatus == D_ATC_PIN_STATUS_PIN_REQUIRED
        || pPinStatusInd->ucPinStatus == D_ATC_PIN_STATUS_PUK_REQUIRED)
    {
        AtcAp_StrPrintf_AtcRspBuf("+CPIN: %s\r\n", pPinStatusStr[pPinStatusInd->ucPinStatus]);    
    }
    else
    {
        AtcAp_StrPrintf_AtcRspBuf("+NUSIM:%s\r\n", pPinStatusStr[pPinStatusInd->ucPinStatus]);    
    }
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CPINR_Cnf(unsigned char* pRecvMsg)
{
    unsigned char             index;
    ATC_MSG_CPINR_CNF_STRU*   pPinRetriesCnf = (ATC_MSG_CPINR_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n");
    for(index = 0; index < pPinRetriesCnf->ucNum; index++)
    {
        if(pPinRetriesCnf->aPinRetires[index].ucPinType == D_ATC_CPINR_TYPE_PIN1)
        {
            AtcAp_StrPrintf_AtcRspBuf("+CPINR: %s,%d,3\r\n", "\"SIM PIN\"", pPinRetriesCnf->aPinRetires[index].ucRetriesNum);
        }
        if(pPinRetriesCnf->aPinRetires[index].ucPinType == D_ATC_CPINR_TYPE_PUK1)
        {
            AtcAp_StrPrintf_AtcRspBuf("+CPINR: %s,%d,10\r\n", "\"SIM PUK\"", pPinRetriesCnf->aPinRetires[index].ucRetriesNum);
        }
#if 0	//not need on document		
        if(pPinRetriesCnf->aPinRetires[index].ucPinType == D_ATC_CPINR_TYPE_PIN2)
        {
            AtcAp_StrPrintf_AtcRspBuf("+CPINR:%s,%d,3\r\n", "SIM PIN2", pPinRetriesCnf->aPinRetires[index].ucRetriesNum);
        }
        if(pPinRetriesCnf->aPinRetires[index].ucPinType == D_ATC_CPINR_TYPE_PUK2)
        {
            AtcAp_StrPrintf_AtcRspBuf("+CPINR:%s,%d,10\r\n", "SIM PUK2", pPinRetriesCnf->aPinRetires[index].ucRetriesNum);
        }
#endif		
    }
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CRTDCP_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_CRTDCP_IND_STRU     tCrtdcpInd = { 0 };
    unsigned char*              pInd;

    AtcAp_MemCpy((unsigned char*)&tCrtdcpInd, pRecvMsg, offsetof(ATC_MSG_CRTDCP_IND_STRU, usLen) + 2);
    tCrtdcpInd.pucReportData = pRecvMsg + offsetof(ATC_MSG_CRTDCP_IND_STRU, usLen) + 2;
	if(1 == tCrtdcpInd.ucNonIpDataFlg) 
	{
	    AtcAp_MsgProc_NRNPDM_Ind(tCrtdcpInd.ucNrnpdmRepValue, tCrtdcpInd.ucAtCid,tCrtdcpInd.usLen,tCrtdcpInd.pucReportData);
	}

    if(0 == tCrtdcpInd.ucCrtdcpRepValue)
    {
        return;
    }

    if (0 != tCrtdcpInd.usLen)
    {
        pInd = (unsigned char *)AtcAp_Malloc(D_ATC_CRTDCP_IND_LENGTH + tCrtdcpInd.usLen*2 + 1);
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)pInd, (const unsigned char *)"\r\n+CRTDCP:%d,%d,%c", tCrtdcpInd.ucAtCid, tCrtdcpInd.usLen, D_ATC_N_QUOTATION);

        AtcAp_HexToAsc(tCrtdcpInd.usLen, pInd + g_AtcApInfo.stAtRspInfo.usRspLen, tCrtdcpInd.pucReportData);
        g_AtcApInfo.stAtRspInfo.usRspLen += tCrtdcpInd.usLen*2;

        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)pInd + g_AtcApInfo.stAtRspInfo.usRspLen, (const unsigned char *)"%c\r\n", D_ATC_N_QUOTATION);
        AtcAp_SendLongDataInd(&pInd, D_ATC_CRTDCP_IND_LENGTH + tCrtdcpInd.usLen*2);
        
        AtcAp_Free(pInd);
    } 
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
            (const unsigned char *)"\r\n+CRTDCP:%d,%d,%c%c\r\n",
            tCrtdcpInd.ucAtCid, 0, D_ATC_N_QUOTATION, D_ATC_N_QUOTATION);
        AtcAp_SendDataInd();
    }
}

void AtcAp_MsgProc_CGAPNRC_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_CGAPNRC_IND_STRU*  pCgapnrcInd = (ATC_MSG_CGAPNRC_IND_STRU*)pRecvMsg;
    
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
                    (const unsigned char *)"\r\n+CGAPNRC: %d,%d,%d,%d,%d\r\n",
                    pCgapnrcInd->stEpsApnRateCtl.ucCid, 
                    pCgapnrcInd->stEpsApnRateCtl.ucAdditionExcepReportFlg,
                    pCgapnrcInd->stEpsApnRateCtl.ulMaxUplinkRate,
                    pCgapnrcInd->stEpsApnRateCtl.ulUplinkTimeUnit,
                    pCgapnrcInd->stEpsApnRateCtl.usMaxUplinkMsgSize);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGEV_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_CGEV_IND_STRU*    pCgevInd = (ATC_MSG_CGEV_IND_STRU*)pRecvMsg;

    if(pCgevInd->ucCgerepMode == 0)
    {
        return;
    }
    
    switch (pCgevInd->ucCgevEventId)
    {
    /* +CGEV: NW DETACH */
    case D_ATC_CGEV_NW_DETACH:
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:NW DETACH");
        break;
    /* +CGEV: ME DETACH */
    case D_ATC_CGEV_ME_DETACH:
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:ME DETACH");
        break; 
    /* +CGEV: ME PDN ACT <cid>[,<reason>[,<cid_other>]][,<WLAN_Offload>] */
    case D_ATC_CGEV_ME_PDN_ACT:
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:ME PDN ACT %d", pCgevInd->stCgevPara.ucCid );
        if (0xFF != pCgevInd->stCgevPara.ucReason)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)",%d",pCgevInd->stCgevPara.ucReason);
        }
        break;
    /* +CGEV: NW ACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>] */
    case D_ATC_CGEV_NW_ACT:
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:NW ACT %d,%d,%d",
            pCgevInd->stCgevPara.ucPcid, pCgevInd->stCgevPara.ucCid, pCgevInd->stCgevPara.ucEventType);
        break;
    /* +CGEV: ME ACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>] */
    case D_ATC_CGEV_ME_ACT:
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:ME ACT %d,%d,%d",
            pCgevInd->stCgevPara.ucPcid, pCgevInd->stCgevPara.ucCid, pCgevInd->stCgevPara.ucEventType);
        break;
   /* +CGEV: NW PDN DEACT <cid>[,<WLAN_Offload>] */
    case D_ATC_CGEV_NW_PDN_DEACT:
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:NW PDN DEACT %d", pCgevInd->stCgevPara.ucCid);
        break;
   /* +CGEV: ME PDN DEACT <cid>[,<WLAN_Offload>] */
    case D_ATC_CGEV_ME_PDN_DEACT:
       g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:ME PDN DEACT %d", pCgevInd->stCgevPara.ucCid);
        break;
    /* +CGEV: NW DEACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>] */
    case D_ATC_CGEV_NW_DEACT:
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:NW DEACT %d,%d,%d",
            pCgevInd->stCgevPara.ucPcid, pCgevInd->stCgevPara.ucCid, pCgevInd->stCgevPara.ucEventType);
        break;
    /* +CGEV: NW DEACT <p_cid>, <cid>, <event_type>[,<WLAN_Offload>] */
    case D_ATC_CGEV_ME_DEACT :
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:ME DEACT %d,%d,%d",
            pCgevInd->stCgevPara.ucPcid, pCgevInd->stCgevPara.ucCid, pCgevInd->stCgevPara.ucEventType);
        break;
    /* +CGEV: NW MODIFY <cid>, <change_reason>, <event_type>[,<WLAN_Offload>] */
    case D_ATC_CGEV_NW_MODIFY:
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:NW MODIFY %d,%d,%d",
            pCgevInd->stCgevPara.ucCid, pCgevInd->stCgevPara.ucChangeReason, pCgevInd->stCgevPara.ucEventType);
        break;
    /* +CGEV: ME MODIFY <cid>, <change_reason>, <event_type>[,<WLAN_Offload>] */
    case D_ATC_CGEV_ME_MODIFY:
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:ME MODIFY %d,%d,%d",
            pCgevInd->stCgevPara.ucCid, pCgevInd->stCgevPara.ucChangeReason, pCgevInd->stCgevPara.ucEventType);
        break;
    /* +CGEV: OOS */
    case D_ATC_CGEV_OOS:
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:OOS");
        break;
    /* +CGEV: IS */
    case D_ATC_CGEV_IS:
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\n+CGEV:IS");
        break;
    default:
        break;
    }

    if (g_AtcApInfo.stAtRspInfo.usRspLen > 0)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)"\r\n");
        AtcAp_SendDataInd();
    }
}

void AtcAp_MsgProc_CEREG_Ind(unsigned char* pRecvMsg)
{
    unsigned char           ucERegSta;
    unsigned char           ucERegMode;
    unsigned char           aucStr[9]   = {0};
    unsigned char           ucTemp      = 0;
    unsigned char           i;
    ATC_MSG_CEREG_IND_STRU* pCeregInd;

    pCeregInd = (ATC_MSG_CEREG_IND_STRU*)pRecvMsg;
    
    ucERegSta = pCeregInd->stPara.ucEPSRegStatus;
    ucERegMode = pCeregInd->stPara.ucIndPara;

    if(pCeregInd->stPara.ucOnlyUserRegEventIndFlg == 1)
    {
        return;
    }

    //<n>,<stat>
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CEREG:%d", pCeregInd->stPara.ucEPSRegStatus);

    //<tac>,<ci>,<Act>
    if (1 < ucERegMode)
    {
        //[<tac>]two byte tracking area code in hexadecimal format 
        if (pCeregInd->stPara.OP_TacLac)
        {
            g_AtcApInfo.stAtRspInfo.ucHexStrLen = 4;
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)",%h",pCeregInd->stPara.usTacOrLac);
        }
        else
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)",");
        }
        //[<ci>] four byte E-UTRAN cell ID in hexadecimal format
        if (pCeregInd->stPara.OP_CellId)
        {
            g_AtcApInfo.stAtRspInfo.ucHexStrLen = 8;
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)",%h",pCeregInd->stPara.ulCellId);
        }
        else
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)",");
        }
        //[<Act>]
        AtcAp_WriteIntPara_M(pCeregInd->stPara.OP_Act, pCeregInd->stPara.ucAct, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
    }
    
    //[<cause_type>][<reject_cause>]
    if(3 == ucERegMode)
    {
        AtcAp_WriteIntPara_M(pCeregInd->stPara.OP_CauseType, pCeregInd->stPara.ucCauseType, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
        AtcAp_WriteIntPara_M(pCeregInd->stPara.OP_RejectCause, pCeregInd->stPara.ucRejectCause, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
    }
    if (5 == ucERegMode)
    {
        AtcAp_WriteIntPara_M(pCeregInd->stPara.OP_CauseType, pCeregInd->stPara.ucCauseType, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
        AtcAp_WriteIntPara_M(pCeregInd->stPara.OP_RejectCause, pCeregInd->stPara.ucRejectCause, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
    }
    else if(4 == ucERegMode)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)",,");
    }
    
    //<Active-Time><Periodic-TAU>
    if (3 < ucERegMode)
    {
        if(0 == ucERegSta || 2 == ucERegSta || 3 == ucERegSta || 4 == ucERegSta)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)",,");
        }
        else
        {
            //<Active-Time>
            if (pCeregInd->stPara.OP_ActiveTime)
            {
                for (i  = 0; i < 8; i++)
                {
                    ucTemp = (0x80 >> i)& pCeregInd->stPara.ucActiveTime;
                    aucStr[i] = (ucTemp >> (7-i)) + 0x30;
                }
            }
            #if VER_QUECTEL
            AtcAp_WriteStrPara_M_NoQuotation(pCeregInd->stPara.OP_ActiveTime, aucStr );
            #else
            AtcAp_WriteStrPara_M(pCeregInd->stPara.OP_ActiveTime, aucStr );
            #endif
            //<Periodic-TAU>
            if (pCeregInd->stPara.OP_PeriodicTAU)
            {
                for (i  = 0; i < 8; i++)
                {
                    ucTemp = (0x80 >> i)& pCeregInd->stPara.ucPeriodicTAU;
                    aucStr[i] = (ucTemp >> (7-i)) + 0x30;
                }
            }
            #if VER_QUECTEL
            AtcAp_WriteStrPara_M_NoQuotation(pCeregInd->stPara.OP_PeriodicTAU, aucStr );
            #else
            AtcAp_WriteStrPara_M(pCeregInd->stPara.OP_PeriodicTAU, aucStr );
            #endif
        }
    }

    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)"\r\n");
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CSCON_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_CSCON_IND_STRU* pCsconInd = (ATC_MSG_CSCON_IND_STRU*)pRecvMsg;

    if(0 == pCsconInd->ucCsconN)
    {
        return;
    }

    //<n>,<mode>g_AtcMng.ucCsconN 
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CSCON:%d", pCsconInd->stPara.ucMode);

    if(pCsconInd->stPara.OP_State == 1)
    {
        if (1 < pCsconInd->ucCsconN)
        {
            //[<state>]
            AtcAp_WriteIntPara(pCsconInd->stPara.OP_State, pCsconInd->stPara.ucState);
        }

        if (2 < pCsconInd->ucCsconN)
        {
            //[<access>]
            AtcAp_WriteIntPara(pCsconInd->stPara.OP_Access, pCsconInd->stPara.ucAccess);
        }
    }

    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)"\r\n");
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NPTWEDRXP_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_NPTWEDRXP_IND_STRU* pNptwedrxpInd = (ATC_MSG_NPTWEDRXP_IND_STRU*)pRecvMsg;
    unsigned char               aucStr[5]     = { 0 };

    if(2 != pNptwedrxpInd->stPara.ucNptwEDrxMode)
    {
        return;
    }
    
    AtcAp_StrPrintf_AtcRspBuf("\r\n+NPTWEDRXP:%d", pNptwedrxpInd->stPara.ucActType);
    AtcAp_ConvertByte2BitStr(pNptwedrxpInd->stPara.ucReqPagingTimeWin, 4, aucStr);
    AtcAp_StrPrintf_AtcRspBuf(",\"%s\"", aucStr);

    AtcAp_ConvertByte2BitStr(pNptwedrxpInd->stPara.ucReqDRXValue, 4, aucStr);
    AtcAp_StrPrintf_AtcRspBuf(",\"%s\"", aucStr);

    if(0 != pNptwedrxpInd->stPara.ucActType)
    {
        AtcAp_ConvertByte2BitStr(pNptwedrxpInd->stPara.ucNWeDRXValue, 4, aucStr);
        AtcAp_StrPrintf_AtcRspBuf(",\"%s\"", aucStr);

        AtcAp_ConvertByte2BitStr(pNptwedrxpInd->stPara.ucPagingTimeWin, 4, aucStr);
        AtcAp_StrPrintf_AtcRspBuf(",\"%s\"", aucStr);      
    }    
    AtcAp_StrPrintf_AtcRspBuf("\r\n");
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CEDRXP_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_NPTWEDRXP_IND_STRU* pCedrxpInd = (ATC_MSG_NPTWEDRXP_IND_STRU*)pRecvMsg;
    unsigned char i = 0;
    unsigned char ucTemp =0;
    unsigned char aucStr[5] = {0};

    if(2 != pCedrxpInd->stPara.ucNptwEDrxMode)
    {
        return;
    }

    AtcAp_StrPrintf_AtcRspBuf("\r\n+CEDRXP: %d", pCedrxpInd->stPara.ucActType);
    
    AtcAp_ConvertByte2BitStr(pCedrxpInd->stPara.ucReqDRXValue, 4, aucStr);
    AtcAp_StrPrintf_AtcRspBuf(",\"%s\"", aucStr);
    
    if(0 != pCedrxpInd->stPara.ucActType)
    {
        AtcAp_ConvertByte2BitStr(pCedrxpInd->stPara.ucNWeDRXValue, 4, aucStr);
        AtcAp_StrPrintf_AtcRspBuf(",\"%s\"", aucStr);

        AtcAp_ConvertByte2BitStr(pCedrxpInd->stPara.ucPagingTimeWin, 4, aucStr);
        AtcAp_StrPrintf_AtcRspBuf(",\"%s\"", aucStr);      
    }    
    AtcAp_StrPrintf_AtcRspBuf("\r\n");

    AtcAp_SendDataInd();

    return ;
}

void AtcAp_MsgProc_CCIOTOPTI_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_CCIOTOPTI_IND_STRU* pCciotopti = (ATC_MSG_CCIOTOPTI_IND_STRU*)pRecvMsg;

    if(pCciotopti->ucCciotoptN == 0 || pCciotopti->ucCciotoptN == 3)
    {
        return;
    }

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
        (const unsigned char *)"\r\n+CCIOTOPTI: %d\r\n", pCciotopti->ucSupptNwOpt);
    AtcAp_SendDataInd();
}



void AtcAp_MsgProc_L2_THP_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_L2_THP_IND_STRU*    pL2ThpInd = (ATC_MSG_L2_THP_IND_STRU*)pRecvMsg;

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, 
                                    (const unsigned char *)"\r\n%s%d\r\n%s%d\r\n%s%d\r\n%s%d\r\n", 
                                    "+L2_THP,RLC UL:", pL2ThpInd->ulRlcUl, 
                                    "+L2_THP,RLC DL:", pL2ThpInd->ulRlcDl, 
                                    "+L2_THP,MAC UL:", pL2ThpInd->ulMacUl, 
                                    "+L2_THP,MAC DL:", pL2ThpInd->ulMacDl);

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NCCID_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_UICCID_CNF_STRU*    pUccidCnf = (ATC_MSG_UICCID_CNF_STRU*)pRecvMsg;

    if(g_AtcApInfo.ucQccidFlg == 1)
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+QCCID: %s\r\n", pUccidCnf->aucICCIDstring);
        g_AtcApInfo.ucQccidFlg = 0;
    }
    else
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+NCCID:%s\r\n", pUccidCnf->aucICCIDstring);
    }

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_XYIPDNS_Ind(unsigned char* pRecvMsg)
{
    /*API_EPS_PDN_IPDNS_IND_STRU*  pInd = &(((ATC_MSG_XYIPDNS_IND_STRU*)pRecvMsg)->stPara);

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+XYIPDNS:1,%d", pInd->ucCid);
   
    if(D_PDP_TYPE_IPV4 == pInd->ucPdpType)
    {  
        AtcAp_StrPrintf_AtcRspBuf((const char *)",\"%s\"", "IP");

        AtcAp_OutputAddr_IpDns(ATC_AP_FALSE, (unsigned char*)&pInd->aucIPv4Addr[0], (unsigned char*)NULL);
        AtcAp_StrPrintf_AtcRspBuf((const char *)",\"\"");
        
        AtcAp_OutputAddr_IpDns(ATC_AP_FALSE, 
            (unsigned char*)(pInd->stDnsAddr.ucPriDnsAddr_IPv4Flg ? pInd->stDnsAddr.ucPriDnsAddr_IPv4 : NULL),
            (unsigned char*)NULL);
        AtcAp_OutputAddr_IpDns(ATC_AP_FALSE, 
            (unsigned char*)(pInd->stDnsAddr.ucSecDnsAddr_IPv4Flg ? pInd->stDnsAddr.ucSecDnsAddr_IPv4 : NULL), 
            (unsigned char*)NULL);
    }
    else if(D_PDP_TYPE_IPV6 == pInd->ucPdpType)
    {  
        AtcAp_StrPrintf_AtcRspBuf((const char *)",\"%s\"", "IPV6");

        AtcAp_OutputAddr_IpDns(ATC_AP_FALSE, (unsigned char*)NULL, pInd->aucIPv6Addr);
        AtcAp_StrPrintf_AtcRspBuf((const char *)",\"\"");
        
        AtcAp_OutputAddr_IpDns(ATC_AP_FALSE, 
            (unsigned char*)NULL, 
            (unsigned char*)(pInd->stDnsAddr.ucPriDnsAddr_IPv6Flg ? pInd->stDnsAddr.ucPriDnsAddr_IPv6 : NULL));
        AtcAp_OutputAddr_IpDns(ATC_AP_FALSE, 
            (unsigned char*)NULL, 
            (unsigned char*)(pInd->stDnsAddr.ucSecDnsAddr_IPv6Flg ? pInd->stDnsAddr.ucSecDnsAddr_IPv6 : NULL));
    }
    else if(D_PDP_TYPE_IPV4V6 == pInd->ucPdpType)
    {  
        AtcAp_StrPrintf_AtcRspBuf((const char *)",\"%s\"", "IPV4V6");

        AtcAp_OutputAddr_IpDns(ATC_AP_TRUE, pInd->aucIPv4Addr, pInd->aucIPv6Addr);
        AtcAp_StrPrintf_AtcRspBuf((const char *)",\"\"");
        
        if(ATC_AP_TRUE == pInd->stDnsAddr.ucPriDnsAddr_IPv4Flg || ATC_AP_TRUE == pInd->stDnsAddr.ucPriDnsAddr_IPv6Flg)
        {
            AtcAp_OutputAddr_IpDns(ATC_AP_TRUE, pInd->stDnsAddr.ucPriDnsAddr_IPv4, pInd->stDnsAddr.ucPriDnsAddr_IPv6);
        }
        else
        {
            AtcAp_OutputAddr_IpDns(ATC_AP_TRUE, (unsigned char*)NULL, (unsigned char*)NULL);
        }

        if(ATC_AP_TRUE == pInd->stDnsAddr.ucSecDnsAddr_IPv4Flg || ATC_AP_TRUE == pInd->stDnsAddr.ucSecDnsAddr_IPv6Flg)
        {
            AtcAp_OutputAddr_IpDns(ATC_AP_TRUE, pInd->stDnsAddr.ucSecDnsAddr_IPv4, pInd->stDnsAddr.ucSecDnsAddr_IPv6);
        }
        else
        {
            AtcAp_OutputAddr_IpDns(ATC_AP_TRUE, (unsigned char*)NULL, (unsigned char*)NULL);
        }

    }
    else
    {  
        AtcAp_StrPrintf_AtcRspBuf((const char *)",\"%s\"", "Non-IP");
    }

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n");

    AtcAp_SendDataInd();*/
}

void AtcAp_MsgProc_MALLOC_ADDR_Ind(unsigned char* pRecvMsg)
{
    /*ATC_MSG_MALLOC_ADDR_IND_STRU*    pMallocAddrInd = (ATC_MSG_MALLOC_ADDR_IND_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n+DSPNVRAM:0x%x,%d\r\n",  (unsigned long)pMallocAddrInd->ulAddr, pMallocAddrInd->usNvRamLen);
    AtcAp_SendDataInd();*/
}

void AtcAp_MsgProc_IPSN_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_IPSN_IND_STRU*    pIpsnInd = (ATC_MSG_IPSN_IND_STRU*)pRecvMsg;

    /*if(pIpsnInd->usIpSn != 0)
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+IPSEQUENCE:%d,%d\r\n",pIpsnInd->usIpSn, pIpsnInd->ucStatus);
        AtcAp_SendDataInd();
    }*/

    if(ATC_AP_TRUE == pIpsnInd->ucFlowCtrlStatusFlg)
    {
        if(1 == pIpsnInd->ucFlowCtrlStatus)
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+DBGINFO:PsFlowCtrlStart!\r\n");
        }
        else
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+DBGINFO:PsFlowCtrlEnd!\r\n");
        }
        AtcAp_SendDataInd();
    }
}

#ifdef LCS_MOLR_ENABLE
void AtcAp_MsgProc_CMOLRE_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_CMOLRE_IND_STRU*    pCmolrE = (ATC_MSG_CMOLRE_IND_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CMOLRE:%d\r\n", pCmolrE->ucErrCode);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CMOLRG_Ind(unsigned char* pRecvMsg)
{
    unsigned char*              pAtcRspBuf;
    unsigned char*              pData;
    unsigned short              usLen      = 0;
    ATC_MSG_CMOLRG_IND_STRU*    pCmolrG    = (ATC_MSG_CMOLRG_IND_STRU*)pRecvMsg;

    pAtcRspBuf = (unsigned char*)AtcAp_Malloc(D_ATC_CMOLRG_MAX_SIZE + 1);
    pData = pAtcRspBuf;

    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)pData,  (const unsigned char *)"\r\n+CMOLRG:");
    pData += g_AtcApInfo.stAtRspInfo.usRspLen;

    usLen = D_ATC_CMOLRG_MAX_SIZE - usLen - 10;
    g_AtcApInfo.stAtRspInfo.usRspLen += Lcs_MolrResult_OutputXML(pData, &usLen, &pCmolrG->stShapeData, &pCmolrG->stVelData);
    pData += g_AtcApInfo.stAtRspInfo.usRspLen;

    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)pData,  (const unsigned char *)"\r\n");

    AtcAp_SendLongDataInd(&pAtcRspBuf, D_ATC_CMOLRG_MAX_SIZE);
    
    AtcAp_Free(pAtcRspBuf);
}
#endif

void AtcAp_MsgProc_PDNIPADDR_Ind(unsigned char* pRecvMsg)
{
    /*ATC_MSG_PdnIPAddr_IND_STRU*    pPndIpAddrInd = (ATC_MSG_PdnIPAddr_IND_STRU*)pRecvMsg;

     AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CGPADDR:%d", pPndIpAddrInd->ucAtCid);
    if (1 == pPndIpAddrInd->ucIPv4Flg)
    {
        AtcAp_OutputAddr(4, pPndIpAddrInd->aucIPv4Addr, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
    }
    
    if (1 == pPndIpAddrInd->ucIPv6Flg)
    {
        AtcAp_OutputAddr(16, pPndIpAddrInd->aucIPv6Addr, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
    }
    
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n");
    
    AtcAp_SendDataInd();*/
}

void AtcAp_MsgProc_NGACTR_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_NGACTR_IND_STRU*    pNgactrInd = (ATC_MSG_NGACTR_IND_STRU*)pRecvMsg;

    if(1 != pNgactrInd->ucNgactrN)
    {
        return;
    }

    AtcAp_StrPrintf_AtcRspBuf("\r\n+NGACTR:%d,%d,%d\r\n", pNgactrInd->ucAtCid, pNgactrInd->state, pNgactrInd->result);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CELLSRCH_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_CELLSRCH_TEST_STRU* pNCSEARFCN = (ATC_MSG_CELLSRCH_TEST_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n+CELLSRCH:%d,%d\r\n", pNCSEARFCN->ucFstSrchType, pNCSEARFCN->ucCampOnType);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_OPELIST_SRCH_CNF(unsigned char* pRecvMsg)
{
    unsigned char                   i;
    unsigned char                   aucPlmnNum[7] = {0};
    ATC_MSG_OPELIST_SRCH_CNF_STRU*  pSrchCnf      = (ATC_MSG_OPELIST_SRCH_CNF_STRU*)pRecvMsg;

    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
            (const unsigned char *)"\r\n+COPS:(");

    for (i = 0; i < pSrchCnf->stPara.ucOpeInfCnt; i++)
    {
        AtcAp_IntegerToPlmn(pSrchCnf->stPara.aOpeInf[i].aulPlmnNum,aucPlmnNum);

        g_AtcApInfo.stAtRspInfo.usRspLen +=AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                (const unsigned char *)"(%d,,,%c%s%c,%d)", pSrchCnf->stPara.aOpeInf[i].ucState,
                D_ATC_N_QUOTATION, aucPlmnNum, D_ATC_N_QUOTATION,9);
        
        if (i < (pSrchCnf->stPara.ucOpeInfCnt - 1))
        {
            g_AtcApInfo.stAtRspInfo.usRspLen +=AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
                    (const unsigned char *)",");
        }
    }

    g_AtcApInfo.stAtRspInfo.usRspLen +=AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen),
            (const unsigned char *)"),,(0-3),(2)\r\n");

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CSIM_Cnf(unsigned char* pRecvMsg)
{
    unsigned char*                 pDataBuff;
    ATC_MSG_CSIM_CNF_STRU*         pCsimCnf = (ATC_MSG_CSIM_CNF_STRU*)pRecvMsg;

    pDataBuff = (unsigned char*)AtcAp_Malloc(pCsimCnf->usRspLen * 2 + 1);
    
    AtcAp_HexToAsc((unsigned short)pCsimCnf->usRspLen, pDataBuff, pCsimCnf->aucRsp);
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CSIM:%d,%s\r\n", pCsimCnf->usRspLen * 2, pDataBuff);
    AtcAp_Free(pDataBuff);

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CGLA_Cnf(unsigned char* pRecvMsg)
{
    unsigned char*                 pDataBuff;
    ATC_MSG_CGLA_CNF_STRU*         pCglaCnf = (ATC_MSG_CGLA_CNF_STRU*)pRecvMsg;

    pDataBuff = (unsigned char*)AtcAp_Malloc(pCglaCnf->usRspLen * 2 + 1);
    
    AtcAp_HexToAsc((unsigned short)pCglaCnf->usRspLen, pDataBuff, pCglaCnf->aucRsp);
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CGLA:%d,%s\r\n", pCglaCnf->usRspLen * 2, pDataBuff);
    AtcAp_Free(pDataBuff);

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CCHO_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CCHO_CNF_STRU*  pCchoCnf = (ATC_MSG_CCHO_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n%d\r\n", pCchoCnf->ucChannelId);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CCHC_Cnf(unsigned char* pRecvMsg)
{
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CCHC\r\n");
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CRSM_Cnf(unsigned char* pRecvMsg)
{
    unsigned char          *pStrData;
    ATC_MSG_CRSM_CNF_STRU*  pCrsmCnf = (ATC_MSG_CRSM_CNF_STRU*)pRecvMsg;

    if (D_ATC_CRSM_COMMAND_UPDATE_BINARY == pCrsmCnf->ucCommand
        || D_ATC_CRSM_COMMAND_UPDATE_RECORD == pCrsmCnf->ucCommand
        || D_ATC_CRSM_COMMAND_SET_DATA == pCrsmCnf->ucCommand
        || 0 == pCrsmCnf->usRspLen)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+CRSM:%d,%d\r\n", pCrsmCnf->sw1, pCrsmCnf->sw2);
    }
    else
    {
        pStrData = (unsigned char*)AtcAp_Malloc(pCrsmCnf->usRspLen * 2 + 1);
        AtcAp_HexToAsc(pCrsmCnf->usRspLen, pStrData, pCrsmCnf->aucResponse);
        AtcAp_StrPrintf_AtcRspBuf("\r\n+CRSM:%d,%d,%s\r\n", pCrsmCnf->sw1, pCrsmCnf->sw2, pStrData);
        AtcAp_Free(pStrData);
    }
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_LOCALTIMEINFO_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_LOCALTIMEINFO_IND_STRU* pLocalTimeInfo = (ATC_MSG_LOCALTIMEINFO_IND_STRU*)pRecvMsg;
    if(g_NITZ_mode != 0)
    {
        /* Report +XYCTZEU*/
        /*AtcAp_OutputTimeZone_XY(pLocalTimeInfo->ucCtzrReport, &pLocalTimeInfo->stPara);
        AtcAp_OutputUniversalTime(&pLocalTimeInfo->stPara);
        if (0 != g_AtcApInfo.stAtRspInfo.usRspLen)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)"\r\n");
            AtcAp_SendDataInd();
        }*/

        switch (pLocalTimeInfo->ucCtzrReport)
        {
        case 0:
            break;
        case 1:
            AtcAp_OutputTimeZone(pLocalTimeInfo->ucCtzrReport, &pLocalTimeInfo->stPara);
            break;
        case 2:
            AtcAp_OutputTimeZone(pLocalTimeInfo->ucCtzrReport, &pLocalTimeInfo->stPara);
            AtcAp_OutputLocalTime(&pLocalTimeInfo->stPara);
            break;
        case 3:
            AtcAp_OutputTimeZone(pLocalTimeInfo->ucCtzrReport, &pLocalTimeInfo->stPara);
            AtcAp_OutputUniversalTime(&pLocalTimeInfo->stPara);
            break;
        default:
            break;
        }

        if (0 != g_AtcApInfo.stAtRspInfo.usRspLen)
        {
            g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), (const unsigned char *)"\r\n");
            AtcAp_SendDataInd();
        }
    }
}

void AtcAp_MsgProc_NoCarrier_Ind(unsigned char* pRecvMsg)
{
    g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf, (const unsigned char *)"\r\nNO CARRIER\r\n");
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_PSINFO_Ind(unsigned char* pRecvMsg)
{
    /*ATC_MSG_PSINFO_IND_STRU* pPsInfo = (ATC_MSG_PSINFO_IND_STRU*)pRecvMsg;
    
    if(pPsInfo->ucType == D_MSG_PSINFO_TYPE_SIMVCC)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+PSINFO:SIMVCC,%d\r\n", pPsInfo->ucValue);
    }
    else if(pPsInfo->ucType == D_MSG_PSINFO_TYPE_NVWRITE)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+PSINFO:NVWRITE,%d\r\n", pPsInfo->ucValue);
    }

    AtcAp_SendDataInd();*/
}

void AtcAp_MsgProc_CSODCPR_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_CSODCPR_IND_STRU* pCsodcprInd = (ATC_MSG_CSODCPR_IND_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n+CSODCPR:%d,%d,%d\r\n", pCsodcprInd->ucAtCid, pCsodcprInd->ucSequence, pCsodcprInd->ucStatus);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NSNPDR_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_NSNPDR_IND_STRU* pNsnpdrInd = (ATC_MSG_NSNPDR_IND_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n+NSNPDR:%d,%d,%d\r\n", pNsnpdrInd->ucAtCid, pNsnpdrInd->ucSequence, pNsnpdrInd->ucStatus);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NIPINFO_Ind(unsigned char* pRecvMsg)
{
    char                      aucPdpType[][7] = { "","IP","IPV6","IPV4V6" };
    ATC_MSG_NIPINFO_IND_STRU* pIpInfoInd      = (ATC_MSG_NIPINFO_IND_STRU*)pRecvMsg;

    
    if(pIpInfoInd->ucResult == D_RESULT_SUCCESS)
    {
        if (1 == pIpInfoInd->ucIPv4Flg)
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+NIPINFO:%d,\"%s\"", pIpInfoInd->ucAtCid, aucPdpType[pIpInfoInd->ucPdpType]);
            AtcAp_OutputAddr(4, pIpInfoInd->aucIPv4Addr, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
        }
        
        if (1 == pIpInfoInd->ucIPv6Flg)
        {
            AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+NIPINFO:%d,\"%s\"", pIpInfoInd->ucAtCid, aucPdpType[pIpInfoInd->ucPdpType]);
            AtcAp_OutputAddr_IPv6ColonFormat(pIpInfoInd->aucIPv6Addr, g_AtcApInfo.stAtRspInfo.aucAtcRspBuf);
        }
        AtcAp_StrPrintf_AtcRspBuf("\r\n");
    }
    else
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+NIPINFO:%d,\"%s\",%d\r\n", pIpInfoInd->ucAtCid, aucPdpType[pIpInfoInd->ucPdpType], pIpInfoInd->ucFailCause);
    }

    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_CNEC_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_CNEC_IND_STRU* pCnecInd      = (ATC_MSG_CNEC_IND_STRU*)pRecvMsg;

    if(pCnecInd->ucType == D_ATC_CNEC_TYPE_EMM)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+CNEC_EMM:%d", pCnecInd->ucErrCode);
    }
    else
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+CNEC_ESM:%d", pCnecInd->ucErrCode);
    }

    if(pCnecInd->ucAtCidFlg != 0)
    {
        AtcAp_StrPrintf_AtcRspBuf(",%d", pCnecInd->ucAtCid);
    }
    AtcAp_StrPrintf_AtcRspBuf("\r\n");
    
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NRNPDM_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_NRNPDM_R_CNF_STRU* pNrnpdmR=(ATC_MSG_NRNPDM_R_CNF_STRU*)pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n+NRNPDM:%d\r\n", pNrnpdmR->ucReporting);
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_NRNPDM_Ind(unsigned char ucNrnpdmRepValue, unsigned char ucAtCid,unsigned short usLen,unsigned char* pucReportData)
{
    unsigned char*              pInd;

	if(0 == ucNrnpdmRepValue)
	{
		return;
	}
	
    if (0 != usLen)
    {
        pInd = (unsigned char *)AtcAp_Malloc(D_ATC_NRNPDM_IND_LENGTH + usLen*2 + 1);
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)pInd, (const unsigned char *)"\r\n+NRNPDM:%d,%d,%c", ucAtCid, usLen, D_ATC_N_QUOTATION);

        AtcAp_HexToAsc(usLen, pInd + g_AtcApInfo.stAtRspInfo.usRspLen, pucReportData);
        g_AtcApInfo.stAtRspInfo.usRspLen += usLen*2;

        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)pInd + g_AtcApInfo.stAtRspInfo.usRspLen, 
(const unsigned char *)"%c\r\n", D_ATC_N_QUOTATION);
        AtcAp_SendLongDataInd(&pInd, D_ATC_NRNPDM_IND_LENGTH + usLen*2);
        
        AtcAp_Free(pInd);
    } 
    else
    {
        g_AtcApInfo.stAtRspInfo.usRspLen = AtcAp_StrPrintf((unsigned char *)g_AtcApInfo.stAtRspInfo.aucAtcRspBuf,
            (const unsigned char *)"\r\n+NRNPDM:%d,%d,%c%c\r\n",ucAtCid, 0, D_ATC_N_QUOTATION, D_ATC_N_QUOTATION);
        AtcAp_SendDataInd();
    }
}

void AtcAp_MsgProc_MNBIOTEVENT_Ind(unsigned char* pRecvMsg)
{
    ATC_MSG_MNBIOTEVENT_IND_STRU* pInd = (ATC_MSG_MNBIOTEVENT_IND_STRU*)pRecvMsg;
    char* pPsmState[2]                 = { "ENTER PSM", "EXIT PSM" };

    if(pInd->ucPsmEnable == 0)
    {
        return;
    }
    
    if(pInd->ucQnbioteventFlg == 1)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+QNBIOTEVENT:%c%s%c\r\n", D_ATC_N_QUOTATION, pInd->ucPsmState == D_ATC_MNBIOTEVENT_ENTER_PSM ? pPsmState[0] : pPsmState[1], D_ATC_N_QUOTATION);
    }
    
    if(pInd->ucPsmStateRptFlg == 1)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+MNBIOTEVENT:%c%s%c\r\n", D_ATC_N_QUOTATION, pInd->ucPsmState == D_ATC_MNBIOTEVENT_ENTER_PSM ? pPsmState[0] : pPsmState[1], D_ATC_N_QUOTATION);
    }
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_MNBIOTEVENT_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_MNBIOTEVENT_R_CNF_STRU*  pMnbiotevevtR =(ATC_MSG_MNBIOTEVENT_R_CNF_STRU*) pRecvMsg;
    if(pMnbiotevevtR->ucQnbioteventFlg == 1)
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+QNBIOTEVENT:%d,1\r\n",pMnbiotevevtR->ucPsmStateRptValue);
        AtcAp_SendDataInd();
    }
}

void AtcAp_MsgProc_CGPIAF_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_CGPIAF_R_CNF_STRU*  pInd =(ATC_MSG_CGPIAF_R_CNF_STRU*) pRecvMsg;
    AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+CGPIAF:%d,%d,%d,%d\r\n",pInd->ucIpv6AddressFormat,pInd->ucIpv6SubnetNotation,pInd->ucIpv6LeadingZeros,pInd->ucIpv6CompressZeros);
    AtcAp_SendDataInd();

}

#if VER_QUCTL260
void AtcAp_MsgProc_QPLMNS_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_QPLMNS_R_CNF_STRU*  pCnf =(ATC_MSG_QPLMNS_R_CNF_STRU*) pRecvMsg;

    if(pCnf->ucState == D_ATC_QPLMNS_OOS)
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+QPLMNS: %d,%d\r\n",pCnf->ucState, pCnf->ulOosTimerLeftLen);
    }
    else
    {
        AtcAp_StrPrintf_AtcRspBuf((const char *)"\r\n+QPLMNS: %d\r\n",pCnf->ucState);
    }
    AtcAp_SendDataInd();
}
#endif

void AtcAp_MsgProc_QLOCKF_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_QLOCKF_R_CNF_STRU*  pQlockfR =(ATC_MSG_QLOCKF_R_CNF_STRU*) pRecvMsg;
    unsigned char   ucEarfcnNum = 0;

    if(pQlockfR->ucEarLockFlg == 1)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+QLOCKF: 1,%u",pQlockfR->ulLockEar);
        if(pQlockfR->ucPciLockFlg == 1)
        {
            AtcAp_StrPrintf_AtcRspBuf(",%u",pQlockfR->usLockPci);
        }
    }
    if(pQlockfR->ucEarfcnNum > 0)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n+QLOCKF: 2");
        for(ucEarfcnNum = 0; ucEarfcnNum <= (pQlockfR->ucEarfcnNum - 1); ucEarfcnNum++)
        {
            AtcAp_StrPrintf_AtcRspBuf(",%u",pQlockfR->aulEarfcnList[ucEarfcnNum]);
        }
    }
    if(pQlockfR->ucEarLockFlg == 1 || pQlockfR->ucEarfcnNum > 0)
    {
        AtcAp_StrPrintf_AtcRspBuf("\r\n");
    }
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_ECURC_R_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_ECURC_R_CNF_STRU*  pECURCR =(ATC_MSG_ECURC_R_CNF_STRU*) pRecvMsg;

    AtcAp_StrPrintf_AtcRspBuf("\r\n+ECURC:");
    AtcAp_StrPrintf_AtcRspBuf("\"%s\":%d,", (char*)ATC_ECURC_Table[D_ATC_ECURC_CEREG].aucStr, pECURCR->ucCeregFlg);
    AtcAp_StrPrintf_AtcRspBuf("\"%s\":%d,", (char*)ATC_ECURC_Table[D_ATC_ECURC_CEDRXP].aucStr, pECURCR->ucCedexpFlg);
    AtcAp_StrPrintf_AtcRspBuf("\"%s\":%d,", (char*)ATC_ECURC_Table[D_ATC_ECURC_CCIOTOPTI].aucStr, pECURCR->ucCciotoptiFlg);
    AtcAp_StrPrintf_AtcRspBuf("\"%s\":%d,", (char*)ATC_ECURC_Table[D_ATC_ECURC_CSCON].aucStr, pECURCR->ucCsconFlg);
    AtcAp_StrPrintf_AtcRspBuf("\"%s\":%d,", (char*)ATC_ECURC_Table[D_ATC_ECURC_CTZEU].aucStr, pECURCR->ucCtzeuFlg);
    AtcAp_StrPrintf_AtcRspBuf("\"%s\":%d\r\n", (char*)ATC_ECURC_Table[D_ATC_ECURC_CGEV].aucStr, pECURCR->ucCgevFlg);
       
    AtcAp_SendDataInd();
}

void AtcAp_MsgProc_QENG_Cnf(unsigned char* pRecvMsg)
{
    ATC_MSG_QENG_CNF_STRU*  pQengCnf =(ATC_MSG_QENG_CNF_STRU*) pRecvMsg;
    unsigned char              aucMCC[4] = { 0 };
    unsigned char              aucMNC[5] = { 0 };
    unsigned char  ucIdx = 0;
    unsigned short u16Rsrp;
    unsigned short u16Rsrq;
    unsigned char     abEMM_State[][11]= { "NULL", "DEREG", "REG INIT", "REG", "DEREG INIT", "TAU INIT", "SR INIT", "UNKNOWN"};
    unsigned char     abEMM_Mode[][10]= {"IDLE","PSM","CONNECTED","UNKNOWN"};
    unsigned char     abPLMN_State[][10]= {"NO PLMN", "SEARCHING", "SELECTED", "UNKNOWN"};
    unsigned char     abPLMN_Type[][8]= { "HPLMN", "EHPLMN", "VPLMN", "UPLMN", "OPLMN", "UNKNOWN"};

    if(pQengCnf->ucQengValue == 0)
    {
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                        (const unsigned char *)"\r\n+QENG: 0,");
        AtcAp_StrPrintf_AtcRspBuf("%u,",pQengCnf->stRadio.last_earfcn_value);
        AtcAp_StrPrintf_AtcRspBuf("0,%u,",pQengCnf->stRadio.last_pci_value);
        AtcAp_StrPrintf_AtcRspBuf("%c%08X%c,",D_ATC_N_QUOTATION, pQengCnf->stRadio.last_cell_ID, D_ATC_N_QUOTATION);
        AtcAp_StrPrintf_AtcRspBuf("%d,",pQengCnf->stRadio.rsrp / 10);
        AtcAp_StrPrintf_AtcRspBuf("%d,",pQengCnf->stRadio.rsrq / 10);
        AtcAp_StrPrintf_AtcRspBuf("%d,",pQengCnf->stRadio.rssi / 10);
        AtcAp_StrPrintf_AtcRspBuf("%d,",pQengCnf->stRadio.last_snr_value / 10);
        AtcAp_StrPrintf_AtcRspBuf("%d,",pQengCnf->stRadio.band);
        AtcAp_StrPrintf_AtcRspBuf("%c%04X%c,", D_ATC_N_QUOTATION, pQengCnf->stRadio.current_tac, D_ATC_N_QUOTATION);
        AtcAp_StrPrintf_AtcRspBuf("%u,",pQengCnf->stRadio.last_ECL_value);
        AtcAp_StrPrintf_AtcRspBuf("%d,",pQengCnf->stRadio.current_tx_power_level / 10);
        AtcAp_StrPrintf_AtcRspBuf("%d",pQengCnf->stRadio.operation_mode);
        
        if(pQengCnf->stCell.stCellList.ucCellNum > 1)
        {
            for (ucIdx = 1; ucIdx < pQengCnf->stCell.stCellList.ucCellNum; ucIdx++)
            {
                g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                    (const unsigned char *)"\r\n+QENG:1,%d,%d,", pQengCnf->stCell.stCellList.aNCell[ucIdx].ulDlEarfcn, pQengCnf->stCell.stCellList.aNCell[ucIdx].usPhyCellId);
        
                if(0 == pQengCnf->stCell.stCellList.aNCell[ucIdx].sRsrp)
                {
                    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                        (const unsigned char *)"%s", "0,");
                }
                else
                {
                    u16Rsrp = (~(pQengCnf->stCell.stCellList.aNCell[ucIdx].sRsrp - 1)) & 0x7FFF;
                    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                        (const unsigned char *)"%c%d%c", '-', u16Rsrp / 10, ',');
                }
        
                if(pQengCnf->stCell.stCellList.aNCell[ucIdx].sRsrq < 0)
                {
                    u16Rsrq = (~(pQengCnf->stCell.stCellList.aNCell[ucIdx].sRsrq - 1)) & 0x7FFF;
                    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                        (const unsigned char *)"%c%d", '-', u16Rsrq / 10);
                }
                else
                {
                    u16Rsrq = pQengCnf->stCell.stCellList.aNCell[ucIdx].sRsrq;
                    g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                        (const unsigned char *)"%d", u16Rsrq / 10);
                }
            }
        }
    }

    if(pQengCnf->ucQengValue == 3)
    {
        if(0 != pQengCnf->stPlmnStatus.ulSelectPlmn)
        {
            AtcAp_IntegerToMCCMNC(pQengCnf->stPlmnStatus.ulSelectPlmn, aucMCC, aucMNC);
        }
        else
        {
            aucMCC[0] = 0x30;
            aucMNC[0] = 0x30;
        }
        g_AtcApInfo.stAtRspInfo.usRspLen += AtcAp_StrPrintf((unsigned char *)(g_AtcApInfo.stAtRspInfo.aucAtcRspBuf + g_AtcApInfo.stAtRspInfo.usRspLen), 
                        (const unsigned char *)"\r\n+QENG: 4,");
        AtcAp_StrPrintf_AtcRspBuf("%c%s%c,", D_ATC_N_QUOTATION, abEMM_State[pQengCnf->stPlmnStatus.ucEmmState], D_ATC_N_QUOTATION);
        AtcAp_StrPrintf_AtcRspBuf("%c%s%c,", D_ATC_N_QUOTATION, abEMM_Mode[pQengCnf->stPlmnStatus.ucEmmMode], D_ATC_N_QUOTATION);
        AtcAp_StrPrintf_AtcRspBuf("%c%s%c,", D_ATC_N_QUOTATION, abPLMN_State[pQengCnf->stPlmnStatus.ucPlmnState], D_ATC_N_QUOTATION);
        AtcAp_StrPrintf_AtcRspBuf("%c%s%c,", D_ATC_N_QUOTATION, abPLMN_Type[pQengCnf->stPlmnStatus.ucPlmnType], D_ATC_N_QUOTATION);
        AtcAp_StrPrintf_AtcRspBuf("%c0x%s,0x%s%c", D_ATC_N_QUOTATION, aucMCC, aucMNC, D_ATC_N_QUOTATION);
    }
    AtcAp_StrPrintf_AtcRspBuf("\r\n");
    AtcAp_SendDataInd();
}

