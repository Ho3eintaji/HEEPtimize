# TSMC 16nm PAD Docs

PDDWUWSWCDGS_V and PDDWUWSWCDGS_H are the vertical and horizontal version of the IO_cell for the pads.

Inputs:
  ST = enable schmidt trigger (pad_attributes_i[3], default=1)
  IE = input enable (pad_attributes_i[2], default=1)
  PU = pullup enable (pad_attributes_i[0], default=0)
  PD = pulldown enable (pad_attributes_i[1], default=0)
  DS[0..3] = 4 bit bus to set the drive strength (pad_attributes_i[4..7], default=TBD)
  I = output value to drive (defined at instantiation)
  OEN = output enable (defined at instantiation)
  RTE = retention enable, keep at 0.
Outputs:
  C = input value
  PAD = the bond pad