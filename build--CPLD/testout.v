// Output module - passes signals from STM32 to connector
module testout(
  IO0_IN,
  IO0_OUT,
  IO1_IN,
  IO1_OUT,
  IO2_IN,
  IO2_OUT,
  IO3_IN,
  IO3_OUT,
  IO4_IN,
  IO4_OUT,
  IO5_IN,
  IO5_OUT,
  IO6_IN,
  IO6_OUT,
  IO7_IN,
  IO7_OUT,
  IO8_IN,
  IO8_OUT
);

input  IO0_IN;
output IO0_OUT;
input  IO1_IN;
output IO1_OUT;
input  IO2_IN;
output IO2_OUT;
input  IO3_IN;
output IO3_OUT;
input  IO4_IN;
output IO4_OUT;
input  IO5_IN;
output IO5_OUT;
input  IO6_IN;
output IO6_OUT;
input  IO7_IN;
output IO7_OUT;
input  IO8_IN;
output IO8_OUT;

assign IO0_OUT = IO0_IN;
assign IO1_OUT = IO1_IN;
assign IO2_OUT = IO2_IN;
assign IO3_OUT = IO3_IN;
assign IO4_OUT = IO4_IN;
assign IO5_OUT = IO5_IN;
assign IO6_OUT = IO6_IN;
assign IO7_OUT = IO7_IN;
assign IO8_OUT = IO8_IN;

endmodule
