proc boxes2box {boxes {offset {0 0 0 0}}} {
    set min_x 100000000000
    set min_y 100000000000
    set max_x 0
    set max_y 0
    set l [join [join $boxes]]
    
    for {set i 0} {$i < [llength $l]} {incr i 2} {
        set max_x [expr max($max_x, [lindex $l $i])]
        set max_y [expr max($max_y, [lindex $l [expr $i + 1]])]
        set min_x [expr min($min_x, [lindex $l $i])]
        set min_y [expr min($min_y, [lindex $l [expr $i + 1]])]
    }
    if [llength $offset] {
        set min_x [expr $min_x - [lindex $offset 0]]
        set min_y [expr $min_y - [lindex $offset 1]]
        set max_x [expr $max_x + [lindex $offset 2]]
        set max_y [expr $max_y + [lindex $offset 3]]        
    }
    return "$min_x $min_y $max_x $max_y"
}

proc box_around_point {x y {width 0.5} {height 0.5 }} {
    set x1 [expr $x - $width/2.0]
    set x2 [expr $x + $width/2.0]
    set y1 [expr $y - $height/2.0]
    set y2 [expr $y + $height/2.0]
    return "$x1 $y1 $x2 $y2"
}

proc box2region {box} {
    set x1 [lindex $box 0]
    set x2 [lindex $box 2]
    set y1 [lindex $box 1]
    set y2 [lindex $box 3]

    return "$x1 $y1 $x1 $y2 $x2 $y2 $x2 $y1 $x1 $y1"
}

proc resizeBox {box {offset {0 0 0 0}}} {
    set min_x [expr [lindex $box 0] - [lindex $offset 0]]
    set min_y [expr [lindex $box 1] - [lindex $offset 1]]
    set max_x [expr [lindex $box 2] + [lindex $offset 2]]
    set max_y [expr [lindex $box 3] + [lindex $offset 3]]
    return "$min_x $min_y $max_x $max_y"
}

proc resizeBoxList {boxList {offset {0 0 0 0}}} {
    set l {}
    foreach box $boxList {
        lappend l [resizeBox $box $offset]
    }
    return $l
}
