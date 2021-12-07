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
 * core.c - 07/12/2021
 *
 * Fonctionnement :
 *
 * - Attente de l'appel d'un callback de declenchement.
 * - Lance une tache dans la file d'attente d'execution du kernel Zephyr.
 * - A chaque execution, la tache controle si elle doit être reexecutee
 * (allumage / coupure de la LED)
 * - Quand le cycle d'execution est termine, la tache se termine sans se
 * remettre en attente
 * - Le systeme repasse en fonctionnement callback : une nouvelle demande lance
 * un nouveau cycle
 * - Si une demande d'arret est reçue pendant un cycle, a la prochaine execution
 * la fonction s'assure que la LED soit eteinte et retourne en attente de
 * nouveau cycle.
 */

#include <include/ble_service_config.h>
#include <include/ble_service_user.h>
#include <include/core.h>
#include <include/settings.h>

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/led.h>
#include <errno.h>
#include <settings/settings.h>
#include <zephyr.h>

#include <stdbool.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(esirem_quantum_main_core, CONFIG_LOG_MAX_LEVEL);

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0	DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN	    DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS	DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0	""
#define PIN	0
#define FLAGS	0
#endif

enum esirem_quantum_main_core_state
{
    ESIREM_QUANTUM_MAIN_CORE_STATE_IDLE  = 0x00UL,
    ESIREM_QUANTUM_MAIN_CORE_STATE_ON    = 0x01UL,
    ESIREM_QUANTUM_MAIN_CORE_STATE_OFF   = 0x02UL,
    ESIREM_QUANTUM_MAIN_CORE_STATE_ERROR = 0x03UL,
    ESIREM_QUANTUM_MAIN_CORE_STATE_INIT  = 0x04UL,
};

/*
 * Utilise une fonction commit pour valider
 * le chargement des valeurs au démarrage et calculer le count.
 *
 * Utilise une fonction set pour enregistrer une
 * nouvelle valeur qui sera utilisée a la prochaine
 * execution de la fonction run.
 *
 * !! ATTENTION !!
 * Toutes ces variables doivent etres protegees contre les "race conditions"
 * et doivent donc utiliser l'API atomic et donc etre sur 32 bits
 *
 * Si il n'est pas possible d'utiliser une valeur 32 bits (necessite une
 * structure ou autre) utiliser une fonction de lecture et une d'ecriture et
 * proteger la variable par une barriere memoire (mutex, sempahore, a voir
 * suivant les cas)
 */

/**@brief Durée de la sequence de clignotement */
static uint32_t esirem_quantum_main_core_setting_led_seq_duration_ms = 15000;
/**@brief Temps ON LED */
static uint32_t esirem_quantum_main_core_setting_led_ton_duration_ms = 500;
/**@brief Temps OFF LED */
static uint32_t esirem_quantum_main_core_setting_led_toff_duration_ms = 500;
/**@brief Nombre de periodes ON/OFF dans un cycle */
static uint32_t esirem_quantum_main_core_setting_led_period_count = 50;

/*
 * Noms des parametres
 */

const char esirem_quantum_main_core_setting_key_module[] = "esirem_quantum_main";
const char esirem_quantum_main_core_setting_key_led_seq_duration_ms[] =
    "cfg/led/seq_duration_ms";
const char esirem_quantum_main_core_setting_key_led_ton_duration_ms[]  = "cfg/led/ton_ms";
const char esirem_quantum_main_core_setting_key_led_toff_duration_ms[] = "cfg/led/toff_ms";

const struct esirem_quantum_main_core_setting_map_uuid_keyptr
    esirem_quantum_main_core_setting_map_uuid_keyptr[] = {
        {
            .uuid =
                (const struct
                 bt_uuid*) &esirem_quantum_main_ble_uuid_service_config_chrc_led_seq_duration_ms,
            .key    = esirem_quantum_main_core_setting_key_led_seq_duration_ms,
            .ptrval = &esirem_quantum_main_core_setting_led_seq_duration_ms,
            .keylen = sizeof(esirem_quantum_main_core_setting_key_led_seq_duration_ms) - 1,
            .minval = NULL,
            .maxval = NULL,
        },
        {
            .uuid   = (const struct
                     bt_uuid*) &esirem_quantum_main_ble_uuid_service_config_chrc_led_ton_ms,
            .key    = esirem_quantum_main_core_setting_key_led_ton_duration_ms,
            .ptrval = &esirem_quantum_main_core_setting_led_ton_duration_ms,
            .keylen = sizeof(esirem_quantum_main_core_setting_key_led_ton_duration_ms) - 1,
            .minval = NULL,
            .maxval = NULL,
        },
        {
            .uuid   = (const struct
                     bt_uuid*) &esirem_quantum_main_ble_uuid_service_config_chrc_led_toff_ms,
            .key    = esirem_quantum_main_core_setting_key_led_toff_duration_ms,
            .ptrval = &esirem_quantum_main_core_setting_led_toff_duration_ms,
            .keylen = sizeof(esirem_quantum_main_core_setting_key_led_toff_duration_ms) - 1,
            .minval = NULL,
            .maxval = NULL,
        },
};

/* Fonction d'execution d'un declenchement : controle des LEDs */
static atomic_t esirem_quantum_main_led_core_state     = ATOMIC_INIT(ESIREM_QUANTUM_MAIN_CORE_STATE_INIT);
static uint32_t esirem_quantum_main_led_core_work_stop = 0;
static struct k_work_delayable esirem_quantum_main_led_core_work;

const struct device* dev_led = NULL;

static void esirem_quantum_main_led_core_work_run_fn(struct k_work* work)
{
    static uint8_t cur_cycle_count = 0;
    int err                        = 0;

    enum esirem_quantum_main_core_state cur_state =
        (enum esirem_quantum_main_core_state) atomic_get(&esirem_quantum_main_led_core_state);

    uint32_t led_ton_duration_ms =
        (uint32_t) atomic_get(&esirem_quantum_main_core_setting_led_ton_duration_ms);
    uint32_t led_toff_duration_ms =
        (uint32_t) atomic_get(&esirem_quantum_main_core_setting_led_toff_duration_ms);
    uint32_t led_period_count =
        (uint32_t) atomic_get(&esirem_quantum_main_core_setting_led_period_count);

    if (cur_state == ESIREM_QUANTUM_MAIN_CORE_STATE_ERROR)
    {
        LOG_DBG("Exec in error : early exiting");
        return;
    }

    LOG_DBG("Run esirem_quantum_main_core fn");
    switch (cur_state)
    {
        case ESIREM_QUANTUM_MAIN_CORE_STATE_IDLE:
            /* Declenche un nouveau cycle */
            cur_cycle_count = 0;
            /* Pas de break : la LED était eteinte on l'allume */

        case ESIREM_QUANTUM_MAIN_CORE_STATE_OFF:
            /* Passe la LED ON */
            LOG_DBG("Switching LED ON");
            err = gpio_pin_set(dev_led, PIN, 1);
            if (err)
            {
                LOG_ERR("Cannot set PWM");
                atomic_set(
                    &esirem_quantum_main_led_core_state, (atomic_val_t) ESIREM_QUANTUM_MAIN_CORE_STATE_ERROR);
                return;
            }
            atomic_set(&esirem_quantum_main_led_core_state, (atomic_val_t) ESIREM_QUANTUM_MAIN_CORE_STATE_ON);
            /* On planifie la prochaine execution de la fonction pour
             * couper la LED */
            k_work_schedule(&esirem_quantum_main_led_core_work, K_MSEC(led_ton_duration_ms));
            if (!cur_cycle_count)
            {
                esirem_quantum_main_ble_service_user_chrc_state_indicate_change(
                    (bool) ESIREM_QUANTUM_MAIN_CORE_DEVICE_STATE_RUNNING);
            }
            break;

        case ESIREM_QUANTUM_MAIN_CORE_STATE_INIT:
            /* On passe le nombre de cycles au max pour sortir après init
             * comme si on venait de terminer un cycle */
            cur_cycle_count = led_period_count;
        case ESIREM_QUANTUM_MAIN_CORE_STATE_ON:
        default:
            LOG_DBG("Switching LED OFF");
            /* Repasse la LED OFF */
            err = gpio_pin_set(dev_led, PIN, 0);
            if (err)
            {
                LOG_ERR("Cannot set LED OFF");
                atomic_set(
                    &esirem_quantum_main_led_core_state, (atomic_val_t) ESIREM_QUANTUM_MAIN_CORE_STATE_ERROR);
                return;
            }

            if (++cur_cycle_count >= led_period_count
                || (uint32_t) atomic_get(&esirem_quantum_main_led_core_work_stop))
            {
                atomic_set(&esirem_quantum_main_led_core_work_stop, 0x00U);
                atomic_set(
                    &esirem_quantum_main_led_core_state, (atomic_val_t) ESIREM_QUANTUM_MAIN_CORE_STATE_IDLE);
                esirem_quantum_main_ble_service_user_chrc_state_indicate_change(
                    (bool) ESIREM_QUANTUM_MAIN_CORE_DEVICE_STATE_IDLE);
            }
            else
            {
                /* Sinon, on replanifie une execution de la fonction pour
                 * allumer la LED */
                atomic_set(
                    &esirem_quantum_main_led_core_state, (atomic_val_t) ESIREM_QUANTUM_MAIN_CORE_STATE_OFF);
                k_work_schedule(
                    &esirem_quantum_main_led_core_work, K_MSEC(led_toff_duration_ms));
            }
            break;
    }
    return;
}

/* Requete de declenchement d'un nouveau cycle */
int esirem_quantum_main_core_trig_new_cycle(void)
{
    enum esirem_quantum_main_core_state cur_led_state =
        (enum esirem_quantum_main_core_state) atomic_get(&esirem_quantum_main_led_core_state);
    
    if (cur_led_state != ESIREM_QUANTUM_MAIN_CORE_STATE_IDLE)
    {
        return -EBUSY;
    }

    if (!k_work_delayable_is_pending(&esirem_quantum_main_led_core_work))
    {
        k_work_schedule(&esirem_quantum_main_led_core_work, K_NO_WAIT);
    }
    return 0;
}

int esirem_quantum_main_core_stop_cycle(void)
{
    enum esirem_quantum_main_core_state cur_led_state =
        (enum esirem_quantum_main_core_state) atomic_get(&esirem_quantum_main_led_core_state);

    if (cur_led_state == ESIREM_QUANTUM_MAIN_CORE_STATE_IDLE)
        return 0;

    if (cur_led_state != ESIREM_QUANTUM_MAIN_CORE_STATE_OFF
         && cur_led_state != ESIREM_QUANTUM_MAIN_CORE_STATE_ON
         && cur_led_state != ESIREM_QUANTUM_MAIN_CORE_STATE_IDLE)
    {
        return -EBUSY;
    }

    if (cur_led_state != ESIREM_QUANTUM_MAIN_CORE_STATE_IDLE)
        atomic_set(&esirem_quantum_main_led_core_work_stop, 0x01U);

    return 0;
}

bool esirem_quantum_main_core_error_occured()
{
    enum esirem_quantum_main_core_state cur_led_state =
        (enum esirem_quantum_main_core_state) atomic_get(&esirem_quantum_main_led_core_state);

    if (cur_led_state == ESIREM_QUANTUM_MAIN_CORE_STATE_ERROR)
        return true;

    return false;
}

/* Renvoie true si un declenchement est en cours, false sinon */
uint8_t esirem_quantum_main_core_device_running(void)
{
    enum esirem_quantum_main_core_state cur_led_state =
        (enum esirem_quantum_main_core_state) atomic_get(&esirem_quantum_main_led_core_state);

    if (cur_led_state != ESIREM_QUANTUM_MAIN_CORE_STATE_IDLE)
    {
        return 0xFF;
    }
    return 0x00;
}

/* Initialisation du esirem_quantum_main_core */
int esirem_quantum_main_core_init(void)
{
    int scheduled;
    int ret;

    LOG_DBG("Init esirem_quantum_main_core");

    dev_led = device_get_binding(LED0);
    if (NULL == dev_led)
    {
        LOG_ERR("Led device not found");
        return -EIO;
    }

    ret = gpio_pin_configure(dev_led, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret < 0) {
		return -EIO;
	}

    gpio_pin_set(dev_led, PIN, 0);

    atomic_set(
        &esirem_quantum_main_core_setting_led_period_count,
        atomic_get(&esirem_quantum_main_core_setting_led_seq_duration_ms)
            / (atomic_get(&esirem_quantum_main_core_setting_led_ton_duration_ms)
               + atomic_get(&esirem_quantum_main_core_setting_led_toff_duration_ms)));

    /* On lance une premiere execution de la fonction, l'etat
     * est a init, la fonction coupe la LED et repasse en IDLE */
    k_work_init_delayable(&esirem_quantum_main_led_core_work, esirem_quantum_main_led_core_work_run_fn);
    scheduled = k_work_schedule(&esirem_quantum_main_led_core_work, K_NO_WAIT);
    if (!scheduled)
    {
        LOG_ERR("Error while submitting esirem_quantum_main_led_core_work in init");
        return -EIO;
    }
    
    return 0;
}

/*
 * Gestion des parametres
 */

/*
 * Acces aux parametres
 */

static uint32_t esirem_quantum_main_core_setting_map_uuid_keyptr_sz =
    sizeof(esirem_quantum_main_core_setting_map_uuid_keyptr)
    / sizeof(struct esirem_quantum_main_core_setting_map_uuid_keyptr);

uint32_t esirem_quantum_main_core_setting_get_map_uuid_keyptr_size()
{
    return esirem_quantum_main_core_setting_map_uuid_keyptr_sz;
}

static int esirem_quantum_main_core_settings_retrieve_map_uuid_keyptr(
    const char* name,
    const struct esirem_quantum_main_core_setting_map_uuid_keyptr** match_uuid_keyptr)
{
    *match_uuid_keyptr = NULL;

    for (uint8_t i = 0; i < esirem_quantum_main_core_setting_map_uuid_keyptr_sz; i++)
    {
        if (!strncmp(
                name, esirem_quantum_main_core_setting_map_uuid_keyptr[i].key,
                esirem_quantum_main_core_setting_map_uuid_keyptr[i].keylen))
        {
            *match_uuid_keyptr = &esirem_quantum_main_core_setting_map_uuid_keyptr[i];
            return 0;
        }
    }
    LOG_ERR("Invalid name, cannot retrieve data for %s", log_strdup(name));
    return -EINVAL;
}

/**@brief Appele pendant le chargement / la modification de valeur via settings
 * API pour modifier la valeur d'un parametre.
 */
static int esirem_quantum_main_core_settings_set(
    const char* name, size_t len, settings_read_cb read_cb, void* cb_arg)
{
    int status                                                     = 0;
    uint32_t tmp_val                                               = 0;
    const struct esirem_quantum_main_core_setting_map_uuid_keyptr* map_uuid_keyptr = NULL;

    LOG_DBG("Write config value");
    status = esirem_quantum_main_core_settings_retrieve_map_uuid_keyptr(name, &map_uuid_keyptr);
    if (status || !map_uuid_keyptr)
    {
        return status;
    }

    if (len != sizeof(uint32_t))
    {
        LOG_ERR("Invalid size");
        return -EINVAL;
    }

    status = read_cb(cb_arg, &tmp_val, sizeof(uint32_t));
    if (status < 0)
    {
        LOG_ERR("Failed to read settings value");
        return status;
    }

    if ((map_uuid_keyptr->minval && tmp_val < *map_uuid_keyptr->minval)
        || (map_uuid_keyptr->maxval && tmp_val > *map_uuid_keyptr->maxval))
    {
        LOG_ERR(
            "Invalid value for setting %s", log_strdup(map_uuid_keyptr->key));
        return -EINVAL;
    }

    if (
        map_uuid_keyptr->ptrval == &esirem_quantum_main_core_setting_led_seq_duration_ms
        || map_uuid_keyptr->ptrval == &esirem_quantum_main_core_setting_led_ton_duration_ms
        || map_uuid_keyptr->ptrval == &esirem_quantum_main_core_setting_led_toff_duration_ms)
    {
        atomic_set(
            &esirem_quantum_main_core_setting_led_period_count,
            atomic_get(&esirem_quantum_main_core_setting_led_seq_duration_ms)
                / (atomic_get(&esirem_quantum_main_core_setting_led_ton_duration_ms)
                   + atomic_get(&esirem_quantum_main_core_setting_led_toff_duration_ms)));
    }

    LOG_DBG("Config value %s changed", log_strdup(map_uuid_keyptr->key));
    return 0;
}

static int esirem_quantum_main_core_settings_get(const char* key, char* val, int val_len_max)
{
    int status                                                     = 0;
    const struct esirem_quantum_main_core_setting_map_uuid_keyptr* map_uuid_keyptr = NULL;

    LOG_DBG("Getting esirem_quantum_maincore settings");
    status = esirem_quantum_main_core_settings_retrieve_map_uuid_keyptr(key, &map_uuid_keyptr);
    if (status)
    {
        return status;
    }

    if (val_len_max < sizeof(uint32_t))
    {
        LOG_ERR("buf for value too short");
        return -EINVAL;
    }

    memcpy(val, map_uuid_keyptr->ptrval, sizeof(uint32_t));
    return sizeof(uint32_t);
}

/**@brief appele apres la fin du chargement des parametres. */
static int esirem_quantum_main_core_settings_commit(void)
{
    atomic_set(
        &esirem_quantum_main_core_setting_led_period_count,
        atomic_get(&esirem_quantum_main_core_setting_led_seq_duration_ms)
            / (atomic_get(&esirem_quantum_main_core_setting_led_ton_duration_ms)
               + atomic_get(&esirem_quantum_main_core_setting_led_toff_duration_ms)));

    return 0;
}

struct settings_handler esirem_quantum_main_core_settings_hdlrs = {
    .name     = esirem_quantum_main_core_setting_key_module,
    .h_set    = esirem_quantum_main_core_settings_set,
    .h_get    = esirem_quantum_main_core_settings_get,
    .h_commit = esirem_quantum_main_core_settings_commit,
};

int esirem_quantum_main_core_setting_get_full_key(
    const struct esirem_quantum_main_core_setting_map_uuid_keyptr* map_uuid_keyptr,
    char* setting_full_key, size_t setting_full_key_sz)
{
    if (!map_uuid_keyptr || !setting_full_key
        || setting_full_key_sz < ESIREM_QUANTUM_MAIN_CORE_SETTINGS_KEY_STR_MAX_LEN)
    {
        return -EINVAL;
    }

    memcpy(
        setting_full_key, esirem_quantum_main_core_setting_key_module,
        sizeof(esirem_quantum_main_core_setting_key_module) - 1);
    setting_full_key[sizeof(esirem_quantum_main_core_setting_key_module) - 1] = '/';

    if (map_uuid_keyptr->keylen > ESIREM_QUANTUM_MAIN_CORE_SETTINGS_KEY_STR_MAX_LEN
                                      - sizeof(esirem_quantum_main_core_setting_key_module))
    {
        LOG_ERR("buffer too short for full key name");
        return -EINVAL;
    }

    strncpy(
        &setting_full_key[sizeof(esirem_quantum_main_core_setting_key_module)],
        map_uuid_keyptr->key, map_uuid_keyptr->keylen);

    return 0;
}
