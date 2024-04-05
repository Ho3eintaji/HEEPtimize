module switch_cell_mem (
`ifdef USE_PG_PIN
  inout  VIN,
  inout  VOUT,
  inout  VSS,
`endif
  input  VCTRL,     // Switch Signal Input
  output VCTRLFBn,  //Negated Schmitt Trigger Output
  output VCTRLFB,   //Schmitt Trigger Output
  output VCTRL_BUF  //ACK signal Output
);


`ifdef USE_PG_PIN
  assign VOUT = VCTRL == '0 ? VIN : 1'b0;
`endif

  logic VCTRLFB_int, VCTRL_BUF_int;

  assign VCTRLFB       = VCTRLFB_int;

  assign VCTRLFB_int   = ~VCTRL;
  assign VCTRL_BUF_int = VCTRL;

  assign VCTRL_BUF     = VCTRL_BUF_int;
  assign VCTRLFBn      = ~VCTRLFB_int;


endmodule
