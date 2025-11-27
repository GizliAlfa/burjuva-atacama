// ============================================================================
// Pilot Hardware CPLD Top Module
// Connects Raspberry Pi SPI to STM32 and routes module connectors
// ============================================================================

module top(
	RPI_SPI_CLK,
	RPI_SCL,
	RPI_SPI_MOSI,
	RPI_SPI_MISO,
	RPI_SDA,

	PA_0,
	PA_1,
	PA_2,
	PA_3,
	PA_4,
	PA_5,
	PA_6,
	PA_7,
	PA_8,
	PA_11,
	PA_12,
	
	PB_0,
	PB_1,
	PB_5,
	PB_6,
	PB_7,
	PB_8,
	PB_9,
	PB_10,
	PB_11,
	PB_12,
	PB_13,
	PB_14,
	PB_15,

	PC_4,
	PC_5,
	PC_6,
	PC_7,
	PC_8,
	PC_9,
	PC_10,
	PC_11,
	PC_12,
	PC_13,

	PD_2,

	CON0_IO0,
	CON0_IO1,
	CON0_IO2,
	CON0_IO3,
	CON0_IO4,
	CON0_IO5,
	CON0_IO6,
	CON0_IO7,
	CON0_IO8,
	CON1_IO0,
	CON1_IO1,
	CON1_IO2,
	CON1_IO3,
	CON1_IO4,
	CON1_IO5,
	CON1_IO6,
	CON1_IO7,
	CON1_IO8,
	CON2_IO0,
	CON2_IO1,
	CON2_IO2,
	CON2_IO3,
	CON2_IO4,
	CON2_IO5,
	CON2_IO6,
	CON2_IO7,
	CON2_IO8,
	CON3_IO0,
	CON3_IO1,
	CON3_IO2,
	CON3_IO3,
	CON3_IO4,
	CON3_IO5,
	CON3_IO6,
	CON3_IO7,
	CON3_IO8
);

// Raspberry Pi SPI Interface
(* chip_pin="12" *)input RPI_SPI_CLK; 
(* chip_pin="14" *)input RPI_SCL; 
(* chip_pin="27" *)input RPI_SPI_MOSI; 
(* chip_pin="28" *)output RPI_SPI_MISO; 
(* chip_pin="52" *)input RPI_SDA; 

// Module Connectors (CON0-3, 4x 9-pin connectors)
(* chip_pin="76" *)input CON0_IO0;
(* chip_pin="75" *)input CON0_IO1;
(* chip_pin="74" *)input CON0_IO2;
(* chip_pin="73" *)input CON0_IO3;
(* chip_pin="72" *)input CON0_IO4;
(* chip_pin="71" *)input CON0_IO5;
(* chip_pin="70" *)input CON0_IO6;
(* chip_pin="69" *)input CON0_IO7;
(* chip_pin="68" *)input CON0_IO8;

(* chip_pin="21" *)input CON1_IO0; 
(* chip_pin="20" *)input CON1_IO1; 
(* chip_pin="19" *)input CON1_IO2; 
(* chip_pin="18" *)input CON1_IO3; 
(* chip_pin="17" *)input CON1_IO4; 
(* chip_pin="16" *)input CON1_IO5; 
(* chip_pin="15" *)input CON1_IO6; 
(* chip_pin="8" *) input CON1_IO7;
(* chip_pin="7" *) input CON1_IO8;

(* chip_pin="29" *)output CON2_IO0;
(* chip_pin="30" *)output CON2_IO1;
(* chip_pin="33" *)output CON2_IO2;
(* chip_pin="34" *)output CON2_IO3;
(* chip_pin="35" *)output CON2_IO4;
(* chip_pin="36" *)output CON2_IO5;
(* chip_pin="37" *)output CON2_IO6;
(* chip_pin="38" *)output CON2_IO7;
(* chip_pin="39" *)output CON2_IO8;

(* chip_pin="40" *)output CON3_IO0; 
(* chip_pin="41" *)output CON3_IO1; 
(* chip_pin="42" *)output CON3_IO2; 
(* chip_pin="43" *)output CON3_IO3; 
(* chip_pin="44" *)output CON3_IO4; 
(* chip_pin="47" *)output CON3_IO5; 
(* chip_pin="48" *)output CON3_IO6; 
(* chip_pin="49" *)output CON3_IO7; 
(* chip_pin="50" *)output CON3_IO8; 

// STM32 GPIO Ports (PA, PB, PC, PD)
// PA = 11 pins (8 GPIO + 3 SPI)
(* chip_pin="77" *)input PA_0;
(* chip_pin="78" *)input PA_1;
(* chip_pin="66" *)input PA_2;
(* chip_pin="61" *)input PA_3;
(* chip_pin="58" *)input PA_4;
(* chip_pin="62" *)output PA_5; // SPI_CLK to STM32
(* chip_pin="57" *)input  PA_6; // SPI_MISO from STM32
(* chip_pin="56" *)output PA_7; // SPI_MOSI to STM32
(* chip_pin="91" *)input PA_8;
(* chip_pin="90" *)input PA_11;
(* chip_pin="89" *)input PA_12;

// PB = 13 pins
(* chip_pin="6" *)  input PB_0;
(* chip_pin="5" *)  output PB_1;
(* chip_pin="84" *) output PB_5;
(* chip_pin="64" *) output PB_6;
(* chip_pin="83" *) output PB_7;
(* chip_pin="82" *) output PB_8;
(* chip_pin="81" *) output PB_9;
(* chip_pin="4" *)  output PB_10;
(* chip_pin="3" *)  input PB_11;
(* chip_pin="2" *)  input PB_12;
(* chip_pin="100" *)input PB_13;
(* chip_pin="99" *) input PB_14;
(* chip_pin="98" *) input PB_15;
 
// PC = 10 pins
(* chip_pin="55" *)input PC_4;
(* chip_pin="54" *)input PC_5;
(* chip_pin="97" *)input PC_6;
(* chip_pin="96" *)input PC_7;
(* chip_pin="95" *)output PC_8;
(* chip_pin="92" *)output PC_9;
(* chip_pin="85" *)output PC_10;
(* chip_pin="86" *)output PC_11;
(* chip_pin="87" *)output PC_12;
(* chip_pin="67" *)output PC_13;

// PD = 1 pin
(* chip_pin="88" *)output PD_2;

	// ========================================
	// Raspberry Pi SPI Bridge
	// ========================================
	rpi rpi_0 (
		.SPI_CLK_IN(RPI_SPI_CLK),
		.SPI_CLK_OUT(PA_5),
		.SPI_MOSI_IN(RPI_SPI_MOSI),
		.SPI_MOSI_OUT(PA_7),
		.SPI_MISO_IN(PA_6),
		.SPI_MISO_OUT(RPI_SPI_MISO),
		.RPI_SDA(RPI_SDA),
		.RPI_SCL(RPI_SCL)
	); 

	// ========================================
	// Module Routing (STM32 <-> Connectors)
	// Total: 8 + 13 + 10 + 1 = 32 GPIO
	// ========================================

	// STM32 Port A -> CON2 (Output Module Slot 1)
	testout test_1_out(
		.IO0_IN (PA_0),
		.IO0_OUT(CON2_IO0),
		.IO1_IN (PA_1),
		.IO1_OUT(CON2_IO1),
		.IO2_IN (PA_2),
		.IO2_OUT(CON2_IO2),
		.IO3_IN (PA_3),
		.IO3_OUT(CON2_IO3),
		.IO4_IN (PA_4),
		.IO4_OUT(CON2_IO4),
		.IO5_IN (PA_8),
		.IO5_OUT(CON2_IO5),
		.IO6_IN (PA_11),
		.IO6_OUT(CON2_IO6),
		.IO7_IN (PA_12),
		.IO7_OUT(CON2_IO7),
		.IO8_IN (PB_0),
		.IO8_OUT(CON2_IO8)
	);

	// CON0 -> STM32 Port B (Input Module Slot 2)
	testin test_2_in(
		.IO0_IN (CON0_IO0),
		.IO0_OUT(PB_1),
		.IO1_IN (CON0_IO1),
		.IO1_OUT(PB_5),
		.IO2_IN (CON0_IO2),
		.IO2_OUT(PB_6),
		.IO3_IN (CON0_IO3),
		.IO3_OUT(PB_7),
		.IO4_IN (CON0_IO4),
		.IO4_OUT(PB_8),
		.IO5_IN (CON0_IO5),
		.IO5_OUT(PB_9),
		.IO6_IN (CON0_IO6),
		.IO7_IN (CON0_IO7),
		.IO8_IN (CON0_IO8),
		.IO678_OUT(PB_10)
	);

	// STM32 Port B/C -> CON3 (Output Module Slot 3)
	testout test_3_out(
		.IO0_IN (PB_11),
		.IO0_OUT(CON3_IO0),
		.IO1_IN (PB_12),
		.IO1_OUT(CON3_IO1),
		.IO2_IN (PB_13),
		.IO2_OUT(CON3_IO2),
		.IO3_IN (PB_14),
		.IO3_OUT(CON3_IO3),
		.IO4_IN (PB_15),
		.IO4_OUT(CON3_IO4),
		.IO5_IN (PC_4),
		.IO5_OUT(CON3_IO5),
		.IO6_IN (PC_5),
		.IO6_OUT(CON3_IO6),
		.IO7_IN (PC_6),
		.IO7_OUT(CON3_IO7),
		.IO8_IN (PC_7),
		.IO8_OUT(CON3_IO8)
	);

	// CON1 -> STM32 Port C/D (Input Module Slot 4)
	testin test_4_in(
		.IO0_IN (CON1_IO0),
		.IO0_OUT(PC_8),
		.IO1_IN (CON1_IO1),
		.IO1_OUT(PC_9),
		.IO2_IN (CON1_IO2),
		.IO2_OUT(PC_10),
		.IO3_IN (CON1_IO3),
		.IO3_OUT(PC_11),
		.IO4_IN (CON1_IO4),
		.IO4_OUT(PC_12),
		.IO5_IN (CON1_IO5),
		.IO5_OUT(PC_13),
		.IO6_IN (CON1_IO6),
		.IO7_IN (CON1_IO7),
		.IO8_IN (CON1_IO8),
		.IO678_OUT(PD_2)
	);

endmodule
