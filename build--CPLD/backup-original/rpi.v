// ============================================================================
// RPI SPI Bridge Module
// Raspberry Pi SPI <-> STM32 SPI Bridge
// ============================================================================

module rpi(
	input  SPI_CLK_IN,
	output SPI_CLK_OUT,
	input  SPI_MOSI_IN,
	output SPI_MOSI_OUT,
	input  SPI_MISO_IN,
	output SPI_MISO_OUT,
	input  RPI_SDA,
	input  RPI_SCL
);

// Simple passthrough for SPI signals
// RPI -> STM32
assign SPI_CLK_OUT = SPI_CLK_IN;
assign SPI_MOSI_OUT = SPI_MOSI_IN;

// STM32 -> RPI
assign SPI_MISO_OUT = SPI_MISO_IN;

// I2C signals are not used in this simple bridge
// They could be used for module EEPROM access in future

endmodule
