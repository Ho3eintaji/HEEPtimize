#!/usr/bin/env python3
import pandas as pd
from mako.template import Template
import os

# for modifications of the IO pin table please edit the google sheet:
# https://docs.google.com/spreadsheets/d/1R42f33qJquhNsswMwyr-gp6fZFzjrIKRiU6ttj8dK-8/edit#gid=1605553209

pin_table_url = "https://docs.google.com/spreadsheets/d/e/2PACX-1vSxj3JT6EABDUgFf5RpNZIOyjmZz3Dl7S7QBn7IkNJgQFUEoNI14sn_1hdoIiEM-R0L0GYIwjkseEDs/pub?gid=1605553209&single=true&output=csv"
pin_table = pd.read_csv(pin_table_url)
pin_table.sort_values(by="layout_pin_number")

lvs_file = "calibre_v2lvs.tcl"

sides = {}



class Pad:

    def __init__(self, ring_side, instance, actual_instance, vdd_ring):
        self.ring_side = ring_side
        self.instance = instance
        self.actual_instance = actual_instance
        self.vdd_ring = vdd_ring
    def __str__(self):
        return "# {}: {}/{} @ {}".format(self.ring_side, self.instance, self.actual_instance, self.vdd_ring)

pads  = []
i = 0

for ring in pin_table.ring_segment.tolist():
    if str(ring) != "nan":
        instance  = pin_table.instance.tolist()[i].split("/")[1]
        actual_instance  = pin_table.instance.tolist()[i].split("/")[-1]
        ring_side = ring
        vdd_ring  = pin_table.vdd_ring.tolist()[i]
        pads.append(Pad(ring_side, instance, actual_instance, vdd_ring))
    i = i + 1

pads_map = {
    "pads" : pads,
}

lvstemplate = Template(filename=lvs_file+'.tpl')
with open(lvs_file, "w") as f:
    f.write(lvstemplate.render(**pads_map))
    
