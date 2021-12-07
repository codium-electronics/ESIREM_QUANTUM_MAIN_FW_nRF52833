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
 * ble_service_config.c - 07/12/2021
 * Donne acces a la configuration du dispositif
 */

#include <include/ble_uuid.h>
#include <include/common.h>
#include <include/ble_service_config.h>
#include <include/core.h>

#include <zephyr/types.h>

#include <errno.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>

#include <settings/settings.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(esirem_quantum_main_service_config, CONFIG_LOG_MAX_LEVEL);

/*
 * Fourni un service BLE avec une caracteristique par option de config
 * disponible.
 */

const struct bt_uuid_128 esirem_quantum_main_ble_uuid_service_config =
    BT_UUID_INIT_128(ESIREM_QUANTUM_MAIN_BLE_UUID_ENCODE_SERVICE(ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_CONFIG));

const struct bt_uuid_128 esirem_quantum_main_ble_uuid_service_config_chrc_led_seq_duration_ms =
    BT_UUID_INIT_128(ESIREM_QUANTUM_MAIN_BLE_UUID_ENCODE_SERVICE_CHRC(
        ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_CONFIG,
        ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_CONFIG_CHRC_LED_SEQ_DURATION_MS));
const struct bt_uuid_128 esirem_quantum_main_ble_uuid_service_config_chrc_led_ton_ms =
    BT_UUID_INIT_128(ESIREM_QUANTUM_MAIN_BLE_UUID_ENCODE_SERVICE_CHRC(
        ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_CONFIG,
        ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_CONFIG_CHRC_LED_TON_MS));
const struct bt_uuid_128 esirem_quantum_main_ble_uuid_service_config_chrc_led_toff_ms =
    BT_UUID_INIT_128(ESIREM_QUANTUM_MAIN_BLE_UUID_ENCODE_SERVICE_CHRC(
        ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_CONFIG,
        ESIREM_QUANTUM_MAIN_BLE_UUID_SERVICE_CONFIG_CHRC_LED_TOFF_MS));

static int service_config_get_attr_map_uuid_keyptr(
    const struct bt_gatt_attr* attr,
    const struct esirem_quantum_main_core_setting_map_uuid_keyptr** match_uuid_keyptr)
{
    if (!attr || !match_uuid_keyptr)
    {
        return -EINVAL;
    }

    *match_uuid_keyptr = NULL;
    uint32_t size      = esirem_quantum_main_core_setting_get_map_uuid_keyptr_size();
    for (uint8_t i = 0; i < size; i++)
    {
        if (!bt_uuid_cmp(esirem_quantum_main_core_setting_map_uuid_keyptr[i].uuid, attr->uuid))
        {
            *match_uuid_keyptr = &esirem_quantum_main_core_setting_map_uuid_keyptr[i];
            return 0;
        }
    }

    LOG_ERR("UUID attr not found");
    return -EINVAL;
}

static int service_config_get_full_key_name_from_attr(
    const struct bt_gatt_attr* attr, char* setting_full_key,
    size_t setting_full_key_sz)
{
    const struct esirem_quantum_main_core_setting_map_uuid_keyptr* map_uuid_keyptr = NULL;
    int status                                                     = 0;

    status = service_config_get_attr_map_uuid_keyptr(attr, &map_uuid_keyptr);
    if (status)
    {
        LOG_ERR("Failed to get settings map_uuid_keyptr for attr");
        return status;
    }
    status = esirem_quantum_main_core_setting_get_full_key(
        map_uuid_keyptr, setting_full_key, setting_full_key_sz);
    if (status)
    {
        LOG_ERR("Failed to get full key name for attr");
        return status;
    }
    return 0;
}

static ssize_t service_config_write_cb(
    struct bt_conn* conn, const struct bt_gatt_attr* attr, const void* buf,
    uint16_t len, uint16_t offset, uint8_t flags)
{
    char settings_key_str[ESIREM_QUANTUM_MAIN_CORE_SETTINGS_KEY_STR_MAX_LEN];
    int status = 0;

    LOG_DBG("Write config value");
    status = service_config_get_full_key_name_from_attr(
        attr, settings_key_str, sizeof(settings_key_str));
    if (status)
    {
        return status;
    }
    LOG_DBG("Received write request for %s", log_strdup(settings_key_str));

    status = settings_runtime_set(settings_key_str, buf, sizeof(uint32_t));
    if (status)
    {
        LOG_ERR("Failed to write settings, err: %d", status);
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }
    status = settings_save_one(settings_key_str, buf, sizeof(uint32_t));
    if (status)
    {
        LOG_ERR("Failed to save settings to flash, err: %d", status);
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }

    return len;
}

static ssize_t service_config_read_cb(
    struct bt_conn* conn, const struct bt_gatt_attr* attr, void* buf,
    uint16_t len, uint16_t offset)
{
    char settings_key_str[ESIREM_QUANTUM_MAIN_CORE_SETTINGS_KEY_STR_MAX_LEN];
    int status       = 0;
    ssize_t data_len = 0;

    LOG_DBG("Read config value");
    status = service_config_get_full_key_name_from_attr(
        attr, settings_key_str, sizeof(settings_key_str));
    if (status)
    {
        return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
    }
    LOG_DBG("Received read request for %s", log_strdup(settings_key_str));

    data_len = settings_runtime_get(settings_key_str, buf, len);
    if (data_len < 0)
    {
        LOG_ERR("Failed to read settings, err: %d", data_len);
        return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
    }

    return data_len;
}

static const char service_config_chrc_led_seq_duration_ms_cud_str[] =
    "Durée total séquence LED (ms)";
static const char service_config_chrc_led_ton_ms_cud_str[]  = "Ton LED (ms)";
static const char service_config_chrc_led_toff_ms_cud_str[] = "Toff LED (ms)";

static const struct bt_gatt_cpf chrc_cpf = {
    .format      = 0x08, /* uint32 */
    .exponent    = 0,
    .unit        = 0,
    .name_space  = 0,
    .description = 0,
};

/* Declaration du service configuration ESIREM_QUANTUM_MAIN */
BT_GATT_SERVICE_DEFINE(
    esirem_quantum_main_service_config,
    BT_GATT_PRIMARY_SERVICE((struct bt_uuid*) &esirem_quantum_main_ble_uuid_service_config),
    BT_GATT_CHARACTERISTIC(
        (struct bt_uuid*) &esirem_quantum_main_ble_uuid_service_config_chrc_led_seq_duration_ms,
        BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, service_config_read_cb,
        service_config_write_cb, NULL),
    BT_GATT_CUD(
        service_config_chrc_led_seq_duration_ms_cud_str, BT_GATT_PERM_READ),
    BT_GATT_CPF(&chrc_cpf),
    BT_GATT_CHARACTERISTIC(
        (struct bt_uuid*) &esirem_quantum_main_ble_uuid_service_config_chrc_led_ton_ms,
        BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, service_config_read_cb,
        service_config_write_cb, NULL),
    BT_GATT_CUD(service_config_chrc_led_ton_ms_cud_str, BT_GATT_PERM_READ),
    BT_GATT_CPF(&chrc_cpf),
    BT_GATT_CHARACTERISTIC(
        (struct bt_uuid*) &esirem_quantum_main_ble_uuid_service_config_chrc_led_toff_ms,
        BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, service_config_read_cb,
        service_config_write_cb, NULL),
    BT_GATT_CUD(service_config_chrc_led_toff_ms_cud_str, BT_GATT_PERM_READ),
    BT_GATT_CPF(&chrc_cpf),);
