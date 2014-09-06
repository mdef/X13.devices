// EEPROM Emulation with FRAM

void eeprom_init_hw(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  SPI_InitTypeDef   SPI_InitStructure;

  // Enable Periphery Clocks
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
  
  // M_NSS
  GPIO_InitStructure.GPIO_Pin     = M_NSS_PIN;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType   = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd    = GPIO_PuPd_NOPULL;
  GPIO_Init(M_NSS_PORT, &GPIO_InitStructure);
  GPIO_SetBits(M_NSS_PORT, M_NSS_PIN);

  // Configure SPI pins
  GPIO_InitStructure.GPIO_Pin     = SPIm_SCK_PIN | SPIm_MISO_PIN | SPIm_MOSI_PIN;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF;
  GPIO_Init(SPIm_PORT, &GPIO_InitStructure);
  GPIO_PinAFConfig(SPIm_PORT, SPIm_SCK_PINSRC,  SPIm_PINS_AF);
  GPIO_PinAFConfig(SPIm_PORT, SPIm_MISO_PINSRC, SPIm_PINS_AF);
  GPIO_PinAFConfig(SPIm_PORT, SPIm_MOSI_PINSRC, SPIm_PINS_AF);
  
  // SPIm Config
  SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
  SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial     = 7;
  SPI_Init(SPIm, &SPI_InitStructure);

  SPI_RxFIFOThresholdConfig(SPIm, SPI_RxFIFOThreshold_QF);
  // SPIm enable
  SPI_Cmd(SPIm, ENABLE);
}

//Op-code Commands 
#define FRAM_CMD_WRSR                   0x01    // Write Status Register
#define FRAM_CMD_WRITE                  0x02    // Write Memory Data
#define FRAM_CMD_READ                   0x03    // Read Memory Data
#define FRAM_CMD_WRDI                   0x04    // Write Disable
#define FRAM_CMD_RDSR                   0x05    // Read Status Register
#define FRAM_CMD_WREN                   0x06    // Set Write Enable Latch

void eeprom_read(uint8_t *pBuf, uint32_t Addr, uint32_t Len)
{
  GPIO_ResetBits(M_NSS_PORT, M_NSS_PIN);        // Select Chip
  SPI_SendData8(SPIm, FRAM_CMD_READ);
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready
  SPI_SendData8(SPIm, (Addr >> 8) & 0xFF);                    // Send High Address
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready
  SPI_SendData8(SPIm, Addr & 0xFF);                           // Send Low Address
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready

  while((SPIm->SR & SPI_I2S_FLAG_RXNE)  != (uint16_t)RESET)   // flush FIFO
    *pBuf = SPI_ReceiveData8(SPIm);

  while(Len)
  {
    Len--;
    SPI_SendData8(SPIm, 0xFF);
    while((SPIm->SR & SPI_I2S_FLAG_RXNE) == (uint16_t)RESET);
    *pBuf = SPI_ReceiveData8(SPIm);
    pBuf++;
  }
  while((SPIm->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET);
  GPIO_SetBits(M_NSS_PORT, M_NSS_PIN);
}

void eeprom_write(uint8_t *pBuf, uint32_t Addr, uint32_t Len)
{
  // Write Enable Latch
  GPIO_ResetBits(M_NSS_PORT, M_NSS_PIN);
  SPI_SendData8(SPIm, FRAM_CMD_WREN);
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready
  while((SPIm->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET);
  GPIO_SetBits(M_NSS_PORT, M_NSS_PIN);                        // Release Chip
  // Write Header
  GPIO_ResetBits(M_NSS_PORT, M_NSS_PIN);                    
  SPI_SendData8(SPIm, FRAM_CMD_WRITE);                        // Send Command - Write
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready
  SPI_SendData8(SPIm, (Addr >> 8) & 0xFF);                    // Send High Address
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready
  SPI_SendData8(SPIm, Addr & 0xFF);                           // Send Low Address
  while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);    // Wait until SPI is ready
  // Send Data
  while(Len)
  {
    SPI_SendData8(SPIm, *pBuf);
    pBuf++;
    Len--;
    while((SPIm->SR & SPI_I2S_FLAG_TXE) == (uint16_t)RESET);  // Wait until SPI is ready
  }
  while((SPIm->SR & SPI_I2S_FLAG_BSY) != (uint16_t)RESET);
  GPIO_SetBits(M_NSS_PORT, M_NSS_PIN);
}
