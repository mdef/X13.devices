// low level SPI exchange
uint8_t hal_cc11_spiExch(uint8_t data)
{
  RF_SPI_DATA = data;
  while(RF_SPI_BISY);                 // Wait until SPI operation is terminated
  return RF_SPI_DATA;
}

ISR(RF_INT_vect)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  CC11_IRQ_Handler(&xHigherPriorityTaskWoken);
}
