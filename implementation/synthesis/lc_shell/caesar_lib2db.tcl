set CAESAR_LIB_DIR ../../../hw/vendor/nm-caesar-backend-opt/implementation/pnr/outputs/nm-caesar/lib/
set CAESAR_DB_DIR ./nm-caesar/db

set lib_files [glob -directory $CAESAR_LIB_DIR -- "*.lib"]

file mkdir $CAESAR_DB_DIR



foreach caesar_lib $lib_files {
    echo $caesar_lib
}

foreach caesar_lib $lib_files {

    set mem [file rootname $caesar_lib]
    set path_name [split $mem /]
    set mem_name [lindex $path_name end]
    set db_file [concat $mem_name.db]

    puts "Converting $caesar_lib into $db_file"
    set libraries [read_lib $caesar_lib -return_lib_collection] ;
    after 5000
    set library [get_object_name $libraries]
    puts "Writing $library to $CAESAR_DB_DIR/$db_file"
    write_lib $library -format db -output $CAESAR_DB_DIR/$db_file ;
}


exit
