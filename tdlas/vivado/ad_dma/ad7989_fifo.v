`timescale 1ns / 1ps
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


module ad7989_fifo #(parameter [7:0] CHANNEL_NUM = 8'd1)
                   (
                     input    ad_clk,rst_n,ad_start,//ad_start: ad_start是由PS部分发送过来的ad采样控制信号 
                     input    trig_in,//trig_in由DA部分(DDS)传过来脉冲信号，启动ad转换
                     // AD SPI port
                     input    ad_sdo, //ad转换串型数据
                     output   ad_cnv,
                     output   ad_sclk,
                     output   ad_ncs,     
                     // FIFO2 port
                     input    dma_clk,
                     input    rst1_n,  
                     input    rd_en,
                     output   fifo2_empty,
                     output   fifo2_full,
                     output   almost_empty,
                     output   [31:0] fifo_dout             
                   );
    // intern signal               
    wire    ad_data_rdy;
    wire    [17:0] ad_data;
    wire    [31:0] sample_data;
    wire    [17:0] fifo1_dout;
    wire    [31:0] fifo2_din;
    wire    fifo1_empty;
    wire    wr_fifo1_en;
    wire    rd_fifo1_en;
    wire    wr_fifo2_en;
    
    wire    trig_in;
    wire    trigger;
    wire    fifo1_full;
    
    // reg variable
    reg    [31:0] block_num = 0; //块序列号
    reg    [31:0] trig_channel_info = 0; //[31:24]通道号、[23:16]触发状态、[15:0]触发位置
    reg    [31:0] crc_code = 32'd0;  //crc校验码
    reg    [31:0] reserve_register = 32'd0; //备用寄存器
    
    // trig_in detect
    reg           trig_latch_en = 0;
    reg    [31:0] trig_channel_reg = 32'd0;
    reg    [15:0] trig_position = 16'd0; //触发信号来时对应的AD数据位置信息
    reg    [7:0] trig_flag = 8'd0; //trig状态标识，0表示该计数周期(1024个采样点)无trig信号，255表示有trig
    reg    trig_reg1 = 0;
    reg    trig_reg2 = 0;
    
    // FIFO_1
    reg    [9:0] wr_fifo1_count = 0; //Sample data 计数器 4096Byte数据(1020次)一次循环

    // FIFO_2
    reg    wr_fifo2_en_tmp1;
    reg    wr_fifo2_en_tmp2;
    reg    [10:0]  wr_fifo2_count = 0;
    reg    [10:0]  rd_fifo1_count = 0;
    reg    [31:0]  fifo2_din_reg;  
           
    assign    sample_data = ({{15{fifo1_dout[17]}},fifo1_dout[16:0]});

    assign    wr_fifo1_en = ad_data_rdy && !fifo1_full;//
    assign    rd_fifo1_en = !fifo1_empty && !fifo2_full && (rd_fifo1_count < 10'd1020 );     
    
      //=========== fifo1_data count ===========
       always  @(posedge ad_clk or negedge rst_n) begin
         if(!rst_n)            
              wr_fifo1_count <= 10'd0;
          else if(wr_fifo1_en) begin
               if(wr_fifo1_count >= 10'd1020)
                        wr_fifo1_count <= 10'd1; //一个packet包含1020个ADC数据
               else  
                     wr_fifo1_count <= wr_fifo1_count + 1'b1;
               end       
          else
             wr_fifo1_count <= wr_fifo1_count;
       end

     //=========== fifo1_data read count ===========
       always  @(posedge dma_clk or negedge rst_n) begin
         if(!rst_n)            
              rd_fifo1_count <= 10'd0;
          else begin
              if(rd_fifo1_en)
                  rd_fifo1_count <= rd_fifo1_count + 1'b1;
                else
                  rd_fifo1_count <= rd_fifo1_count;
               if (wr_fifo2_count >= 11'd1024)  rd_fifo1_count <=0;
               end        
       end
       
    //========== trig_position ==========        
    always @(posedge ad_clk or negedge rst_n) begin
    if(!rst_n) begin
      trig_reg1 <= 0;
      trig_reg2 <= 0;    
    end
    else begin
         trig_reg1 <= trig_in;  // trig_in是边沿信号
         trig_reg2 <= trig_reg1;
         end
    end 
    
    assign trigger = trig_reg1 & (!trig_reg2);
    
    always @(posedge ad_clk or negedge rst_n ) begin // trig_in是边沿信号
    if(!rst_n) begin
       trig_position <= 0;
       trig_flag <= 0;
    end
    else if(trigger == 1) begin
         trig_position <= wr_fifo1_count; //trig_position
         trig_flag <= 8'd255;
         end 
         else if ((wr_fifo1_en ==1) && (wr_fifo1_count == 10'd1020)) begin //1个packet有1024个字,包含1020个采样点数据
               trig_position <= 0; //wr_fifo2_count = 1022时,将trig_position写入fifo中
               trig_flag <= 0;
               end
              else begin
                trig_position <= trig_position;
                trig_flag <= trig_flag;
                end
     end   
    
    // ========== block_num ==========
    always @(posedge dma_clk or negedge rst1_n) begin
     if(!rst1_n) block_num <=0;
     else if(wr_fifo2_count == 11'd1024 && wr_fifo2_en == 1) block_num <= block_num + 1'b1;
          else block_num <= block_num;
    end
    
    // ========== trig_channel_info ==========
     // speed of reading fifo1 must be fast than taht of writing it
     // or trigger position may be lost 
        
       always @(posedge ad_clk or negedge rst_n) begin
         if(!rst_n) trig_channel_reg <= 32'd0;
         else if(wr_fifo1_count == 10'd1020) trig_channel_reg <= {CHANNEL_NUM,trig_flag,trig_position};
         else trig_channel_reg <= trig_channel_reg;
       end      
           
      always @(posedge dma_clk or negedge rst1_n) begin //跨时钟域锁存数据
        if(!rst1_n) trig_channel_info <= 32'd0;
        else if(wr_fifo2_count == 10'd1020) trig_channel_info <= trig_channel_reg;
             else trig_channel_info <= trig_channel_info;  
      end
    
    // ========== crc_code ==========
    always @(posedge dma_clk or negedge rst_n) begin
      if(!rst_n) crc_code <= 0;
      else if(wr_fifo2_count < 11'd1020 && wr_fifo2_en == 1) crc_code <= crc_code ^ fifo2_din_reg;
      else case(wr_fifo2_count)
        11'd1020: crc_code <= crc_code ^ block_num;
        11'd1021: crc_code <= crc_code ^ trig_channel_info;
        11'd1022: crc_code <= crc_code ^ reserve_register;
        11'd1023: crc_code <= crc_code;
        11'd1024: crc_code <= 0;
      endcase      
    end 
    
    // ========== wr_fifo2_en ==========
    always @(posedge dma_clk or negedge rst1_n) begin
      if(!rst1_n) begin
      wr_fifo2_en_tmp1 <= 0;
      wr_fifo2_en_tmp2 <= 0;      
      end 
      else begin    
      wr_fifo2_en_tmp1 <= rd_fifo1_en;
      wr_fifo2_en_tmp2 <= wr_fifo2_en_tmp1;
      end
    end
   
    assign    wr_fifo2_en = (wr_fifo2_count <= 11'd1020) ? wr_fifo2_en_tmp2 : 1'b1; //连续写3个字的控制信息: 块序列号、crc校验码、触发信号来时对应的AD数据位置信息、通道号

    // ========== fifo2 data count ==========  
    always @(posedge dma_clk or negedge rst1_n) begin
      if(!rst1_n) wr_fifo2_count <= 0;    
      else if(wr_fifo2_count == 11'd1024) wr_fifo2_count <= 0;
      else if(wr_fifo2_en == 1) wr_fifo2_count <= wr_fifo2_count + 1'b1;
      else if(wr_fifo2_count>= 11'd1020) wr_fifo2_count <= wr_fifo2_count + 1'b1;
      else wr_fifo2_count <= wr_fifo2_count;
    end
    
    // ========== fifo2_din ==========
    assign  fifo2_din = fifo2_din_reg;
    
    always @(posedge dma_clk or negedge rst1_n) begin
      if(!rst1_n) fifo2_din_reg <= 0;
      else case(wr_fifo2_count)
        11'd1020: fifo2_din_reg <= block_num;
        11'd1021: fifo2_din_reg <= trig_channel_info;
        11'd1022: fifo2_din_reg <= reserve_register;
        11'd1023: fifo2_din_reg <= crc_code;
        default: fifo2_din_reg <= sample_data;
      endcase
    end
                
    ad7989_dev_if   U_ad(
                          .ad_clk(ad_clk),
                          .rst_n(rst_n),
                          .ad_start(ad_start),//ad_start: ad_start是由PS部分发送过来的ad采样控制信号 
                          .ad_sdo(ad_sdo),                      //ad转换串型数据
                          .ad_cnv(ad_cnv),
                          .ad_sclk(ad_sclk),
                          .ad_ncs(ad_ncs),                       
                          .ad_data(ad_data),
                          .ad_data_rdy(ad_data_rdy)   // data is available
                         );  
                                                  
    fifo_sample_data    U_ad_sample_fifo (
                                           .rst(!rst_n),    // input wire rst
                                           .wr_clk(ad_clk),      // input wire wr_clk
                                           .rd_clk(dma_clk),    //input wire rd_clk                                           
                                           .din(ad_data),      // input wire [17 : 0] din
                                           .wr_en(wr_fifo1_en),  // input wire wr_en
                                           .rd_en(rd_fifo1_en),  // input wire rd_en
                                           .dout(fifo1_dout),    // output wire [17 : 0] dout
                                           .full(fifo1_full),    // output wire full
                                           .empty(fifo1_empty)  // output wire empty
                                         );    
                                         
    fifo_dma_stream_0    U_dma_fifo (
                                       .clk(dma_clk),        // input wire rst
                                       .srst(!rst1_n),  // input wire wr_clk
                                       .din(fifo2_din),        // input wire [31 : 0] din
                                       .wr_en(wr_fifo2_en),    // input wire wr_en
                                       .rd_en(rd_en),    // input wire rd_en
                                       .dout(fifo_dout),      // output wire [31 : 0] dout
                                       .full(fifo2_full),      // output wire full
                                       .empty(fifo2_empty),    // output wire empty
                                       .prog_empty(almost_empty)  // output wire prog_empty = 16
                                     );
       
endmodule
