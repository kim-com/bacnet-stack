/**
 * @file
 * @brief Example gateway_with_virtual_device application using the BACnet
 * Stack. Code for this project began with code from the apps/gateway project
 * @author Kim Hyeongjun <hjun1.kim@samsung.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/iam.h"
#include "bacnet/dcc.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
/* include the device object */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/object/lc.h"
#ifdef BACNET_TEST_VMAC
#include "bacnet/basic/bbmd6/vmac.h"
#endif
/* me! */
#include "gateway_with_virtual_device.h"
#include "bacnet/memcopy.h"

#include "bacnet/whois.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/ao.h"

/* h_rp.c */
#include "bacnet/abort.h"
#include "bacnet/reject.h"
#if (BACNET_PROTOCOL_REVISION >= 17)
#include "bacnet/basic/object/netport.h"
#endif

#include "bacnet/basic/sys/debug.h"

static void Init_Service_Handlers(void);

/* (Doxygen note: The next two lines pull all the following Javadoc
 *  into the gateway_with_virtual_device module.) */
/** @addtogroup gateway_with_virtual_device */
/*@{*/

/** Buffer used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/** The list of DNETs that our router can reach.
 *  Only one entry since we don't support downstream routers.
 */
int DNET_list[] = {
    VIRTUAL_DNET, -1 /* Need -1 terminator */
};

/* current version of the BACnet stack */
static const char *BACnet_Version = BACNET_VERSION_TEXT;

/* routed devices - I-Am on startup */
#ifdef STARTUP_SEND_IAM
static unsigned Routed_Device_Index;
#endif

/** Initialize the Gateway and Virtual Device Objects and each of the child
 Object instances.
 * @param first_object_instance Set the first (gateway) Device to this
            instance number, and subsequent devices to incremented values.
 */
static void Create_Gateway_And_Virtual_Devices(uint32_t first_object_instance)
{
    int i;
    char nameText[MAX_DEV_NAME_LEN];
    char descText[MAX_DEV_DESC_LEN];
    BACNET_CHARACTER_STRING name_string;

    Routed_Device_Set_Description(DEV_DESCR_GATEWAY, strlen(DEV_DESCR_GATEWAY));

    uint32_t gateway_ai_instance = 1000;
    Analog_Input_Create(gateway_ai_instance);
    Analog_Input_Name_Set(gateway_ai_instance, "Gateway AI 1");
    Analog_Input_Present_Value_Set(gateway_ai_instance, 25.0f);
    printf(
        "[KHJ] Gateway: Created Analog Input instance %d with name 'Gateway AI "
        "1'\n",
        gateway_ai_instance);

    for (i = 1; i < MAX_NUM_DEVICES; i++) {
        debug_fprintf(stdout,"==============================================");
        debug_fprintf(stdout,"Device Inx : %d", first_object_instance + i);
        debug_fprintf(stdout,"==============================================");
        snprintf(nameText, MAX_DEV_NAME_LEN, "%s %d", DEV_NAME_BASE, i + 1);

        snprintf(descText, MAX_DEV_DESC_LEN, "%s %d", DEV_DESCR_REMOTE, i);

        characterstring_init_ansi(&name_string, nameText);

        Add_Routed_Device((first_object_instance + i), &name_string, descText);

        uint32_t virtual_ai_instance = 2000 + i;

        static char ai_name[MAX_NUM_DEVICES][MAX_DEV_NAME_LEN] = {
            0,
        };
        snprintf(ai_name[i], MAX_DEV_NAME_LEN, "AI %d", virtual_ai_instance);

        float ai_initial_value = 20.0f + (i * 10.0f);

        Analog_Input_Create(virtual_ai_instance);
        Analog_Input_Name_Set(virtual_ai_instance, ai_name[i]);
        Analog_Input_Present_Value_Set(virtual_ai_instance, ai_initial_value);
        debug_fprintf(stdout,
            "Virtual Device %d: Created Analog Input instance %d with name "
            "'%s' (value: %.1f)",
            i, virtual_ai_instance, ai_name[i], ai_initial_value);

        uint32_t virtual_ao_instance = 3000 + i;

        static char ao_name[MAX_NUM_DEVICES][MAX_DEV_NAME_LEN] = {
            0,
        };
        snprintf(ao_name[i], MAX_DEV_NAME_LEN, "AO %d", virtual_ao_instance);

        float ao_initial_value = 20.0f + (i * 10.0f);
        uint32_t ao_instance = Analog_Output_Create(virtual_ao_instance);
        bool name_set = Analog_Output_Name_Set(virtual_ao_instance, ao_name[i]);
        bool relinquish = Analog_Output_Relinquish_Default_Set(
            virtual_ao_instance, ao_initial_value);
        bool units_set =
            Analog_Output_Units_Set(virtual_ao_instance, UNITS_DEGREES_CELSIUS);
        bool present_value_set = Analog_Output_Present_Value_Set(
            virtual_ao_instance, ao_initial_value, BACNET_MAX_PRIORITY);
        bool description_set = Analog_Output_Description_Set(
            virtual_ao_instance, "Analog Output Description");

        debug_fprintf(stdout, "ao_instance: %d", ao_instance);
        debug_fprintf(stdout, "name_set: %d", name_set);
        debug_fprintf(stdout, "relinquish: %d", relinquish);
        debug_fprintf(stdout, "units_set: %d", units_set);
        debug_fprintf(stdout, "ao_initial_value: %.2f", ao_initial_value);
        debug_fprintf(stdout, "present_value_set: %d", present_value_set);
        debug_fprintf(stdout, "description_set: %d", description_set);
        debug_fprintf(stdout, "==============================================");
    }
}

/** Initialize the BACnet Device Addresses for each Device object.
 * The gateway has already gotten the normal address (eg, PC's IP for BIP) and
 * the remote devices get
 * - For BIP, the IP address reversed, and 4th byte equal to index.
 * (Eg, 11.22.33.44 for the gateway becomes 44.33.22.01 for the first remote
 * device.) This is sure to be unique! The port number stays the same.
 * - For MS/TP, [Steve inserts a good idea here]
 */
static void Initialize_Device_Addresses(void)
{
    int i = 0; /* First entry is Gateway Device */
    // uint32_t virtual_mac = 0;
    BACNET_ADDRESS virtual_address = { 0 };
    DEVICE_OBJECT_DATA *pDev = NULL;

    pDev = Get_Routed_Device_Object(i);

#if defined(BACDL_BIP)
    bip_get_my_address(&virtual_address);

    printf(
        "[KHJ] virtual_address.mac: %d:%d:%d:%d\n", virtual_address.mac[0],
        virtual_address.mac[1], virtual_address.mac[2], virtual_address.mac[3]);
    printf(
        "[KHJ] virtual_address.net: virtual_address.net: %d\n",
        virtual_address.net);

#elif defined(BACDL_MSTP)
    dlmstp_get_my_address(&virtual_address);
#elif defined(BACDL_ARCNET)
    arcnet_get_my_address(&virtual_address);
#elif defined(BACDL_ETHERNET)
    ethernet_get_my_address(&virtual_address);
#elif defined(BACDL_BIP6)
    bip6_get_my_address(&virtual_address);
#else
#error "No support for this Data Link Layer type "
#endif

    bacnet_address_copy(&pDev->bacDevAddr, &virtual_address);
    Send_I_Am(&Handler_Transmit_Buffer[0]);
    for (i = 1; i < MAX_NUM_DEVICES; i++) {
        pDev = Get_Routed_Device_Object(i);
        if (pDev == NULL) {
            continue;
        }
        
        pDev->bacDevAddr.net = VIRTUAL_DNET;
        pDev->bacDevAddr.mac_len = 0;
        
        memset(pDev->bacDevAddr.mac, 0, MAX_MAC_LEN);
        pDev->bacDevAddr.adr[0] = 192;
        pDev->bacDevAddr.adr[1] = 168;
        pDev->bacDevAddr.adr[2] = 1;
        pDev->bacDevAddr.adr[3] = 100;

        uint16_t port = 47809 + (i - 1);
        pDev->bacDevAddr.adr[4] = (port >> 8) & 0xFF;
        pDev->bacDevAddr.adr[5] = port & 0xFF;
        pDev->bacDevAddr.len = 6;

        printf(
            "[KHJ] Virtual Device %d: DNET=%d, IP=%d.%d.%d.%d:%d\n", i,
            VIRTUAL_DNET, pDev->bacDevAddr.adr[0], pDev->bacDevAddr.adr[1],
            pDev->bacDevAddr.adr[2], pDev->bacDevAddr.adr[3], port);
    }
}

static void Initialize_Device_System(uint32_t first_object_instance)
{
    Device_Init(NULL);
    Routing_Device_Init(first_object_instance);
}

static void Initialize_Network_System(void)
{
    dlenv_init();
    atexit(datalink_cleanup);
}

static void Initialize_Service_System(void)
{
    Init_Service_Handlers();
}

static void Setup_All_Devices(uint32_t first_object_instance)
{
    Create_Gateway_And_Virtual_Devices(first_object_instance);
    Initialize_Device_Addresses();
}

static void Initialize_Router_Announcements(void)
{
    /* broadcast an I-am-router-to-network on startup */
    printf("Remote Network DNET Number %d \n", DNET_list[0]);
    Send_I_Am_Router_To_Network(DNET_list);

    for (int i = 0; DNET_list[i] != -1; i++) {
        Send_Network_Number_Is(NULL, DNET_list[i], NETWORK_NUMBER_CONFIGURED);
        printf(
            "[KHJ] Sent Network-Number-Is for DNET %d (Status: Configured)\n",
            DNET_list[i]);
    }
}

static void Main_Event_Loop(void)
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 1000; /* milliseconds */
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    uint32_t elapsed_seconds = 0;
    uint32_t elapsed_milliseconds = 0;

    /* configure the timeout values */
    last_seconds = time(NULL);

    for (;;) {
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

        if (pdu_len) {
            debug_fprintf(stdout,"Received PDU: %d bytes from network %d", pdu_len, src.net);
            routing_npdu_handler(&src, DNET_list, &Rx_Buf[0], pdu_len);
        }

        current_seconds = time(NULL);
        elapsed_seconds = current_seconds - last_seconds;
        if (elapsed_seconds) {
            last_seconds = current_seconds;
            dcc_timer_seconds(elapsed_seconds);
            datalink_maintenance_timer(elapsed_seconds);
            dlenv_maintenance_timer(elapsed_seconds);
            elapsed_milliseconds = elapsed_seconds * 1000;
            tsm_timer_milliseconds(elapsed_milliseconds);
            Device_Timer(elapsed_milliseconds);
        }

        handler_cov_task();

#ifdef STARTUP_SEND_IAM
        if (Routed_Device_Index < MAX_NUM_DEVICES) {
            Routed_Device_Index++;
            Get_Routed_Device_Object(Routed_Device_Index);
            if (Routed_Device_Index == 0) {
                Send_I_Am(&Handler_Transmit_Buffer[0]);
            } else {
                Send_I_Am_Virtual_Device(
                    Routed_Device_Index, &Handler_Transmit_Buffer[0]);
            }
        }
#endif
    }
}

/** Initialize the handlers we will utilize.
 * @see Device_Init, apdu_set_unconfirmed_handler, apdu_set_confirmed_handler
 */
static void Init_Service_Handlers(void)
{
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_RANGE, handler_read_range);

#if defined(BACFILE)
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_ATOMIC_READ_FILE, handler_atomic_read_file);

    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_ATOMIC_WRITE_FILE, handler_atomic_write_file);
#endif
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, handler_timesync_utc);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);

    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);

    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_CREATE_OBJECT, handler_create_object);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DELETE_OBJECT, handler_delete_object);
}

int main(int argc, char *argv[])
{
    uint32_t first_object_instance = FIRST_DEVICE_NUMBER;
#ifdef BACNET_TEST_VMAC
    /* Router data */
    BACNET_DEVICE_PROFILE *device;
    BACNET_VMAC_ADDRESS adr;
#endif

    /* allow the device ID to be set */
    if (argc > 1) {
        first_object_instance = strtol(argv[1], NULL, 0);
        if ((first_object_instance == 0) ||
            (first_object_instance > BACNET_MAX_INSTANCE)) {
            printf("Error: Invalid Object Instance %s \n", argv[1]);
            printf("Provide a number from 1 to %ul \n", BACNET_MAX_INSTANCE);
            exit(1);
        }
    }
    printf(
        "BACnet Router Demo\n"
        "BACnet Stack Version %s\n"
        "BACnet Device ID: %u\n"
        "Max APDU: %d\n"
        "Max Devices: %d\n",
        BACnet_Version, first_object_instance, MAX_APDU, MAX_NUM_DEVICES);

    Initialize_Device_System(first_object_instance);
    Initialize_Network_System();
    Initialize_Service_System();
    Setup_All_Devices(first_object_instance);
    Initialize_Router_Announcements();

#ifdef BACNET_TEST_VMAC
    /* initialize vmac table and router device */
    device = vmac_initialize(99, 2001);
    debug_printf(device->name, "ROUTER:%u", vmac_get_subnet());
#endif

    Main_Event_Loop();

    return 0;
}

/* End group gateway_with_virtual_device */
