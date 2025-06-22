#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net_buf.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/random/random.h>

#define UART_DEVICE_NODE DT_NODELABEL(usart3)

static const struct device * const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define RX_CHUNK_LEN 32

#define RX_BUFFER_NUM 2


LOG_MODULE_REGISTER(main_mb, LOG_LEVEL_DBG);

static uint8_t rx_buffer[RX_BUFFER_NUM][RX_CHUNK_LEN];
static volatile uint8_t index_rx_buff;

static void uart_callback_func( const struct device * dev, struct uart_event *evt, void * user_data){
     LOG_DBG("UART event received on %s", dev->name);

    int rc;

    switch (evt->type) {
        case UART_RX_RDY:

            LOG_HEXDUMP_INF(evt->data.rx.buf + evt->data.rx.offset,
				evt->data.rx.len, "RX_RDY");
            break;
        case UART_RX_BUF_REQUEST:
            LOG_DBG("Providing RX buffer idx: %d", index_rx_buff);
            rc = uart_rx_buf_rsp(dev, rx_buffer[index_rx_buff], RX_CHUNK_LEN);
            __ASSERT_NO_MSG(rc == 0);
            index_rx_buff = (index_rx_buff + 1) % RX_BUFFER_NUM;
            break;


        case UART_RX_BUF_RELEASED:
            LOG_DBG("RX buffer released");
            break;
        
        case UART_RX_DISABLED:
            LOG_INF("RX disabled");
            rc = uart_rx_enable(dev, rx_buffer[index_rx_buff], RX_CHUNK_LEN, 100);

            if (rc != 0){
                LOG_ERR("Failed to enable RX... %d", rc);
            }
            break;

        default:
            LOG_WRN("Unhandled event %d", evt->type);
            break;


    }
}

int main(void){
    int rc;

    k_sleep(K_MSEC(100));

    if (!device_is_ready(uart_dev)){
        LOG_ERR("Device is not ready\n");
        return -ENODEV;
    }

    index_rx_buff = 0;

    uart_callback_set(uart_dev, uart_callback_func, NULL);

    rc = uart_rx_enable(uart_dev, rx_buffer[index_rx_buff], RX_CHUNK_LEN, 100);

    if (rc){
        LOG_ERR("failed to enable to uart: %d", rc);
        return rc;
    }

    LOG_INF("Enable Uart RX, listening... \n ");

    while (1){
        k_sleep(K_SECONDS(10));
    }
}