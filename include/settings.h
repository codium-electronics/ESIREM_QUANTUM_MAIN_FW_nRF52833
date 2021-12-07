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
 * settings.h - 07/12/2021
 * Gestion des parametres en flash
 */

#ifndef ESIREM_QUANTUM_MAIN_INCLUDE_SETTINGS_H_INCLUDED
#define ESIREM_QUANTUM_MAIN_INCLUDE_SETTINGS_H_INCLUDED

#include <zephyr/types.h>

extern const uint32_t settings_val_max_percent;

int esirem_quantum_main_settings_init();

#endif // ESIREM_QUANTUM_MAIN_INCLUDE_SETTINGS_H_INCLUDED
