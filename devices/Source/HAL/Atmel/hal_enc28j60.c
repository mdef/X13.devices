void enc28j60_init_hw(void)
{
  LAN_PORT |= (1<<LAN_PIN_SS);
  LAN_DDR  &= ~(1<<LAN_PIN_MISO);
  LAN_DDR  |= (1<<LAN_PIN_SS) | (1<<LAN_PIN_MOSI) | (1<<LAN_PIN_SCK);
  
  SPCR = (1<<SPE) | (1<<MSTR);
  SPSR |= (1<<SPI2X);
}


uint8_t enc28j60exchg(uint8_t data)
{
  SPDR = data;
  while(!(SPSR &(1<<SPIF)));
  return SPDR;
}
