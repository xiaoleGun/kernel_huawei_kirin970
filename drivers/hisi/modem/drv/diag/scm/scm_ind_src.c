/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


/*****************************************************************************
  1 ??????????
**************************************************************************** */
#include <product_config.h>
#include <osl_sem.h>
#include <soc_socp_adapter.h>
#include <bsp_socp.h>
#include <securec.h>
#include "OmCommonPpm.h"
#include "scm_common.h"
#include "scm_ind_src.h"
#include "scm_debug.h"


#define SOCP_CODER_SRC_PS_IND    SOCP_CODER_SRC_LOM_IND1

/* ****************************************************************************
  2 ????????????
**************************************************************************** */
SCM_CODER_SRC_CFG_STRU      g_astSCMIndCoderSrcCfg =  
{
    SCM_CHANNEL_UNINIT, 
    SOCP_CODER_SRC_PS_IND,
    SOCP_CODER_DST_OM_IND,   
    SOCP_DATA_TYPE_0, 
    SOCP_ENCSRC_CHNMODE_CTSPACKET, 
    SOCP_CHAN_PRIORITY_2,   
    SOCP_TRANS_ID_EN,
    SOCP_PTR_IMG_DIS,   
    SCM_CODER_SRC_BDSIZE, 
    SCM_CODER_SRC_RDSIZE, 
    NULL, 
    NULL, 
    NULL, 
    NULL
};


extern SCM_CODER_SRC_DEBUG_STRU g_astScmIndSrcDebugInfo;

u32 scm_init_ind_src_buff(void)
{
    u32                          ulRet;



    ulRet = scm_create_ind_src_buff(&g_astSCMIndCoderSrcCfg.pucSrcBuf,
                                &g_astSCMIndCoderSrcCfg.pucSrcPHY,
                                SCM_CODER_SRC_IND_BUFFER_SIZE);
    if(BSP_OK != ulRet)
    {
        g_astSCMIndCoderSrcCfg.enInitState   = SCM_CHANNEL_MEM_FAIL;  
        return (u32)BSP_ERROR;
    }
    g_astSCMIndCoderSrcCfg.ulSrcBufLen  = SCM_CODER_SRC_IND_BUFFER_SIZE;
    g_astSCMIndCoderSrcCfg.enInitState  = SCM_CHANNEL_INIT_SUCC;  

    return BSP_OK;
}


/* ****************************************************************************
 ?? ?? ??  : scm_create_cnf_src_buff
 ????????  : ??????????buffer????
 ????????  :
**************************************************************************** */
u32 scm_create_ind_src_buff(u8 **pBufVir, u8 **pBufPhy, u32 ulLen)
{
    unsigned long ulRealAddr;

    /*????uncache????????????*/
    *pBufVir = (u8*)scm_UnCacheMemAlloc(ulLen, &ulRealAddr);

    /* ???????????? */
    if (NULL == *pBufVir)
    {
        return (u32)BSP_ERROR;
    }

    /* ????buf?????? */
    *pBufPhy = (u8*)ulRealAddr;

    return BSP_OK;
}




u32 scm_get_ind_src_buff(
                                    u32 ulDataLen, 
                                    SCM_CODER_SRC_PACKET_HEADER_STRU** pstCoderHeader,
                                    SOCP_BUFFER_RW_STRU *pstSocpBuf)
{
    SOCP_BUFFER_RW_STRU                 stRwBuf = {0};
    SCM_CODER_SRC_PACKET_HEADER_STRU    *pstBuff;
    u32                                 *pstBuftmp;
    u32                                 ulTrueLen;
    SOCP_CODER_SRC_ENUM_U32             enChanlID;


    /* ????????????????4K */
    if ((0 == ulDataLen) || (ulDataLen > SCM_CODER_SRC_MAX_LEN))
    {
        (void)scm_printf("ulDataLen %d.\n", ulDataLen);
        return (u32)BSP_ERROR;
    }

    if (g_astSCMIndCoderSrcCfg.enInitState != SCM_CHANNEL_INIT_SUCC)/* ???????????? */
    {
        scm_printf("ind channel buff is not init\n");
        return (u32)BSP_ERROR;/* ???????? */
    }

    if(SOCP_ENCSRC_CHNMODE_LIST == g_astSCMIndCoderSrcCfg.enCHMode)
    {
        scm_printf("ind channel mode is list, error\n");
        return (u32)BSP_ERROR;
    }

    enChanlID = g_astSCMIndCoderSrcCfg.enChannelID;

    if(BSP_OK != bsp_socp_get_write_buff(enChanlID, &stRwBuf))
    {
        g_astScmIndSrcDebugInfo.ulGetWriteBufErr ++;
        return (u32)BSP_ERROR;/* ???????? */
    }

    ulTrueLen = ALIGN_DDR_WITH_8BYTE(ulDataLen);
    if((stRwBuf.u32Size + stRwBuf.u32RbSize) >= (ulTrueLen + SCM_HISI_HEADER_LENGTH))
    {
        /*??????????????????????*/
        pstBuff = (SCM_CODER_SRC_PACKET_HEADER_STRU*)scm_UncacheMemPhyToVirt((u8*)stRwBuf.pBuffer,
                                    g_astSCMIndCoderSrcCfg.pucSrcPHY,
                                    g_astSCMIndCoderSrcCfg.pucSrcBuf,
                                    g_astSCMIndCoderSrcCfg.ulSrcBufLen);

        if(stRwBuf.u32Size >= SCM_HISI_HEADER_LENGTH)
        {
            pstBuff->ulHisiMagic = SCM_HISI_HEADER_MAGIC;
            pstBuff->ulDataLen   = ulDataLen;
        }
        else if(stRwBuf.u32Size >= 4)
        {
            pstBuff->ulHisiMagic = SCM_HISI_HEADER_MAGIC;

            pstBuftmp       = (u32*)g_astSCMIndCoderSrcCfg.pucSrcBuf;
            *pstBuftmp      = ulDataLen;
        }
        else    /* TODO: ????????stRwBuf.u32Size??0?????? */
        {
            pstBuftmp = (u32*)g_astSCMIndCoderSrcCfg.pucSrcBuf;

            *(pstBuftmp++)  = SCM_HISI_HEADER_MAGIC;
            *pstBuftmp      = ulDataLen;
        }

        *pstCoderHeader     = pstBuff;
        (void)memcpy_s(pstSocpBuf, sizeof(*pstSocpBuf), &stRwBuf, sizeof(stRwBuf));

        return BSP_OK;
    }
    else
    {
        g_astScmIndSrcDebugInfo.ulGetCoherentBuffErr++;
        return (u32)BSP_ERROR;
    }

}



void scm_ind_src_buff_mempy(SCM_CODER_SRC_MEMCPY_STRU *pInfo, SOCP_BUFFER_RW_STRU *pstSocpBuf)
{
    void    *pDst;

    /* ???????????????????????????????????????????????????? */
    if(pInfo->uloffset < pstSocpBuf->u32Size)
    {
        if((pInfo->uloffset + pInfo->ulLen) <= pstSocpBuf->u32Size)
        {
            (void)memcpy_s(((u8*)pInfo->pHeader + pInfo->uloffset), pstSocpBuf->u32Size-pInfo->uloffset, pInfo->pSrc, pInfo->ulLen);          
            scm_FlushCpuWriteBuf();
        }
        else
        {
            (void)memcpy_s(((u8*)pInfo->pHeader + pInfo->uloffset), pstSocpBuf->u32Size-pInfo->uloffset, pInfo->pSrc, (pstSocpBuf->u32Size - pInfo->uloffset));
            scm_FlushCpuWriteBuf();
            
            pDst = g_astSCMIndCoderSrcCfg.pucSrcBuf;

            (void)memcpy_s(pDst,pInfo->uloffset + pInfo->ulLen - pstSocpBuf->u32Size,
                ((u8*)pInfo->pSrc + (pstSocpBuf->u32Size - pInfo->uloffset)),(pInfo->uloffset + pInfo->ulLen - pstSocpBuf->u32Size));           
            scm_FlushCpuWriteBuf();
            
        }
    }
    else
    {
        pDst = g_astSCMIndCoderSrcCfg.pucSrcBuf;

        pDst = (u8*)pDst + (pInfo->uloffset - pstSocpBuf->u32Size);
        (void)memcpy_s(pDst, pInfo->ulLen, pInfo->pSrc, pInfo->ulLen);
        scm_FlushCpuWriteBuf();
        
    }
}

static u32 scm_send_ind_src_data_list(SOCP_CODER_SRC_ENUM_U32 enChanlID, SOCP_BUFFER_RW_STRU stRwBuf, u8 *pucSendDataAddr, u32 ulSendLen)
{
    u32 ulBDNum;
    SOCP_BD_DATA_STRU stBDData = {};

    /* ????????BD???? */
    ulBDNum = (stRwBuf.u32Size + stRwBuf.u32RbSize) / sizeof(SOCP_BD_DATA_STRU);

    /* ???????????????? */
    if (1 >= ulBDNum)
    {
        return (u32)BSP_ERROR;
    }

    stRwBuf.pBuffer = (char*)scm_UncacheMemPhyToVirt((u8*)stRwBuf.pBuffer,
                                    g_astSCMIndCoderSrcCfg.pucSrcPHY,
                                    g_astSCMIndCoderSrcCfg.pucSrcBuf,
                                    g_astSCMIndCoderSrcCfg.ulSrcBufLen);

    
	memset_s(&stBDData, sizeof(stBDData), 0, sizeof(stBDData));

	stBDData.ulDataAddr = (u32)((unsigned long)pucSendDataAddr);
    stBDData.usMsgLen   = (u16)ulSendLen;
    stBDData.enDataType = SOCP_BD_DATA;

    if(stRwBuf.u32Size >= sizeof(stBDData))
    {
        (void)memcpy_s(stRwBuf.pBuffer, stRwBuf.u32Size, &stBDData, sizeof(stBDData));    /* ???????????????????? */
        scm_FlushCpuWriteBuf();            
    }
    else  /*??????????*/
    {
        (void)memcpy_s(stRwBuf.pBuffer, stRwBuf.u32Size, &stBDData, stRwBuf.u32Size);
        (void)memcpy_s(stRwBuf.pRbBuffer, stRwBuf.u32RbSize, ((u8*)(&stBDData))+stRwBuf.u32Size, sizeof(stBDData)-stRwBuf.u32Size);
        scm_FlushCpuWriteBuf();             
    }

    if (BSP_OK != bsp_socp_write_done(enChanlID, sizeof(stBDData)))   /* ???????????????? */
    {
        SCM_CODER_SRC_ERR("SCM_SendCoderSrc: Write Buffer is Error", enChanlID, 0);/* ????Log */
        return (u32)BSP_ERROR;/* ???????? */
    }

    return BSP_OK;
}

static u32 scm_send_ind_src_data_ctspacket(SOCP_CODER_SRC_ENUM_U32 enChanlID, SOCP_BUFFER_RW_STRU stRwBuf, u8 *pucSendDataAddr, u32 ulSendLen)
{
    SCM_CODER_SRC_PACKET_HEADER_STRU*   pstCoderHeader;
    
    if(stRwBuf.u32Size < SCM_HISI_HEADER_LENGTH)
    {
        g_astScmIndSrcDebugInfo.ulSendFirstNotEnough ++;
        return (u32)BSP_ERROR;
    }

    stRwBuf.pBuffer = (char*)scm_UncacheMemPhyToVirt((u8*)stRwBuf.pBuffer,
                                    g_astSCMIndCoderSrcCfg.pucSrcPHY,
                                    g_astSCMIndCoderSrcCfg.pucSrcBuf,
                                    g_astSCMIndCoderSrcCfg.ulSrcBufLen);
    if(stRwBuf.pBuffer != (char*)pucSendDataAddr)
    {
        g_astScmIndSrcDebugInfo.ulSendAddrErr++;
        return (u32)BSP_ERROR;
    }

    pstCoderHeader = (SCM_CODER_SRC_PACKET_HEADER_STRU*)pucSendDataAddr;
    if((pstCoderHeader->ulDataLen != ulSendLen)||(pstCoderHeader->ulHisiMagic != SCM_HISI_HEADER_MAGIC))
    {
        g_astScmIndSrcDebugInfo.ulSendHeaderErr++;
        return (u32)BSP_ERROR;
    }
    scm_FlushCpuWriteBuf();
    /*??????????????????HISI????????*/
    ulSendLen = ALIGN_DDR_WITH_8BYTE(ulSendLen);
    if(BSP_OK != bsp_socp_write_done(enChanlID, (ulSendLen + SCM_HISI_HEADER_LENGTH)))
    {
        g_astScmIndSrcDebugInfo.ulSendWriteDoneErr ++;
        return (u32)BSP_ERROR;
    }
    g_astScmIndSrcDebugInfo.ulSendDataLen += ulSendLen;
    g_astScmIndSrcDebugInfo.ulSendPacketNum ++; 

    return BSP_OK;
}



u32 scm_send_ind_src_data(u8 *pucSendDataAddr, u32 ulSendLen)
{
    SOCP_BUFFER_RW_STRU                 stRwBuf = {0};
    SOCP_CODER_SRC_ENUM_U32             enChanlID;
    u32 ret;

    /* ??????????????????????????????????????4K */
    if ((NULL == pucSendDataAddr)
        ||(0 == ulSendLen)
        /*||(SCM_CODER_SRC_MAX_LEN < ulSendLen)*/)
    {
        return (u32)BSP_ERROR;
    }

    if (g_astSCMIndCoderSrcCfg.enInitState != SCM_CHANNEL_INIT_SUCC)/* ???????????? */
    {
        scm_printf("cnf channel buff is not init\n");
        return (u32)BSP_ERROR;/* ???????? */
    }

    enChanlID = g_astSCMIndCoderSrcCfg.enChannelID;
    if (BSP_OK != bsp_socp_get_write_buff(enChanlID, &stRwBuf))
    {
        g_astScmIndSrcDebugInfo.ulGetWriteBufErr ++;
        return (u32)BSP_ERROR;/* ???????? */
    }

    if(SOCP_ENCSRC_CHNMODE_LIST == g_astSCMIndCoderSrcCfg.enCHMode)
    {
        ret = scm_send_ind_src_data_list(enChanlID, stRwBuf, pucSendDataAddr, ulSendLen);
    }
    else if(SOCP_ENCSRC_CHNMODE_CTSPACKET == g_astSCMIndCoderSrcCfg.enCHMode)
    {
        ret = scm_send_ind_src_data_ctspacket(enChanlID, stRwBuf, pucSendDataAddr, ulSendLen);
    }
    else
    {
        return (u32)BSP_ERROR;
    }
    return ret;
}


u32 scm_ind_src_chan_init(void)
{

    if (BSP_OK != scm_ind_src_chan_cfg(&g_astSCMIndCoderSrcCfg))
    {
        scm_printf("cfg ind src fail\n");
        g_astSCMIndCoderSrcCfg.enInitState = SCM_CHANNEL_CFG_FAIL;  /* ?????????????????????? */

        return (u32)BSP_ERROR;/* ???????? */
    }

    if(BSP_OK != bsp_socp_start(g_astSCMIndCoderSrcCfg.enChannelID))
    {
        scm_printf("start ind src fail\n");
        g_astSCMIndCoderSrcCfg.enInitState = SCM_CHANNEL_START_FAIL;  /* ???????????????????? */
        
        return ERR_MSP_SCM_START_SOCP_FAIL;/* ???????? */
    }

    g_astSCMIndCoderSrcCfg.enInitState = SCM_CHANNEL_INIT_SUCC;     /* ?????????????????????? */

    return BSP_OK;/* ???????? */
}



u32 scm_ind_src_chan_cfg(SCM_CODER_SRC_CFG_STRU *pstCfg)
{
    SOCP_CODER_SRC_CHAN_S               stChannel;          /* ?????????????????? */

    stChannel.u32DestChanID = pstCfg->enDstCHID;            /*  ????????ID */
    stChannel.eDataType     = pstCfg->enDataType;           /*  ?????????????????????????????????????????? */
    stChannel.eMode         = pstCfg->enCHMode;             /*  ???????????? */
    stChannel.ePriority     = pstCfg->enCHLevel;            /*  ?????????? */
	stChannel.eTransIdEn    = pstCfg->enTransIdEn;          /*  SOCP Trans Id?????? */
	stChannel.ePtrImgEn     = pstCfg->enPtrImgEn;           /*  SOCP ?????????????? */
    stChannel.u32BypassEn   = SOCP_HDLC_ENABLE;             /*  ????bypass???? */
    stChannel.eDataTypeEn   = SOCP_DATA_TYPE_EN;            /*  ?????????????? */
    stChannel.eDebugEn      = SOCP_ENC_DEBUG_DIS;           /*  ?????????? */

    stChannel.sCoderSetSrcBuf.pucInputStart  = pstCfg->pucSrcPHY;                             /*  ???????????????? */
    stChannel.sCoderSetSrcBuf.pucInputEnd    = (pstCfg->pucSrcPHY + pstCfg->ulSrcBufLen)-1;   /*  ???????????????? */
    stChannel.sCoderSetSrcBuf.pucRDStart     = pstCfg->pucRDPHY;                              /* RD buffer???????? */
    stChannel.sCoderSetSrcBuf.pucRDEnd       = (pstCfg->pucRDPHY + pstCfg->ulRDBufLen)-1;     /*  RD buffer???????? */
    stChannel.sCoderSetSrcBuf.u32RDThreshold = SCM_CODER_SRC_RD_THRESHOLD;                    /* RD buffer???????????? */

    if (BSP_OK != bsp_socp_coder_set_src_chan(pstCfg->enChannelID, &stChannel))
    {
        SCM_CODER_SRC_ERR("SCM_CoderSrcChannelCfg: Search Channel ID Error", pstCfg->enChannelID, 0);/* ???????? */

        return (u32)BSP_ERROR;/* ???????? */
    }

    pstCfg->enInitState = SCM_CHANNEL_INIT_SUCC; /* ?????????????????????? */

    return BSP_OK;/* ???????? */
}





