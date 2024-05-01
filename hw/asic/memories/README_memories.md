# ARM 16nm memories

The memories are located in `../symlinks/ARM_Memories`. To generate the memories, run the following command:

~~~bash
./gen-mem.sh ../symlinks/ARM_Memories
~~~

The actual generation is done in the `../symlinks/ARM_Memories` directory and the generated files are simply simlinked here.

This script generates NLDM liberty models for the SRAMs. If you want CCS models, you can generate them using the GUI.

The documentation for the memory compilers is found in `../symlinks/ARM_Memories/ARM16nm_Memory_Compilers/**/doc/*.pdf`.
