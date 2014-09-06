void enc28j60_init_hw(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  SPI_InitTypeDef   SPI_InitStructure;

  // Enable Periphery Clocks
  ENC_ENABLE_CLOCK();
  
  // ENC_NSS_PIN
  GPIO_InitStructure.GPIO_Pin     = ENC_NSS_PIN;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
  GPIO_Init(ENC_NSS_PORT, &GPIO_InitStructure);
  GPIO_SetBits(ENC_NSS_PORT, ENC_NSS_PIN);

  // Configure SPI pins
  GPIO_InitStructure.GPIO_Pin     = SPIe_SCK_PIN | SPIe_MISO_PIN | SPIe_MOSI_PIN;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF;
  GPIO_Init(SPIe_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(SPIe_PORT, SPIe_SCK_PINSRC,  SPIe_PINS_AF);
  GPIO_PinAFConfig(SPIe_PORT, SPIe_MISO_PINSRC, SPIe_PINS_AF);
  GPIO_PinAFConfig(SPIe_PORT, SPIe_MOSI_PINSRC, SPIe_PINS_AF);
  
  // SPIe Config
  SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;  // 12M
  SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial     = 7;
  SPI_Init(SPIe, &SPI_InitStructure);

  SPI_RxFIFOThresholdConfig(SPIe, SPI_RxFIFOThreshold_QF);
  // SPIm enable
  SPI_Cmd(SPIe, ENABLE);
}

uint8_t enc28j60exchg(uint8_t data)
{
  SPI_SendData8(SPIe,data);
  while((SPIe->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
  return SPI_ReceiveData8(SPIe);
}
