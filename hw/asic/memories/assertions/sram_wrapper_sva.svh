`ifndef SRAM_WRAPPER_SVA_SVH_
`define SRAM_WRAPPER_SVA_SVH_

// Parameter check
initial begin
  if (DataWidth != 32) begin
    $error("DataWidth must be 32. Other sizes are not supported.");
  end
  case (NumWords)
    64, 128, 256, 2048, 4096, 8192:; // OK
    default: begin
      error("NumWords must be 64, 128, 256, 2048, 4096 or 8192. Other sizes are not supported.");
    end
  endcase
end

`endif /* SRAM_WRAPPER_SVA_SVH_ */
