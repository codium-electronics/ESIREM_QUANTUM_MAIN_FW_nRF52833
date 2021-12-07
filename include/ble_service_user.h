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
 * ble_service_user.h - 07/12/2021
 */

#ifndef ESIREM_QUANTUM_MAIN_BLE_SERVICE_USER_H_INCLUDED
#define ESIREM_QUANTUM_MAIN_BLE_SERVICE_USER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <bluetooth/uuid.h>

/**@brief UUIDs du service utilisateur (declenchement du dispositif) */
#define ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_USER 0x02

/**@brief UUIDs caracteristique etat du dispositif */
#define ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_USER_CHRC_STATE 0x01

enum esirem_quantum_main_ble_service_user_state {
    ESIREM_QUANTUM_MAIN_SERVICE_USER_STATE_OFF = 0x00UL,
    ESIREM_QUANTUM_MAIN_SERVICE_USER_STATE_ON = 0x01,
};

int esirem_quantum_main_ble_service_user_chrc_state_indicate_change(const bool device_state);

#ifdef __cplusplus
}
#endif

#endif // ESIREM_QUANTUM_MAIN_BLE_SERVICE_USER_H_INCLUDED
