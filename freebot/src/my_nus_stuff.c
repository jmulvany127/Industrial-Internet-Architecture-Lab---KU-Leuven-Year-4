#include "uart_async_adapter.h"

#include <zephyr/logging/log.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <soc.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include <bluetooth/services/nus.h>

#include <stdio.h>

#include <zephyr/settings/settings.h>
#include "fb_io.h"
#include "fb_motor.h"
#include "fb_pwr.h"

#include "my_nus_stuff.h"

LOG_MODULE_REGISTER(my_nus, LOG_LEVEL_INF);

// #define CONFIG_BT_NUS_UART_BUFFER_SIZE 40
// #define CONFIG_BT_NUS_UART_RX_WAIT_TIME 50

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define UART_WAIT_FOR_BUF_DELAY K_MSEC(50)
#define UART_WAIT_FOR_RX CONFIG_BT_NUS_UART_RX_WAIT_TIME

extern struct k_sem ble_init_ok;
static K_SEM_DEFINE(sem_reading, 0, 1); // semaphore for the cyclical readings buffer

extern struct k_fifo fifo_tx_data;
extern struct k_fifo fifo_rx_data;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        LOG_ERR("Connection failed (err %u)", err);
        return;
    }

    // bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected:(reason %u)", reason);
}

static void on_security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err)
    {
        LOG_INF("Security changed: %s level %u", addr, level);
    }
    else
    {
        LOG_WRN("Security failed: %s level %u err %d", addr, level, err);
    }
}

struct bt_conn_cb connection_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = on_security_changed,
};
static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
    char addr[BT_ADDR_LE_STR_LEN] = {0};

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));

    LOG_INF("Received data from: %s", addr);

    for (uint16_t pos = 0; pos != len;)
    {
        struct packet_data_t *tx = k_malloc(sizeof(*tx));

        if (!tx)
        {
            LOG_WRN("Not able to allocate UART send data buffer");
            return;
        }

        /* Keep the last byte of TX buffer for potential LF char. */
        size_t tx_data_size = sizeof(tx->data) - 1;

        if ((len - pos) > tx_data_size)
        {
            tx->len = tx_data_size;
        }
        else
        {
            tx->len = (len - pos);
        }

        memcpy(tx->data, &data[pos], tx->len);

        pos += tx->len;

        /* Append the LF character when the CR character triggered
         * transmission from the peer.
         */
        if ((pos == len) && (data[len - 1] == '\r'))
        {
            tx->data[tx->len] = '\n';
            tx->len++;
        }

        // process the received command
        k_fifo_put(&fifo_rx_data, tx);
    }
}
static struct bt_nus_cb nus_cb = {
    .received = bt_receive_cb,
};

void error(void)
{
    // SET LEDS TO SHOW there is an ERROR
    fb_set_led(D15);
    fb_set_led(D16);

    while (true)
    {
        /* Spin for ever */
        k_sleep(K_MSEC(1000));
    }
}

int my_nus_main()
{
    int err;

    k_sem_give(&sem_reading);

    bt_conn_cb_register(&connection_callbacks);

    err = bt_enable(NULL);
    if (err)
    {
        error();
    }

    LOG_INF("Bluetooth initialized");

    k_sem_give(&ble_init_ok);

    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        settings_load();
    }
    err = bt_nus_init(&nus_cb);
    if (err)
    {
        LOG_ERR("Failed to initialize UART service (err: %d)", err);
        return -1;
    }

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err)
    {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return -1;
    }

    return 0;
}

void writeToBufAsHex(const char *buf, const size_t len, char *t)
{
    for (size_t j = 0; j < len; j++)
    {
        t[5 * j + 0] = '0';
        t[5 * j + 1] = 'x';

        int t0 = buf[j];
        int t1 = t0 / 16;
        int t2 = t0 % 16;

        if (t1 > 9)
        {
            t[5 * j + 2] = (char)(0x41 + t1 - 10);
        }
        else
        {
            t[5 * j + 2] = (char)(0x30 + t1);
        }
        if (t2 > 9)
        {
            t[5 * j + 3] = (char)(0x41 + t2 - 10);
        }
        else
        {
            t[5 * j + 3] = (char)(0x30 + t2);
        }
        t[5 * j + 4] = ' ';
    }
}

void encodeIntegerIntoChar(const uint32_t value, char *buf, const size_t max_len, size_t *len)
{
    if (max_len < 4)
        return;
    buf[0] = 0x000000ff & value << 0;
    buf[1] = 0x0000ff00 & value << 8;
    buf[2] = 0x00ff0000 & value << 16;
    buf[3] = 0xff000000 & value << 24;
    *len = 4;
    return;
}
void _encodeIntegerIntoCharUNSAFE(uint32_t value, uint8_t *buf)
{

    value = 0x54455354;
    buf[0] = (uint8_t)(0x000000ff & value << 0);
    buf[1] = (uint8_t)(0x0000ff00 & value << 8);
    buf[2] = (uint8_t)(0x00ff0000 & value << 16);
    buf[3] = (uint8_t)(0xff000000 & value << 24);
}

#define READING_MAX_SIZE 5
static struct reading
{
    fb_motor_angle_t motor_angles;
    fb_motor_speed_t motor_speeds;
    int v_cap;
} readings[READING_MAX_SIZE];
static size_t reading_pos = 0;
static size_t readings_filled = 0;

// comma seperated
static fb_motor_angle_t motor_angles;
static fb_motor_speed_t motor_speeds;

// blocking write
static inline void writeToCycReadingsBuf(struct reading r)
{
    k_sem_take(&sem_reading, K_FOREVER);
    readings[reading_pos] = r; // passed by value or reference! (if ref => floating pointer)
    reading_pos = (reading_pos + 1) % READING_MAX_SIZE;
    if (readings_filled < READING_MAX_SIZE)
        readings_filled++;
    k_sem_give(&sem_reading);
}

// does not purge, just resets the pointers
static inline void resetReadingsBuf()
{
    reading_pos = 0;
    readings_filled = 0;
}

void sensor_read_thread(void)
{
    for (;;)
    {
        fb_get_motor_angle(&motor_angles);
        fb_get_motor_speed(&motor_speeds);
        int vcap = fb_v_measure();

        struct reading r = {
            .motor_angles = {
                .back_left = motor_angles.back_left,
                .back_right = motor_angles.back_right,
                .front_left = motor_angles.front_left,
                .front_right = motor_angles.front_right},
            .motor_speeds = {
                .back_left = motor_speeds.back_left,
                .back_right = motor_speeds.back_right,
                .front_left = motor_speeds.front_left,
                .front_right = motor_speeds.front_right,
            },
            .v_cap = vcap};

        writeToCycReadingsBuf(r);
        k_sleep(K_MSEC(200));
    }
}
K_THREAD_DEFINE(sensor_read_thread_id, STACKSIZE, sensor_read_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

enum readingType_t
{
    AFL,
    AFR,
    ABL,
    ABR,
    DFL,
    DFR,
    DBL,
    DBR,
    VCA,
};

const enum readingType_t toTransmit[] = {
    VCA,
    DFL,
    DFR,
    DBL,
    DBR,
};

/*
    motor_angles.front_left, //Z
    motor_angles.front_right, //Y
    motor_angles.back_left, //X
    motor_angles.back_right); //W
    motor_speeds.front_left, //V
    motor_speeds.front_right, //U
    motor_speeds.back_left, //T
    motor_speeds.back_right); //S
    fb_v_measure_select(V_CAP); //R
    '_' if unknown
*/
char convertReadingTypeToChar(const enum readingType_t t)
{
    switch (t)
    {
    case AFL:
        return 'Z';
    case AFR:
        return 'Y';
    case ABL:
        return 'X';
    case ABR:
        return 'W';
    case DFL:
        return 'V';
    case DFR:
        return 'U';
    case DBL:
        return 'T';
    case DBR:
        return 'S';
    case VCA:
        return 'R';
    default:
        return '_';
    }
}
int readVal(const enum readingType_t t, const struct reading *r)
{
    switch (t)
    {
    case AFL:
        return r->motor_angles.front_left;
    case AFR:
        return r->motor_angles.front_right;
    case ABL:
        return r->motor_angles.back_left;
    case ABR:
        return r->motor_angles.back_right;
    case DFL:
        return r->motor_speeds.front_left;
    case DFR:
        return r->motor_speeds.front_right;
    case DBL:
        return r->motor_speeds.back_left;
    case DBR:
        return r->motor_speeds.back_right;
    case VCA:
        return r->v_cap;
    default:
        return 0;
    }
}

/**
 * @brief Serialize the average reading of the last 5
 *
 * @param dest
 * @param t
 * @param r
 * @return int
 */
int serialize_reading(uint8_t *dest, const enum readingType_t t, const struct reading *r)
{
    size_t pos = 0; // position in the buffer

    size_t p_s = 0; // read from here
    size_t p_e = 0; // till here (exclusiding the element @p_e)
    if (readings_filled == READING_MAX_SIZE)
    {
        // run from p -> p+f under mod READING_MAX_SIZE
        p_s = reading_pos;
        p_e = reading_pos + readings_filled;
    }
    else if (readings_filled > READING_MAX_SIZE)
    {
        // something went wrong, read from 0 -> READING_MAX_SIZE - 1
        p_s = 0;
        p_e = READING_MAX_SIZE;
    }
    else
    {
        // run from 0 -> f (=p-1) under mod READING_MAX_SIZE, here the mod can be dropped since f < READING_MAX_SIZE (see if statement)
        p_s = 0;
        p_e = readings_filled;
    }

    // *(uint16_t*)&(dest[pos]) = 0x4100;
    // return 2;

    int total_value = 0;

    for (size_t i = p_s; i < p_e; i++)
    {
        // read sensor values as an absolute value
        int val = readVal(t, &(r[i % READING_MAX_SIZE]));
        if (val < 0)
            val *= -1;
        total_value += val;
    }

    if (p_e - p_s == 0)
        *(uint16_t *)&(dest[pos]) = 0x0000;
    else
        *(uint16_t *)&(dest[pos]) = (uint16_t)(total_value / (p_e - p_s));
    pos += sizeof(uint16_t);
    return pos;
}

/**
 * @brief
 *
 * @param dest
 * @return int the nubmer of characters written
 */
int serialize_readings_buf(uint8_t *dest)
{
    size_t pos = 0;
    dest[pos++] = '1';
    dest[pos++] = '3';
    k_sem_take(&sem_reading, K_FOREVER);
    for (size_t i = 0; i < sizeof(toTransmit) / sizeof(enum readingType_t); i++)
    {
        pos += serialize_reading(&dest[pos], toTransmit[i], readings);
        if (i < sizeof(toTransmit) / sizeof(enum readingType_t) - 1)
            dest[pos++] = ',';
    }
    k_sem_give(&sem_reading);

    return pos;
}

uint8_t bt_tx_buffer[40] = {0};
void ble_write_thread(void)
{
    for (;;)
    {
        int pos = serialize_readings_buf(bt_tx_buffer);

        k_sem_take(&ble_init_ok, K_FOREVER);
        /* Send data over Bluetooth LE to remote device(s) */
        if (bt_nus_send(NULL, bt_tx_buffer, pos))
        {
            LOG_WRN("Failed to send data over BLE connection");
        }
        k_sem_give(&ble_init_ok);
        // shred old message. (not per se required)
        memset(bt_tx_buffer, 0, pos);
        k_sleep(K_MSEC(1000));
    }
}
/* Create a dedicated thread for sending the data over Bluetooth LE. */
K_THREAD_DEFINE(ble_write_thread_id, STACKSIZE, ble_write_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

#define CMD_RUN_DELAY K_MSEC(500)
void processCommand(const char *cmd, const size_t len)
{
    if (len != 5)
        return;
    char *prependedCmd = "13<";
    if (memcmp(cmd, prependedCmd, 3) != 0)
        return;
    if (cmd[len - 1] != '>')
        return; // mallformed commands

    switch (cmd[len - 2])
    {
    case 'F':
        fb_straight_forw();
        break;
    case 'B':
        fb_straight_back();
        break;
    case 'L':
        fb_side_left();
        break;
    case 'R':
        fb_side_right();
        break;
    case '1':
        fb_side_d45();
        break;
    case '2':
        fb_side_d135();
        break;
    case '3':
        fb_side_d225();
        break;
    case '4':
        fb_side_d315();
        break;
    case 'C':
        fb_rotate_cw();
        break;
    case 'D':
        fb_rotate_ccw();
        break;

    default:
        break;
    }

    /*
    F = forward
    B = back
    R = right
    L = left
    1 = 45
    2 = 135
    3 = 225
    4 = 315
    C = clock
    D = counter clock
    */
}

void read_thread(void)
{
    for (int i = 0;; i++)
    {
        /* Wait indefinitely for data from the UART peripheral */
        struct packet_data_t *buf = k_fifo_get(&fifo_rx_data, K_FOREVER);

        if (buf->len == 4 && buf->data[buf->len - 1] == '\000')
            processCommand(buf->data, 3);
        else
            processCommand(buf->data, buf->len);
        k_free(buf);
        k_sleep(CMD_RUN_DELAY);
        fb_stop();
    }
}
/* Create a dedicated thread for sending the data over Bluetooth LE. */
K_THREAD_DEFINE(ble_read_thread_id, STACKSIZE, read_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
