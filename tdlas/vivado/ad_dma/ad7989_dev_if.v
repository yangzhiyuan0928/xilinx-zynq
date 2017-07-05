`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2016/07/05 10:40
// Design Name: 
// Module Name: ad7989_dev_if
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 采用三线CS模式，CS始终为高电平
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 注:ADC采样率100KSPS
//////////////////////////////////////////////////////////////////////////////////


module   ad7989_dev_if (
                           input    ad_clk,rst_n,ad_start,//ad_start: ad_start是由PS部分发送过来的ad采样控制信号 
                           input    ad_sdo,                      //ad转换串型数据
                           output           ad_cnv,
                           output           ad_sclk,
                           output           ad_ncs,                       
                           output    [17:0] ad_data,
                           output           ad_data_rdy   // data is available
                        );                                          
    parameter    FPGA_CLOCK_FREQ    = 50; //50MHz
    parameter    TCYC               = 10;
    parameter    ADC_CYC_CNT        = FPGA_CLOCK_FREQ * TCYC - 1;
    
    localparam    IDLE    = 3'b001;
    localparam    READ    = 3'b010;
    localparam    DONE    = 3'b100;
    
    reg    [9:0]  adc_tcyc_cnt;
    reg    [2:0]  main_state,next_state;
    reg    [4:0]  sclk_cnt;
    reg    [17:0] serial_buffer;
    reg    serial_read_done;
    reg    [17:0] ad_data_reg;
    
    wire    tmsb_almost_valid;
    wire    buffer_reset_s;
        
    assign    ad_cnv    =    (adc_tcyc_cnt > 25 && ad_start == 1) ? 1'b1 : 1'b0;  //adc_tcyc_cnt = 25时,cnv变为低电平
    assign    tmsb_almost_valid     =     (adc_tcyc_cnt == 26 ) ? 1'b1 : 1'b0; 
    assign    ad_sclk     =    (adc_tcyc_cnt > 6 && adc_tcyc_cnt < 25) ? ad_clk : 1'b0; //adc_tcyc_cnt = 24时读MSB
    assign    buffer_reset_s    =    (adc_tcyc_cnt == 26) ? 1'b1: 1'b0;
    assign    ad_data_rdy = (serial_read_done == 1'b1 && adc_tcyc_cnt == 5) ? 1'b1:1'b0;
    assign    ad_data    =    (ad_data_rdy == 1'b1) ? serial_buffer : ad_data_reg; //ad_data修改成ad_data_reg
    assign    ad_ncs    =    1'b1; //采用三线CS模式，nCS一直为高电平
    
    //ad_data_reg缓存输出数据
    always @(posedge ad_clk or negedge rst_n) begin
      if(!rst_n) ad_data_reg <= 18'd0;
      else ad_data_reg <= ad_data;
    end
     
    //adc_tcyc_cnt控制AD采样率 
    always @(posedge ad_clk,negedge rst_n) begin
      if(~rst_n) adc_tcyc_cnt <= ADC_CYC_CNT;
      else if(adc_tcyc_cnt != 0 && ad_start == 1) adc_tcyc_cnt <= adc_tcyc_cnt - 1'b1;
      else adc_tcyc_cnt <= ADC_CYC_CNT;
    end
    
    always @(posedge ad_clk,negedge rst_n) begin
      if(~rst_n) main_state <= IDLE;
      else main_state <= next_state;
    end
    
    always @(*)
      begin
        case(main_state)
          IDLE: begin
            if(tmsb_almost_valid) next_state = READ;
            else next_state = IDLE;
          end
          READ: begin
            if(sclk_cnt == 0) next_state = DONE;
            else next_state = READ;
          end
          DONE: next_state = READ;
          default: next_state = IDLE;
        endcase
      end
    
    always @(posedge ad_clk) begin
      case(main_state)
        IDLE: serial_read_done <= 1'b1;
        READ: serial_read_done <= 1'b0;
        DONE: serial_read_done <= 1'b1;
        default: serial_read_done <= 1'b0;
      endcase
    end
    
    always @(posedge ad_clk or negedge rst_n) begin
      if(!rst_n) begin
        serial_buffer <= 18'd0;
        sclk_cnt <= 5'd18;
      end
      else if(buffer_reset_s == 1'b1) begin
        serial_buffer <= 18'd0;
        sclk_cnt <= 5'd18;
      end
      else if(sclk_cnt > 5'd0) begin
        sclk_cnt <= sclk_cnt - 5'd1;
        serial_buffer <= {serial_buffer[16:0],ad_sdo};
      end
    end
endmodule
