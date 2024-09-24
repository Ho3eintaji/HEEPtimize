#!/bin/bash

echo "Extracting simulation timing data from transcript..."

for dir in "$1" "$2"; do
    if [ -d "$dir" ]; then
        echo "Processing directory: $dir"
        pushd "$dir" > /dev/null
        if [ -f "transcript" ]; then
            mkdir -p logs
            awk '
            /VCD file initialized:/ {
                vcd_file=$NF
                sub("logs/", "", vcd_file)
                print "Found VCD:", vcd_file
                split(vcd_file, arr, "[-.]")
                for (i in arr) {
                    print "arr[" i "] =", arr[i]
                }
                if (length(arr) > 1) {
                    print "Found VCD index:", arr[2]
                } else {
                    print "Error: Failed to extract index from", vcd_file
                    exit 1
                }
            }
            /VCD dump ON/ {
                start_time = gensub(/[\[\]ns]/, "", "g", $2)
            }
            /VCD dump OFF/ {
                end_time = gensub(/[\[\]ns]/, "", "g", $2)
                elapsed_time = end_time - start_time
                print "Writing to logs/time-" arr[2] ".txt with elapsed time:", elapsed_time "ns"
                print elapsed_time > "logs/time-" arr[2] ".txt"
            }' transcript
            echo "Timing data extracted to logs/time-*.txt in $dir"
        else
            echo "No transcript file found in $dir"
        fi
        popd > /dev/null
    else
        echo "Directory $dir does not exist or is not accessible"
    fi
done
