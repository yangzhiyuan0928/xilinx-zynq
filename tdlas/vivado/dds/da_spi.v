`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    15:29:21 03/30/2016 
// Design Name: 
// Module Name:    da_spi 
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
 module da_spi
       (input          da_start,clk_spi,rst_n,
        input  [15:0]  da_data,
        output reg     cs_n,
//        output reg   [1:0]  main_state,next_state,
//        output reg   [4:0]  transmit_count, 
        output         sclk,dout,da_clr);
					
 localparam  [1:0]     IDLE            = 2'b00;
 localparam  [1:0]     WR_START        = 2'b01;
 localparam  [1:0]     WR_DATA         = 2'b10;
 localparam  [1:0]     STOP            = 2'b11;
 
 reg   [1:0]  main_state;
 reg   [1:0]  next_state; 
 reg   [15:0] data_to_send;
 reg   [4:0]  transmit_count; 
 reg          sclk_en;
     
 
 assign  dout = ( (main_state == WR_DATA)&&(cs_n == 1'b0) ) ? data_to_send[15] : 1'b0;
 assign  sclk = (sclk_en) ? clk_spi : 1'b1;
 assign  da_clr = rst_n;

  always @ (posedge clk_spi, negedge rst_n)
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
             if (da_start == 1'd1)
			    next_state = WR_START;
             else 
                next_state = IDLE;
        WR_START:
             next_state = WR_DATA;                     	
        WR_DATA:  
			 if (transmit_count >= 5'd16)
                next_state = STOP;//all 16 bits are sent
             else 
                next_state = WR_DATA;			        			  		      
        STOP:	
			if(da_start == 1'd1)   
               next_state = WR_START;
			 else
  			   next_state = IDLE;				 
       endcase
     end
 

  always @(posedge clk_spi)
     begin 
         case(main_state)
         IDLE:
             begin

             sclk_en <= 1'b0;
             transmit_count <= 5'd0;
             data_to_send <= 15'd0;
             end
        WR_START:
			 begin 
             transmit_count <= transmit_count + 1'd1;	
             data_to_send <= da_data;     

             sclk_en <= 1'b1;
             end
        WR_DATA:  
             begin
             transmit_count <= transmit_count + 1'd1;	
             data_to_send<={data_to_send[14:0],1'b0};
             end			        		        			  		      
        STOP:	
            begin
            sclk_en <= 1'b0;
            transmit_count <= 5'd0;
            end

       endcase
     end

  always @(negedge clk_spi) 
     begin 
      case(main_state)
      IDLE:
          cs_n <= 1'b1;
      WR_START:      
          cs_n <= 1'b0;
      WR_DATA:          
          cs_n <= 1'b0;          
      STOP:	                
          cs_n <= 1'b1;	
      endcase
    end
endmodule
