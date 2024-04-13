set output "twoRayModelPlot.pdf"
set term pdfcairo size 15cm,10cm
set print "-"

set xrange  [0*10**0:7*10**0]
set yrange  [-35:80]
# set y2range [-200:200]
# set logscale x 
#set logscale y 
set xtics 1
set ytics 10
# set y2tics 10
set mxtics 5
set mytics 5
set grid xtics ytics
# set format x "%.0e"
#set format y "%.2e"
# set format y2 "%g°"

set samples 100000

# placement of legend
set key vertical maxrows 4 width 0
set key outside below

# define axis labels
set xlabel "d[m]"
set ylabel "Path loss [dB]"


#                                                                                           #
#############################################################################################
#                                      Prepare gnuplot                                      #

# Define i = sqrt(-1)
i = {0.0,1.0}

# Define s as 2pi * f * i
# f is frequency in Hz
# s(f) = 2 * pi * f * i

# speed of light in vacuum
c = 300 * 10**6


#                                                                                           #
#############################################################################################
#                                         Settings                                          #

# relative dielectric constant
epsilon_r = 3.4

# height of antenna 1
hAnt1 = 1

Gant1 = 2.5

# height of antenna 2
hAnt2 = 1

Gant2 = 2.5

# fittingscoef
lf = 6

# Carrier wave frequency
f = 2.4 *10**9
# f = 915 *10**6
lambda = c / f
beta = 2 * pi / lambda

#                                                                                           #
#############################################################################################
#                                      Plot functions                                       #
magnitude(x)  = 20*log10(abs(x))
phase(x)      = (180*arg(x))/pi

#                                                                                           #
#############################################################################################
#                                        Free space                                         #
freeSpace(d) = magnitude(4 * pi * d / (lambda * sqrt(Gant1*Gant2)))


#                                                                                           #
#############################################################################################
#                                          Two ray                                          #
theta(d) = atan((hAnt1 + hAnt1) / d)
Gamma(d) = (sin(theta(d))-sqrt(epsilon_r - (cos(theta(d)))**2))/sin(theta(d))-sqrt(epsilon_r + (cos(theta(d)))**2)

twoRay(d) = 1 * magnitude(1/(1+Gamma(d)*exp(i * beta * (sqrt((hAnt1 + hAnt2)**2 + d**2) - sqrt((hAnt1 - hAnt2)**2 + d**2)))))

#                                                                                           #
#############################################################################################
#                                       Fitting coef                                        #
fitting(d) = lf*log10(50*d)


calcFitting(d, meas) = (meas - freeSpace(d) - twoRay(d))/(log10(50*d))
print "fitting average"
print "\tfitting is: ", calcFitting(1.0, 39.67)
print "\tfitting is: ", calcFitting(2.0, 44.00)
print "\tfitting is: ", calcFitting(3.0, 47.67)
print "\tfitting is: ", calcFitting(4.0, 46.56)
print "\tfitting is: ", calcFitting(5.0, 46.44)
print "\tfitting is: ", calcFitting(6.0, 51.56)

print "fitting median"
print "\tfitting is: ", calcFitting(1.0, 40.00)
print "\tfitting is: ", calcFitting(2.0, 44.00)
print "\tfitting is: ", calcFitting(3.0, 48.00)
print "\tfitting is: ", calcFitting(4.0, 47.00)
print "\tfitting is: ", calcFitting(5.0, 46.00)
print "\tfitting is: ", calcFitting(6.0, 52.00)

# plot freeSpace(x) title "free space"
plot twoRay(x) title "Two ray", freeSpace(x) title "Free space", twoRay(x)+freeSpace(x) title "Two ray + free space", twoRay(x)+freeSpace(x)+fitting(x) title "Two ray + free space + fitting lf=6"