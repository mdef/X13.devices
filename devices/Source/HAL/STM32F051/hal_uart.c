void uart_init_hw(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef UART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  // Init Port
  UART_ENABLE_CLOCK();
  GPIO_PinAFConfig(GPIOA, UART_GPIO_PinSourceRX, GPIO_AF_1);
  GPIO_PinAFConfig(GPIOA, UART_GPIO_PinSourceTX, GPIO_AF_1);
  GPIO_InitStructure.GPIO_Pin = UART_GPIO_PinRX | UART_GPIO_PinTX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

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
  NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void uart_enable(void)
{
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

