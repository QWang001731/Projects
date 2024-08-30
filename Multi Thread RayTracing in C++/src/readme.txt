Description: Ray tracing calculation can be time-consuming due to the large amount 
of computation time. C++ provides libraries like <future.h> and <thread.h> that 
support a solution of using multiple thread to do computation simultaneously to 
significantly reduce the computation time.

This program uses one and two threads to do the required computation of raytracing of a sphere
, so that it can be seen that the required computation time in 2 thread mode is almost  halved. 
The output file "grid.txt" is the recorded lumination of the sphere. The PLOT.py program takes "grid.txt" 
as input and plots this sphere.
