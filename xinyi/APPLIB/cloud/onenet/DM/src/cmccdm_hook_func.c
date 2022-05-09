/** 
* @file        cmccdm_hook_func.c
* @brief       中移的DM自注册功能二次开发的hook接口
*/

#include "factory_nv.h"
#include <string.h>


/**
 * Get device info
**/
void cmccdm_getDevinfo(char* devInfo, unsigned int len)
{
	char* dev = "unknown";
	memcpy(devInfo, dev, strlen(dev)<len?strlen(dev):len);
}

/**
 * Get app info
**/
void cmccdm_getAppinfo(char* appInfo, unsigned int len)
{
	char* app = "unknown";
	memcpy(appInfo, app, strlen(app)<len?strlen(app):len);
}

/**
 * Get app info
**/
void cmccdm_getMacinfo(char* macInfo, unsigned int len)
{
	char* mac = "unknown";
	memcpy(macInfo, mac, strlen(mac)<len?strlen(mac):len);
}

/**
 * Get rom size
**/
void cmccdm_getRominfo(char* romInfo, unsigned int len)
{
	char* rom = "2MB";
	memcpy(romInfo, rom, strlen(rom)<len?strlen(rom):len);
}

/**
 * Get ram size
**/
void cmccdm_getRaminfo(char* ramInfo, unsigned int len)
{
	char* ram = "900KB";
	memcpy(ramInfo, ram, strlen(ram)<len?strlen(ram):len);
}

/**
 * Get CPU info
**/
void cmccdm_getCpuinfo(char* CpuInfo, unsigned int len)
{
	char* cpu = "unknown";
	memcpy(CpuInfo, cpu, strlen(cpu)<len?strlen(cpu):len);
}

/**
 * Get system version
**/
void cmccdm_getSysinfo(char* sysInfo, unsigned int len)
{
	char* sysVerion = "LiteOSV200R001C50B021";
	memcpy(sysInfo, sysVerion, strlen(sysVerion)<len?strlen(sysVerion):len);
}

/**
 * Get soft firmware verion
**/
void cmccdm_getSoftVer(char* softInfo, unsigned int len)
{
	char* fmware = (char *)(g_softap_fac_nv->versionExt);
	memcpy(softInfo, fmware, strlen(fmware)<len?strlen(fmware):len);
}

/**
 * Get soft firmware name
**/
void cmccdm_getSoftName(char* softname, unsigned int len)
{
	char* fmname = "XY1100";
	memcpy(softname, fmname, strlen(fmname)<len?strlen(fmname):len);
}

/**
 * Get Volte info
**/
void cmccdm_getVolteinfo(char* volInfo, unsigned int len)
{
	char* volte = "0";
	memcpy(volInfo, volte, strlen(volte)<len?strlen(volte):len);
}

/**
 * Get net type
**/
void cmccdm_getNetType(char* netType, unsigned int len)
{
	char* type = "NBIoT";
	memcpy(netType, type, strlen(type)<len?strlen(type):len);
}

/**
 * Get net account
**/
void cmccdm_getNetAccount(char* netInfo, unsigned int len)
{
	char* account = "unknown";
	memcpy(netInfo, account, strlen(account)<len?strlen(account):len);
}

/**
 * Get phone number
**/
void cmccdm_getPNumber(char* pNumber, unsigned int len)
{
	char* number = "unknown";
	memcpy(pNumber, number, strlen(number)<len?strlen(number):len);
}

/**
 * Get location info
**/
void cmccdm_getLocinfo(char* locInfo, unsigned int len)
{
	char* loction = "unknown";
	memcpy(locInfo, loction, strlen(loction)<len?strlen(loction):len);
}

/**
 * Get route mac
**/
void cmccdm_getRouteMac(char* routeMac, unsigned int len)
{
	char* route = "unknown";
	memcpy(routeMac, route, strlen(route)<len?strlen(route):len);
}

/**
 * Get brand info
**/
void cmccdm_getBrandinfo(char* brandInfo, unsigned int len)
{
	char* brand = "XY1100";
	memcpy(brandInfo, brand, strlen(brand)<len?strlen(brand):len);
}

/**
 * Get GPU info
**/
void cmccdm_getGPUinfo(char* GPUInfo, unsigned int len)
{
	char* gpu = "unknown";
	memcpy(GPUInfo, gpu, strlen(gpu)<len?strlen(gpu):len);
}

/**
 * Get Board info
**/
void cmccdm_getBoardinfo(char* boardInfo, unsigned int len)
{
	char* board = "unknown";
	memcpy(boardInfo, board, strlen(board)<len?strlen(board):len);
}

/**
 * Get Model info
**/
void cmccdm_getModelinfo(char* modelInfo, unsigned int len)
{
	char* model = "1100";
	memcpy(modelInfo, model, strlen(model)<len?strlen(model):len);
}

/**
 * Get resolution info
**/
void cmccdm_getResinfo(char* resInfo, unsigned int len)
{
	char* resolution = "unknown";
	memcpy(resInfo, resolution, strlen(resolution)<len?strlen(resolution):len);
}



