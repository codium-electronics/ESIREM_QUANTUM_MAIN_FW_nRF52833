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
 * ble_service_config.h - 07/12/2021
 * Definitions service BLE configuration ESIREM Quantum board
 * Donne acces a la configuration du dispositif
 */

#ifndef ESIREM_QUANTUM_MAIN_BLE_SERVICE_CONFIG_H_INCLUDED
#define ESIREM_QUANTUM_MAIN_BLE_SERVICE_CONFIG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <bluetooth/uuid.h>

/**@bried UUIDs du service installateur / configuration */
#define ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_CONFIG 0x01

/**@bried Contrôle du clignotement de la LED */
#define ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_CONFIG_CHRC_LED_SEQ_DURATION_MS 0x01
#define ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_CONFIG_CHRC_LED_TON_MS 0x02
#define ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_CONFIG_CHRC_LED_TOFF_MS 0x03

/**@brief Structures UUIDs BLE pour le service configuration */
extern const struct bt_uuid_128 esirem_quantum_main_ble_uuid_service_config;

extern const struct bt_uuid_128 esirem_quantum_main_ble_uuid_service_config_chrc_led_seq_duration_ms;
extern const struct bt_uuid_128 esirem_quantum_main_ble_uuid_service_config_chrc_led_ton_ms;
extern const struct bt_uuid_128 esirem_quantum_main_ble_uuid_service_config_chrc_led_toff_ms;

#ifdef __cplusplus
}
#endif

#endif // ESIREM_QUANTUM_MAIN_BLE_CONFIG_SERVICE_CONFIG_H_INCLUDED

