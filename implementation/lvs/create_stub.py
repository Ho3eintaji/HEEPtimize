import subprocess
import os
import shutil

verilog_stubs_path = "./verilog_stubs"

if os.path.exists(verilog_stubs_path):
    shutil.rmtree(verilog_stubs_path, ignore_errors=True)

os.makedirs(verilog_stubs_path)

filename_in = "verilog.inc"
filename_out = "verilog.stub.inc"

with open(filename_out, "w") as fileout:

    with open(filename_in, "r") as filein:
        lines=filein.readlines()

        for line in lines:
            if ("include" in line):
                if ("{do_not_override") in line:
                    fileout.write(' '.join(line.split(' ')[:-1]) +"\n")
                else:
                    original_name = line.split()[1]
                    original_name_nopath = original_name.split("/")[-1]
                    new_file_name = original_name_nopath.split(".")[0] + "_stub.v"
                    fileout.write("`include " + verilog_stubs_path + "/" + new_file_name +"\n")
                    call_script = "python3 py_parser_lvs.py --netlist netlist_lvs_stubs.v --module " + original_name + " --output " + verilog_stubs_path + "/" + new_file_name
                    print("calling " + call_script)
                    subprocess.call(call_script, shell=True)
