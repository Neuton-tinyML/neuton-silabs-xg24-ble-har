/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#include "bluetooth.h"

#define DEBUG_BLE_REPORT 0

static uint8_t notification_enabled = 0;
static uint8_t connection_handle = 0xff;
uint8_t current_report_page = 0; // 0x01 is for vanilla keyboard report, 0x02 is for media keys
uint8_t current_key = 0;
uint8_t current_modifier_key = 0;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
void print_report(uint8_t * report, uint8_t len)
{
  for (uint8_t i = 0; i < len; i++) {
    app_log("%02X ", report[i]);
  }
  app_log("\n");
}
/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:

      sl_bt_sm_set_bondable_mode(1);
      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);
      app_assert_status(sc);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Set advertising interval to 100ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle,
        160, // min. adv. interval (milliseconds * 1.6)
        160, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events
      app_assert_status(sc);

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;
    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      notification_enabled = 0;
      connection_handle = evt->data.evt_connection_opened.connection;
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      notification_enabled = 0;
      connection_handle = 0xff;

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_advertiser_connectable_scannable);
      app_assert_status(sc);
      break;
    case sl_bt_evt_gatt_server_characteristic_status_id:
      if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config)
        {
          notification_enabled = evt->data.evt_gatt_server_characteristic_status.client_config_flags & sl_bt_gatt_server_notification;
        }
      break;
    case sl_bt_evt_sm_bonding_failed_id:
      sl_bt_sm_delete_bondings();
      break;
    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////
    case  sl_bt_evt_system_external_signal_id:

      break;
    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}

bool sl_bt_is_connected(void)
{
  return (connection_handle != 0xff) ? true : false;
}

bool sl_bt_send_remote_ctrl_data(const char* data)
{
  if (!sl_bt_is_connected())
    {
      return false;
    }

  if (!notification_enabled)
    {
      return false;
    }

  sl_status_t sc;

  sc = sl_bt_gatt_server_notify_all(gattdb_remote_ctrl, strlen(data), (const uint8_t*) data);

  if (sc != SL_STATUS_OK)
    {
      return false;
    }

  return true;
}
