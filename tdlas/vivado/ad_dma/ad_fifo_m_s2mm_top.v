`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2016/06/20 21:33:05
// Design Name: 
// Module Name: top_test
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


module AD_FIFO_M_S2MM #(
                          parameter integer M_AXIS_TDATA_WIDTH = 32,
                          parameter integer C_M_START_COUNT = 32,
                          parameter [7:0] CHANNEL_Num = 8'd1
                       )
                       (
                          input            ad_start_sync,      //synchonize all ad start
                          //output           LED,     
                          // Trigger signal
                          input            trig_in, 
                          // AD SPI port
                          input    ad_sdo, //ad转换串型数据
                          output   ad_cnv,
                          output   ad_sclk,
                          output   ad_ncs,      
                          // Sys clock                                         
                          input            s_axi_aresetn,   // System reset
                          input            s_axi_aclk,      // System clock = 50MHz                        
                          // axi4-Lite interface                                
                          input            s_axi_awvalid,  //write addr
                          input    [5:0]   s_axi_awaddr,
                          output           s_axi_awready, 
                          input            s_axi_wvalid,   //write data
                          input    [31:0]  s_axi_wdata,
                          input    [3:0]   s_axi_wstrb,  
                          output           s_axi_wready,
                          input            s_axi_bready,  //write response
                          output           s_axi_bvalid,  
                          output     [1:0] s_axi_bresp,                   
                          // axi4-Stream interface
                          input             M_AXIS_ACLK,
                          input             M_AXIS_ARESETN,
                          input             M_AXIS_TREADY,
                          output            M_AXIS_TVALID,
                          output            M_AXIS_TLAST,
                          output      [3:0] M_AXIS_TSTRB,
                          output      [31:0] M_AXIS_TDATA
                      );
    //intern signal
    wire    ad_start;
    wire    [31:0] FIFO_DATA;     
    wire    FIFO_ALMOST_EMPTY;
    wire    FIFO_RD_EN;  
  
    reg    ad_start_1;
    
 //   assign    LED = ad_start;  //PL通过AXI4接收到ad_start信号就点亮LED
    always @(posedge s_axi_aclk or negedge s_axi_aresetn  )
    begin
      if (!s_axi_aresetn)  ad_start_1<=0;
      else if ((ad_start_sync ==1) && (ad_start ==1))
            ad_start_1<=1;
    end
    
                
    ad7989_fifo      
          #(
              .CHANNEL_NUM( CHANNEL_Num )
             )
           U_ad_fifo (
                                .ad_clk(s_axi_aclk),
                                .rst_n(s_axi_aresetn),
                                .ad_start(ad_start_1),//ad_start: ad_start是由PS部分发送过来的ad采样控制信号 
                                .trig_in(trig_in),//trig_in由DA部分(DDS)传过来脉冲信号，启动ad转换
                                // AD SPI port
                                .ad_sdo(ad_sdo), //ad转换串型数据
                                .ad_cnv(ad_cnv),
                                .ad_sclk(ad_sclk),
                                .ad_ncs(ad_ncs),
                                // FIFO2 port
                                .dma_clk(M_AXIS_ACLK),
                                .rd_en(FIFO_RD_EN),
                                .rst1_n(M_AXIS_ARESETN),
                                .fifo2_empty(),
                                .fifo2_full(),
                                .almost_empty(FIFO_ALMOST_EMPTY),   
                                .fifo_dout(FIFO_DATA)                                               
                             );
                                  
    M_AXIS_S2MM_v1_0  # (
                          .C_M_AXIS_TDATA_WIDTH(M_AXIS_TDATA_WIDTH),
                          .C_M_START_COUNT(C_M_START_COUNT)
                         )     
                        M_AXIS_S2MM (
                                      // Users to add ports here
                                      .FIFO_DATA(FIFO_DATA),
                                      .FIFO_ALMOST_EMPTY(FIFO_ALMOST_EMPTY),
                                      .FIFO_RD_EN(FIFO_RD_EN),
                                      // User ports ends
                                      // Do not modify the ports beyond this line
                              
                                      // Global ports
                                      .M_AXIS_ACLK(M_AXIS_ACLK),
                                      // 
                                      //.M_AXIS_ARESETN(M_AXIS_ARESETN),
                                      .M_AXIS_ARESETN(M_AXIS_ARESETN),
                                      // Master Stream Ports. TVALID indicates that the master is driving a valid transfer, A transfer takes place when both TVALID and TREADY are asserted. 
                                      .M_AXIS_TVALID(M_AXIS_TVALID),
                                      // TDATA is the primary payload that is used to provide the data that is passing across the interface from the master.
                                      .M_AXIS_TDATA(M_AXIS_TDATA),
                                      // TSTRB is the byte qualifier that indicates whether the content of the associated byte of TDATA is processed as a data byte or a position byte.
                                      .M_AXIS_TSTRB(M_AXIS_TSTRB),
                                      // TLAST indicates the boundary of a packet.
                                      .M_AXIS_TLAST(M_AXIS_TLAST),
                                      // TREADY indicates that the slave can accept a transfer in the current cycle.
                                      .M_AXIS_TREADY(M_AXIS_TREADY)
                                  );
                                     
    up_axi_AD_start    U_ad_start (
                                    .S_AXI_ARESETN(s_axi_aresetn),   //
                                    .S_AXI_ACLK(s_axi_aclk),         //System clock = 50MHz
                                    // axi4 interface                                
                                    .S_AXI_AWVALID(s_axi_awvalid),  //write addr
                                    .S_AXI_AWADDR(s_axi_awaddr),
                                    .S_AXI_AWREADY(s_axi_awready), 
                                    .S_AXI_WVALID(s_axi_wvalid),   //write data
                                    .S_AXI_WDATA( s_axi_wdata),
                                    .S_AXI_WSTRB(s_axi_wstrb),
                                    .S_AXI_WREADY(s_axi_wready),
                                    .S_AXI_BREADY(s_axi_bready),  //write response
                                    .S_AXI_BVALID(s_axi_bvalid),  
                                    .S_AXI_BRESP(s_axi_bresp),                  
                                    .ad_start(ad_start) //ad_start
                                  );
                               
endmodule
