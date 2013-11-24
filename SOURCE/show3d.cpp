#include "easy.h"
#include "show3d.h"
#include "queue.h"

#include <math.h>

Program program; // must be in every program

BitmapPS *bitmap=0; // bitmap for the picture

double eyes=0.5; // eye distance
double pagesize=0.8; // size of printer page
int resolution=2; // lazer resolution (1=300 DPI, 2=150 DPI)
int screencolors=2;

class Cut
// Berechnet die Musterverl„ufe fr Schnitte durch die Figur.
// Dies ist abh„ngig von der gew„hlten Figur.
{   protected :
	Thread *T;
	double Eyes;
	public :
	virtual void getcut (double y, int refs[], int width, Thread &t) {}
	int expected () { return T->expected(); }
};

class FunctionCut : public Cut
{	public :
	virtual void getcut (double y, int refs[], int width, Thread &t);
	virtual double f (double x, double y) { return 0; }
};

void FunctionCut::getcut (double y, int refs[], int width, Thread &t)
{   int i;
	double x=2,xd,z,xl,xlold,factor,xold;
	int l,r;
	int first=1;
	xd=1.0/width;
	for (i=0; i<width; i++) refs[i]=-1;
	while (x>-2)
	{   xold=x;
		again :
		x-=xd;
		if (t.expected()) return;
		z=-1+f(x,y);
		if (!first) xlold=xl;
		xl=(2*(x+eyes)/(2-z)+1-eyes)/2*width;
		if (!first)
		{	factor=fabs(xl-xlold);
			if (factor<0.5) factor=0.5;
			xd/=factor/0.9;
			if (factor>1)
			{	x=xold; goto again;
			}
		}
		first=0;
		l=floor(xl);
		r=floor((2*(x-eyes)/(2-z)+1+eyes)/2*width);
		if (l<0 || l>=width) continue;
		if (r>=width) continue;
		if (refs[l]<0) refs[l]=r;
		else if (refs[l]>r) refs[l]=r;
	}
}

class WaveFunctionCut : public FunctionCut
{	public :
	virtual double f (double x, double y)
	{	double r=sqrt(x*x+y*y);
		return cos(12*r)*exp(-2*r)*0.2;
	}
} wave;

class AppleFunctionCut : public FunctionCut
{	public :
	virtual double f (double x, double y);
} apple;

double AppleFunctionCut::f (double x, double y)
{	double zre,zim,h;
	int i;
	x-=0.7;
	zre=x; zim=y;
	for (i=0; i<8; i++)
	{ 	h=x+zre*zre-zim*zim;
		zim=y+2*zre*zim; zre=h;
	}
	double r=zre*zre+zim*zim;
	if (r<4) r=4;
	r=log(r)-log(4);
	if (r>200) r=200;
	return (r/200)*0.2;
}

class BallFunctionCut : public FunctionCut
{	public :
	virtual double f (double x, double y);
} ball;

double BallFunctionCut::f (double x, double y)
{	double r=sqrt(x*x+y*y);
	if (r>0.7) return 0;
	else return sqrt(0.7*0.7-r*r)/2;
}

double ff (double x, double y);
class FFunctionCut : public FunctionCut
{	public :
	virtual double f (double x, double y)
	{	return ff(x,y);
	}
} function;

class MyThread : public Thread
{   double Done; // done percentage
	public :
	MyThread (int (*f) (Parameter)) : Thread(f) {}
	double done () { return Done; }
	void done (double done) { Done=done; }
};

Cut *actcut=&wave;

class Image
// This class generates the image line by line
// can be on a bitmap, a PM printer, or a raw printer
{   protected :
	long Width,Height;
	public :
	virtual void point (long i, ULONG color) {}
	virtual void nextline () {}
	virtual long width () { return Width; }
	virtual long height () { return Height; }
	virtual void erase () {}
};

class BitmapImage : public Image
{   BitmapPS *Ps;
	long line;
	public :
	BitmapImage (BitmapPS &ps) : Ps(&ps)
	{	line=ps.height()-1;
		Height=ps.height(); Width=ps.width();
	}
	virtual void point (long i, ULONG color)
	{	Ps->point(i,line,color);
	}
	virtual void nextline () { line--; }
	virtual void erase () { Ps->erase(); }
};

class PrinterImage : public Image
{   PrinterPS *Ps;
	long line;
	unsigned char *Data;
	public :
	PrinterImage (PrinterPS &ps) : Ps(&ps)
	{	line=ps.height()-1;
		Height=ps.height(); Width=ps.width();
		Data=new unsigned char [Width/8+1];
		memset(Data,0,Width/8);
	}
	~PrinterImage ()
	{	delete Data;
	}
	virtual void point (long i, ULONG color);
	virtual void nextline ();
};

void PrinterImage::point (long i, ULONG color)
{	if (color!=Colors::black) return;
	Data[i>>3]|=(unsigned char)(128>>(i&7));
}

void PrinterImage::nextline ()
{	long i,total=Width/8,w;
	for (i=0; i<total; i+=128)
	{   Ps->move(i*8,line);
		w=128;
		if (i+w>total) w=total-i;
		Ps->image(w*8,1,Data+i);
	}
	memset(Data,0,Width/8);
	line--;
}

class LaserImage : public Image
{	Queue *Q;
	unsigned char *Data;
	public :
	LaserImage ();
	~LaserImage ();
	virtual void point (long i, ULONG color);
	virtual void nextline ();
};

LaserImage::LaserImage ()
{	Q=new Queue("HPLaserJ","Show3D");
	*Q << "\x1B""*t150R"; // 300 DPI resolution
	double lasersize=2400;
	Width=Height=((int)(lasersize/resolution*pagesize)/8)*8;
	*Q << Format("\x1B""*p%dx%dY",
		(int)((lasersize-Width*resolution)/2),
		(int)((lasersize*sqrt(2.0)-Height*resolution)/2));
		// center raster image
	*Q << "\x1B""*r1A"; // start raster graphics left flush
	Data=new unsigned char[4096];
	memset(Data,0,Width/8);
}

LaserImage::~LaserImage ()
{   *Q << "\x1B""*rB"; // end raster graphics
	delete Data;
	delete Q;
}

void LaserImage::point (long i, ULONG color)
{	if (color!=Colors::black) return;
	Data[i>>3]|=(unsigned char)(128>>(i&7));
}

void LaserImage::nextline ()
{  	*Q << Format("\x1B""*b%dW",Width/8); // start raster line
	Q->write(Data,Width/8);
	memset(Data,0,Width/8);
}

class Figure
// controls drawing an image.
// Figure is used in a separate thread and generated by it.
{   int *Left;
	ULONG *Color;
	int Width,Height,Colors;
	MyThread *T; // the generating thread
	Cut *F;
	public :
	Figure (MyThread &thread, Cut &f, int nc=screencolors)
		: Colors(nc),T(&thread),F(&f) {}
	void draw (Image &image);
	void getcut (double y);
	void colors (int i) { Colors=i; }
	int expected () { return T->expected(); }
};

void Figure::draw (Image &image)
{   int i,row;
	image.erase();
	Width=image.width(); Height=image.height();
	Left=new int[Width];
	Color=new ULONG[Width];
	for (row=0; row<Height; row++)
	{	F->getcut(1.5-3*(double)row/Height,Left,Width,*T);
		if (expected()) return;
		for (i=Width-1; i>=0; i--)
		{	if (Left[i]<0 || Left[i]==Left[i+1])
			{	switch (rand()%Colors)
				{	case 0 : Color[i]=Colors::white; break;
					case 1 : Color[i]=Colors::black; break;
					case 2 : Color[i]=Colors::blue; break;
					case 3 : Color[i]=Colors::red; break;
					default : Color[i]=Colors::green;
				}
			}
			else
			{	Color[i]=Color[Left[i]];
			}
			image.point(i,Color[i]);
			if (expected()) return;
		}
		image.nextline();
		T->done((double)row/Height);
	}
	delete Color;
	delete Left;
}

class MyWindow : public StandardWindow
{   int Activate; // must be activated
	public :
	MyWindow () :
		StandardWindow(ID_Window,"Show 3D",
			StandardWindow::normal|StandardWindow::menu),Activate(0)
	{	init();
	}
	virtual void sized ();
	virtual void redraw (PS &ps);
	void activate () { Activate=1; sized(); }
	virtual int close ();
	virtual void timer (int i); // copy bitmap to screen every 1 sec
} mywindow;
Menu menu(mywindow);
Help help (mywindow,ID_Help,"show3d.hlp","");

void MyWindow::redraw (PS &ps)
// this is easy
{   bitmap->copy(ps);
}

void MyWindow::timer (int i)
// called every 1 sec
{	bitmap->copy(WindowPS(mywindow));
}

int draw (Parameter p);
MyThread drawthread (draw); // the drawing thread

int draw (Parameter p)
{   Figure figure(drawthread,*actcut);
	BitmapImage image(*bitmap);
	mywindow.starttimer(1);
	drawthread.done(0);
	figure.draw(image);
	mywindow.update();
	mywindow.stoptimer();
	return 0;}

int print (Parameter p);
MyThread printthread (print); // the printing thread

int print (Parameter p)
// called from printing thread
{   Figure figure(printthread,*actcut,2);
	PrinterPS printer;
	long x,y,w,h;
	x=printer.width()*(1-pagesize)/2;
	w=((pagesize*printer.width())/8+1)*8;
	y=printer.height()/2-w/2;
	h=w;
	printer.offset(x,y);
	printer.clip(0,0,w,h);
	PrinterImage image(printer);
	printthread.done(0);
	figure.draw(image);
	Beep(440,1);
	return 0;
}

int laser (Parameter p);
MyThread laserthread (laser); // the printing thread

int laser (Parameter p)
// called from printing thread
{   Figure figure(laserthread,*actcut,2);
	LaserImage image;
	figure.draw(image);
	Beep(440,1);
	return 0;
}

void MyWindow::sized ()
{   drawthread.wait();
	if ((bitmap &&
		(width()!=bitmap->width() || height()!=bitmap->height()))
			|| !bitmap)
	{	if (bitmap) delete bitmap;
		bitmap=new BitmapPS (mywindow);
	}
	if (!Activate) return;
	drawthread.start();
	update();
}

void start ()
{	drawthread.wait();
	drawthread.start();
}

int abortprint ()
{	if (printthread.started())
	{   char msg[256];
		sprintf(msg,"%2.3g percent done.\nAbort Print?",
			printthread.done()*100);
		if (Question(msg,"3D")==Answers::no) return 0;
		printthread.wait();
	}	return 1;
}

int abortlaser ()
{	if (laserthread.started())
	{   char msg[256];
		sprintf(msg,"%2.3g percent done.\nAbort Laser Print?",
			laserthread.done()*100);
		if (Question(msg,"3D")==Answers::no) return 0;
		laserthread.wait();
	}	return 1;
}

int MyWindow::close ()
{ 	if (!abortprint()) return 0;
	if (!abortlaser()) return 0;
	drawthread.wait();	return 1;
}

void doexit ()
{   mywindow.quit();
}

void doprint ()
{   if (!abortprint()) return;
	printthread.wait();	printthread.start();
}

void dolaserjet ()
{   if (!abortlaser()) return;
	laserthread.wait();	laserthread.start();
}

void dowave ()
{	actcut=&wave; start();
}

void doapple ()
{	actcut=&apple; start();
}

void doball ()
{	actcut=&ball; start();
}

void dofunction ()
{	actcut=&function; start();
}

void dooptions ()
{   drawthread.wait();
	Dialog options(mywindow,ID_Options,help,400);
	SliderItem slider(IDO_Eyes,options,eyes*100);
	SliderItem page(IDO_Pagesize,options,pagesize*100);
	options.carryout();
	if (options.result()!=DID_OK) return;
	eyes=slider/100.0;
	pagesize=page/100.0;
	mywindow.update();
	start();
}

void dogeneral ()
{	help.general();
}

int main ()
{   mywindow.size(350,350);
	mywindow.top();
	mywindow.activate();
	menu.add(IDM_Exit,doexit);
	menu.add(IDM_Print,doprint);
	menu.add(IDM_Laserjet,dolaserjet);
	menu.add(IDM_Wave,dowave);
	menu.add(IDM_Apple,doapple);
	menu.add(IDM_Options,dooptions);
	menu.add(IDM_Ball,doball);
	menu.add(IDM_General,dogeneral);
	menu.add(IDM_Function,dofunction);
	program.loop();
	delete bitmap;
	return 0;
}

