#!/usr/local/bin/gnuplot --persist

# read command line arguments
script = ARG0
results = ARG1
type = ARG2

# fallback to default dataset file if provided one doesn't exist
if (!exists("results")) results='results.csv'

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
xequiv = 200
yequiv = 5

# aspect ratio
ar = yspan/xspan * xequiv/yequiv

# plot dimensions
ydim = 1200
xdim = ydim/ar

# set x and y tic intervals
set xtics 200 rotate by -45
set ytics 5

# set x and y ranges
set xrange [GPVAL_DATA_X_MIN:GPVAL_DATA_X_MAX]
set yrange [GPVAL_DATA_Y_MIN:GPVAL_DATA_Y_MAX]

# format y labels
set format y "%.3f"

# set plot title and labels
plotTitle = "Relaxed Jacobi (" . type . ")"
set title plotTitle font ",20" tc rgb "#606060"
set key left box autotitle columnhead
set xlabel "Matrix size (n times n)"
set ylabel "Time (ms)"

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
    "" notitle with yerrorbars ls 3#, \
#    "" smooth bezier title "Bezier" with lines ls 4

# save plot to image
plotImageName = "./doc/img/results-" . type . ".png"
print "Saving plot to " . plotImageName . "..."
set terminal pngcairo nocrop enhanced font "Lato-Medium,8" size xdim,ydim
set output plotImageName
set size ratio ar

replot
