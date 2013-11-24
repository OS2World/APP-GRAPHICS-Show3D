/*
You can program your function here.
The value of x will be between -1.5 and 1.5,
the value of x between -2 and 2.
The program should return the value of a continuous
function ff(x,y) at the point (x,y).
This value should be between -0.2 and 0.2.
The function must have the name ff.
*/

double ff (double x, double y)
{	double z=x*y;
	if (z>1) z=1;
	if (z<-1) z=-1;
	return z*0.2;
}