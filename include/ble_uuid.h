/*
 *   ____ ___  ____ ___ _   _ __  __
 *  / ___/ _ \|  _ \_ _| | | |  \/  |
 * | |  | | | | | | | || | | | |  | |
 * | |__| |_| | |_| | || |_| | |  | |
 *  \____\___/|____/___|\___/|_|  |_|
 *
 * (c) 2021 - Codium Electronique
 * Tous droits reserves
 * Ce fichier fait partie du projet ESIREM Quantum main board
 *
 * ble_uuid.h - 07/12/2021
 * Definitions macros UUIDs pour les services BLE
 */

#ifndef ESIREM_QUANTUM_MAIN_BLE_UUID_H_INCLUDED
#define ESIREM_QUANTUM_MAIN_BLE_UUID_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

#include <zephyr/types.h>

/**@brief Timeout pour process d'une requete long write complete */
#define ESIREM_QUANTUM_MAIN_BLE_LONG_WRITE_TIMEOUT_MS (300)

/**@brief Partie vendeur de l'UUID : ESIREM en hex */
#define ESIREM_QUANTUM_MAIN_BLE_UUID_PART_VENDOR 0x45534952454d

/**@brief Partie client de l'UUID : MM en hex */
#define ESIREM_QUANTUM_MAIN_BLE_UUID_PART_CLIENT 0x4d4d

/**@brief Partie projet de l'UUID : MM en hex */
#define ESIREM_QUANTUM_MAIN_BLE_UUID_PART_PROJECT 0x4d4d

/**@brief Partie ID projet de l'UUID */
#define ESIREM_QUANTUM_MAIN_BLE_UUID_PART_PROJECT_ID 0x0001

/**@brief Partie random de l'UUID (2 octets) decales pour
 * laisser la place pour les UUIDs services / caracteristiques */
#define ESIREM_QUANTUM_MAIN_BLE_UUID_PART_RANDOM (0x53a8 << 16)

/**@brief UUID de base pour le projet */
#define ESIREM_QUANTUM_MAIN_BLE_UUID_BASE_VAL                                   \
    BT_UUID_128_ENCODE(                                         \
        ESIREM_QUANTUM_MAIN_BLE_UUID_PART_RANDOM, ESIREM_QUANTUM_MAIN_BLE_UUID_PART_PROJECT_ID, \
        ESIREM_QUANTUM_MAIN_BLE_UUID_PART_PROJECT, ESIREM_QUANTUM_MAIN_BLE_UUID_PART_CLIENT,    \
        ESIREM_QUANTUM_MAIN_BLE_UUID_PART_VENDOR)

#define ESIREM_QUANTUM_MAIN_BLE_UUID_BASE BT_UUID_DECLARE_128(ESIREM_QUANTUM_MAIN_BLE_UUID_BASE_VAL)

#define ESIREM_QUANTUM_MAIN_BLE_UUID_ENCODE_SERVICE(_service)                    \
    BT_UUID_128_ENCODE(                                          \
        (ESIREM_QUANTUM_MAIN_BLE_UUID_PART_RANDOM | (_service << 8)),            \
        ESIREM_QUANTUM_MAIN_BLE_UUID_PART_PROJECT_ID, ESIREM_QUANTUM_MAIN_BLE_UUID_PART_PROJECT, \
        ESIREM_QUANTUM_MAIN_BLE_UUID_PART_CLIENT, ESIREM_QUANTUM_MAIN_BLE_UUID_PART_VENDOR)

#define ESIREM_QUANTUM_MAIN_BLE_UUID_ENCODE_SERVICE_CHRC(_service, _chrc)        \
    BT_UUID_128_ENCODE(                                          \
        (ESIREM_QUANTUM_MAIN_BLE_UUID_PART_RANDOM | (_service << 8) | (_chrc)),  \
        ESIREM_QUANTUM_MAIN_BLE_UUID_PART_PROJECT_ID, ESIREM_QUANTUM_MAIN_BLE_UUID_PART_PROJECT, \
        ESIREM_QUANTUM_MAIN_BLE_UUID_PART_CLIENT, ESIREM_QUANTUM_MAIN_BLE_UUID_PART_VENDOR)

#define ESIREM_QUANTUM_MAIN_BLE_UUID_DECLARE_SERVICE(_service) \
    BT_UUID_DECLARE_128(ESIREM_QUANTUM_MAIN_BLE_UUID_ENCODE_SERVICE(_service))

#define ESIREM_QUANTUM_MAIN_BLE_UUID_DECLARE_SERVICE_CHRC(_service, _chrc) \
    BT_UUID_DECLARE_128(ESIREM_QUANTUM_MAIN_BLE_UUID_ENCODE_SERVICE_CHRC(_service, _chrc))

#define ESIREM_QUANTUM_MAIN_BLE_SERVICE_CHRC_CUD_STR_MAX_LEN 20

#ifdef __cplusplus
}
#endif

#endif // ESIREM_QUANTUM_MAIN_BLE_UUID_H_INCLUDED
