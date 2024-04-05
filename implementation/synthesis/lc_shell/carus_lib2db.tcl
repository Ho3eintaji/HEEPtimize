set CARUS_LIB_DIR ../../../hw/vendor/nm-carus-backend-opt/implementation/pnr/outputs/nm-carus/lib/
set CARUS_DB_DIR ./nm-carus/db

set lib_files [glob -directory $CARUS_LIB_DIR -- "*.lib"]

file mkdir $CARUS_DB_DIR



foreach carus_lib $lib_files {
    echo $carus_lib
}

foreach carus_lib $lib_files {

    set mem [file rootname $carus_lib]
    set path_name [split $mem /]
    set mem_name [lindex $path_name end]
    set db_file [concat $mem_name.db]

    puts "Converting $carus_lib into $db_file"
    set libraries [read_lib $carus_lib -return_lib_collection] ;
    after 5000
    set library [get_object_name $libraries]
    puts "Writing $library to $CARUS_DB_DIR/$db_file"
    write_lib $library -format db -output $CARUS_DB_DIR/$db_file ;
}


exit
