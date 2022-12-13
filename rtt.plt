# set the terminal, i.e., the figure format (eps) and font (Helvetica, 20pt)
set term postscript eps enhanced "Helvetica" 20 

# reset all options to default, just for precaution
reset

# set the figure size
set size 0.7,0.7

# set the figure name
set output "rtt.eps"

# set the x axis
set xrange [0:2.6]
set xlabel "Simulation time (s)"
set xtics 0,2.6
set mxtics 2

# set the y axis
set yrange [0:0.001]
set ylabel "RTT (s)"
set ytics 0,0.001
set mytics 2

# set the grid (grid lines start from tics on both x and y axis)
set grid xtics ytics

# plot the data from the log file along with the theoretical curve
plot "rtt.log" u 1:2 t "RTT" w p pt 1 ps 1 lc rgb "#AAAAAA"