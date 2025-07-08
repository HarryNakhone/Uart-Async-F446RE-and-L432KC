#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(main_rx, LOG_LEVEL_DBG);


#define UART_DEVICE_NODE DT_NODELABEL(usart1)

static const struct device * const dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define RING_BUF_SIZE 512

#define RX_CHUNK_LEN 32

#define MSG_SIZE 8

K_SEM_DEFINE(rx_disabled, 0, 1);

#define RX_MSG_QUEUE 8

struct rx_msg {
    uint8_t bytes[MSG_SIZE];
    uint32_t length;
};

volatile int bytes_claimed;

static uint8_t rx_buffer[2][RX_CHUNK_LEN];
static volatile uint8_t idx_rx_buffer;

K_MSGQ_DEFINE(rx_msg_queue, sizeof(struct rx_msg), RX_MSG_QUEUE , 4);


static void uart_callback_func(const struct device * dev, struct uart_event *evt, void * user_data){
    LOG_DBG("UART event received on %s", dev->name);

    static struct rx_msg new_message;

    int rc;

    switch (evt->type){
    case UART_RX_RDY:
        //Display content
        LOG_HEXDUMP_INF(evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len, "RX_RDY");

        memcpy(new_message.bytes,evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len); 
        new_message.length = evt->data.rx.len;
        if (k_msgq_put(&rx_msg_queue, &new_message, K_NO_WAIT) != 0){
            LOG_ERR("Error: RX queue full!");
        }

        break;

    case UART_RX_BUF_REQUEST:
        LOG_DBG("Requesting new buffer");

        //Chaneg to next buff
        LOG_DBG("Providing Rx buffer idx: %d", idx_rx_buffer);
        rc = uart_rx_buf_rsp(dev, rx_buffer[idx_rx_buffer], RX_CHUNK_LEN);
        __ASSERT_NO_MSG(rc==0);
        idx_rx_buffer = (idx_rx_buffer + 1) % 2;


        break;
       
    case UART_RX_BUF_RELEASED:
        LOG_DBG("RX buffer released");
        break;

    case UART_RX_DISABLED:
       LOG_INF("RX disabled");
        
       rc = uart_rx_enable(dev, rx_buffer[idx_rx_buffer], RX_CHUNK_LEN, 100);
        if (rc != 0){
                LOG_ERR("Failed to enable RX... %d", rc);
        } else{
            LOG_INF("RX re-enabled");
        }
        break;
    default:
        LOG_WRN("Unhandled event %d", evt->type);
        break;
    
    }
}

int main(void){



    k_sleep(K_MSEC(100));

    if (!device_is_ready(dev)){
        LOG_ERR("Uart device is not ready");
        return -ENODEV;
    }

    idx_rx_buffer =0;

    uart_callback_set(dev, uart_callback_func, NULL);



   int rx_test = uart_rx_enable(dev, rx_buffer[idx_rx_buffer], RX_CHUNK_LEN, 100);
    if (rx_test != 0) {
        LOG_ERR("Initial uart_rx_enable() failed: %d", rx_test);
    return rx_test;
    }


    LOG_INF("Uart enabled, listening....");

    uint8_t temp_msg[MSG_SIZE];

    uint8_t lines = 0;


    while(1){
        struct rx_msg incoming_message;

        k_msgq_get(&rx_msg_queue, &incoming_message, K_FOREVER);

        for (size_t i = 0; i < incoming_message.length ; i++){
            uint8_t charac = incoming_message.bytes[i];

            if ( lines < MSG_SIZE - 1){
                temp_msg[lines++] = charac;

                if (charac == '\n'){
                    temp_msg[lines] = '\0';
                    LOG_INF("Rx length %i, %s",lines, temp_msg);
                    lines = 0;
                }

            }else {
                LOG_WRN("Buffer overflow ");
                lines = 0;
            }

        }



    }


}