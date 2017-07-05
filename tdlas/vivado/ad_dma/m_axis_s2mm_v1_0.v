`timescale 1 ns / 1 ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2016/06/20 11:03:04
// Design Name: 
// Module Name: ad7989_fifo
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module M_AXIS_S2MM_v1_0 # (
                            parameter integer C_M_AXIS_TDATA_WIDTH	= 32,
		                    parameter integer C_M_START_COUNT	= 32
	                      )
	                      (		
	                        // Users to add ports here
                            input wire [C_M_AXIS_TDATA_WIDTH-1 : 0] FIFO_DATA,
                            input wire  FIFO_ALMOST_EMPTY,
                            output wire FIFO_RD_EN,
                            // User port ends
                            // AXI4 Stream Port
		                    input wire  M_AXIS_ACLK,		
		                    input wire  M_AXIS_ARESETN,		
		                    output wire  M_AXIS_TVALID,		
		                    output wire [C_M_AXIS_TDATA_WIDTH-1 : 0] M_AXIS_TDATA,	
		                    output wire [(C_M_AXIS_TDATA_WIDTH/8)-1 : 0] M_AXIS_TSTRB,		
		                    output wire  M_AXIS_TLAST,		
		                    input wire  M_AXIS_TREADY
	                      );
    localparam NUMBER_OF_OUTPUT_WORDS = 1024;  //Package size                                              
                                                                                
	// function called clogb2 that returns an integer which has the                      
	// value of the ceiling of the log base 2.                                           
	function integer clogb2 (input integer bit_depth);                                   
	  begin                                                                              
	    for(clogb2=0; bit_depth>0; clogb2=clogb2+1)                                      
	      bit_depth = bit_depth >> 1;                                                    
	  end                                                                                
	endfunction                                                                          
                               
    localparam integer WAIT_COUNT_BITS = clogb2(C_M_START_COUNT-1);                
    localparam bit_num  = clogb2(NUMBER_OF_OUTPUT_WORDS);                                
                                                                                     
    localparam    [1:0]  IDLE          = 2'b00;                                                                                      
    localparam    [1:0]  INIT_COUNTER  = 2'b01;  
    localparam    [1:0]  SEND_STREAM   = 2'b10;                        
    // intern reg                                  
    reg [1:0] mst_exec_state;    
    reg [WAIT_COUNT_BITS-1 : 0]  count;                                                                                                  
    reg [10:0] read_pointer; 
    reg [C_M_AXIS_TDATA_WIDTH-1 : 0] read_count;                                                     
    reg  	tx_done;
    reg  	axis_tvalid_delay;
    reg  	axis_tlast_delay;
    
    // intern signal
    wire  	axis_tvalid;
    wire  	axis_tlast;
    wire  	tx_en;
    
    assign tx_en            = M_AXIS_TREADY && axis_tvalid ;   
    assign FIFO_RD_EN       = tx_en; 
    
    assign M_AXIS_TVALID    = axis_tvalid_delay;
    assign M_AXIS_TDATA     = FIFO_DATA;
    assign M_AXIS_TLAST     = axis_tlast_delay;
    assign M_AXIS_TSTRB     = {(C_M_AXIS_TDATA_WIDTH/8){1'b1}}; //F

    //========== mst_exec_state ===========                      
    always @(posedge M_AXIS_ACLK) begin                                                                                                                
      if (!M_AXIS_ARESETN) begin                                                                                                                                                         
        mst_exec_state <= IDLE;                                             
        count <= 0;                                                      
      end                                                                   
      else begin                                                                    
        case (mst_exec_state)                                                 
          IDLE: mst_exec_state  <= INIT_COUNTER;                                                                                                                                                                                                 
          INIT_COUNTER: begin                                                                        
            if(count == C_M_START_COUNT - 1)  mst_exec_state  <= SEND_STREAM;                                                                                     
            else begin                                                                                                                        
              count <= count + 1;                                           
              mst_exec_state  <= INIT_COUNTER;                              
            end
          end                                                                                                                                     
          SEND_STREAM: begin                                                        
            if (tx_done) begin                                                     
              mst_exec_state <= IDLE; 
              count <= 0;                                        
            end                                                             
            else  mst_exec_state <= SEND_STREAM;   
          end                                                                                                                
        endcase
      end                                                               
    end                                                                       

    assign axis_tvalid = ((mst_exec_state == SEND_STREAM) && (read_pointer < NUMBER_OF_OUTPUT_WORDS) && (!FIFO_ALMOST_EMPTY));                                               
    assign axis_tlast = (read_pointer == NUMBER_OF_OUTPUT_WORDS-1);                                
                                                                                                                                                                                                                                                
    always @(posedge M_AXIS_ACLK) begin                                                                                                                                                          
      if (!M_AXIS_ARESETN) begin                                                                                                                                           
        axis_tvalid_delay <= 1'b0;                                                               
        axis_tlast_delay <= 1'b0;                                                                
      end                                                                                        
      else begin                                                                                                                                                                               
        axis_tvalid_delay <= axis_tvalid;                                                        
        axis_tlast_delay <= axis_tlast;                                                          
      end                                                                                        
    end                                                                                            

    //========== read_pointer ==========
    always@(posedge M_AXIS_ACLK) begin                                                                                                                           
      if(!M_AXIS_ARESETN) begin                                                                                                                                    
        read_pointer <= 0;
        read_count <= 0;                                                         
        tx_done <= 1'b0;                                                           
      end                                                                          
      else begin                                                                          
        if (read_pointer <= NUMBER_OF_OUTPUT_WORDS-1) begin                                                                                                   
          if (tx_en) begin
            read_pointer <= read_pointer + 1;
          end                                                                                              
          tx_done <= 1'b0;	                                                                         
        end                                                                        
        else if (read_pointer == NUMBER_OF_OUTPUT_WORDS) begin                                                                                                 
          read_pointer <=0;  
          read_count <= read_count + 1;                                                
          tx_done <= 1'b1;                                                         
        end                                                                        
      end  
    end 
                                                                               
endmodule
