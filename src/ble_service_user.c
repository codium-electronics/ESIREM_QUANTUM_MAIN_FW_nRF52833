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
 * ble_service_user.c - 07/12/2021
 * Implementation service utilisateur
 */

#include <include/ble_uuid.h>
#include <include/ble_service_user.h>
#include <include/common.h>
#include <include/core.h>

#include <zephyr/types.h>

#include <errno.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(esirem_quantum_main_service_user, CONFIG_LOG_MAX_LEVEL);

static bool notification_enabled = false;

/*
 * Gestion des notifications / indications
 *
 * La pile BLE se charge de garder en mémoire les connexions
 * et paramètres précédents pour chaque pair, et appelle le
 * callback chaque fois que nécessaire (quand un device active
 * un niveau de notification plus élevé, ou quand tous les devices
 * ont désactivés leurs notifs / sont déconnectés).
 *
 * On est assurés que notification_enabled sera toujours a 1
 * tant qu'au moins un appareil a activé les notifications pour
 * l'attribut concerné.
 *
 * La pile maintient un enregistrement de notification pour
 * chaque attribut, chaque attribut peut donc avoir un
 * comportement de notification différent. Ici on utilise
 * qu'un seul attribut avec notification au niveau utilisateur.
 *
 * Voir les fonctions en rapport dans gatt.c
 * (envoi des notifs, update_ccc, gatt_ccc_changed, etc...)
 */

static void
service_user_ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value)
{
    LOG_DBG("CCC config changed: %hx", value);
    notification_enabled = (value == BT_GATT_CCC_NOTIFY);
    return;
}

static ssize_t service_user_state_write_cb(
    struct bt_conn* conn, const struct bt_gatt_attr* attr, const void* buf,
    uint16_t len, uint16_t offset, uint8_t flags)
{
    int ret = 0;

    /* On appelle le callback de changement d'état */
    LOG_DBG("Write user state");
    if (((uint8_t *)buf)[0] != 0x00)
    {
        ret = esirem_quantum_main_core_trig_new_cycle();
        if (ret)
        {
            LOG_DBG("Failed to set state ON");
            return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
        }
    }
    else
    {
        ret = esirem_quantum_main_core_stop_cycle();
        if (ret)
        {
            LOG_DBG("Failed to set state OFF");
            return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
        }
    }

    return len;
}

static ssize_t service_user_state_read_cb(
    struct bt_conn* conn, const struct bt_gatt_attr* attr, void* buf,
    uint16_t len, uint16_t offset)
{
    uint8_t device_state = esirem_quantum_main_core_device_running();
    ssize_t len_read     = 0;

    LOG_DBG("Read user state");
    if (0 == len)
    {
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }

    len_read = bt_gatt_attr_read(
        conn, attr, buf, len, offset, &device_state, sizeof(device_state));
    return len_read;
}

static struct bt_uuid_128 service_user_uuid =
    BT_UUID_INIT_128(ESIREM_QUANTUM_MAIN_BLE_UUID_ENCODE_SERVICE(ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_USER));
static struct bt_uuid_128 service_user_chrc_state_uuid =
    BT_UUID_INIT_128(ESIREM_QUANTUM_MAIN_BLE_UUID_ENCODE_SERVICE_CHRC(
        ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_USER, ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_USER_CHRC_STATE));

static const char service_user_chrc_state_cud_str[] = "Etat Quantum main";

static const struct bt_gatt_cpf chrc_state_cpf = {
    .format      = 0x01, /* boolean */
    .exponent    = 0,
    .unit        = 0,
    .name_space  = 0,
    .description = 0,
};

/* Declaration du service utilisateur ESIREM_QUANTUM_MAIN */
BT_GATT_SERVICE_DEFINE(
    esirem_quantum_main_service_user,
    BT_GATT_PRIMARY_SERVICE((struct bt_uuid*) &service_user_uuid),
    BT_GATT_CHARACTERISTIC(
        (struct bt_uuid*) &service_user_chrc_state_uuid,
        BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, service_user_state_read_cb,
        service_user_state_write_cb, NULL),
    BT_GATT_CUD(service_user_chrc_state_cud_str, BT_GATT_PERM_READ_ENCRYPT),
    BT_GATT_CCC(
        service_user_ccc_cfg_changed,
        BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),
    BT_GATT_CPF(&chrc_state_cpf));

/* Envoi d'une notification quand l'etat du device a change */
int esirem_quantum_main_ble_service_user_chrc_state_indicate_change(const bool device_state)
{
    int err;
    uint8_t buf[1] = {0};

    if (!notification_enabled)
    {
        return 0;
    }

    LOG_DBG("Sending notification state changed");

    buf[0] = device_state;

    err = bt_gatt_notify(NULL, &esirem_quantum_main_service_user.attrs[1], buf, sizeof(buf));
    if (err)
    {
        LOG_ERR("Failed to send state indication, err: %d", err);
    }
    return err;
}
