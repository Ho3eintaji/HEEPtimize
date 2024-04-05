set FLL_DIR ./lib
set FLL_DB_DIR ../symlinks/ETH_FLL/db

set lib_files [glob -directory $FLL_DIR -- "*.lib"]

foreach fll_lib $lib_files {
    echo $fll_lib
}

foreach fll_lib $lib_files {

    set fll [file rootname $fll_lib]
    set path_name [split $fll /]
    set fll_name [lindex $path_name end]
    set db_file [concat $fll_name.db]

    puts "Converting $fll_lib into $db_file"
    set libraries [read_lib $fll_lib -return_lib_collection] ;
    after 5000
    set library [get_object_name $libraries]
    puts "Writing $library to $FLL_DB_DIR/$db_file"
    write_lib $library -format db -output $FLL_DB_DIR/$db_file ;
}


exit
