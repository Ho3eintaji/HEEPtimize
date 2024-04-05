set MEMORIES_LIB_DIR $::env(MEMORIES_LIB_DIR)
set MEMORIES_DB_DIR $::env(MEMORIES_DB_DIR)

set lib_files [glob -directory $MEMORIES_LIB_DIR -- "*.lib"]

foreach mem_lib $lib_files {
    set mem [file rootname $mem_lib]
    set path_name [split $mem /]
    set mem_name [lindex $path_name end]
    set db_file [concat $mem_name.db]

    puts "Converting $mem_lib into $db_file"
    set libraries [read_lib $mem_lib -return_lib_collection]
    # after 5000
    set library [get_object_name $libraries]
    puts "Writing $library to $MEMORIES_DB_DIR/$db_file"
    write_lib $library -format db -output $MEMORIES_DB_DIR/$db_file
}

exit