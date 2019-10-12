
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <PeripheralRegs.h>
#include "contiki-conf.h"
#include "dev/uart1.h"
#include "uart-driver.h"

#define uart_dev E_AHI_UART_1

/* Valid range for TXBUFSIZE and RXBUFSIZE: 16-2047 */
#define RXBUFSIZE 2047
#if UART_XONXOFF_FLOW_CTRL
#define TXBUFSIZE 16
#else
#define TXBUFSIZE 2047
#endif

static unsigned char txbuf_data[TXBUFSIZE];
static unsigned char rxbuf_data[RXBUFSIZE];
static int (*uart1_input)(unsigned char c);

uint8_t
uart1_active(void)
{
  return uart_driver_tx_in_progress(uart_dev);
}

void
uart1_set_input(int
(*input)(unsigned char c))
{
  uart1_input = input;
  uart_driver_set_input(uart_dev, uart1_input);
}

void
uart1_writeb(unsigned char c)
{
  uart_driver_write_buffered(uart_dev, c);
}

void
uart1_init(uint8_t br)
{
  uart_driver_init(uart_dev, br, txbuf_data, TXBUFSIZE, rxbuf_data, RXBUFSIZE, uart1_input);
}

void
uart1_write_direct(unsigned char c)
{
  uart_driver_write_direct(uart_dev, c);
}

void
uart1_invoke_read(void)
{
  uart_driver_rx_handler(uart_dev);
}

void
uart1_disable_interrupts(void)
{
  uart_driver_disable_interrupts(uart_dev);
}

void
uart1_enable_interrupts(void)
{
  uart_driver_enable_interrupts(uart_dev);
}

int8_t
uart1_interrupt_is_enabled(void)
{
  return uart_driver_interrupt_is_enabled(uart_dev);
}

void
uart1_restore_interrupts(void)
{
  uart_driver_restore_interrupts(uart_dev);
}

void
uart1_store_interrupts(void)
{
  uart_driver_store_interrupts(uart_dev);
}
