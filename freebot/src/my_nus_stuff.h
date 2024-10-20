#ifndef MY_NUS_STUFF_H
#define MY_NUS_STUFF_H

#include <zephyr/kernel.h>

#define STACKSIZE CONFIG_BT_NUS_THREAD_STACK_SIZE
#define PRIORITY 7

#define PASSKEY 99

static K_FIFO_DEFINE(fifo_tx_data);
static K_FIFO_DEFINE(fifo_rx_data);
static K_SEM_DEFINE(ble_init_ok, 0, 1);

/* STEP 6.2 - Declare the struct of the data item of the FIFOs */
struct packet_data_t
{
    void *fifo_reserved;
    uint8_t data[CONFIG_BT_NUS_UART_BUFFER_SIZE];
    uint16_t len;
};

int my_nus_main();

#endif // MY_NUS_STUFF_H
