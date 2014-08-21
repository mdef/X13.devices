/*
Copyright (c) 2011-2014 <comparator@gmx.de>

This file is part of the X13.Home project.
http://X13home.org
http://X13home.net
http://X13home.github.io/

BSD New License
See LICENSE file for license details.
*/

// UART interface

#include "../../config.h"

#ifdef UART_PHY

static QueueHandle_t      uart_in_queue;
static QueueHandle_t      uart_out_queue;
static SemaphoreHandle_t  uart_isr_free_flag;
static MQ_t             * pUart_Tx_Buf;

void UART_Init(void)
{
  uart_in_queue  = xQueueCreate(8, sizeof(void *));
  uart_out_queue = xQueueCreate(4, sizeof(void *));
  uart_isr_free_flag = xSemaphoreCreateBinary();
  xSemaphoreGive(uart_isr_free_flag);

  uart_init_hw();
}

void UART_Send(void *pBuf)
{
  if(xSemaphoreTake(uart_isr_free_flag, 0) == pdTRUE)
  {
    pUart_Tx_Buf = pBuf;
    UART_TX_ENABLE_INT();                                 // Enable the UARTx Transmit interrupt
  }
  else
    xQueueSend(uart_in_queue, &pBuf, portMAX_DELAY);
}

void UART_Get(void *pBuf)
{
  xQueueReceive(uart_out_queue, pBuf, portMAX_DELAY);
}

void uart_rx_handler(BaseType_t * pxHigherPriorityTaskWoken)
{
  static uint8_t  rx_pos = 0;
  static uint8_t  rx_len = 0;
  static MQ_t   * pRx_buf;
  static bool     rx_db = false;

  // Read one byte from the receive data register
  uint8_t data = UART_RX_DATA;

  // Convert from SLEEP to RAW data
  if(data == 0xC0)
  {
    if((rx_len > 1) && (rx_len == (rx_pos - 1)))
    {
      UART_ADDR_t s_addr = UART_PHY;
      memcpy(pRx_buf->addr.UART_ADDR, &s_addr, sizeof(UART_ADDR_t));
      xQueueSendFromISR(uart_out_queue, &pRx_buf, pxHigherPriorityTaskWoken);
    }

    rx_len = 0;
    rx_pos = 0;
    rx_db = false;
    return;
  }
  else if(rx_db)
  {
    rx_db = false;
    data ^= 0x20;
  }
  else if(data == 0xDB)
  {
    rx_db = true;
    return;
  }

  if(rx_pos == 0)  // Get Length
  {
    if((data > 1) && (data <= sizeof(MQTTSN_MESSAGE_t)) && ((pRx_buf = pvPortMalloc(sizeof(MQ_t))) != NULL))
    {
      rx_len = data;
      pRx_buf->Length = data;
      rx_pos = 1;
    }
    else
    {
      rx_len = 0;
      rx_pos = 0xFF;
    }
  }
  else if(rx_pos <= rx_len)
  {
    pRx_buf->u.raw[rx_pos - 1] = data;
    rx_pos++;
  }
  else  // overflow
  {
    rx_len = 0;
    rx_pos = 0xFF;
  }
}

void uart_tx_handler(BaseType_t * pxHigherPriorityTaskWoken)
{
  static uint8_t  tx_pos = 0;
  static uint8_t  tx_len = 0;
  static bool     tx_db = false;

  uint8_t data;

start_tx_handler:

  if(tx_pos == 0)             // Send Length
  {
    data = pUart_Tx_Buf->Length;
    tx_len = data;
  }
  else if(tx_pos <= tx_len)
    data = pUart_Tx_Buf->u.raw[tx_pos - 1];
  else if(tx_pos == (tx_len + 1))
  {
    taskENTER_CRITICAL();
    vPortFree(pUart_Tx_Buf);
    UART_TX_DATA = 0xC0;      // Send End Of Frame
    tx_pos++;
    taskEXIT_CRITICAL();
    return;
  }
  else
  {
    tx_pos = 0;
    if(xQueueReceiveFromISR(uart_in_queue, &pUart_Tx_Buf, pxHigherPriorityTaskWoken) == pdFALSE)  // Queue is empty
    {
      // Disable the UARTx Transmit interrupt
      UART_TX_DISABLE_INT();
      
      xSemaphoreGiveFromISR(uart_isr_free_flag, pxHigherPriorityTaskWoken);
      return;
    }
    goto start_tx_handler;
  }
  
  // Convert data from RAW to SLEEP format
  if((data == 0xC0) || (data == 0xDB))
  {
    if(tx_db)
    {
      tx_db = false;
      data ^= 0x20;
      tx_pos++;
    }
    else
    {
      tx_db = true;
      data = 0xDB;
    }
  }
  else
    tx_pos++;

  UART_TX_DATA = data;
}

#endif  //  UART_PHY
