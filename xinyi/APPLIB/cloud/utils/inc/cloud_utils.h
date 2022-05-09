#ifndef _CLOUD_UTILS__H
#define _CLOUD_UTILS__H
#define MUTEX_LOCK_INFINITY 0xFFFFFFFF

#include "lwip/sockets.h"

typedef enum {
    CDP_IP_TYPE = 0U,
    ONENET_IP_TYPE ,
    SOCKET_IP_TYPE,
    CTWING_IP_TYPE,
    CLOUD_IP_TYPE_MAX,
}cloud_ip_type_e;

typedef struct _net_infos_t{
    unsigned short  local_port;
    unsigned short  remote_port;
    char remote_ip[40];
    ip_addr_t local_ip;
    char is_dm;
}net_infos_t;

uint64_t cloud_gettime_ms(void);
unsigned int cloud_get_ResveredMem();
int cloud_mutex_create(osMutexId_t *pMutexId);
int cloud_mutex_destroy(osMutexId_t *pMutexId);
int cloud_mutex_lock(osMutexId_t *pMutexId, uint32_t timeout);
int cloud_mutex_unlock(osMutexId_t *pMutexId);
void cloud_fota_init(void);
enum lwip_ip_addr_type cloud_get_ip_type(cloud_ip_type_e cloud_type);
void cloud_set_ip_type(cloud_ip_type_e cloud_type,enum lwip_ip_addr_type ip_addr_type);

#endif /* _CLOUD_UTILS__H */
