#include <drivers/serial/uart_dev.h>
#include <drivers/serial/diag_serial.h>
#include <drivers/gpio/gpio.h>
#include <drivers/ttys.h>
#include "plib035_uart.h"
#include "semaphore.h"

#define BAUDRATE OPTION_GET(NUMBER, baudrate)

extern const struct uart_ops k1921vk035_uart_ops;

typedef enum {
    IO_REQUEST_NONE = 0,
    IO_REQUEST_READ = 1,
    IO_REQUEST_WRITE = 2,
} io_type;

typedef struct {
    sem_t semaphore;
    io_type type;
    char* buffer;
    size_t count;
    struct uart* uart;
} io_request_t;

irq_return_t uart1_handler(unsigned int irq_nr, void *data) {
    io_request_t* io_req = (io_request_t* )data;
    int ch = uart_getc(io_req->uart);
#if 1
    switch(io_req->type) {
        case IO_REQUEST_NONE:
            return IRQ_HANDLED;

        case IO_REQUEST_READ:
            io_req->buffer[0] = ch;
            io_req->buffer++;
            io_req->count--;
            if(io_req->count == 0) {
                sem_post(&io_req->semaphore);
            }

        case IO_REQUEST_WRITE:
            return IRQ_HANDLED;
    }
#endif

    return IRQ_HANDLED;
}

static struct uart uart1 = {
		.uart_ops = &k1921vk035_uart_ops,
		.irq_num = UART1_RX_IRQn, // TODO: Which irq should we use?
		.base_addr = (uint32_t)UART1,
        .irq_handler = uart1_handler,
        .params = {
            .baud_rate = BAUDRATE,
            .uart_param_flags = UART_PARAM_FLAGS_8BIT_WORD | UART_PARAM_FLAGS_DEV_TYPE_UART | UART_PARAM_FLAGS_USE_IRQ,
        },
};

static io_request_t io_request = {
    .type = IO_REQUEST_NONE,
    .buffer = NULL,
    .count = 0,
    .uart = &uart1,
};

void io_request_clean() {
    io_request.type = IO_REQUEST_NONE;
    io_request.buffer = NULL;
    io_request.count = 0;
}

int io_request_read(char* buf, size_t count) {
    irq_lock();
    {
        io_request.type = IO_REQUEST_READ;
        io_request.buffer = buf;
        io_request.count = count;
    }
    irq_unlock();
    // sched_wait_timeout(clock_t timeout, clock_t *remain);
    sem_wait(&io_request.semaphore);
    // sem_timedwait(&io_request.semaphore);
    return count;
}

void io_request_init() {
    sem_init(&io_request.semaphore, 0, 0);
    io_request.type = IO_REQUEST_NONE;
    io_request.buffer = NULL;
    io_request.count = 0;
    io_request.uart = &uart1;
}

STATIC_IRQ_ATTACH(30, uart1_handler, &io_request);
