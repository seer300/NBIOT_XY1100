/*******************************************************************************
 *							 Include header files							   *
 ******************************************************************************/
#include "xy_ps_api.h"
#include "xy_net_api.h"
#include "at_com.h"
#include "at_ctl.h"
#include "at_ps_cmd.h"
#include "inter_core_msg.h"
#include "ipcmsg.h"
#include "ps_netif_api.h"
#if !ATC_DSP
#include "shm_msg_api.h"
#endif
#include "xy_system.h"
#include "xy_utils.h"
#include "xy_atc_interface.h"

typedef struct
{
    unsigned char                       ucSetVal;
    unsigned char                       aucBitStr[5];
    unsigned long                       ulMS;
} ST_XY_ATC_EDRX_VALUE_TABLE;

const ST_XY_ATC_EDRX_VALUE_TABLE xy_atc_eDrxValue_Tab[10] = 
{
    { 2,  "0010", 20480    },
    { 3,  "0011", 40960    },
    { 5,  "0101", 81920    },
    { 9,  "1001", 163840   },
    { 10, "1010", 327680   },
    { 11, "1011", 655360   },
    { 12, "1100", 1310720  },
    { 13, "1101", 2621440  },
    { 14, "1110", 5242880  },
    { 15, "1111", 10485760 },
};

const ST_XY_ATC_EDRX_VALUE_TABLE xy_atc_eDrxPtw_Tab[15] = 
{
    { 0,  "0000", 2560  },
    { 1,  "0001", 5120  },
    { 2,  "0010", 7680  },
    { 3,  "0011", 10240 },
    { 4,  "0100", 12800 },
    { 5,  "0101", 15360 },
    { 6,  "0110", 17920 },
    { 7,  "0111", 20480 },
    { 8,  "1000", 23040 },
    { 9,  "1001", 25600 },
    { 10, "1010", 28160 },
    { 11, "1011", 30720 },
    { 12, "1100", 33280 },
    { 13, "1101", 35840 },
    { 14, "1110", 38400 },
};

static char* xy_atc_GeteDrxValueBitStr(unsigned long ulMs)
{
    int i;

    for(i = 0; i < sizeof(xy_atc_eDrxValue_Tab)/sizeof(ST_XY_ATC_EDRX_VALUE_TABLE); i++)
    {
        if(ulMs == xy_atc_eDrxValue_Tab[i].ulMS)
        {
            return xy_atc_eDrxValue_Tab[i].aucBitStr;
        }
    }

    return NULL;
}

static unsigned long xy_atc_GeteDrxValueMS(unsigned char uceDrxValue)
{
    int i;

    for(i = 0; i < sizeof(xy_atc_eDrxValue_Tab)/sizeof(ST_XY_ATC_EDRX_VALUE_TABLE); i++)
    {
        if(uceDrxValue == xy_atc_eDrxValue_Tab[i].ucSetVal)
        {
            return xy_atc_eDrxValue_Tab[i].ulMS;
        }
    }

    return 0;
}

static unsigned long xy_atc_GetPtwValueMS(unsigned char ucPtwValue)
{
    int i;

    for(i = 0; i < sizeof(xy_atc_eDrxPtw_Tab)/sizeof(ST_XY_ATC_EDRX_VALUE_TABLE); i++)
    {
        if(ucPtwValue == xy_atc_eDrxPtw_Tab[i].ucSetVal)
        {
            return xy_atc_eDrxPtw_Tab[i].ulMS;
        }
    }

    return 0;
}

/*******************************************************************************
 *						Global function implementations						   *
 ******************************************************************************/
int xy_cfun_excute(int status)
{
    char aucCmd[20] = { 0 };

    if(status != NET_CFUN0 && status != NET_CFUN1)
    {
        return XY_ERR;
    }

    sprintf(aucCmd, "AT+CFUN=%d\r\n", status);
    return xy_atc_interface_call(aucCmd, NULL, (void*)NULL);
}

int xy_cfun_read(int *cfun)
{
    ATC_MSG_CFUN_R_CNF_STRU tCfunRCnf = { 0 };

    if(cfun == NULL)
    {
        return XY_ERR;
    }

    if(xy_atc_interface_call("AT+CFUN?\r\n", NULL, (void*)&tCfunRCnf) == XY_ERR)
    {
		return XY_ERR;
    }

    *cfun = tCfunRCnf.ucFunMode;
    return XY_OK;
}

int xy_get_CGACT(int *cgact)
{
    ATC_MSG_CGACT_R_CNF_STRU tCgactRCnf = { 0 };

    if(cgact == NULL)
    {
        return XY_ERR;
    }
    
    if(xy_atc_interface_call("AT+CGACT?\r\n", NULL, (void*)&tCgactRCnf) == XY_ERR)
		return XY_ERR;

    *cgact = tCgactRCnf.stState.aCidSta[0].ucState;
    return XY_OK;
}

int xy_cereg_read(int *cereg)
{
    ATC_MSG_CEREG_R_CNF_STRU tCeregRCnf = { 0 };
    
    if(xy_atc_interface_call("AT+CEREG?\r\n", NULL, (void*)&tCeregRCnf) == XY_ERR)
		return XY_ERR;

    *cereg = tCeregRCnf.tRegisterState.ucEPSRegStatus;
    return XY_OK;
}

int xy_set_SN(char* sn, int len)
{
    char *pucCmd;
    int   ret;

    if(NULL == sn || strlen(sn) > 64)
    {
        return XY_ERR;
    }

    pucCmd = (char*)xy_zalloc(80);
    sprintf(pucCmd, "AT+NSET=\"SETSN\",\"%s\"\r\n", sn);
    ret = xy_atc_interface_call(pucCmd, (func_AppInterfaceCallback)NULL, (void*)NULL);
    xy_free(pucCmd);
   
    return ret;
}

int xy_get_SN(char *sn, int len)
{
    ATC_MSG_CGSN_CNF_STRU* pCgsnCnf;

    if (len < SN_LEN || sn == NULL)
    {
		return XY_ERR;
    }

    pCgsnCnf = (ATC_MSG_CGSN_CNF_STRU*)xy_zalloc(sizeof(ATC_MSG_CGSN_CNF_STRU) + NVM_MAX_SN_LEN + 1- 4);
    if(XY_OK != xy_atc_interface_call("AT+CGSN=0\r\n", (func_AppInterfaceCallback)NULL, (void*)pCgsnCnf))
    {
        softap_printf(USER_LOG, WARN_LOG, "get sn fail!!!");
        xy_free(pCgsnCnf);
        return XY_ERR;
    }

    if(0 != pCgsnCnf->ucLen)
    {
        strncpy(sn, pCgsnCnf->aucData, pCgsnCnf->ucLen);
        xy_assert(strlen(sn) <= NVM_MAX_SN_LEN);
    }
    xy_free(pCgsnCnf);

    return XY_OK;
}

int xy_get_RSSI(int *rssi)
{
    ATC_MSG_CSQ_CNF_STRU tCsqCnf = { 0 };

    if (rssi == NULL)
    {
        return XY_ERR;
    }
    
    if(xy_atc_interface_call("AT+CSQ\r\n", NULL, (void*)&tCsqCnf) != XY_OK)
    {
        return XY_ERR;
    }

    *rssi = tCsqCnf.ucRxlev;
    return XY_OK;
}

int xy_get_IMEI(char *imei, int len)
{
     ATC_MSG_CGSN_CNF_STRU*      pCgsnCnf;
     static char                 s_Imei[IMEI_LEN]  = { 0 };
     
	if (len < IMEI_LEN || imei == NULL)
    {
        return XY_ERR;    
    }

    if(strlen(s_Imei) == 0)
    {
        pCgsnCnf = (ATC_MSG_CGSN_CNF_STRU*)xy_zalloc(sizeof(ATC_MSG_CGSN_CNF_STRU) + IMEI_LEN + 1 - 4);
        if(xy_atc_interface_call("AT+CGSN=1\r\n", (func_AppInterfaceCallback)NULL, (void*)pCgsnCnf) != XY_OK)
        {
            xy_free(pCgsnCnf);
            return XY_ERR;
        }
        strcpy(s_Imei, (char*)pCgsnCnf->aucData);
        xy_assert(strlen(s_Imei) < IMEI_LEN);
        xy_free(pCgsnCnf);
    }
    
    strcpy(imei, s_Imei);    
    xy_assert(strlen(imei) < IMEI_LEN);
    
	return XY_OK;
}

int xy_get_IMSI(char *imsi, int len)
{
    ATC_MSG_CIMI_CNF_STRU tImsiCnf          = { 0 };
    static char           s_Imsi[IMSI_LEN]  = { 0 };
    
	if (len < IMSI_LEN || imsi == NULL)
    {
        return XY_ERR;
    }   

    if(strlen(s_Imsi) == 0)
    {		
        if(xy_atc_interface_call("AT+CIMI\r\n", (func_AppInterfaceCallback)NULL, (void*)&tImsiCnf) == XY_ERR)
        {
            return XY_ERR;
        }
        strcpy(s_Imsi, tImsiCnf.stImsi.aucImsi);
        xy_assert(strlen(s_Imsi) < IMSI_LEN);
    }
    
    strcpy(imsi, s_Imsi);
    xy_assert(strlen(imsi) < IMSI_LEN);

	return XY_OK;
}

int xy_get_CELLID(int *cell_id)
{
    ATC_MSG_NGACTR_R_CNF_STRU tNgactrRCnf = { 0 };

    if(cell_id == NULL)
    {
        return XY_ERR;
    }
    
	if(xy_atc_interface_call("AT+NGACTR?\r\n", NULL, (void*)&tNgactrRCnf) != XY_OK)
    {
        return XY_ERR;
    }

    *cell_id = tNgactrRCnf.stRegContext.ulCellId;
	return XY_OK;
}

int xy_get_NCCID(char *ccid, int len)
{
    static char             s_Iccid[UCCID_LEN]  = { 0 };
    ATC_MSG_UICCID_CNF_STRU tUccidCnf           = { 0 };
    
	if (len < UCCID_LEN)
    {
        return XY_ERR;    
    }   

    if(0 == strlen(s_Iccid))
    {
        if(XY_OK != xy_atc_interface_call("AT+NCCID\r\n", NULL, (void*)&tUccidCnf))
        {
            return XY_ERR;
        }
        strcpy(s_Iccid, tUccidCnf.aucICCIDstring);
        xy_assert(strlen(s_Iccid) < UCCID_LEN);
    }

    strcpy(ccid, s_Iccid);
    xy_assert(strlen(ccid) < UCCID_LEN);
    
	return XY_OK;
}

int xy_get_PDP_APN(char *apn_buf, int len, int cid)
{
    ATC_MSG_CGCONTRDP_CNF_STRU  *pCgcontrdpCnf;
    char                         aucAtCmd[20]  = { 0 };
    int                          query_cid     = 0;

    if (len < APN_LEN || apn_buf == NULL)
    {   
        return XY_ERR;
    }

    query_cid = (cid == -1 ? g_working_cid : cid);
    pCgcontrdpCnf = (ATC_MSG_CGCONTRDP_CNF_STRU*)xy_zalloc(sizeof(ATC_MSG_CGCONTRDP_CNF_STRU));
    sprintf(aucAtCmd, "AT+CGCONTRDP=%d\r\n", query_cid);   
    if(xy_atc_interface_call(aucAtCmd, NULL, (void*)pCgcontrdpCnf) != XY_OK)
    {
        xy_free(pCgcontrdpCnf);
        return XY_ERR;
    }

    if(0 != pCgcontrdpCnf->stPara.ucValidNum && 0 != pCgcontrdpCnf->stPara.aucPdpDynamicInfo[0].ucApnLen)
    {
        strncpy(apn_buf, pCgcontrdpCnf->stPara.aucPdpDynamicInfo[0].aucApn, pCgcontrdpCnf->stPara.aucPdpDynamicInfo[0].ucApnLen);
        xy_assert(strlen(apn_buf) <= pCgcontrdpCnf->stPara.aucPdpDynamicInfo[0].ucApnLen);
    }

    xy_free(pCgcontrdpCnf);
    return XY_OK;
}

int xy_get_working_cid()
{
	return g_working_cid;
}

int xy_get_T_ACT(int *t3324)
{
    ATC_MSG_NGACTR_R_CNF_STRU tNgactrRCnf = { 0 };

    if(t3324 == NULL)
    {
        return  XY_ERR;
    }
    
    if(xy_atc_interface_call("AT+NGACTR?\r\n", NULL, (void*)&tNgactrRCnf) != XY_OK)
    {
        return XY_ERR;
    }
    
    *t3324 = tNgactrRCnf.stRegContext.ulActTime;
	return XY_OK;
}

int xy_get_T_TAU(int *tau)
{
    ATC_MSG_NGACTR_R_CNF_STRU tNgactrRCnf = { 0 };

    if(tau == NULL)
    {
        return  XY_ERR;
    }

    if(xy_atc_interface_call("AT+NGACTR?\r\n", NULL, (void*)&tNgactrRCnf) != XY_OK)
    {
        return XY_ERR;
    }

    *tau = tNgactrRCnf.stRegContext.ulTauTime;
	return XY_OK;
}

int xy_send_rai()
{
    return xy_atc_interface_call("AT+RAI=1\r\n", NULL, (void*)NULL);
}

int xy_get_UICC_TYPE(int *uicc_type)
{
    ATC_MSG_NGACTR_R_CNF_STRU tNgactrRCnf = { 0 };
    static int                s_uiccType  = 0;

    if(uicc_type == NULL)
    {
        return  XY_ERR;
    }

    if(s_uiccType == 0)
    {
        if(xy_atc_interface_call("AT+NGACTR?\r\n", NULL, (void*)&tNgactrRCnf) != XY_OK)
        {
            return XY_ERR;
        }
        s_uiccType = tNgactrRCnf.usOperType + 1;
    }

    *uicc_type = s_uiccType;
	return XY_OK;
}

int xy_get_eDRX_value_MS(unsigned char* pucActType, unsigned long* pulEDRXValue, unsigned long* pulPtwValue)
{
    ATC_MSG_CEDRXRDP_CNF_STRU tCderxrdpCnf = { 0 };
    
    if(XY_OK != xy_atc_interface_call("AT+CEDRXRDP\r\n", (func_AppInterfaceCallback)NULL, (void*)&tCderxrdpCnf))
    {
        return XY_ERR;
    }

    if(pucActType != NULL)
    {
        *pucActType = tCderxrdpCnf.stPara.ucActType;
    }

    if(pulEDRXValue != NULL)
    {
        *pulEDRXValue = xy_atc_GeteDrxValueMS(tCderxrdpCnf.stPara.ucNWeDRXValue);
    }

    if(pulPtwValue != NULL)
    {
        *pulPtwValue = xy_atc_GetPtwValueMS(tCderxrdpCnf.stPara.ucPagingTimeWin);
    }
    
    return XY_OK;
}

int xy_set_eDRX_value(unsigned char modeVal, unsigned char actType, unsigned long ulDrxValue)
{
    char*             pDrxValueBitStr;
    char*             pucCmd;
    int               ret;

    pDrxValueBitStr = xy_atc_GeteDrxValueBitStr(ulDrxValue);
    if(pDrxValueBitStr == NULL)
    {
        return XY_ERR;
    }
   
    pucCmd = (char*)xy_zalloc(30);
    sprintf(pucCmd, "AT+CEDRXS=%d,%d,\"%s\"\r\n", modeVal, actType, pDrxValueBitStr);
    ret = xy_atc_interface_call(pucCmd, (func_AppInterfaceCallback)NULL, (void*)NULL);
    xy_free(pucCmd);
    
    return ret;
}

int xy_get_eDRX_value(float *eDRX_value, float *ptw_value)
{
    unsigned long ulEDRXValue;
    unsigned long ulPtwValue;

    if(eDRX_value == NULL || ptw_value == NULL)
    {
        return XY_ERR;
    }

    if(xy_get_eDRX_value_MS(NULL, &ulEDRXValue, &ulPtwValue) != XY_OK)
    {
        return XY_ERR;
    }

    *eDRX_value = ulEDRXValue / 1000;
    *ptw_value = ulPtwValue / 1000;
    
	return XY_OK;
}

int xy_get_servingcell_info(ril_serving_cell_info_t *rcv_serving_cell_info)
{
    ATC_MSG_NUESTATS_CNF_STRU* pNueStatsCnf;

    xy_assert(rcv_serving_cell_info != NULL);

    pNueStatsCnf = (ATC_MSG_NUESTATS_CNF_STRU*)xy_zalloc(sizeof(ATC_MSG_NUESTATS_CNF_STRU));
    if(XY_OK != xy_atc_interface_call("AT+NUESTATS=RADIO\r\n", (func_AppInterfaceCallback)NULL, (void*)pNueStatsCnf))
    {
        xy_free(pNueStatsCnf);
        return XY_ERR;
    }

    memset(rcv_serving_cell_info, 0, sizeof(ril_serving_cell_info_t));
    
    rcv_serving_cell_info->Signalpower = pNueStatsCnf->stRadio.rsrp;
    rcv_serving_cell_info->Totalpower = pNueStatsCnf->stRadio.rssi;
    rcv_serving_cell_info->TXpower = pNueStatsCnf->stRadio.current_tx_power_level;
    rcv_serving_cell_info->CellID  = pNueStatsCnf->stRadio.last_cell_ID;
    rcv_serving_cell_info->ECL = pNueStatsCnf->stRadio.last_ECL_value;
    rcv_serving_cell_info->SNR = pNueStatsCnf->stRadio.last_snr_value;
    rcv_serving_cell_info->EARFCN = pNueStatsCnf->stRadio.last_earfcn_value;
    rcv_serving_cell_info->PCI = pNueStatsCnf->stRadio.last_pci_value;
    rcv_serving_cell_info->RSRQ = pNueStatsCnf->stRadio.rsrq;
    sprintf(rcv_serving_cell_info->tac, "%d", pNueStatsCnf->stRadio.current_tac);
    rcv_serving_cell_info->sband = pNueStatsCnf->stRadio.band;

    xy_free(pNueStatsCnf);
    return XY_OK;
}

int xy_get_neighborcell_info(ril_neighbor_cell_info_t *ril_neighbor_cell_info)
{
    ATC_MSG_NUESTATS_CNF_STRU* pNueStatsCnf;
    int                        i;

    xy_assert(ril_neighbor_cell_info != NULL);

    pNueStatsCnf = (ATC_MSG_NUESTATS_CNF_STRU*)xy_zalloc(sizeof(ATC_MSG_NUESTATS_CNF_STRU));
    if(XY_OK != xy_atc_interface_call("AT+NUESTATS=CELL\r\n", (func_AppInterfaceCallback)NULL, (void*)pNueStatsCnf))
    {
        xy_free(pNueStatsCnf);
        return XY_ERR;
    }
        
    if(pNueStatsCnf->type != ATC_NUESTATS_TYPE_CELL)
    {
        xy_free(pNueStatsCnf);
        return XY_ERR;
    }

    memset(ril_neighbor_cell_info, 0, sizeof(ril_neighbor_cell_info_t)); 
    for(i = 0; i < pNueStatsCnf->stCell.stCellList.ucCellNum && i < 5; i++)
    {
        ril_neighbor_cell_info->neighbor_cell_info[i].nc_earfcn = pNueStatsCnf->stCell.stCellList.aNCell[i].ulDlEarfcn;
        ril_neighbor_cell_info->neighbor_cell_info[i].nc_pci = pNueStatsCnf->stCell.stCellList.aNCell[i].usPhyCellId;
        ril_neighbor_cell_info->neighbor_cell_info[i].nc_rsrp = pNueStatsCnf->stCell.stCellList.aNCell[i].sRsrp;
        ril_neighbor_cell_info->nc_num++;
    }

    xy_free(pNueStatsCnf);
    return XY_OK;
}

int xy_get_phy_info(ril_phy_info_t *rcv_phy_info)
{
    ATC_MSG_NUESTATS_CNF_STRU* pNueStatsCnf;

    xy_assert(rcv_phy_info != NULL);

    pNueStatsCnf = (ATC_MSG_NUESTATS_CNF_STRU*)xy_zalloc(sizeof(ATC_MSG_NUESTATS_CNF_STRU));
    if(XY_OK != xy_atc_interface_call("AT+NUESTATS=BLER\r\n", (func_AppInterfaceCallback)NULL, (void*)pNueStatsCnf))
    {
        xy_free(pNueStatsCnf);
        return XY_ERR;
    }
        
    if(pNueStatsCnf->type != ATC_NUESTATS_TYPE_BLER)
    {
        xy_free(pNueStatsCnf);
        return XY_ERR;
    }

    rcv_phy_info->RLC_UL_BLER = pNueStatsCnf->stBler.rlc_ul_bler;
    rcv_phy_info->RLC_DL_BLER = pNueStatsCnf->stBler.rlc_dl_bler;
    rcv_phy_info->MAC_UL_BLER = pNueStatsCnf->stBler.mac_ul_bler;
    rcv_phy_info->MAC_DL_BLER = pNueStatsCnf->stBler.mac_dl_bler;
    rcv_phy_info->MAC_UL_total_bytes = pNueStatsCnf->stBler.total_bytes_transmit;
    rcv_phy_info->MAC_DL_total_bytes = pNueStatsCnf->stBler.total_bytes_receive;
    rcv_phy_info->MAC_UL_total_HARQ_TX = pNueStatsCnf->stBler.transport_blocks_send;
    rcv_phy_info->MAC_DL_total_HARQ_TX = pNueStatsCnf->stBler.transport_blocks_receive;
    rcv_phy_info->MAC_UL_HARQ_re_TX = pNueStatsCnf->stBler.transport_blocks_retrans;
    rcv_phy_info->MAC_DL_HARQ_re_TX = pNueStatsCnf->stBler.total_ackOrNack_msg_receive;

    //AT+NUESTATS=THP
    memset(pNueStatsCnf, 0, sizeof(ATC_MSG_NUESTATS_CNF_STRU));
    if(XY_OK != xy_atc_interface_call("AT+NUESTATS=THP\r\n", (func_AppInterfaceCallback)NULL, (void*)pNueStatsCnf))
    {
        xy_free(pNueStatsCnf);
        return XY_ERR;
    }

    if(pNueStatsCnf->type != ATC_NUESTATS_TYPE_THP)
    {
        xy_free(pNueStatsCnf);
        return XY_ERR;
    }

    rcv_phy_info->RLC_UL_tput = pNueStatsCnf->stThp.rlc_ul;
    rcv_phy_info->RLC_DL_tput = pNueStatsCnf->stThp.rlc_dl;
    rcv_phy_info->MAC_UL_tput = pNueStatsCnf->stThp.mac_ul;
    rcv_phy_info->MAC_DL_tput = pNueStatsCnf->stThp.mac_dl;

    xy_free(pNueStatsCnf);
    return XY_OK;
}

int xy_get_radio_info(ril_radio_info_t *rcv_radio_info)
{
	xy_assert(rcv_radio_info != NULL);
	memset(rcv_radio_info, 0, sizeof(ril_radio_info_t));

	if (xy_get_servingcell_info(&rcv_radio_info->serving_cell_info) != XY_OK)
	{
		return XY_ERR;
	}

	if (xy_get_neighborcell_info(&rcv_radio_info->neighbor_cell_info) != XY_OK)
	{
		return XY_ERR;
	}

	if (xy_get_phy_info(&rcv_radio_info->phy_info) != XY_OK)
	{
		return XY_ERR;
	}
	return XY_OK;
}

