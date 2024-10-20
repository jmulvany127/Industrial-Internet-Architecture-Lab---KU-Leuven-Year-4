/**
 * @file main.c
 * @author Lowie Deferme <lowie.deferme@kuleuven.be>
 * @brief FreeBot zephyr example
 * @version 0.1
 * @date 2024-03-26
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <soc.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

// #include <bluetooth/services/lbs.h>
#include <bluetooth/services/nus.h>

#include <zephyr/settings/settings.h>

#include <stdio.h>

#include "my_nus_stuff.h"

extern struct k_fifo fifo_tx_data;
extern struct k_fifo fifo_rx_data;
extern struct k_sem ble_init_ok;
// #include "bt_lbs.h"

#define USER_BUTTON DK_BTN1_MSK
#define RUN_STATUS_LED DK_LED1
/* STEP 3.1 - Define an LED to show the connection status */
#define CONNECTION_STATUS_LED DK_LED2
#define RUN_LED_BLINK_INTERVAL 1000

LOG_MODULE_REGISTER(freebot, LOG_LEVEL_DBG);

#include "freebot.h"
#define FB_DEMO_DELAY K_MSEC(500)

static fb_motor_angle_t motor_angles;
static fb_motor_speed_t motor_speeds;

void pwr_measure_demo(void)
{
    int v_cap = fb_v_measure();
    LOG_DBG("Vcap = %dmV", v_cap);
    k_sleep(FB_DEMO_DELAY);
}

void motor_drive_demo(void)
{
    fb_straight_forw();
    LOG_DBG("Robot going forward");
    k_sleep(FB_DEMO_DELAY);

    fb_straight_back();
    LOG_DBG("Robot going backward");
    k_sleep(FB_DEMO_DELAY);

    fb_side_right();
    LOG_DBG("Robot going right");
    k_sleep(FB_DEMO_DELAY);

    fb_side_left();
    LOG_DBG("Robot going left");
    k_sleep(FB_DEMO_DELAY);

    fb_side_d45();
    LOG_DBG("Robot going 45°");
    k_sleep(FB_DEMO_DELAY);

    fb_side_d135();
    LOG_DBG("Robot going 135°");
    k_sleep(FB_DEMO_DELAY);

    fb_side_d225();
    LOG_DBG("Robot going 225°");
    k_sleep(FB_DEMO_DELAY);

    fb_side_d315();
    LOG_DBG("Robot going 315°");
    k_sleep(FB_DEMO_DELAY);

    fb_rotate_cw();
    LOG_DBG("Robot rotating clockwise");
    k_sleep(FB_DEMO_DELAY);

    fb_rotate_ccw();
    LOG_DBG("Robot rotating counterclockwise");
    k_sleep(FB_DEMO_DELAY);

    fb_stop();
    LOG_DBG("Robot stopped");
}

// motor_angles.front_left, //Z
// motor_angles.front_right, //Y
// motor_angles.back_left, //X
// motor_angles.back_right); //W
// motor_speeds.front_left, //V
// motor_speeds.front_right, //U
// motor_speeds.back_left, //T
// motor_speeds.back_right); //S
// fb_v_measure_select(V_CAP); //R
// <A|val,B|val,C|val>

void motor_measure_demo(void)
{
    fb_get_motor_angle(&motor_angles);
    LOG_DBG("{%d°, %d°, %d°, %d°}",
            motor_angles.front_left,  // Z
            motor_angles.front_right, // Y
            motor_angles.back_left,   // X
            motor_angles.back_right); // W

    fb_rotate_cw();
    k_sleep(K_MSEC(200));
    fb_get_motor_speed(&motor_speeds);
    LOG_DBG("{%d rpm, %d rpm, %d rpm, %d rpm}",
            motor_speeds.front_left,  // V
            motor_speeds.front_right, // U
            motor_speeds.back_left,   // T
            motor_speeds.back_right); // S
    fb_stop();
    for (int i = 0; i < 4; i++)
    {
        k_sleep(K_MSEC(100));
        fb_get_motor_speed(&motor_speeds);
        LOG_DBG("{%d rpm, %d rpm, %d rpm, %d rpm}",
                motor_speeds.front_left,
                motor_speeds.front_right,
                motor_speeds.back_left,
                motor_speeds.back_right);
    }

    fb_get_motor_angle(&motor_angles);
    LOG_DBG("{%d°, %d°, %d°, %d°}",
            motor_angles.front_left,
            motor_angles.front_right,
            motor_angles.back_left,
            motor_angles.back_right);
}

// #define DEVICE_NAME CONFIG_BT_DEVICE_NAME
// #define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

// struct bt_conn *my_conn = NULL;

// static const struct bt_data ad[] = {
// 	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
// 	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
// };

// static const struct bt_data sd[] = {
// 	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
// };

// // /* STEP 5.1 - Create the advertising parameter for connectable advertising */
// // static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
// // 	(BT_LE_ADV_OPT_CONNECTABLE |
// // 	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
// // 	800, /* Min Advertising Interval 500ms (800*0.625ms) */
// // 	801, /* Max Advertising Interval 500.625ms (801*0.625ms) */
// // 	NULL); /* Set to NULL for undirected advertising */
// static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
// 	(BT_LE_ADV_OPT_CONNECTABLE |
// 	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
// 	BT_GAP_ADV_FAST_INT_MIN_1, /* 0x30 units, 48 units, 30ms */
// 	BT_GAP_ADV_FAST_INT_MAX_1, /* 0x60 units, 96 units, 60ms */
// 	NULL); /* Set to NULL for undirected advertising */

// //nof connected ble devices
// static bool bleConnected = false;

// /* STEP 2.2 - Implement the callback functions */
// void on_connected(struct bt_conn *conn, uint8_t err)
// {
// 	if (err) {
// 		LOG_ERR("Connection error %d", err);
// 		return;
// 	}
// 	LOG_INF("Connected");
// 	my_conn = bt_conn_ref(conn);
//     bleConnected = true;

// 	/* STEP 3.2  Turn the connection status LED on */
//     // fb_toggle_led(D15);

// }

// void on_disconnected(struct bt_conn *conn, uint8_t reason)
// {
// 	LOG_INF("Disconnected. Reason %d", reason);
// 	bt_conn_unref(my_conn);
//     bleConnected = false;

// 	/* STEP 3.3  Turn the connection status LED off */
//     // fb_toggle_led(D15);
// }

// /* STEP 2.1 - Declare the connection_callback structure */
// struct bt_conn_cb connection_callbacks = {
// 	.connected = on_connected,
// 	.disconnected = on_disconnected,
// };

// /* STEP 8.2 - Define the application callback function for controlling the LED */
// static void app_led_cb(bool led_state)
// {
//     if (led_state) {
//         fb_set_led(D16);
//         fb_rotate_cw();
//         LOG_DBG("Robot going forward");
//     } else {
//         fb_clear_led(D16);
//         fb_stop();
//         LOG_DBG("Robot stopped");
//     }
//     k_sleep(FB_DEMO_DELAY);
// }

// /* STEP 9.2 - Define the application callback function for reading the state of the button */
// static int app_button_cb(void)
// {
//     fb_get_motor_angle(&motor_angles);
//     // return motor_angles.front_right;
//     return 0xF5;
// }

// /* STEP 10 - Declare a varaible app_callbacks of type my_lbs_cb and initiate its members to the applications call back functions we developed in steps 8.2 and 9.2. */
// static struct my_lbs_cb app_callbacks = {
//     .led_cb = app_led_cb,
//     .button_cb = app_button_cb,
// };

// // -----------------
// //  COMMAND PARSING
// // -----------------
// // void processCommand(const unsigned char* buffer, const size_t length) {
// //     int command = 0;

// //     return;
// // }

extern int bleConnected;

int main(void)
{
    LOG_DBG("Starting Robot");
    fb_init();
    fb_v_measure_select(V_CAP); // R

    int err;
    err = my_nus_main();

    // /* STEP 2.3 - Register our custom callbacks */
    // bt_conn_cb_register(&connection_callbacks);

    // err = bt_enable(NULL);
    // if (err) {
    // 	printk("Bluetooth init failed (err %d)\n", err);
    // 	return -1;
    // }
    // LOG_DBG("Bluetooth initialized\n");

    // /* STEP 11 - Pass your application callback functions stored in app_callbacks to the MY LBS service */
    // err = my_lbs_init(&app_callbacks);
    // if (err) {
    // 	printk("Failed to init LBS (err:%d)\n", err);
    // 	return -1;
    // }

    // // advertise
    // err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    // if (err) {
    // 	printk("Advertising failed to start (err %d)\n", err);
    // 	return -1;
    // }

    for (;;)
    {
        // Flash both leds in out of sync to indicate ready
        fb_set_led(D15);
        fb_clear_led(D16);
        for (;;)
        {
            k_sleep(K_MSEC(2000));
            fb_toggle_led(D15);
            fb_toggle_led(D16);

            // if (bleConnected)
            // {
            //     LOG_DBG("ble connected");

            //     // err = bt_lbs_send_button_state(true);
            //     if (err) {
            //         LOG_ERR("Couldn't send notification. err: %d", err);
            //     }
            //     k_sleep(K_MSEC(300));
            //     fb_set_led(D15);
            //     fb_set_led(D16);
            //     while (1) {};
            // }
        }

        // Turn both leds off during operation
        fb_clear_led(D15);
        fb_clear_led(D16);

        // pwr_measure_demo();
        // motor_measure_demo();
        // motor_drive_demo();
    }
    return 0;
}

// void move_thread(void)
// {
// 	for (int i = 0;;i++) {
//         // motor_drive_demo();
//         fb_straight_forw();
//         k_sleep(K_MSEC(4000));
//         fb_straight_back();
//         k_sleep(K_MSEC(4000));
//         fb_stop();
//         k_sleep(K_MSEC(4000));

// 	}
// }
// /* STEP 9.2 - Create a dedicated thread for sending the data over Bluetooth LE. */
// K_THREAD_DEFINE(move_thread_id, STACKSIZE, move_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
