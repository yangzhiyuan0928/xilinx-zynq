`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    09:32:01 04/21/2016 
// Design Name: 
// Module Name:    square_dds 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.1 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module square1x2x_genrator
       #(parameter [31:0] CLK_FREQ = 50000000 )
      (output reg    square_1x,square_2x,
       input         out_en,clk_in,rst_n,
       input  [31:0] freq_square_1x,freq_square_2x,phase_square_1x,phase_square_2x);
       
 
  localparam  [31:0]  PHASE_ACC_MAX   = CLK_FREQ ; 
  localparam          IDLE            =1'b0;
  localparam          R0              =1'b1;

 
  wire   [31:0]   phase_acc_1x;
  reg    [31:0]   phase_acc_temp_1x;  
  wire   [31:0]   phase_acc_2x;
  reg    [31:0]   phase_acc_temp_2x;  
  
  reg             main_state ;
  reg             next_state = IDLE;
  
 assign phase_acc_1x     =  (phase_acc_temp_1x < PHASE_ACC_MAX) ? phase_acc_temp_1x : 
							phase_acc_temp_1x - PHASE_ACC_MAX; 
 assign phase_acc_2x     =  (phase_acc_temp_2x < PHASE_ACC_MAX) ? phase_acc_temp_2x : 
							 phase_acc_temp_2x - PHASE_ACC_MAX;
 
   always @(posedge clk_in)
      begin 
        case(main_state)
        IDLE:
          begin
          square_1x <= 1'b0;
          square_2x <= 1'b0;
          phase_acc_temp_1x <=  phase_square_1x;
          phase_acc_temp_2x <=  phase_square_2x; 
          end
        R0:  
          begin
          phase_acc_temp_1x <= phase_acc_1x + freq_square_1x;
          phase_acc_temp_2x <= phase_acc_2x + freq_square_2x;
          square_1x <= (phase_acc_1x < CLK_FREQ/2 ) ? 1 : 0;          
          square_2x <= (phase_acc_2x < CLK_FREQ/2 ) ? 1 : 0;
          end
         
        endcase
      end
  
   always @ (posedge clk_in, negedge rst_n)
      begin
      if (~rst_n )
         main_state <= IDLE;
      else   
         main_state <= next_state; 
      end              
 
 always @*
      begin 
        case(main_state)
        IDLE:
		  if (out_en == 1'b1  )
              next_state = R0;	
          else
              next_state = IDLE;    
        R0:  
          if (out_en == 1'b0 )
              next_state = IDLE;					  
	      else 
              next_state = R0;
       
       endcase
     end	      	

endmodule
