/*
 *   ____ ___  ____ ___ _   _ __  __
 *  / ___/ _ \|  _ \_ _| | | |  \/  |
 * | |  | | | | | | | || | | | |  | |
 * | |__| |_| | |_| | || |_| | |  | |
 *  \____\___/|____/___|\___/|_|  |_|
 *
 * CONFIDENTIEL
 * (c) 2021 - Codium Electronique
 * Tous droits reserves
 * Ce fichier fait partie du projet ESIREM Quantum main board
 *
 * settings.c - 07/12/2021
 * Gestion des parametres en flash. Utilise le systeme NVS pour stocker
 * les donnees en flash comme spécifié dans la documentation :
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/1.5.0/zephyr
 *
 * Voir les exemples suivants :
 * https://github.com/nrfconnect/sdk-zephyr/blob/v2.4.99-ncs1/samples/subsys/nvs/src/main.c
 * https://github.com/nrfconnect/sdk-zephyr/blob/v2.4.99-ncs1/samples/subsys/settings/src/main.c
 *
 * Les données spécifiques à la carte (numéro de série, révision hardware)
 * sont stockés dans une partition spécifique de la flash. Voir la documentation
 * https://developer.nordicsemi.com/nRF_Connect_SDK/doc/1.5.0/zephyr/reference/storage/flash_map/flash_map.html
 */

#include <include/common.h>
#include <include/settings.h>

#include <bluetooth/uuid.h>
#include <device.h>
#include <drivers/flash.h>
#include <fs/nvs.h>
#include <logging/log.h>
#include <settings/settings.h>
#include <storage/flash_map.h>

#include <include/core.h>

LOG_MODULE_REGISTER(esirem_quantum_main_settings, CONFIG_LOG_MAX_LEVEL);

/*
 * Standard GATT Device Information Services characteristics
BT_UUID_DIS_SYSTEM_ID
BT_UUID_DIS_MODEL_NUMBER
BT_UUID_DIS_SERIAL_NUMBER
BT_UUID_DIS_FIRMWARE_REVISION
BT_UUID_DIS_HARDWARE_REVISION
BT_UUID_DIS_SOFTWARE_REVISION
BT_UUID_DIS_MANUFACTURER_NAME
BT_UUID_DIS_PNP_ID -> donnees d'identification fournisseur supplementaires
*/

/*
 * DIS characteristics implemented in dis.c
 * Stored in settings in package bt/dis
 * manuf -> manufacturer
 * model -> model number
 * serial -> serial number
 * fw -> firmware version
 * hw -> hardware version
 * sw -> software version
 */

const uint32_t settings_val_max_percent = 100;

int esirem_quantum_main_settings_init()
{
    int status = 0;

    LOG_DBG("Initializing settings: ");
    status = settings_subsys_init();
    if (status)
    {
        LOG_ERR(STR_LOG_ERR_STATUS, status);
        return -1;
    }
    LOG_DBG(STR_LOG_SUCCESS);

    settings_register(&esirem_quantum_main_core_settings_hdlrs);

    return 0;
}
