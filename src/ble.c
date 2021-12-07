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
 * ble.c - 07/12/2021
 * Fonctions de gestion du BLE
 */

#include <include/ble.h>

#include <zephyr.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>

#include <settings/settings.h>

#include <logging/log.h>

#include <include/ble_uuid.h>
#include <include/common.h>
#include <include/ble_service_config.h>

LOG_MODULE_REGISTER(esirem_quantum_main_ble, CONFIG_LOG_MAX_LEVEL);

/* TODO: ajouter la fin du NS dans le nom BLE*/
#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ble_advert[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data ble_service_discovery[] = {
    BT_DATA_BYTES(
        BT_DATA_UUID128_ALL,
        ESIREM_QUANTUM_MAIN_BLE_UUID_ENCODE_SERVICE(ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_CONFIG)),
};

static void connected(struct bt_conn* conn, uint8_t err)
{
    char str_peer_addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(
        bt_conn_get_dst(conn), str_peer_addr, sizeof(str_peer_addr));
    if (err)
    {
        LOG_ERR(
            "Connection to peer %s failed (err %u)\n",
            log_strdup(str_peer_addr), err);
        return;
    }

    // Sécurité level 2 : chiffrement sans authentification (fait par notre cle
    // libsodium avec fonctions maison)
    bt_conn_set_security(conn, BT_SECURITY_L2);

    LOG_DBG("Connected to peer %s", log_strdup(str_peer_addr));
    return;
}

static void disconnected(struct bt_conn* conn, uint8_t reason)
{
    char str_peer_addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(
        bt_conn_get_dst(conn), str_peer_addr, sizeof(str_peer_addr));
    LOG_DBG(
        "Disconnected from peer %s, reason %u\n", log_strdup(str_peer_addr),
        reason);
    return;
}

static void security_changed(
    struct bt_conn* conn, bt_security_t level, enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err)
    {
        LOG_DBG("Security changed: %s level %u\n", log_strdup(addr), level);
    }
    else
    {
        LOG_ERR(
            "Security failed: %s level %u err %d\n", log_strdup(addr), level,
            err);
    }
}

static struct bt_conn_cb conn_callbacks = {
    .connected        = connected,
    .disconnected     = disconnected,
    .security_changed = security_changed,
};

static void pairing_confirm(struct bt_conn* conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    bt_conn_auth_pairing_confirm(conn);

    LOG_DBG("Pairing confirmed: %s\n", log_strdup(addr));
}

static void pairing_complete(struct bt_conn* conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Pairing completed: %s, bonded: %d\n", log_strdup(addr), bonded);
}

static void pairing_failed(struct bt_conn* conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_DBG("Pairing failed conn: %s, reason %d\n", log_strdup(addr), reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display  = NULL, // auth_passkey_display,
    .cancel           = NULL, // auth_cancel,
    .pairing_confirm  = pairing_confirm,
    .pairing_complete = pairing_complete,
    .pairing_failed   = pairing_failed,
};

int ble_init(void)
{
    int err;

    bt_conn_cb_register(&conn_callbacks);
    bt_conn_auth_cb_register(&conn_auth_callbacks);

    err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Failed to init ble, err : %d\n", err);
        return err;
    }
    LOG_DBG("BLE initialized\n");

    settings_load();

    err = bt_le_adv_start(
        BT_LE_ADV_CONN, ble_advert, ARRAY_SIZE(ble_advert),
        ble_service_discovery, ARRAY_SIZE(ble_service_discovery));
    if (err)
    {
        LOG_ERR("Failed to start advertising, err: %d\n", err);
        return err;
    }
    LOG_DBG("BLE advertising started\n");

    return 0;
}