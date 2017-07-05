`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    20:39:32 05/02/2016 
// Design Name: 
// Module Name:    dds_saw 
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
 module dds_saw(
        input             clk_dds, rst_n,out_en,
        input      [31:0] delay_saw,freq_saw,
        input      [15:0] amp_saw,
        input      [31:0] dc_data1,dc_data2,
        output     [31:0] saw_out,
        output  reg      dc_sel);

  localparam  [31:0]    PHASE_ACC_MAX   = 32'd20000000; 
  localparam  [1:0]     IDLE            = 2'b01;
  localparam  [1:0]     R0              = 2'b10;
  localparam  [1:0]     R1              = 2'b11;
  localparam  [1:0]     DC_SEL_DELAY    = 2'd2;
  
  wire   [31:0]   phase_acc;
  reg    [31:0]   phase_acc_temp;  
  wire   [13:0]   saw_addr; 
  wire   [15:0]   saw_wave;
  wire   [31:0]   saw_wave_1;
  reg    [2:0]    main_state;
  reg    [2:0]    next_state = IDLE;
  reg    [31:0]   count_saw;
  reg    [1:0]    dc_sel_delay;
  
 assign  saw_addr = phase_acc[24:11];   
 assign  phase_acc =  (phase_acc_temp <= PHASE_ACC_MAX) ? phase_acc_temp: 
                       phase_acc_temp - PHASE_ACC_MAX; 
 assign saw_out = (dc_sel) ?  saw_wave_1+dc_data2: dc_data1;


    always @(posedge clk_dds)
      begin 
        case(main_state)  
            IDLE:
              begin
              phase_acc_temp <= 32'd0;                  					     
              count_saw <= 32'd0;
              dc_sel <=1'b0;
              dc_sel_delay <= 0;
              end
            R0:
              begin
              if(dc_sel_delay < DC_SEL_DELAY)
                  dc_sel_delay <= dc_sel_delay + 1'b1;
               else
                  begin
                  dc_sel_delay <= dc_sel_delay;
                  dc_sel <=1'b0;            
                  end
              if(count_saw < delay_saw)
                 count_saw <= count_saw + 1'b1;
              else
                 begin
                 phase_acc_temp <= 32'd0;
                 dc_sel_delay <= 0;
                 end
              end
            R1: 
              begin
              if(dc_sel_delay < DC_SEL_DELAY)
                  dc_sel_delay <= dc_sel_delay + 1'b1;
               else
                  begin
                  dc_sel_delay <= dc_sel_delay;
                  dc_sel <=1'b1;            
                  end
              if(phase_acc_temp >=PHASE_ACC_MAX)
                 begin
                 count_saw <= 32'd0;
                 dc_sel_delay <= 0;                
                 end 
              else
                  phase_acc_temp <= phase_acc+ freq_saw; 
              end
           default:
              begin
                count_saw <= 32'd0;
                phase_acc_temp <= 32'd0;
                dc_sel <=1'b0;
              end
        endcase
     end	 
 
  always @ (posedge clk_dds, negedge rst_n)
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
              if (out_en == 1'b1 )
                  next_state = R0;	
              else
                  next_state = IDLE;                    					     
            R0:
              if (out_en == 1'b0)
                  next_state = IDLE;
              else if(count_saw < delay_saw)
                     next_state = R0;
                   else
                     next_state = R1;
            R1: 
              if (out_en == 1'b0)
                  next_state = IDLE;
              else if(phase_acc_temp >=PHASE_ACC_MAX) 
                      next_state = R0;
                   else
                     next_state = R1;
            default:
                next_state = IDLE;
        endcase
     end	      	

  mult_32_signed saw_mult_32 (
//  .sclr(~rst_n),
//   .ce(out_en),
   .CLK(clk_dds),
   .A(saw_wave),
   .B(amp_saw),    
   .P(saw_wave_1)
  ); 

  blk_saw_9765  saw_wavegen  (    
    .clka(clk_dds),
    .addra(saw_addr),
    .douta(saw_wave)	
  ); 

endmodule
