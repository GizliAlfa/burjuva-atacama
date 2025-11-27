module rpi(
	SPI_CLK_IN,
	SPI_CLK_OUT,
	SPI_MOSI_IN,
	SPI_MOSI_OUT,
	SPI_MISO_IN,
	SPI_MISO_OUT,
	SPI_NSS_IN,
	SPI_NSS_OUT,
	RPI_SDA,
	RPI_SCL
);

input SPI_CLK_IN; 
output SPI_CLK_OUT; 
input SPI_MOSI_IN; 
output SPI_MOSI_OUT; 
input SPI_MISO_IN; 
output SPI_MISO_OUT; 
input SPI_NSS_IN; 
output SPI_NSS_OUT; 
input RPI_SDA; 
input RPI_SCL; 



assign SPI_CLK_OUT = SPI_CLK_IN;
assign SPI_MOSI_OUT = SPI_MOSI_IN;
assign SPI_MISO_OUT = SPI_MISO_IN;
assign SPI_NSS_OUT = SPI_NSS_IN;


endmodule
