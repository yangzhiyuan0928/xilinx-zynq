`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2016/05/04 15:53:15
// Design Name: 
// Module Name: dds_spi
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


module dds_axi_spi (
       input            s_axi_aresetn,   //System reset
       input            s_axi_aclk,         //System clock = 50MHz
       // axi4 interface                                
       input            s_axi_awvalid,  //write addr
       input    [5:0]  s_axi_awaddr,
       output   reg     s_axi_awready, 
       input            s_axi_wvalid,   //write data
       input    [31:0]  s_axi_wdata,
//       input    [3:0]   s_axi_wstrb,  
       output   reg     s_axi_wready,
       input            s_axi_bready,  //write response
       output   reg     s_axi_bvalid,  
       output   reg [1:0]  s_axi_bresp,                  
       // spi interface
       output           cs_n,
       output           sclk,da_clr,dout,
       // square output
       output           square_1x,square_2x );
           

    
//========= dds control words =========    
    reg    [31:0] freq_sin;
    reg    [31:0] phase_sin_init;
    reg    [15:0] amp_sin;
    reg    [31:0] delay_saw;
    reg    [31:0] freq_saw;
    reg    [31:0] dc_data1;
    reg    [31:0] dc_data2;  
    reg    [15:0] amp_saw;
    reg    [31:0] freq_square_1x;
    reg    [31:0] freq_square_2x;
    reg    [31:0] phase_square_1x;
    reg    [31:0] phase_square_2x;
    reg           out_en;
    reg    [13:0] axi_waddr_reg;
   
    reg    [1:0]  main_state,next_state;
//======= axi-common interconnect ========
    localparam   C_BASEADDR = 32'hFFFF_FFFF;  
    localparam   C_HIGHADDR = 32'h0000_0000; 
    
    localparam    IDLE = 2'b00;
    localparam    R0   = 2'b01;   
    localparam    R1   = 2'b10;    
    localparam    R2   = 2'b11;    
    
      always@(posedge s_axi_aclk, negedge s_axi_aresetn)
         begin
           if(!s_axi_aresetn)
             begin 
             main_state <= IDLE;
             end              
           else 
             main_state <= next_state;
        end 
  
     always@*
      begin
        case (main_state)
        IDLE:
          begin
            if(s_axi_awvalid) next_state = R0;
            else  next_state = IDLE;
          end
        R0:
          begin
            if(s_axi_wvalid) next_state = R1;
            else next_state = R0; 
          end
        R1:       
          begin
            if(s_axi_bready) next_state = R2;
            else next_state = R1;             
          end
        R2:
          next_state = IDLE;
       endcase     
      end
 
     always@(posedge s_axi_aclk)
        begin
          case (main_state)
          IDLE:
            begin
            if(s_axi_awvalid)
              begin
               s_axi_awready <=1'b1;
               axi_waddr_reg <= s_axi_awaddr[5:2];              
              end 
            else s_axi_awready <=1'b0;
            s_axi_wready  <=1'b0;
            s_axi_bvalid  <=1'b0;
            s_axi_bresp  <= 2'b11;             
            end
          R0:
            begin
              s_axi_awready <=1'b0;
              if(s_axi_wvalid)
                begin
                  s_axi_wready <=1'b1;
                  case (axi_waddr_reg[3:0])
                  14'd1:  freq_square_1x <= s_axi_wdata;                                  
                  14'd2:  freq_square_2x <= s_axi_wdata;                                           
                  14'd3:  phase_square_1x <= s_axi_wdata;                                    
                  14'd4:  phase_square_2x <= s_axi_wdata;                                       
                   //========== sin_wave ========== 
                  14'd5:  freq_sin <= s_axi_wdata;                                       
                  14'd6:  phase_sin_init <= s_axi_wdata;
                  14'd7:  amp_sin <= s_axi_wdata;
                  14'd8:  freq_saw <= s_axi_wdata;
                  14'd9:  amp_saw <= s_axi_wdata;
                   //============== DC & Delay control ==============     
                  14'd10: dc_data1 <= s_axi_wdata;
                  14'd11: dc_data2 <= s_axi_wdata;
                  14'd12: delay_saw <= s_axi_wdata;
                   //============= Output enable =============                                                                                                                             
                  14'd13: out_en <= s_axi_wdata[0];
                  default:out_en <=1'b0;                                 
                  endcase  
                end 
            end
          R1:       
            begin
              s_axi_wready <=1'b0; 
              if(s_axi_bready)
                 begin
                   s_axi_bvalid <= 1'b1;
                   s_axi_bresp  <= 2'b00;  //OKAY              
                 end                           
            end
          R2:
            s_axi_bvalid <= 1'b0; 
         endcase                
       end
 
    
    dds_spi    u_dds_spi (
              .clk_dds(s_axi_aclk),
              .rst_n(s_axi_aresetn),
              .out_en(out_en),
              .freq_sin(freq_sin),
              .phase_sin_init(phase_sin_init),
              .delay_saw(delay_saw),
              .freq_saw(freq_saw),
              .dc_data1(dc_data1),
              .dc_data2(dc_data2),
              .freq_square_1x(freq_square_1x),
              .freq_square_2x(freq_square_2x),
              .phase_square_1x(phase_square_1x),
              .phase_square_2x(phase_square_2x), 
              .amp_sin(amp_sin),
              .amp_saw(amp_saw), 
              .cs_n(cs_n),
              .sclk(sclk),
              .dout(dout),
              .da_clr(da_clr),                                                                            
              .square_1x(square_1x),
              .square_2x(square_2x));
          
                                              
endmodule
