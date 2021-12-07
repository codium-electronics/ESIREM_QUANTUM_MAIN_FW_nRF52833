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
 * core.h - 07/12/2021
 * Gestion du declenchement / arret du dispositif
 */

#ifndef ESIREM_QUANTUM_MAIN_INCLUDE_CORE_H_INCLUDED
#define ESIREM_QUANTUM_MAIN_INCLUDE_CORE_H_INCLUDED

#include <zephyr.h>
#include <zephyr/types.h>

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    enum esirem_quantum_main_core_device_state
    {
        ESIREM_QUANTUM_MAIN_CORE_DEVICE_STATE_IDLE    = 0x00UL,
        ESIREM_QUANTUM_MAIN_CORE_DEVICE_STATE_RUNNING = 0x01UL,
    };

#define ESIREM_QUANTUM_MAIN_CORE_SETTINGS_KEY_STR_MAX_LEN (40)

    extern const char esirem_quantum_main_core_setting_key_module[];
    extern const char esirem_quantum_main_core_setting_key_led_seq_duration_ms[];
    extern const char esirem_quantum_main_core_setting_key_led_ton_duration_ms[];
    extern const char esirem_quantum_main_core_setting_key_led_toff_duration_ms[];

    struct esirem_quantum_main_core_setting_map_uuid_keyptr
    {
        const struct bt_uuid* uuid;
        const char* key;
        uint32_t* ptrval;
        uint16_t keylen;
        const uint32_t* minval;
        const uint32_t* maxval;
    };

    extern const struct esirem_quantum_main_core_setting_map_uuid_keyptr
        esirem_quantum_main_core_setting_map_uuid_keyptr[];

    extern struct settings_handler esirem_quantum_main_core_settings_hdlrs;

    int esirem_quantum_main_core_trig_new_cycle(void);
    int esirem_quantum_main_core_stop_cycle(void);

    uint8_t esirem_quantum_main_core_device_running(void);

    int esirem_quantum_main_core_init(void);

    int esirem_quantum_main_core_setting_get_full_key(
        const struct esirem_quantum_main_core_setting_map_uuid_keyptr* map_uuid_keyptr,
        char* setting_full_key, size_t setting_full_key_sz);
    
    uint32_t esirem_quantum_main_core_setting_get_map_uuid_keyptr_size();

    bool esirem_quantum_main_core_error_occured();

#ifdef __cplusplus
}
#endif

#endif // ESIREM_QUANTUM_MAIN_INCLUDE_CORE_H_INCLUDED
