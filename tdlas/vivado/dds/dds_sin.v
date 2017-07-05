`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    20:39:32 05/01/2016 
// Design Name: 
// Module Name:    dds_sin 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.1
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
 module dds_sin
        (input	            clk_dds,rst_n,out_en,
         input       [31:0] freq_sin,phase_sin_init,
         input       [15:0] amp_sin,
         output      [31:0] sin_out);
 

 
  wire   [31:0]   phase_acc;
  reg    [31:0]   phase_acc_temp; 
  wire   [13:0]   sin_addr;
  wire   [15:0]   sin_wave;
  reg             main_state,next_state;

  localparam  [31:0]      PHASE_ACC_MAX= 32'd20000000;  
  localparam              IDLE=1'b0;
  localparam              R0  =1'b1;
   
 assign  sin_addr  = phase_acc[24:11];	
 assign  phase_acc = (phase_acc_temp < PHASE_ACC_MAX) ? phase_acc_temp:
                      phase_acc_temp - PHASE_ACC_MAX; 

//data path  
  always @ (posedge clk_dds)
    begin 
      case(main_state)  
          IDLE:
              phase_acc_temp <= phase_sin_init ;                                        					  				  
            R0:            
              phase_acc_temp <= phase_acc + freq_sin;  
      endcase
    end	      	


//FSM
  always @ (posedge clk_dds, negedge rst_n)
    begin
      if (~rst_n )
         begin
         main_state <= IDLE;
         end   
      else   
         main_state <= next_state; 
    end              
 
    
 always @*
   begin 
     case(main_state)  
          IDLE:
             if (out_en == 1'b1 )
                 next_state = R0;	
             else
                 next_state = IDLE;	
                                        					  				  
          R0:            
             if (out_en == 1'b0)
                 next_state = IDLE; 
             else
                 next_state = R0;
        endcase
    end	      	
	
 mult_32_signed a_mult_32 (
    .CLK(clk_dds),    // input wire CLK
    .A(sin_wave),        // input wire [15 : 0] A
    .B(amp_sin),        // input wire [15 : 0] B
 //   .CE(out_en),      // input wire CE
//    .SCLR(~rst_n),  // input wire SCLR
    .P(sin_out)        // output wire [31 : 0] P
  );
  
  blk_sin_9765   u_sin (                  
   .clka(clk_dds),
   .addra(sin_addr),
   .douta(sin_wave)
 ); 

 endmodule
