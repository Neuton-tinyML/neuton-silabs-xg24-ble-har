// ///////////////////////// Package Header Files ////////////////////////////
#include <neuton/neuton.h>
// /////////////////// Application Global Header Files ///////////////////////
#include <drivers/icm20689_utils.h>
#include <app_log.h>
#include <bluetooth/bluetooth.h>
#include <neuton_har_app/neuton_har_app.h>
#include <neuton_har_app/neuton_result_postprocessing.h>
#include <sl_simple_led_instances.h>
#include <sl_simple_button_instances.h>
// /////////////////// 3rd Party Software Header Files ///////////////////////
// ////////////////////// Standard C++ Header Files //////////////////////////
// /////////////////////// Standard C Header Files ///////////////////////////
#include <string.h>
#include <stdio.h>
//////////////////////////////////////////////////////////////////////////////


#define BLE_ADVERTISING_LED_TOGGLE_PERIOD_MS        (500U)
#define BLE_CONNECTED_LED_TOGGLE_PERIOD_MS          (100U)
#define PREDICTION_TIMEOUT_MS                       (800U)
#define PREDICTION_TIMEOUT_US                       (PREDICTION_TIMEOUT_MS * 1000)
#define SLEEPTIMER_TICK_TO_US(x)                    ((uint64_t)(float)(x) * 30.5)
#define SLEEPTIMER_TICK_TO_MS(x)                    (SLEEPTIMER_TICK_TO_US(x) / 1000)

#define ICM20689_DATA_RATE_HZ                       (100)
#define ACCEL_AXIS_NUM                              (3U)
#define GYRO_AXIS_NUM                               (3U)
#define NEUTON_INPUT_DATA_LEN                       (ACCEL_AXIS_NUM + GYRO_AXIS_NUM)
#define BUTTON_PRESSED_STATE                        (1U)

#define DEQUANTIZE_PROBABILITY(q_prob)              (((float)(q_prob) / 65535))

//////////////////////////////////////////////////////////////////////////////

typedef enum
{
  LED_STATE_BLE_ADVERTISING_LED_ON = 0,///< LED_STATE_BLE_ADVERTISING_LED_ON
  LED_STATE_BLE_ADVERTISING_LED_OFF,   ///< LED_STATE_BLE_ADVERTISING_LED_OFF
  LED_STATE_BLE_CONNECTED_LED_ON,      ///< LED_STATE_BLE_CONNECTED_LED_ON
  LED_STATE_BLE_CONNECTED_LED_OFF,     ///< LED_STATE_BLE_CONNECTED_LED_OFF
} led_state_t;

typedef enum
{
  LED_INSTANCE_ID_RED = 0,    ///< LED_INSTANCE_ID_RED
  LED_INSTANCE_ID_GREEN       ///< LED_INSTANCE_ID_GREEN
} led_instance_id_t;

//////////////////////////////////////////////////////////////////////////////

static void icm20689_data_rdy_cb_(void);
static void neuton_prediction_subscriber_(const neuton_class_label_t class_label, 
                                        const float probability,
                                        const char* class_name);
static void led_indication_(void);

//////////////////////////////////////////////////////////////////////////////

/** ICM20689 sensor data ready flag */
static volatile bool icm20689_data_ready_ = false;

/** ICM20689 sensor operating configuration */
static icm20689_config_t icm20689_config_ = 
{
    .sensors =
    {
        .accel = true,
        .gyro = true
    },
     .datarate = ICM20689_DATA_RATE_HZ,
     .accel_fullscale = sl_accelFullscale4G,
     .gyro_fullscale = sl_gyroFullscale1000Dps,
     .on_data_rdy = &icm20689_data_rdy_cb_
};

/** Current active led color indication*/
static const sl_led_t* p_active_led_ = NULL;

/** Current active led indication state */
static led_state_t next_led_state_ = LED_STATE_BLE_ADVERTISING_LED_OFF;

//////////////////////////////////////////////////////////////////////////////

void neuton_har_app_init(void)
{
    app_log_info("Neuton.AI Human Activity Recognition Demo\r\n");

    /** Setup power manager */
    sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);

    /** Initialize ICM20689 IMU sensor */
    sl_status_t err = icm20689_init(&icm20689_config_);
    EFM_ASSERT((err == SL_STATUS_OK));

    /** Initialize Neuton.AI library */
    neuton_nn_setup();

    EFM_ASSERT((neuton_nn_uniq_inputs_num() == NEUTON_INPUT_DATA_LEN));

    app_log_info("Neuton.AI solution id: %s\r\n", neuton_nn_solution_id_str());

    /** Enable user button */
    sl_button_enable(&sl_button_btn0);

    /** Set initial LED indication color */
    p_active_led_ =  sl_simple_led_array[LED_INSTANCE_ID_RED];
}

//////////////////////////////////////////////////////////////////////////////
///

void neuton_har_app_dowork(void)
{
    /** Handle LED indication */
    led_indication_();

    /** Check if the new sensor data samples is ready to read */
    if (!icm20689_data_ready_)
        return;

    icm20689_data_ready_ = false;

    neuton_input_t input_data[NEUTON_INPUT_DATA_LEN];
    sl_status_t err;

    /** Read real-time sensor data point */
    err = icm20689_read_raw_accel(input_data);
    EFM_ASSERT((err == SL_STATUS_OK));

    err = icm20689_read_raw_gyro(input_data + ACCEL_AXIS_NUM);
    EFM_ASSERT((err == SL_STATUS_OK));

    /** Feeding real-time sensor data point to Neuton library to collect data window */
    neuton_input_features_t* p_input = neuton_nn_feed_inputs(input_data, NEUTON_INPUT_DATA_LEN);

    /** Check if input data window is ready for inference */
    if (p_input)
    {
        neuton_u16_t predicted_target;
        const neuton_output_t* p_probabilities;

        /** Run Neuton model inference */
        neuton_i16_t targets_num = neuton_nn_run_inference(p_input, &predicted_target,
                                                           &p_probabilities);

        /** Handle Neuton inference results if the prediction was successful */
        if (targets_num > 0)
        {
            neuton_result_postprocess(predicted_target, DEQUANTIZE_PROBABILITY(p_probabilities[predicted_target]),
                                        neuton_prediction_subscriber_);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

static void icm20689_data_rdy_cb_(void)
{
    icm20689_data_ready_ = true;
}

//////////////////////////////////////////////////////////////////////////////

static void neuton_prediction_subscriber_(const neuton_class_label_t class_label, 
                                        const float probability,
                                        const char* class_name)
{
   app_log("Predicted %s, with probability %0.3f\r", class_name, probability);

    static char send_buff[20] = {0};
    memset(send_buff, 0, sizeof(send_buff));

    snprintf(send_buff, sizeof(send_buff), "%d, %d", (int)class_label, (int)(probability * 100));
    sl_bt_send_data_str(send_buff);
}

//////////////////////////////////////////////////////////////////////////////

static void led_indication_(void)
{
  static uint32_t previous_toggle_time = 0;

  static const uint32_t LED_STATE_VS_PERIOD_MS[] =
  {
      [LED_STATE_BLE_ADVERTISING_LED_ON]  = 25,
      [LED_STATE_BLE_ADVERTISING_LED_OFF] = 1000,
      [LED_STATE_BLE_CONNECTED_LED_ON]    = 25,
      [LED_STATE_BLE_CONNECTED_LED_OFF]   = 200,
  };

  uint32_t current_time_ms = SLEEPTIMER_TICK_TO_MS(sl_sleeptimer_get_tick_count64());

  if (current_time_ms - previous_toggle_time >= LED_STATE_VS_PERIOD_MS[next_led_state_])
  {
      sl_led_toggle(p_active_led_);
      previous_toggle_time = current_time_ms;

      switch(next_led_state_)
      {
        case LED_STATE_BLE_ADVERTISING_LED_ON:
          next_led_state_ = sl_bt_is_connected() ? LED_STATE_BLE_CONNECTED_LED_OFF : LED_STATE_BLE_ADVERTISING_LED_OFF;
          break;
        case LED_STATE_BLE_ADVERTISING_LED_OFF:
          next_led_state_ = LED_STATE_BLE_ADVERTISING_LED_ON;
          break;
        case LED_STATE_BLE_CONNECTED_LED_ON:
          next_led_state_ = sl_bt_is_connected() ? LED_STATE_BLE_CONNECTED_LED_OFF : LED_STATE_BLE_ADVERTISING_LED_OFF;
          break;
        case LED_STATE_BLE_CONNECTED_LED_OFF:
          next_led_state_ = LED_STATE_BLE_CONNECTED_LED_ON;
          break;
        default:
          return;
      }
  }
}

//////////////////////////////////////////////////////////////////////////////

// Weak redefinition of button handler
void sl_button_on_change(const sl_button_t* p_handle)
{
    if (p_handle->get_state(p_handle) == BUTTON_PRESSED_STATE)
    {
        /** Change LED indication state */
        sl_led_turn_off(p_active_led_);
        next_led_state_ = sl_bt_is_connected() ?
                            LED_STATE_BLE_CONNECTED_LED_OFF :
                            LED_STATE_BLE_ADVERTISING_LED_OFF;
    }
}
