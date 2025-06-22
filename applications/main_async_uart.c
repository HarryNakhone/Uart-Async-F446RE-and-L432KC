#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net_buf.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/random/random.h>


#define UART_DEVICE_NODE DT_NODELABEL(usart3)

static const struct device * const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define TX_PACK_LEN_MAX 32
#define RX_CHUNK_LEN 32

#define LOOP_NUM 4

LOG_MODULE_REGISTER(main_ma, LOG_LEVEL_DBG);

NET_BUF_POOL_DEFINE(tx_pool, LOOP_NUM, TX_PACK_LEN_MAX, 0, NULL);

struct k_fifo tx_queue;
struct net_buf *tx_pending_buffer;  /// used for freeing memory (inside callback)

uint8_t rx_buffer[2][RX_CHUNK_LEN];

static volatile uint8_t index_rx_buff;

static void uart_callback_func(const struct device *dev, struct uart_event *evt, void *user_data){

    struct net_buf *buf;
    int rc;

    LOG_INF("EVENT: %d", evt->type);

    switch (evt->type){
        case UART_TX_DONE:

            LOG_HEXDUMP_INF(tx_pending_buffer->data, tx_pending_buffer->len, "TX Sent");
            LOG_INF("TX complete %p", tx_pending_buffer);

            net_buf_unref(tx_pending_buffer);
            tx_pending_buffer = NULL;

            buf = k_fifo_get(&tx_queue, K_NO_WAIT);
            if (buf != NULL){
                rc = uart_tx(dev, buf->data, buf->len, 0);
                if (rc != 0){
                    LOG_ERR("TX from ISR failed (%d)", rc);
                    net_buf_unref(buf);
                }else{
                    tx_pending_buffer = buf;
                }
            }
            break;


        case UART_RX_RDY:
            LOG_HEXDUMP_INF(evt->data.rx.buf + evt->data.rx.offset, evt-> data.rx.len, "RX_RDY");
            break;
        case UART_RX_BUF_RELEASED:
            LOG_INF("Buff released");
        case UART_RX_DISABLED:
            LOG_INF("Buff disabled");
            break;
        case UART_RX_BUF_REQUEST:
            LOG_INF("Providing buffer index %d", index_rx_buff);

            rc = uart_rx_buf_rsp(dev, rx_buffer[index_rx_buff] ,  sizeof(rx_buffer[0]));
            __ASSERT_NO_MSG(rc == 0);
            index_rx_buff = index_rx_buff ? 0 : 1;
            break;
        default:
            LOG_WRN("Unhandled event %d", evt->type);
            break;

    }

}

int main(void){

    if (!device_is_ready(uart_dev)){
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }
    bool rx_enabled = false;
    struct net_buf *tx_buffer;
    int loop_count = 0;
    uint8_t rand_num;
    int tx_length;
    int rc;

    uart_callback_set(uart_dev, uart_callback_func, (void *)uart_dev);

    while(1){
        k_sleep(K_SECONDS(6));

        rand_num = (sys_rand32_get() % LOOP_NUM) +  1;
        LOG_INF("Loop %d: Sending total %d pk", loop_count, rand_num );

        for (int i = 0; i < rand_num; i++){

            tx_buffer = net_buf_alloc(&tx_pool, K_FOREVER);

            tx_length = snprintk(tx_buffer->data, net_buf_tailroom(tx_buffer), "Hello\r\n");

            net_buf_add(tx_buffer, tx_length); // Telling zephyr the size of buf to send

            rc = uart_tx(uart_dev, tx_buffer->data, tx_buffer->len, SYS_FOREVER_US);

            if (rc == 0){

                tx_pending_buffer = tx_buffer; /// The reason we do this is because we cant use tx_buffer outta scope
            } else if (rc == -EBUSY){
                LOG_INF("Queue current buffer %p", tx_buffer);
                k_fifo_put(&tx_queue, tx_buffer);
            } else {
                LOG_ERR("Error sending pack %i", i);
            }
        }

        if(rx_enabled){
            uart_rx_disable(uart_dev);
        } else {
            index_rx_buff = 1;
            uart_rx_enable(uart_dev, rx_buffer[0], RX_CHUNK_LEN, 100);

        }
        
        rx_enabled = !rx_enabled;
        LOG_INF("RX is now %s", rx_enabled ? "enabled" : "disabled");

        loop_count += 1;


    }
    
}

