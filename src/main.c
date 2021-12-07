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
 * main.c - 07/12/2021
 * Point d'entree du programme
 */

#include <include/ble.h>
#include <include/core.h>
#include <include/settings.h>

#include <sys/reboot.h>
#include <zephyr.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(esirem_quantum_main, CONFIG_LOG_MAX_LEVEL);

void main(void)
{
    esirem_quantum_main_settings_init();
    if (esirem_quantum_main_core_init())
    {
        LOG_ERR("CRITICAL: failed to init esirem_quantum_main core, entering infinite loop");
        while (1)
        {
            k_sleep(K_MSEC(CONFIG_CODIUM_APP_MAIN_RUN_INTERVAL_MS));
        };
    }
    ble_init();

    for (;;)
    {
        if (esirem_quantum_main_core_error_occured())
        {
            LOG_ERR("Error occured, cold rebooting");
            k_sleep(K_MSEC(1000));
            sys_reboot(SYS_REBOOT_COLD);
        }
        k_sleep(K_MSEC(CONFIG_CODIUM_APP_MAIN_RUN_INTERVAL_MS));
    }
}
