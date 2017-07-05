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


module dds_spi
      (input           clk_dds,rst_n,out_en,
       input    [31:0] freq_sin,phase_sin_init,
       input    [31:0] delay_saw,freq_saw,dc_data1,dc_data2,
       input    [31:0] freq_square_1x,freq_square_2x,phase_square_1x,phase_square_2x, 
       input    [15:0] amp_sin,amp_saw,
//       output   [31:0] sin_out,saw_out,
 //      output   [15:0] signal_out,
       output          square_1x,square_2x,
       output          cs_n,
       output          sclk,dout,da_clr );
                            
    localparam    IDLE = 2'b01;
    localparam    R0   = 2'b10;   //da_start = 1'b1
    localparam    R1   = 2'b11;   //da_start = 1'b0                     
      
    wire           dc_sel; 
    wire    [31:0] signal_add;
    wire           square_out_en;    
 
    reg          da_start = 1'b0;
    reg    [1:0] da_main_state,da_next_state;
    reg    [4:0] count_clk = 5'd0;
    reg    [2:0] out_en_delay = 3'd0;
    
    wire   [31:0] sin_out,saw_out;
    wire   [15:0] signal_out;
   
   //sin only output during saw with dc2 
    assign signal_add = (dc_sel) ? sin_out + saw_out : saw_out;  //saw_out最高位要取反，加上sin_out后，在将结果的最高位取反，完成补码与编码得转换 
    assign signal_out = signal_add[31:16];   
    assign square_out_en = out_en_delay[2]; 
    
    always@(posedge clk_dds)
        out_en_delay <= {out_en_delay[1:0],out_en};
 
 // for da_start generation,18 clks high, following 2 clks low 
    always@(posedge clk_dds, negedge rst_n) begin
      if(!rst_n) 
        da_main_state <= IDLE;
      else 
        da_main_state <= da_next_state;
    end
    
    always@*
      begin
        case(da_main_state)
          IDLE:  begin
            if(out_en == 1'b1) da_next_state = R0;
            else da_next_state = IDLE;  
          end
          R0:  begin
            if(out_en == 1'b0) da_next_state = IDLE;
            else if(count_clk < 5'd18) da_next_state = R0;
                 else  da_next_state = R1;
          end
          R1:  begin
            if(out_en == 1'b0) da_next_state = IDLE;
            else if(count_clk >= 5'd20) da_next_state = R0;
                 else da_next_state = R1;
          end
          default:  da_next_state = IDLE;
        endcase
    end
    
    always@(posedge clk_dds)
      begin
       case(da_main_state)     
       IDLE:
         begin
           count_clk <= 5'd1;
           da_start <= 1'b0;
           end
       R0:
         begin
         count_clk <= count_clk + 1'b1; 
         da_start <= 1'b1;
         end     
      R1:
        begin
        if(count_clk >= 5'd20) 
          count_clk <= 5'd1;
        else 
          count_clk <= count_clk + 1'b1;
        da_start <= 1'b0;
        end
      default:
        begin
        da_start <= 1'b0; 
        count_clk <= 5'd1;       
        end      
      endcase
     end       
               
    dds_sin    u_sin (
               .clk_dds(clk_dds),
               .rst_n(rst_n),
               .out_en(out_en),
               .freq_sin(freq_sin),
               .phase_sin_init(phase_sin_init),
               .amp_sin(amp_sin),
               .sin_out(sin_out) );    
                               
    dds_saw    u_dds_saw (
               .clk_dds(clk_dds),
               .rst_n(rst_n),
               .out_en(out_en),
               .delay_saw(delay_saw),
               .freq_saw(freq_saw),
               .dc_data1(dc_data1),
               .dc_data2(dc_data2),
               .amp_saw(amp_saw),
               .saw_out(saw_out),   // 32bit signal out 
               .dc_sel(dc_sel) );
                                          
    square1x2x_genrator  
                # (.CLK_FREQ ( 20000000))
                u_square (
                          .clk_in(clk_dds),
                          .rst_n(rst_n),
                          .out_en(square_out_en),    
                          .freq_square_1x(freq_square_1x),
                          .freq_square_2x(freq_square_2x),
                          .phase_square_1x(phase_square_1x),
                          .phase_square_2x(phase_square_2x),                              
                          .square_1x(square_1x),
                          .square_2x(square_2x) );   
                                            
    da_spi    u_da_spi (
              .da_start(da_start),
              .clk_spi(clk_dds),
              .rst_n(rst_n),
              .da_data(signal_out),
              .cs_n(cs_n),
              .sclk(sclk),
              .dout(dout),
              .da_clr(da_clr) );        
                                                
endmodule
