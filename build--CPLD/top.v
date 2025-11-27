module top(
	PB_11,
	PB_10,
	PB_1,
	PB_0,
	RPI_SPI_CLK,
	RPI_SCL,
	CON1_IO4,
	CON1_IO3,
	CON1_IO2,
	CON1_IO1,
	CON1_IO0,
	RPI_SPI_MOSI,
	RPI_SPI_MISO,
	CON2_IO0,
	CON2_IO1,
	CON2_IO2,
	CON2_IO3,
	CON2_IO4,
	CON3_IO0,
	CON3_IO1,
	CON3_IO2,
	CON3_IO3,
	CON3_IO4,
	CON3_IO5,
	CON3_IO6,
	RPI_SPI_CE0,
	RPI_SDA,
	PC_5,
	PC_4,
	PA_7,
	PA_6,
	PA_4,
	PA_3,
	PA_5,
	PA_2,
	PC_13,
	CON0_IO5,
	CON0_IO4,
	CON0_IO3,
	CON0_IO2,
	CON0_IO1,
	CON0_IO0,
	PA_0,
	PA_1,
	PB_15,
	PB_14,
	PB_13
);

(* chip_pin="3" *)output PB_11; 
(* chip_pin="4" *)output PB_10; 
(* chip_pin="5" *)input PB_1; 
(* chip_pin="6" *)output PB_0; 
(* chip_pin="12" *)input RPI_SPI_CLK; 
(* chip_pin="14" *)input RPI_SCL; 
(* chip_pin="17" *)output CON1_IO4; 
(* chip_pin="18" *)output CON1_IO3; 
(* chip_pin="19" *)output CON1_IO2; 
(* chip_pin="20" *)input CON1_IO1; 
(* chip_pin="21" *)input CON1_IO0; 
(* chip_pin="27" *)input RPI_SPI_MOSI; 
(* chip_pin="28" *)output RPI_SPI_MISO; 
(* chip_pin="29" *)input CON2_IO0; 
(* chip_pin="30" *)input CON2_IO1; 
(* chip_pin="33" *)output CON2_IO2; 
(* chip_pin="34" *)output CON2_IO3; 
(* chip_pin="35" *)output CON2_IO4; 
(* chip_pin="40" *)input CON3_IO0; 
(* chip_pin="41" *)input CON3_IO1; 
(* chip_pin="42" *)output CON3_IO2; 
(* chip_pin="43" *)output CON3_IO3; 
(* chip_pin="44" *)output CON3_IO4; 
(* chip_pin="47" *)output CON3_IO5; 
(* chip_pin="48" *)input CON3_IO6; 
(* chip_pin="51" *)input RPI_SPI_CE0; 
(* chip_pin="52" *)input RPI_SDA; 
(* chip_pin="54" *)input PC_5; 
(* chip_pin="55" *)output PC_4; 
(* chip_pin="56" *)output PA_7; 
(* chip_pin="57" *)input PA_6; 
(* chip_pin="58" *)output PA_4; 
(* chip_pin="61" *)output PA_3; 
(* chip_pin="62" *)output PA_5; 
(* chip_pin="66" *)input PA_2; 
(* chip_pin="67" *)input PC_13; 
(* chip_pin="71" *)output CON0_IO5; 
(* chip_pin="72" *)output CON0_IO4; 
(* chip_pin="73" *)output CON0_IO3; 
(* chip_pin="74" *)output CON0_IO2; 
(* chip_pin="75" *)input CON0_IO1; 
(* chip_pin="76" *)input CON0_IO0; 
(* chip_pin="77" *)input PA_0; 
(* chip_pin="78" *)input PA_1; 
(* chip_pin="98" *)input PB_15; 
(* chip_pin="99" *)output PB_14; 
(* chip_pin="100" *)input PB_13; 

wire IO16_SPI_MISO_OUT_0;
wire AIO20_SPI_MISO_OUT_1;
wire FPGA_SPI_MISO_OUT_2;
wire IO16_SPI_MISO_OUT_3;
assign PB_14 = 
 !PC_13 ? IO16_SPI_MISO_OUT_0 : 
 !PA_0 ? AIO20_SPI_MISO_OUT_1 : 
 !PA_1 ? FPGA_SPI_MISO_OUT_2 : 
 !PA_2 ? IO16_SPI_MISO_OUT_3 : 
  0;


	rpi rpi_0 (
		.SPI_CLK_IN(RPI_SPI_CLK),
		.SPI_CLK_OUT(PA_5),
		.SPI_MOSI_IN(RPI_SPI_MOSI),
		.SPI_MOSI_OUT(PA_7),
		.SPI_MISO_IN(PA_6),
		.SPI_MISO_OUT(RPI_SPI_MISO),
		.SPI_NSS_IN(RPI_SPI_CE0),
		.SPI_NSS_OUT(PA_4),
		.RPI_SDA(RPI_SDA),
		.RPI_SCL(RPI_SCL)
	); 

	io16 io16_0 (
		.IO16_SPI_MOSI_OUT(CON2_IO3),
		.IO16_SPI_MOSI_IN(PB_15),
		.IO16_SPI_MISO_OUT(IO16_SPI_MISO_OUT_0),
		.IO16_SPI_MISO_IN(CON2_IO0),
		.IO16_SPI_CLK_OUT(CON2_IO2),
		.IO16_SPI_CLK_IN(PB_13),
		.IO16_SPI_NSS_OUT(CON2_IO4),
		.IO16_SPI_NSS_IN(PC_13),
		.IO16_SPI_INT_IN(CON2_IO1),
		.IO16_SPI_INT_OUT(PA_3)
	); 

	aio20 aio20_1 (
		.AIO20_SPI_MOSI_OUT(CON0_IO3),
		.AIO20_SPI_MOSI_IN(PB_15),
		.AIO20_SPI_MISO_OUT(AIO20_SPI_MISO_OUT_1),
		.AIO20_SPI_MISO_IN(CON0_IO0),
		.AIO20_SPI_CLK_OUT(CON0_IO2),
		.AIO20_SPI_CLK_IN(PB_13),
		.AIO20_SPI_NSS_OUT(CON0_IO4),
		.AIO20_SPI_NSS_IN(PA_0),
		.AIO20_SPI_INT_IN(CON0_IO1),
		.AIO20_SPI_INT_OUT(PC_4),
		.AIO20_CNVT_IN(PC_5),
		.AIO20_CNVT_OUT(CON0_IO5)
	); 

	fpga fpga_2 (
		.FPGA_SPI_MOSI_OUT(CON3_IO3),
		.FPGA_SPI_MOSI_IN(PB_15),
		.FPGA_SPI_MISO_OUT(FPGA_SPI_MISO_OUT_2),
		.FPGA_SPI_MISO_IN(CON3_IO0),
		.FPGA_SPI_CLK_OUT(CON3_IO2),
		.FPGA_SPI_CLK_IN(PB_13),
		.FPGA_SPI_NSS_OUT(CON3_IO4),
		.FPGA_SPI_NSS_IN(PA_1),
		.FPGA_SPI_INT_IN(CON3_IO1),
		.FPGA_SPI_INT_OUT(PB_0),
		.FPGA_CRESET_IN(PB_1),
		.FPGA_CRESET_OUT(CON3_IO5),
		.FPGA_CDONE_IN(CON3_IO6),
		.FPGA_CDONE_OUT(PB_10)
	); 

	io16 io16_3 (
		.IO16_SPI_MOSI_OUT(CON1_IO3),
		.IO16_SPI_MOSI_IN(PB_15),
		.IO16_SPI_MISO_OUT(IO16_SPI_MISO_OUT_3),
		.IO16_SPI_MISO_IN(CON1_IO0),
		.IO16_SPI_CLK_OUT(CON1_IO2),
		.IO16_SPI_CLK_IN(PB_13),
		.IO16_SPI_NSS_OUT(CON1_IO4),
		.IO16_SPI_NSS_IN(PA_2),
		.IO16_SPI_INT_IN(CON1_IO1),
		.IO16_SPI_INT_OUT(PB_11)
	); 


endmodule
