#ifndef USART0_RX_vect
#define USART0_RX_vect  USART_RX_vect
#endif  //  USART0_RX_vect

#ifndef USART0_UDRE_vect
#define USART0_UDRE_vect  USART_UDRE_vect
#endif  //  USART0_UDRE_vect

void uart_init_hw(void)
{
  PRR0 &= ~(1<<PRUSART0);
  UART_PORT |= (1<<UART_RX_PIN) | (1<<UART_TX_PIN);
  UART_DDR |= (1<<UART_TX_PIN);
  UART_DDR &= ~(1<<UART_RX_PIN);

  UBRR0H = (((F_CPU/16/38400) - 1)>>8);
  UBRR0L = (((F_CPU/16/38400) - 1) & 0xFF);
  
  UCSR0B = ((1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0));
  UCSR0C = (3<<UCSZ00);
}

ISR(USART0_RX_vect)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  uart_rx_handler(&xHigherPriorityTaskWoken);
}

ISR(USART0_UDRE_vect)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  uart_tx_handler(&xHigherPriorityTaskWoken);
}
