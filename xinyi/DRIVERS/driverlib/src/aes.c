#include "aes.h"
#include "dma.h"

#if !USE_ROM_AES
/* For CMAC Calculation */
const unsigned char AES_CMAC_Rb[AES_BLOCK_BYTES] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
};
#endif
//suds
#ifdef PLATFORM_DSP_F1
unsigned long DSP_DMA_Addr_Translation(unsigned long ulInputAddr)
{
    unsigned long ulDmaAddr = ulInputAddr;
    
    if(ulInputAddr >= 0x50000000 && ulInputAddr < 0x50080000)
    {
//      ulDmaAddr = ulInputAddr - 0x50000000 + 0x20000000;
        ulDmaAddr = ulInputAddr - 0x30000000;
    }
    else if(ulInputAddr >= 0x50100000 && ulInputAddr < 0x50110000)
    {
//      ulDmaAddr = ulInputAddr - 0x50100000 + 0x24000000;
        ulDmaAddr = ulInputAddr - 0x2C100000;
    }
    else if(ulInputAddr >= 0x51000000 && ulInputAddr < 0x52000000)
    {
//      ulDmaAddr = ulInputAddr - 0x51000000 + 0x27000000;
        ulDmaAddr = ulInputAddr - 0x2A000000;
    }
    else if(ulInputAddr >= 0x80020000 && ulInputAddr < 0x80040000)
    {
//      ulDmaAddr = ulInputAddr - 0x80020000 + 0x26000000;
        ulDmaAddr = ulInputAddr - 0x5A020000;
    }
    
    return ulDmaAddr;
}
#endif

#if !USE_ROM_AES
void AESKeyLenSet(unsigned long ulKeyLenMode)
{
    HWREG(AES_BASE + REG_AES_CTL) = (HWREG(AES_BASE + REG_AES_CTL) & (~AES_CTL_KEY_LEN_Msk)) | ulKeyLenMode;
}
    
void AESKeySet(unsigned char ucOffset, unsigned long ulKeyValue)
{
    HWREG(AES_BASE + REG_AES_KEY0 + ucOffset) = ulKeyValue;
}

void AESIVSet(unsigned char ucOffset, unsigned long ucIV)
{
    HWREG(AES_BASE + REG_AES_IV0 + ucOffset) = ucIV;

}

void AESAADLen0Set(unsigned long ulAadLen0)
{
    HWREG(AES_BASE + REG_AES_AADLEN0) = ulAadLen0;
}

void AESAADLen1Set(unsigned long ulAadLen1)
{
    HWREG(AES_BASE + REG_AES_AADLEN1) = ulAadLen1;
}

void AESPayloadLenSet(unsigned long ulPayloadLenByte)
{
    HWREG(AES_BASE + REG_AES_PLEN) = ulPayloadLenByte;
}

void AESModeSet(unsigned long ucAESMode)
{
    HWREG(AES_BASE + REG_AES_CTL) = (HWREG(AES_BASE + REG_AES_CTL) & (~AES_CTL_MODE_Msk)) | ucAESMode;
}

void AESEncDecSet(unsigned long ucAESEncDec)
{
    HWREG(AES_BASE + REG_AES_CTL) = (HWREG(AES_BASE + REG_AES_CTL) & (~AES_CTL_ENC_DEC_Msk)) | ucAESEncDec;
}

void AESDMAEn(void)
{
    HWREG(AES_BASE + REG_AES_CTL) |= AES_CTL_DMA_EN;
}

void AESDMADis(void)
{
    HWREG(AES_BASE + REG_AES_CTL) &= (~AES_CTL_DMA_EN);
}

void AESBlockDataInput(unsigned char ucOffset, unsigned long ulDataIn)
{
    HWREG(AES_DATA_IN_BASE + ucOffset) = ulDataIn;
}

unsigned long AESBlockDataOutput(unsigned char ucOffset)
{
    return (HWREG(AES_DATA_OUT_BASE + ucOffset));
}

void AESBlockStart(void)
{
    HWREG(AES_BASE + REG_AES_CTL) |= AES_CTL_START;
}

void AESKeyLoadWaitDone(void)
{
    HWREG(AES_BASE + REG_AES_CTL) |= AES_CTL_KEY_LOAD;
    
    while(!(HWREG(AES_BASE + REG_AES_STA) & AES_STA_AES_KEY_DONE))
    {
    }
}

void AESBlockTransWaitDone(void)
{
    while(!(HWREG(AES_BASE + REG_AES_STA) & AES_STA_AES_TRANS_DONE))
    {
    }
}

void AESClockDiv2En(void)
{
    HWREG(AES_BASE + REG_AES_CTL) |= AES_CTL_CLK_DIV_EN;
}

void AESClockDiv2Dis(void)
{
    HWREG(AES_BASE + REG_AES_CTL) &= (~AES_CTL_CLK_DIV_EN);
}

void AESCCMAuthLenSet(unsigned long ucAuthLenByte)
{
    HWREG(AES_BASE + REG_AES_CTL) = (HWREG(AES_BASE + REG_AES_CTL) & (~AES_CTL_CCM_AUTH_LEN_Msk)) | (ucAuthLenByte << AES_CTL_CCM_AUTH_LEN_Pos);
}

void AESCCMLengthLenSet(unsigned char ucLengthLenByte)
{
    HWREG(AES_BASE + REG_AES_CTL) = (HWREG(AES_BASE + REG_AES_CTL) & (~AES_CTL_CCM_LOAD_LEN_Msk)) | (ucLengthLenByte << AES_CTL_CCM_LOAD_LEN_Pos);
}

unsigned long AESTagGet(unsigned char ucOffset)
{
    return (HWREG(AES_BASE + REG_AES_TAG0 + ucOffset));
}

void AESBlock(unsigned char *pucInput, unsigned char *pucKey, unsigned long key_len_bits, unsigned char ucMode, unsigned char *pucOutput)
{	
//    unsigned long tmp;

    if(ucMode != AES_MODE_ENCRYPT && ucMode != AES_MODE_DECRYPT)
    {
        return;
    }
    
    if(key_len_bits == AES_KEY_LEN_BITS_128)
    {
        AESKeyLenSet(AES_CTL_KEY_LEN_128);
    }
    else if(key_len_bits == AES_KEY_LEN_BITS_192)
    {
        AESKeyLenSet(AES_CTL_KEY_LEN_192);
    }
    else if(key_len_bits == AES_KEY_LEN_BITS_256)
    {
        AESKeyLenSet(AES_CTL_KEY_LEN_256);
    }
    else
    {
        return;
    }
	
	AESModeSet(AES_CTL_MODE_ECB);
	
	if(ucMode == AES_MODE_ENCRYPT)
    {
        AESEncDecSet(AES_CTL_ENC);
    }
    else
    {
        AESEncDecSet(AES_CTL_DEC);
    }
    
//    AESClockDiv2Dis();
    
    AESPayloadLenSet(AES_BLOCK_BYTES);
    
    AESDMAEn();

    AESKeySet( 0, pucKey[0]  << 24 | pucKey[1]  << 16 | pucKey[2]  << 8 | pucKey[3]);
	AESKeySet( 4, pucKey[4]  << 24 | pucKey[5]  << 16 | pucKey[6]  << 8 | pucKey[7]);
	AESKeySet( 8, pucKey[8]  << 24 | pucKey[9]  << 16 | pucKey[10] << 8 | pucKey[11]);
	AESKeySet(12, pucKey[12] << 24 | pucKey[13] << 16 | pucKey[14] << 8 | pucKey[15]);
    
    if(key_len_bits >= 192)
    {
        AESKeySet(16, pucKey[16] << 24 | pucKey[17] << 16 | pucKey[18] << 8 | pucKey[19]);
        AESKeySet(20, pucKey[20] << 24 | pucKey[21] << 16 | pucKey[22] << 8 | pucKey[23]);
    }
    
    if(key_len_bits == 256)
    {
        AESKeySet(24, pucKey[24] << 24 | pucKey[25] << 16 | pucKey[26] << 8 | pucKey[27]);
        AESKeySet(28, pucKey[28] << 24 | pucKey[29] << 16 | pucKey[30] << 8 | pucKey[31]);
    }
    
//    // AES IO Mode
//	AESBlockDataInput( 0, pucInput[3]  << 24 | pucInput[2]  << 16 | pucInput[1]  << 8 | pucInput[0]);
//	AESBlockDataInput( 4, pucInput[7]  << 24 | pucInput[6]  << 16 | pucInput[5]  << 8 | pucInput[4]);
//	AESBlockDataInput( 8, pucInput[11] << 24 | pucInput[10] << 16 | pucInput[9]  << 8 | pucInput[8]);
//	AESBlockDataInput(12, pucInput[15] << 24 | pucInput[14] << 16 | pucInput[13] << 8 | pucInput[12]);
//	
//	AESBlockStart();
//	AESBlockTransWaitDone();
//	
//    tmp = AESBlockDataOutput(0);
//    pucOutput[0] = (unsigned char)tmp;
//    pucOutput[1] = (unsigned char)(tmp >> 8);
//    pucOutput[2] = (unsigned char)(tmp >> 16);
//    pucOutput[3] = (unsigned char)(tmp >> 24);
//    
//    tmp = AESBlockDataOutput(4);
//    pucOutput[4] = (unsigned char)tmp;
//    pucOutput[5] = (unsigned char)(tmp >> 8);
//    pucOutput[6] = (unsigned char)(tmp >> 16);
//    pucOutput[7] = (unsigned char)(tmp >> 24);
//    
//    tmp = AESBlockDataOutput(8);
//    pucOutput[8]  = (unsigned char)tmp;
//    pucOutput[9]  = (unsigned char)(tmp >> 8);
//    pucOutput[10] = (unsigned char)(tmp >> 16);
//    pucOutput[11] = (unsigned char)(tmp >> 24);
//    
//    tmp = AESBlockDataOutput(12);
//    pucOutput[12] = (unsigned char)tmp;
//    pucOutput[13] = (unsigned char)(tmp >> 8);
//    pucOutput[14] = (unsigned char)(tmp >> 16);
//    pucOutput[15] = (unsigned char)(tmp >> 24);

    // AES DMA Mode
	DMAChannelControlSet(DMA_CHANNEL_2,DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
	DMAChannelControlSet(DMA_CHANNEL_3,DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);

	AESBlockStart();
    
#ifdef PLATFORM_ARM_M3
    DMAChannelTransferSet(DMA_CHANNEL_2, (void *)pucInput, (void *)AES_DATA_IN_BASE, AES_BLOCK_BYTES);
    DMAChannelTransferSet(DMA_CHANNEL_3, (void *)AES_DATA_OUT_BASE, (void *)pucOutput, AES_BLOCK_BYTES);
#endif

#ifdef PLATFORM_DSP_F1
    DMAChannelTransferSet(DMA_CHANNEL_2, (void *)DSP_DMA_Addr_Translation(pucInput), (void *)AES_DATA_DMA_IN_BASE, AES_BLOCK_BYTES);
    DMAChannelTransferSet(DMA_CHANNEL_3, (void *)AES_DATA_DMA_OUT_BASE, (void *)DSP_DMA_Addr_Translation(pucOutput), AES_BLOCK_BYTES);
#endif

	DMAChannelRequest(DMA_CHANNEL_2);
	DMAChannelRequest(DMA_CHANNEL_3);
	
	DMAWaitAESDone();
    
    AESDMADis();
}

void AESECB(unsigned char *pucInput, unsigned long ulInputLenByte, unsigned char *pucKey, unsigned long key_len_bits, unsigned char ucMode, unsigned char *pucOutput)
{	
//    unsigned long tmp, i;
    unsigned long ulLenBlock;

    if(ucMode != AES_MODE_ENCRYPT && ucMode != AES_MODE_DECRYPT)
    {
        return;
    }
    
    if(key_len_bits == AES_KEY_LEN_BITS_128)
    {
        AESKeyLenSet(AES_CTL_KEY_LEN_128);
    }
    else if(key_len_bits == AES_KEY_LEN_BITS_192)
    {
        AESKeyLenSet(AES_CTL_KEY_LEN_192);
    }
    else if(key_len_bits == AES_KEY_LEN_BITS_256)
    {
        AESKeyLenSet(AES_CTL_KEY_LEN_256);
    }
    else
    {
        return;
    }
	
	AESModeSet(AES_CTL_MODE_ECB);

//    AESClockDiv2Dis();
    
    ulLenBlock = (ulInputLenByte + 15) >> 4;
    
    AESPayloadLenSet(ulLenBlock << 4);
    
    AESDMAEn();

    AESKeySet( 0, pucKey[0]  << 24 | pucKey[1]  << 16 | pucKey[2]  << 8 | pucKey[3]);
	AESKeySet( 4, pucKey[4]  << 24 | pucKey[5]  << 16 | pucKey[6]  << 8 | pucKey[7]);
	AESKeySet( 8, pucKey[8]  << 24 | pucKey[9]  << 16 | pucKey[10] << 8 | pucKey[11]);
	AESKeySet(12, pucKey[12] << 24 | pucKey[13] << 16 | pucKey[14] << 8 | pucKey[15]);
    
    if(key_len_bits >= 192)
    {
        AESKeySet(16, pucKey[16] << 24 | pucKey[17] << 16 | pucKey[18] << 8 | pucKey[19]);
        AESKeySet(20, pucKey[20] << 24 | pucKey[21] << 16 | pucKey[22] << 8 | pucKey[23]);
    }
    
    if(key_len_bits == 256)
    {
        AESKeySet(24, pucKey[24] << 24 | pucKey[25] << 16 | pucKey[26] << 8 | pucKey[27]);
        AESKeySet(28, pucKey[28] << 24 | pucKey[29] << 16 | pucKey[30] << 8 | pucKey[31]);
    }
    
    if(ucMode == AES_MODE_ENCRYPT)
    {
        AESEncDecSet(AES_CTL_ENC);
    }
    else
    {
        AESEncDecSet(AES_CTL_DEC);
        
        AESKeyLoadWaitDone();
    }
    
    
//    // AES IO Mode
//    for(i = 0; i < ulLenBlock; i++)
//    {
//        AESBlockDataInput( 0, pucInput[16*i+3]  << 24 | pucInput[16*i+2]  << 16 | pucInput[16*i+1]  << 8 | pucInput[16*i+0]);
//        AESBlockDataInput( 4, pucInput[16*i+7]  << 24 | pucInput[16*i+6]  << 16 | pucInput[16*i+5]  << 8 | pucInput[16*i+4]);
//        AESBlockDataInput( 8, pucInput[16*i+11] << 24 | pucInput[16*i+10] << 16 | pucInput[16*i+9]  << 8 | pucInput[16*i+8]);
//        AESBlockDataInput(12, pucInput[16*i+15] << 24 | pucInput[16*i+14] << 16 | pucInput[16*i+13] << 8 | pucInput[16*i+12]);

//        AESBlockStart();
//        AESBlockTransWaitDone();

//        tmp = AESBlockDataOutput(0);
//        pucOutput[16*i+0] = (unsigned char)tmp;
//        pucOutput[16*i+1] = (unsigned char)(tmp >> 8);
//        pucOutput[16*i+2] = (unsigned char)(tmp >> 16);
//        pucOutput[16*i+3] = (unsigned char)(tmp >> 24);

//        tmp = AESBlockDataOutput(4);
//        pucOutput[16*i+4] = (unsigned char)tmp;
//        pucOutput[16*i+5] = (unsigned char)(tmp >> 8);
//        pucOutput[16*i+6] = (unsigned char)(tmp >> 16);
//        pucOutput[16*i+7] = (unsigned char)(tmp >> 24);

//        tmp = AESBlockDataOutput(8);
//        pucOutput[16*i+8]  = (unsigned char)tmp;
//        pucOutput[16*i+9]  = (unsigned char)(tmp >> 8);
//        pucOutput[16*i+10] = (unsigned char)(tmp >> 16);
//        pucOutput[16*i+11] = (unsigned char)(tmp >> 24);

//        tmp = AESBlockDataOutput(12);
//        pucOutput[16*i+12] = (unsigned char)tmp;
//        pucOutput[16*i+13] = (unsigned char)(tmp >> 8);
//        pucOutput[16*i+14] = (unsigned char)(tmp >> 16);
//        pucOutput[16*i+15] = (unsigned char)(tmp >> 24);
//    }

    // AES DMA Mode
	DMAChannelControlSet(DMA_CHANNEL_2,DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
	DMAChannelControlSet(DMA_CHANNEL_3,DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);

	AESBlockStart();
    
#ifdef PLATFORM_ARM_M3
    DMAChannelTransferSet(DMA_CHANNEL_2, (void *)pucInput, (void *)AES_DATA_IN_BASE, ulInputLenByte);
    DMAChannelTransferSet(DMA_CHANNEL_3, (void *)AES_DATA_OUT_BASE, (void *)pucOutput, ulInputLenByte);
#endif

#ifdef PLATFORM_DSP_F1
    DMAChannelTransferSet(DMA_CHANNEL_2, (void *)DSP_DMA_Addr_Translation(pucInput), (void *)AES_DATA_DMA_IN_BASE, ulInputLenByte);
    DMAChannelTransferSet(DMA_CHANNEL_3, (void *)AES_DATA_DMA_OUT_BASE, (void *)DSP_DMA_Addr_Translation(pucOutput), ulInputLenByte);
#endif

	DMAChannelRequest(DMA_CHANNEL_2);
	DMAChannelRequest(DMA_CHANNEL_3);
	
	DMAWaitAESDone();
    
    AESDMADis();
}

/***************************************************************************
*
* Function:
*	aes_ccm_main
*
* Description:
*	AES CCM Mode	
*
* Input:
*	aad: Additional authenticated data;
*	aad_len_byte: length of aad, in byte;
*	input_message: Message to authenticate and encrypt/decrypt;
*	input_len_byte: length of input_message, in byte;
*	user_key: aes key;
*	key_len_bits: length of user_key, in bit, should be 128/192/256;
*	M: Number of octets in authentication field, should be 4/6/8/10/12/14/16;
*	L: Number of octets in length field, should be 1~8;
*	nonce: A nonce N of 15-L octets;
*	mode: 1, Encrypt; 0, Decrypt;
*
* Output:
*	output_message: Cipher message(mode == 1) or Plain message(mode == 0),
*	                length equals input_len_byte;
*	mac: Authenticate value, length equals M;
*
****************************************************************************/
void AESCCM(unsigned char *aad, unsigned long aad_len_byte, unsigned char *input_message, unsigned long input_len_byte, unsigned long *user_key, 
            unsigned long key_len_bits, unsigned long M, unsigned long L, unsigned char *nonce, unsigned char mode, unsigned char *output_message, unsigned char *digest)
{
//    unsigned long len_word;
    unsigned long tmp;
    unsigned char i, j;
    unsigned char NonceLen = 15 - L;
    unsigned char tempIV[2][4];
    
    if((M % 2 == 1) || (M < 4) || (M > 16))
    {
        return;
    }
    
    if(L < 1 || L > 8)
    {
        return;
    }
    
    if(mode != AES_MODE_ENCRYPT && mode != AES_MODE_DECRYPT)
    {
        return;
    }
    
    if(aad_len_byte == 0)
    {
        return;
    }
    
    if(key_len_bits == AES_KEY_LEN_BITS_128)
    {
        AESKeyLenSet(AES_CTL_KEY_LEN_128);
    }
    else if(key_len_bits == AES_KEY_LEN_BITS_192)
    {
        AESKeyLenSet(AES_CTL_KEY_LEN_192);
    }
    else if(key_len_bits == AES_KEY_LEN_BITS_256)
    {
        AESKeyLenSet(AES_CTL_KEY_LEN_256);
    }
    else
    {
        return;
    }

    AESModeSet(AES_CTL_MODE_CCM);
    AESCCMAuthLenSet((M - 2) >> 1);
    AESCCMLengthLenSet(L - 1);
    
    if(mode == AES_MODE_ENCRYPT)
    {
        AESEncDecSet(AES_CTL_ENC);
    }
    else
    {
        AESEncDecSet(AES_CTL_DEC);
    }
    
//    AESClockDiv2Dis();
    AESDMAEn();
    
    AESKeySet( 0, user_key[0]);
    AESKeySet( 4, user_key[1]);
    AESKeySet( 8, user_key[2]);
    AESKeySet(12, user_key[3]);
    
    if(key_len_bits >= 192)
    {
        AESKeySet(16, user_key[4]);
        AESKeySet(20, user_key[5]);
    }
    
    if(key_len_bits == 256)
    {
        AESKeySet(24, user_key[6]);
        AESKeySet(28, user_key[7]);
    }

    AESAADLen0Set(aad_len_byte);
    
    AESPayloadLenSet(input_len_byte);

    AESIVSet( 0, ((L - 1)  << 24) | (nonce[0] << 16) | (nonce[1] << 8) | nonce[2]);
    AESIVSet( 4, (nonce[3] << 24) | (nonce[4] << 16) | (nonce[5] << 8) | nonce[6]);
    
    for(i = 0; i < 2; i++)
    {
        for(j = 0; j < 4; j++)
        {
            tempIV[i][j] = 0;
        }
    }
    
    if(NonceLen > 7)
    {
        tempIV[0][0] = nonce[7];
    }
    
    if(NonceLen > 8)
    {
        tempIV[0][1] = nonce[8];
    }
    
    if(NonceLen > 9)
    {
        tempIV[0][2] = nonce[9];
    }
    
    if(NonceLen > 10)
    {
        tempIV[0][3] = nonce[10];
    }
    
    if(NonceLen > 11)
    {
        tempIV[1][0] = nonce[11];
    }
    
    if(NonceLen > 12)
    {
        tempIV[1][1] = nonce[12];
    }
    
    if(NonceLen > 13)
    {
        tempIV[1][2] = nonce[13];
    }
    
    AESIVSet( 8, (tempIV[0][0] << 24) | (tempIV[0][1] << 16) | (tempIV[0][2] << 8) | tempIV[0][3]);
    AESIVSet(12, (tempIV[1][0] << 24) | (tempIV[1][1] << 16) | (tempIV[1][2] << 8) | tempIV[1][3]);
    
//    // AES_IO_Mode
//    // AAD
//    len_word = (aad_len_byte + 3) / 4;
//    
//    for(i = 0; i < len_word / 4; i++)
//    {
//        AESBlockDataInput( 0, aad[16*i+0]  << 24 | aad[16*i+1]  << 16 | aad[16*i+2]  << 8 | aad[16*i+3]);
//        AESBlockDataInput( 4, aad[16*i+4]  << 24 | aad[16*i+5]  << 16 | aad[16*i+6]  << 8 | aad[16*i+7]);
//        AESBlockDataInput( 8, aad[16*i+8]  << 24 | aad[16*i+9]  << 16 | aad[16*i+10] << 8 | aad[16*i+11]);
//        AESBlockDataInput(12, aad[16*i+12] << 24 | aad[16*i+13] << 16 | aad[16*i+14] << 8 | aad[16*i+15]);
//        
//        AESBlockStart();
//        AESBlockTransWaitDone();
//    }
//    
//    if(len_word % 4 == 1)
//    {
//        AESBlockDataInput( 0, aad[16*i+0]  << 24 | aad[16*i+1]  << 16 | aad[16*i+2]  << 8 | aad[16*i+3]);
//        AESBlockDataInput( 4, 0x00000000);
//        AESBlockDataInput( 8, 0x00000000);
//        AESBlockDataInput(12, 0x00000000);
//        
//        AESBlockStart();
//        AESBlockTransWaitDone();
//    }
//    else if(len_word % 4 == 2)
//    {
//        AESBlockDataInput( 0, aad[16*i+0]  << 24 | aad[16*i+1]  << 16 | aad[16*i+2]  << 8 | aad[16*i+3]);
//        AESBlockDataInput( 4, aad[16*i+4]  << 24 | aad[16*i+5]  << 16 | aad[16*i+6]  << 8 | aad[16*i+7]);
//        AESBlockDataInput( 8, 0x00000000);
//        AESBlockDataInput(12, 0x00000000);
//        
//        AESBlockStart();
//        AESBlockTransWaitDone();
//    }
//    else if(len_word % 4 == 3)
//    {
//        AESBlockDataInput( 0, aad[16*i+0]  << 24 | aad[16*i+1]  << 16 | aad[16*i+2]  << 8 | aad[16*i+3]);
//        AESBlockDataInput( 4, aad[16*i+4]  << 24 | aad[16*i+5]  << 16 | aad[16*i+6]  << 8 | aad[16*i+7]);
//        AESBlockDataInput( 8, aad[16*i+8]  << 24 | aad[16*i+9]  << 16 | aad[16*i+10] << 8 | aad[16*i+11]);
//        AESBlockDataInput(12, 0x00000000);
//        
//        AESBlockStart();
//        AESBlockTransWaitDone();
//    }

//    
//    // Message
//    len_word = (input_len_byte + 3) / 4;
//    
//    for(i = 0; i < len_word / 4; i++)
//    {
//        AESBlockDataInput( 0, input_message[16*i+0]  << 24 | input_message[16*i+1]  << 16 | input_message[16*i+2]  << 8 | input_message[16*i+3]);
//        AESBlockDataInput( 4, input_message[16*i+4]  << 24 | input_message[16*i+5]  << 16 | input_message[16*i+6]  << 8 | input_message[16*i+7]);
//        AESBlockDataInput( 8, input_message[16*i+8]  << 24 | input_message[16*i+9]  << 16 | input_message[16*i+10] << 8 | input_message[16*i+11]);
//        AESBlockDataInput(12, input_message[16*i+12] << 24 | input_message[16*i+13] << 16 | input_message[16*i+14] << 8 | input_message[16*i+15]);
//        AESBlockStart();
//        AESBlockTransWaitDone();

//        tmp = AESBlockDataOutput(0);
//        output_message[16*i+0] = (unsigned char)tmp;
//        output_message[16*i+1] = (unsigned char)(tmp >> 8);
//        output_message[16*i+2] = (unsigned char)(tmp >> 16);
//        output_message[16*i+3] = (unsigned char)(tmp >> 24);
//        
//        tmp = AESBlockDataOutput(4);
//        output_message[16*i+4] = (unsigned char)tmp;
//        output_message[16*i+5] = (unsigned char)(tmp >> 8);
//        output_message[16*i+6] = (unsigned char)(tmp >> 16);
//        output_message[16*i+7] = (unsigned char)(tmp >> 24);
//        
//        tmp = AESBlockDataOutput(8);
//        output_message[16*i+8]  = (unsigned char)tmp;
//        output_message[16*i+9]  = (unsigned char)(tmp >> 8);
//        output_message[16*i+10] = (unsigned char)(tmp >> 16);
//        output_message[16*i+11] = (unsigned char)(tmp >> 24);
//        
//        tmp = AESBlockDataOutput(12);
//        output_message[16*i+12] = (unsigned char)tmp;
//        output_message[16*i+13] = (unsigned char)(tmp >> 8);
//        output_message[16*i+14] = (unsigned char)(tmp >> 16);
//        output_message[16*i+15] = (unsigned char)(tmp >> 24);
//    }
//    
//    if(len_word % 4 == 1)
//    {
//        AESBlockDataInput( 0, input_message[16*i+0]  << 24 | input_message[16*i+1]  << 16 | input_message[16*i+2]  << 8 | input_message[16*i+3]);
//        AESBlockDataInput( 4, 0x00000000);
//        AESBlockDataInput( 8, 0x00000000);
//        AESBlockDataInput(12, 0x00000000);
//        
//        AESBlockStart();
//        AESBlockTransWaitDone();
//        
//        tmp = AESBlockDataOutput(0);
//        output_message[16*i+0] = (unsigned char)tmp;
//        output_message[16*i+1] = (unsigned char)(tmp >> 8);
//        output_message[16*i+2] = (unsigned char)(tmp >> 16);
//        output_message[16*i+3] = (unsigned char)(tmp >> 24);
//    }
//    else if(len_word % 4 == 2)
//    {
//        AESBlockDataInput( 0, input_message[16*i+0]  << 24 | input_message[16*i+1]  << 16 | input_message[16*i+2]  << 8 | input_message[16*i+3]);
//        AESBlockDataInput( 4, input_message[16*i+4]  << 24 | input_message[16*i+5]  << 16 | input_message[16*i+6]  << 8 | input_message[16*i+7]);
//        AESBlockDataInput( 8, 0x00000000);
//        AESBlockDataInput(12, 0x00000000);
//        
//        AESBlockStart();
//        AESBlockTransWaitDone();
//        
//        tmp = AESBlockDataOutput(0);
//        output_message[16*i+0] = (unsigned char)tmp;
//        output_message[16*i+1] = (unsigned char)(tmp >> 8);
//        output_message[16*i+2] = (unsigned char)(tmp >> 16);
//        output_message[16*i+3] = (unsigned char)(tmp >> 24);
//        
//        tmp = AESBlockDataOutput(4);
//        output_message[16*i+4] = (unsigned char)tmp;
//        output_message[16*i+5] = (unsigned char)(tmp >> 8);
//        output_message[16*i+6] = (unsigned char)(tmp >> 16);
//        output_message[16*i+7] = (unsigned char)(tmp >> 24);
//    }
//    else if(len_word % 4 == 3)
//    {
//        AESBlockDataInput( 0, input_message[16*i+0]  << 24 | input_message[16*i+1]  << 16 | input_message[16*i+2]  << 8 | input_message[16*i+3]);
//        AESBlockDataInput( 4, input_message[16*i+4]  << 24 | input_message[16*i+5]  << 16 | input_message[16*i+6]  << 8 | input_message[16*i+7]);
//        AESBlockDataInput( 8, input_message[16*i+8]  << 24 | input_message[16*i+9]  << 16 | input_message[16*i+10] << 8 | input_message[16*i+11]);
//        AESBlockDataInput(12, 0x00000000);
//        
//        AESBlockStart();
//        AESBlockTransWaitDone();
//        
//        tmp = AESBlockDataOutput(0);
//        output_message[16*i+0] = (unsigned char)tmp;
//        output_message[16*i+1] = (unsigned char)(tmp >> 8);
//        output_message[16*i+2] = (unsigned char)(tmp >> 16);
//        output_message[16*i+3] = (unsigned char)(tmp >> 24);
//        
//        tmp = AESBlockDataOutput(4);
//        output_message[16*i+4] = (unsigned char)tmp;
//        output_message[16*i+5] = (unsigned char)(tmp >> 8);
//        output_message[16*i+6] = (unsigned char)(tmp >> 16);
//        output_message[16*i+7] = (unsigned char)(tmp >> 24);
//        
//        tmp = AESBlockDataOutput(8);
//        output_message[16*i+8]  = (unsigned char)tmp;
//        output_message[16*i+9]  = (unsigned char)(tmp >> 8);
//        output_message[16*i+10] = (unsigned char)(tmp >> 16);
//        output_message[16*i+11] = (unsigned char)(tmp >> 24);
//    }
  
    DMAChannelControlSet(DMA_CHANNEL_2,DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
    DMAChannelControlSet(DMA_CHANNEL_3,DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
    
    AESBlockStart();
    
#ifdef PLATFORM_ARM_M3
    DMAChannelTransferSet(DMA_CHANNEL_2, (void *)aad, (void *)AES_DATA_IN_BASE, aad_len_byte);
#endif

#ifdef PLATFORM_DSP_F1
    DMAChannelTransferSet(DMA_CHANNEL_2, (void *)DSP_DMA_Addr_Translation((unsigned long)aad), (void *)AES_DATA_DMA_IN_BASE, aad_len_byte);
#endif

    DMAChannelRequest(DMA_CHANNEL_2);
    DMAChannelWaitIdle(DMA_CHANNEL_2);
    DMAIntClear(DMA_CHANNEL_2);
    
#ifdef PLATFORM_ARM_M3
    DMAChannelTransferSet(DMA_CHANNEL_2, (void *)input_message, (void *)AES_DATA_IN_BASE, input_len_byte);
    DMAChannelTransferSet(DMA_CHANNEL_3, (void *)AES_DATA_OUT_BASE, (void *)output_message, input_len_byte);
#endif

#ifdef PLATFORM_DSP_F1
    DMAChannelTransferSet(DMA_CHANNEL_2, (void *)DSP_DMA_Addr_Translation((unsigned long)input_message), (void *)AES_DATA_DMA_IN_BASE, input_len_byte);
    DMAChannelTransferSet(DMA_CHANNEL_3, (void *)AES_DATA_DMA_OUT_BASE, (void *)DSP_DMA_Addr_Translation((unsigned long)output_message), input_len_byte);
#endif
    
    DMAChannelRequest(DMA_CHANNEL_2);
    DMAChannelRequest(DMA_CHANNEL_3);

    DMAWaitAESDone();
    
    AESDMADis();
    
    // Digest
    tmp = AESTagGet(0);
    digest[3] = (unsigned char)tmp;
    digest[2] = (unsigned char)(tmp >> 8);
    digest[1] = (unsigned char)(tmp >> 16);
    digest[0] = (unsigned char)(tmp >> 24);
    
    tmp = AESTagGet(4);
    digest[7] = (unsigned char)tmp;
    digest[6] = (unsigned char)(tmp >> 8);
    digest[5] = (unsigned char)(tmp >> 16);
    digest[4] = (unsigned char)(tmp >> 24);
    
    tmp = AESTagGet(8);
    digest[11] = (unsigned char)tmp;
    digest[10] = (unsigned char)(tmp >> 8);
    digest[9]  = (unsigned char)(tmp >> 16);
    digest[8]  = (unsigned char)(tmp >> 24);
    
    tmp = AESTagGet(12);
    digest[15] = (unsigned char)tmp;
    digest[14] = (unsigned char)(tmp >> 8);
    digest[13] = (unsigned char)(tmp >> 16);
    digest[12] = (unsigned char)(tmp >> 24);
}
#endif

void AES_128_EEA2(unsigned char *pucMessage, unsigned char *pucKey, unsigned char *pucCount, unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit, unsigned char *pucOutMsg)
{
	unsigned long ulLenByte = 0;
    unsigned long ulLenBlock = 0;
    unsigned long ulIV[2] = {0};
//	unsigned long i;
//  unsigned long tmp;
	
	ulIV[0] = (unsigned long)(pucCount[0] << 24 | pucCount[1] << 16 | pucCount[2] << 8 | pucCount[3]);
	ulIV[1] = (unsigned long)((ucBearer << 27) | (ucDir << 26));
	
    AESKeyLenSet(AES_CTL_KEY_LEN_128);
    
	AESKeySet( 0, pucKey[0]  << 24 | pucKey[1]  << 16 | pucKey[2]  << 8 | pucKey[3]);
	AESKeySet( 4, pucKey[4]  << 24 | pucKey[5]  << 16 | pucKey[6]  << 8 | pucKey[7]);
	AESKeySet( 8, pucKey[8]  << 24 | pucKey[9]  << 16 | pucKey[10] << 8 | pucKey[11]);
	AESKeySet(12, pucKey[12] << 24 | pucKey[13] << 16 | pucKey[14] << 8 | pucKey[15]);
	
    ulLenByte  = (ulLengthBit + 7) >> 3;
	ulLenBlock = (ulLengthBit + 127) >> 7;
	
	AESModeSet(AES_CTL_MODE_CTR);
	
    AESEncDecSet(AES_CTL_ENC);
	
//	AESClockDiv2Dis();
    
    AESDMAEn();
	
	AESPayloadLenSet(ulLenBlock << 4);

	AESIVSet( 0, ulIV[0]);
	AESIVSet( 4, ulIV[1]);
	AESIVSet( 8, 0x00000000);
	AESIVSet(12, 0x00000000);
	
//	// AES IO Mode (DMA Mode is Better.)
//	for(i = 0; i < ulLenBlock; i++)
//	{
//		
//		AESBlockDataInput( 0, pucMessage[16*i+3]  << 24 | pucMessage[16*i+2]  << 16 | pucMessage[16*i+1]  << 8 | pucMessage[16*i+0]);
//		AESBlockDataInput( 4, pucMessage[16*i+7]  << 24 | pucMessage[16*i+6]  << 16 | pucMessage[16*i+5]  << 8 | pucMessage[16*i+4]);
//		AESBlockDataInput( 8, pucMessage[16*i+11] << 24 | pucMessage[16*i+10] << 16 | pucMessage[16*i+9]  << 8 | pucMessage[16*i+8]);
//		AESBlockDataInput(12, pucMessage[16*i+15] << 24 | pucMessage[16*i+14] << 16 | pucMessage[16*i+13] << 8 | pucMessage[16*i+12]);
//		
//		AESBlockStart();
//		AESBlockTransWaitDone();
//		
//		tmp = AESBlockDataOutput(0);
//		pucOutMsg[16*i+0] = (unsigned char)tmp;
//		pucOutMsg[16*i+1] = (unsigned char)(tmp >> 8);
//		pucOutMsg[16*i+2] = (unsigned char)(tmp >> 16);
//		pucOutMsg[16*i+3] = (unsigned char)(tmp >> 24);
//		
//		tmp = AESBlockDataOutput(4);
//		pucOutMsg[16*i+4] = (unsigned char)tmp;
//		pucOutMsg[16*i+5] = (unsigned char)(tmp >> 8);
//		pucOutMsg[16*i+6] = (unsigned char)(tmp >> 16);
//		pucOutMsg[16*i+7] = (unsigned char)(tmp >> 24);
//		
//		tmp = AESBlockDataOutput(8);
//		pucOutMsg[16*i+8]  = (unsigned char)tmp;
//		pucOutMsg[16*i+9]  = (unsigned char)(tmp >> 8);
//		pucOutMsg[16*i+10] = (unsigned char)(tmp >> 16);
//		pucOutMsg[16*i+11] = (unsigned char)(tmp >> 24);
//		
//		tmp = AESBlockDataOutput(12);
//		pucOutMsg[16*i+12] = (unsigned char)tmp;
//		pucOutMsg[16*i+13] = (unsigned char)(tmp >> 8);
//		pucOutMsg[16*i+14] = (unsigned char)(tmp >> 16);
//		pucOutMsg[16*i+15] = (unsigned char)(tmp >> 24);
//	}
	
	// AES DMA Mode
	DMAChannelControlSet(DMA_CHANNEL_2,DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
	DMAChannelControlSet(DMA_CHANNEL_3,DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
	
	AESBlockStart();

#ifdef PLATFORM_ARM_M3
    DMAChannelTransferSet(DMA_CHANNEL_2, (void *)pucMessage, (void *)AES_DATA_IN_BASE, ulLenByte);
    DMAChannelTransferSet(DMA_CHANNEL_3, (void *)AES_DATA_OUT_BASE, (void *)pucOutMsg, ulLenByte);
#endif

#ifdef PLATFORM_DSP_F1
    DMAChannelTransferSet(DMA_CHANNEL_2, (void *)DSP_DMA_Addr_Translation((unsigned long)pucMessage), (void *)AES_DATA_DMA_IN_BASE, ulLenByte);
    DMAChannelTransferSet(DMA_CHANNEL_3, (void *)AES_DATA_DMA_OUT_BASE, (void *)DSP_DMA_Addr_Translation((unsigned long)pucOutMsg), ulLenByte);
#endif
	
	DMAChannelRequest(DMA_CHANNEL_2);
	DMAChannelRequest(DMA_CHANNEL_3);
	
	DMAWaitAESDone();
    
    AESDMADis();
}

void AES_128_EIA2(unsigned char *pucMessage, unsigned char *pucKey, unsigned char *pucCount, unsigned char ucBearer, unsigned char ucDir, unsigned long ulLengthBit, unsigned char *pucMACT)
{
	unsigned long i, j;
	unsigned long ulMsgByte = 0;
	unsigned long ulLastBlockOffset = 0;
	unsigned char ucM[400] = {0};
//    unsigned long tmp;
    
    unsigned char pucK1[AES_BLOCK_BYTES];
    unsigned char pucK2[AES_BLOCK_BYTES];
	
    AES_CMAC_Generate_Subkey(pucKey, pucK1, pucK2);
	
	ucM[0] = pucCount[0];
	ucM[1] = pucCount[1];
	ucM[2] = pucCount[2];
	ucM[3] = pucCount[3];
	ucM[4] = ((ucBearer << 3) | (ucDir << 2));
	ucM[5] = 0;
	ucM[6] = 0;
	ucM[7] = 0;
	
	ulMsgByte = (ulLengthBit + 7) >> 3;
	
	// Copy Message to unM (Should use DMA)
	for(i = 0; i < ulMsgByte; i++)
	{
		ucM[8+i] = pucMessage[i];
	}
	
	if(ulLengthBit % 128 == 64)
	{
		// Complete Block, No Pad.
		ulLastBlockOffset = ulMsgByte - 8;
		
		// Mn XOR K1
		i = 0;
		for(j = ulLastBlockOffset; j < ulLastBlockOffset + AES_BLOCK_BYTES; j++)
		{
			ucM[j] ^= pucK1[i++];
		}
	}
	else
	{
		// Need to Pad.
		ulLastBlockOffset = ((8 + (ulLengthBit / 8)) >> 4) << 4;
		
		if(ulLengthBit % 8 == 0)
		{
			ucM[8+i] = 0x80;
			
			for(j = 9+i; j < ulLastBlockOffset + AES_BLOCK_BYTES; j++)
			{
				ucM[j] = 0;
			}
		}
		else
		{
			ucM[7+i] |= (1 << (7 - ulLengthBit % 8));
			
			for(j = 8+i; j < ulLastBlockOffset + AES_BLOCK_BYTES; j++)
			{
				ucM[j] = 0;
			}
		}
		
		// Mn XOR K2
		i = 0;
		for(j = ulLastBlockOffset; j < ulLastBlockOffset + AES_BLOCK_BYTES; j++)
		{
			ucM[j] ^= pucK2[i++];
		}
	}
	
	// AES CBC Mode
	AESModeSet(AES_CTL_MODE_CBC);
	
    AESEncDecSet(AES_CTL_ENC);
    
//    AESClockDiv2Dis();
    
    AESDMAEn();
    
    AESKeyLenSet(AES_CTL_KEY_LEN_128);
    
	AESKeySet( 0, pucKey[0]  << 24 | pucKey[1]  << 16 | pucKey[2]  << 8 | pucKey[3]);
	AESKeySet( 4, pucKey[4]  << 24 | pucKey[5]  << 16 | pucKey[6]  << 8 | pucKey[7]);
	AESKeySet( 8, pucKey[8]  << 24 | pucKey[9]  << 16 | pucKey[10] << 8 | pucKey[11]);
	AESKeySet(12, pucKey[12] << 24 | pucKey[13] << 16 | pucKey[14] << 8 | pucKey[15]);
	
	AESPayloadLenSet(ulLastBlockOffset + AES_BLOCK_BYTES);

	AESIVSet( 0, 0x00000000);
	AESIVSet( 4, 0x00000000);
	AESIVSet( 8, 0x00000000);
	AESIVSet(12, 0x00000000);
	
//	// AES IO Mode (DMA Mode is Better.)
//	for(i = 0; i < (ulLastBlockOffset+16) / AES_BLOCK_BYTES; i++)
//	{
//		
//		AESBlockDataInput( 0, ucM[16*i+3]  << 24 | ucM[16*i+2]  << 16 | ucM[16*i+1]  << 8 | ucM[16*i+0]);
//		AESBlockDataInput( 4, ucM[16*i+7]  << 24 | ucM[16*i+6]  << 16 | ucM[16*i+5]  << 8 | ucM[16*i+4]);
//		AESBlockDataInput( 8, ucM[16*i+11] << 24 | ucM[16*i+10] << 16 | ucM[16*i+9]  << 8 | ucM[16*i+8]);
//		AESBlockDataInput(12, ucM[16*i+15] << 24 | ucM[16*i+14] << 16 | ucM[16*i+13] << 8 | ucM[16*i+12]);
//		
//		AESBlockStart();
//		AESBlockTransWaitDone();
//		
//		tmp = AESBlockDataOutput(0);
//		pucMACT[16*i+0] = (unsigned char)tmp;
//		pucMACT[16*i+1] = (unsigned char)(tmp >> 8);
//		pucMACT[16*i+2] = (unsigned char)(tmp >> 16);
//		pucMACT[16*i+3] = (unsigned char)(tmp >> 24);
//		
//		tmp = AESBlockDataOutput(4);
//		pucMACT[16*i+4] = (unsigned char)tmp;
//		pucMACT[16*i+5] = (unsigned char)(tmp >> 8);
//		pucMACT[16*i+6] = (unsigned char)(tmp >> 16);
//		pucMACT[16*i+7] = (unsigned char)(tmp >> 24);
//		
//		tmp = AESBlockDataOutput(8);
//		pucMACT[16*i+8]  = (unsigned char)tmp;
//		pucMACT[16*i+9]  = (unsigned char)(tmp >> 8);
//		pucMACT[16*i+10] = (unsigned char)(tmp >> 16);
//		pucMACT[16*i+11] = (unsigned char)(tmp >> 24);
//		
//		tmp = AESBlockDataOutput(12);
//		pucMACT[16*i+12] = (unsigned char)tmp;
//		pucMACT[16*i+13] = (unsigned char)(tmp >> 8);
//		pucMACT[16*i+14] = (unsigned char)(tmp >> 16);
//		pucMACT[16*i+15] = (unsigned char)(tmp >> 24);
//	}
	
	// AES DMA Mode
	DMAChannelControlSet(DMA_CHANNEL_2,DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
	DMAChannelControlSet(DMA_CHANNEL_3,DMAC_CTRL_DINC_SET | DMAC_CTRL_SINC_SET | DMAC_CTRL_TC_SET | DMAC_CTRL_INT_SET | DMAC_CTRL_TYPE_MEM_TO_MEM | DMAC_CTRL_BURST_SIZE_4W | DMAC_CTRL_WORD_SIZE_32b);
	
	AESBlockStart();
    
#ifdef PLATFORM_ARM_M3
    DMAChannelTransferSet(DMA_CHANNEL_2, (void *)ucM, (void *)AES_DATA_IN_BASE, ulLastBlockOffset + AES_BLOCK_BYTES);
    DMAChannelTransferSet(DMA_CHANNEL_3, (void *)AES_DATA_OUT_BASE, (void *)ucM, ulLastBlockOffset + AES_BLOCK_BYTES);
#endif

#ifdef PLATFORM_DSP_F1
    DMAChannelTransferSet(DMA_CHANNEL_2, (void *)DSP_DMA_Addr_Translation((unsigned long)ucM), (void *)AES_DATA_DMA_IN_BASE, ulLastBlockOffset + AES_BLOCK_BYTES);
    DMAChannelTransferSet(DMA_CHANNEL_3, (void *)AES_DATA_DMA_OUT_BASE, (void *)DSP_DMA_Addr_Translation((unsigned long)ucM), ulLastBlockOffset + AES_BLOCK_BYTES);
#endif
	
	DMAChannelRequest(DMA_CHANNEL_2);
	DMAChannelRequest(DMA_CHANNEL_3);
	
	DMAWaitAESDone();
    
    AESDMADis();
    
    pucMACT[0] = ucM[ulLastBlockOffset];
    pucMACT[1] = ucM[ulLastBlockOffset + 1];
    pucMACT[2] = ucM[ulLastBlockOffset + 2];
    pucMACT[3] = ucM[ulLastBlockOffset + 3];
}

#if !USE_ROM_AES
/* Basic Functions For CMAC*/
void AES_CMAC_XOR_128(unsigned char *pucA, const unsigned char *pucB, unsigned char *pucOut)
{
    unsigned char i;
  
    for (i = 0; i < AES_BLOCK_BYTES; i++)
    {
        pucOut[i] = pucA[i] ^ pucB[i];
    }
}

void AES_CMAC_Leftshift_Onebit(unsigned char *pucInput, unsigned char *pucOutput)
{
	signed char i;
	unsigned char overflow = 0;
	
	for(i = 15; i >= 0; i--)
	{
		pucOutput[i] = pucInput[i] << 1;
		pucOutput[i] |= overflow;
		overflow = (pucInput[i] & 0x80) ? 1 : 0;
	}
}

void AES_CMAC_Generate_Subkey(unsigned char *pucKey, unsigned char *pucK1, unsigned char *pucK2)
{
	unsigned char L[AES_BLOCK_BYTES];
	unsigned char Z[AES_BLOCK_BYTES];
	unsigned char tmp[AES_BLOCK_BYTES];
	unsigned char i;

	for(i = 0; i < AES_BLOCK_BYTES; i++)
	{
		Z[i] = 0;
	}
	
    AESECB(Z, AES_BLOCK_BYTES, pucKey, 128, AES_MODE_ENCRYPT, L);

	if((L[0] & 0x80) == 0)
	{
		/* If MSB(L) = 0, then K1 = L << 1 */
		AES_CMAC_Leftshift_Onebit(L, pucK1);
	}
	else
	{
		/* Else K1 = ( L << 1 ) (+) Rb */
		AES_CMAC_Leftshift_Onebit(L, tmp);
		AES_CMAC_XOR_128(tmp, AES_CMAC_Rb, pucK1);
	}

	if((pucK1[0] & 0x80) == 0)
	{
		AES_CMAC_Leftshift_Onebit(pucK1, pucK2);
	}
	else
	{
		AES_CMAC_Leftshift_Onebit(pucK1, tmp);
		AES_CMAC_XOR_128(tmp, AES_CMAC_Rb, pucK2);
	}
}
#endif

