void uart_init_hw(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef UART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  // Init Port
  UART_ENABLE_CLOCK();
  // Configure USART Rx Pin as input floating
  GPIO_InitStructure.GPIO_Pin = UART_GPIO_PinRX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(UART_GPIO_PORT, &GPIO_InitStructure );
  // Configure USART Tx Pin as alternate function push-pull
  GPIO_InitStructure.GPIO_Pin = UART_GPIO_PinTX;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(UART_GPIO_PORT, &GPIO_InitStructure );

  // Configure UART
  UART_InitStructure.USART_BaudRate = UART_BaudRate;
  UART_InitStructure.USART_WordLength = USART_WordLength_8b;
  UART_InitStructure.USART_StopBits = USART_StopBits_1;
  UART_InitStructure.USART_Parity = USART_Parity_No;
  UART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  UART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(UARTx, &UART_InitStructure);

  // Configure NVIC
  NVIC_InitStructure.NVIC_IRQChannel = UARTx_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  // UART Enable
  USART_Cmd(UARTx, ENABLE);
  // Enable UART IRQ
  USART_ITConfig(UARTx, USART_IT_RXNE, ENABLE);
}

void UARTx_IRQHandler(void)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // Received data is ready to be read 
  if(USART_GetITStatus(UARTx, USART_IT_RXNE) != RESET)
  {
    uart_rx_handler(&xHigherPriorityTaskWoken);
  }

   // Transmit data register empty
  if(USART_GetITStatus(UARTx, USART_IT_TXE) != RESET)
  {
    uart_tx_handler(&xHigherPriorityTaskWoken);
  }

  portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
