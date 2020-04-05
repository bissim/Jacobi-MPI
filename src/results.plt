#!/usr/local/bin/gnuplot --persist

# read command line arguments
script = ARG0
results = ARG1
type = ARG2

# check whether dataset file doesn't exist
if (!exists("results")) {
    print "Unable to find provided results file: ", results
    exit
}

print "Running ", script, " over ", results

# disable terminal output
set terminal unknown

set datafile separator ','
set key autotitle columnhead

# plot data to get x and y spans
plot results
xspan = GPVAL_DATA_X_MAX - GPVAL_DATA_X_MIN
yspan = GPVAL_DATA_Y_MAX - GPVAL_DATA_Y_MIN

# define axis units
if (type eq "serial") {
    xequiv = 350
    yequiv = 5
} else {
    if (type eq "parallel strong scaling") {
        xequiv = 1
        yequiv = 8
    } else {
        if (type eq "parallel weak scaling") {
            xequiv = 4
            yequiv = 1
        } else {
            print "Unknown type: ", type
            exit
        }
    }
}

# aspect ratio
ar = yspan/xspan * xequiv/yequiv

# plot dimensions
ydim = 800
xdim = ydim/ar

# set x and y tic intervals
if (type eq "serial") {
    set xtics 1024 rotate by -45
    set ytics 10
} else {
    if (type eq "parallel strong scaling") {
        set xtics 2 rotate by -45
        set ytics 10
    } else {
        if (type eq "parallel weak scaling") {
            set xtics 2 rotate by -45
            set ytics 0.5
        }
    }
}

# set x and y ranges
set xrange [GPVAL_DATA_X_MIN:GPVAL_DATA_X_MAX]
set yrange [GPVAL_DATA_Y_MIN:GPVAL_DATA_Y_MAX]

# format y labels
if (type eq "parallel weak scaling") {
    set format y "%.1f"
}

# set plot title and labels
plotTitle = "Relaxed Jacobi (" . type . ")"
set title plotTitle font ",32" tc rgb "#606060"
set key left box autotitle columnhead
if (type eq "serial") {
    set xlabel "Matrix size (n times n)"
} else {
    set xlabel "Number of processors"
}
set ylabel "Time (s)"

# grid style
set style line 1 lt 1 lw 0.5 lc rgb "#C0C0C0"
set grid ls 1

# graph style
set style data lines
set style line 2 lw 2 lc rgb "#26DFD0" #pt 7 ps 1
set style line 3 lw 2 lc black pt 0

# power function style
set style line 4 lw 1 lc rgb "red" dt 4

# plot provided data with specified settings
plot results ls 2, \
    "" notitle with yerrorbars ls 3, \
    results using 1:2:(sprintf("%.2f", $2)) with labels offset char 1, 1 notitle#, \
#    "" smooth bezier title "Bezier" with lines ls 4

# save plot to image
plotImageName = "./doc/img/results-" . type . ".png"
print "Saving plot to " . plotImageName . " as a " . sprintf("%d", xdim) . "x" \
    . sprintf("%d", ydim) . " image..."
set terminal pngcairo nocrop enhanced font "Lato-Medium,18" size xdim,ydim
set output plotImageName
set size ratio ar

replot
