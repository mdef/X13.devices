void enc28j60_init_hw(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  SPI_InitTypeDef   SPI_InitStructure;
  
  // Enable Periphery Clocks
  ENC_ENABLE_CLOCK();

  // ENC_NSS_PIN
  GPIO_InitStructure.GPIO_Pin     = ENC_NSS_PIN;
  GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_Out_PP;
  GPIO_Init(ENC_NSS_PORT, &GPIO_InitStructure);
  GPIO_SetBits(ENC_NSS_PORT, ENC_NSS_PIN);
  
  // Configure SPI pins
  GPIO_InitStructure.GPIO_Pin     = SPIe_SCK_PIN | SPIe_MISO_PIN | SPIe_MOSI_PIN;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF_PP;
  GPIO_Init(SPIe_PORT, &GPIO_InitStructure);
  
  // SPIe Config
  SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
  SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial     = 7;
  SPI_Init(SPIe, &SPI_InitStructure);
  // SPIe enable
  SPI_Cmd(SPIe, ENABLE);
}

uint8_t enc28j60exchg(uint8_t data)
{
  // Loop while DR register in not emplty
  //while(SPI_I2S_GetFlagStatus(SPIe, SPI_I2S_FLAG_TXE) == RESET);
  // Send byte through the SPI1 peripheral
  SPI_I2S_SendData(SPIe, data);
  // Wait to receive a byte
  while(SPI_I2S_GetFlagStatus(SPIe, SPI_I2S_FLAG_RXNE) == RESET);
  // Return the byte read from the SPI bus
  return SPI_I2S_ReceiveData(SPIe);
}
