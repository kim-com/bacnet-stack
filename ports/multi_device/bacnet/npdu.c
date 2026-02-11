/**
 * @file
 * @brief Wrapper for npdu_encode_pdu function to add routing with virtual
 * device support
 * @author Kim Hyeongjun <kimhyeongjun@samsung.com>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include "bacnet/npdu.h"
#include "bacnet/bacdef.h"

/* for npdu_encode_pdu_routing */
#include "bacnet/basic/object/device.h"
#include "bacnet/bacaddr.h"
#include "bacnet/datalink/datalink.h"

/* for debug log*/
#include "bacnet/basic/sys/debug.h"

#ifdef BAC_ROUTING
static int npdu_encode_pdu_routing(BACNET_ADDRESS *src)
{
    DEVICE_OBJECT_DATA *pDev = NULL;
    /* 현재 활성화된 디바이스 정보 획득 */
    pDev = Get_Routed_Device_Object(-1);
    if (pDev == NULL) {
        debug_fprintf(stderr, "Device object not found\n");
        return -1;
    }

    bool is_routed_device = (pDev->bacDevAddr.net != 0);

    if (is_routed_device) {
        bacnet_address_copy(src, &pDev->bacDevAddr);
        debug_fprintf(stdout, 
            "Virtual Device NPDU: SNET=%d, Instance=%u\n", pDev->bacDevAddr.net,
            pDev->bacObj.Object_Instance_Number);
    } else {
        datalink_get_my_address(src);
        debug_fprintf(stdout, 
            "Gateway Device NPDU: Instance=%u\n",
            pDev->bacObj.Object_Instance_Number);
    }

    return 1;
}
#endif

extern int __real_npdu_encode_pdu(
    uint8_t *npdu,
    BACNET_ADDRESS *dest,
    BACNET_ADDRESS *src,
    const BACNET_NPDU_DATA *npdu_data);

BACNET_STACK_EXPORT
int __wrap_npdu_encode_pdu(
    uint8_t *npdu,
    BACNET_ADDRESS *dest,
    BACNET_ADDRESS *src,
    const BACNET_NPDU_DATA *npdu_data)
{
#ifdef BAC_ROUTING
    if (npdu_encode_pdu_routing(src) < 0) {
        return -1;
    }
#endif
    return __real_npdu_encode_pdu(npdu, dest, src, npdu_data);
}