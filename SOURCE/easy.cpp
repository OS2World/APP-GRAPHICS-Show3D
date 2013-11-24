#include "easy.h"

#include <math.h>

static int window_invalid=0;
static StandardWindow *actwindow;

// ******************* Debug tools **********************

void dumplong (long n)
{	char s[256];
	sprintf(s,"%ld, %lx",n,n);
	WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,s,"Dump",0,
		MB_OK);
}

void dump (char *s)
{	WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,s,"Dump",0,
		MB_OK);
}

// *********** Messages etc. ***************************

void Message (char *s, char *title)
{	WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,s,title,0,
		MB_OK|MB_INFORMATION);
}

void Warning (char *s, char *title)
{	WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,s,title,0,
		MB_OK|MB_WARNING);
}

int Question (char *s, char *title)
{	return WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,s,title,0,
		MB_YESNO|MB_QUERY);
}

int QuestionAbort (char *s, char *title)
{	return WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,s,title,0,
		MB_YESNOCANCEL|MB_QUERY);
}

void Beep (int f, double sec)
{	DosBeep(f,sec*1000);
}

// **************** Threads **********************

void ThreadProc (Thread *s)
{	int ret=s->call();
	s->finished();
	DosExit(EXIT_THREAD,ret);
}

void Thread::start (Parameter p)
{   stop();
	P=p;
	Started=1; Expected=0;
	DosCreateThread(&Tid,(PFNTHREAD)ThreadProc,
		Parameter(this),0,Stacksize);
}

void Thread::stop ()
{	if (Started) DosKillThread(Tid);
	Started=0; Expected=0;
}

void Thread::suspend ()
{	if (Started) DosSuspendThread(Tid);
}

void Thread::resume ()
{	if (Started) DosResumeThread(Tid);
}

void Thread::wait ()
{   TID tid=Tid;
	Expected=1;
	if (Started) DosWaitThread(&tid,DCWW_WAIT);
}

// **************** Time *************************

void Time::set ()
{   long sec;
	DosQuerySysInfo(QSV_TIME_LOW,QSV_TIME_LOW,
		&sec,sizeof(ULONG));
	Seconds=sec;
}

// ********************* Strings ************************

long String::defaultsize=256;

String::String (char *text, LONG size)
{   Size=strlen(text)+1;
	if (Size<size+1) Size=size+1;
	P=new char[Size];
	strcpy(P,text);
}

String::String (int id)
{   char s[1024];
	WinLoadString(program.hab(),NULLHANDLE,id,1022,s);
	Size=strlen(s);
	P=new char[Size+1];
	strcpy(P,s);
}


String::String ()
{   Size=defaultsize;
	P=new char[Size+1];
	*P=0;
}

String::~String ()
{	delete P;
}

void String::copy (char *text, long size)
{	delete P;
	Size=strlen(text)+1;
	if (Size<size+1) Size=size+1;
	P=new char[Size+1];
	strcpy(P,text);
}

void String::copy (char *text)
{	delete P;
	Size=strlen(text)+1;
	P=new char[Size+1];
	strcpy(P,text);
}

char *String::filename ()
{	char *p=P+strlen(P);
	while (p>P)
	{	if (*p=='\\' || *p==':') return p+1;
		p--;
	}
	return p;
}

void String::stripfilename ()
{	*filename()=0;
}

int String::todouble (double &x)
{	int n;
	if (sscanf(P,"%lg%n",&x,&n)==0) return 0;
	char *p=P+n;
	if (*p) return 0;
	else return 1;
}

int String::tolong (long &x)
{	int n;
	if (sscanf(P,"%ld%n",&x,&n)==0) return 0;
	char *p=P+n;
	if (*p) return 0;
	else return 1;
}

// ****************** Rectangle *************************

void Rectangle::minsize (LONG wmin, LONG hmin)
{	if (W<0 && -W<wmin) W=-wmin;
	else if (W>=0 && W<wmin) W=wmin;
	if (H<0 && -H<hmin) H=-hmin;
	else if (H>=0 && H<hmin) H=hmin;
}

void Rectangle::hrescale (double scale)
{	H=(LONG)(scale*abs(W))*(H<0?-1:1);
}

void Rectangle::wrescale (double scale)
{	W=(LONG)(scale*abs(H))*(W<0?-1:1);
}

// ******************** Program *************************

MRESULT EXPENTRY MainWindowProc (HWND hwnd, ULONG msg,
	MPARAM mp1, MPARAM mp2)
{	StandardWindow *window;
	clicktype click;
	if (window_invalid) window=actwindow;
	else window=(StandardWindow *)WinQueryWindowPtr(hwnd,QWL_USER);
	POINTS *points;
	switch (msg)
	{   case WM_CREATE :
			window->Handle=hwnd;
			break;
		case WM_PRESPARAMCHANGED :
			window->pres_changed();
			goto def;
		case WM_PAINT :
			{	RedrawPS ps(hwnd,*window);
				window->redraw(ps);
			}
			break;
		case WM_SIZE :
			if (SHORT1FROMMP(mp2)==0) goto def;
			if (window->Width==SHORT1FROMMP(mp2) &&
				window->Height==SHORT2FROMMP(mp2)) goto def;
			window->Width=SHORT1FROMMP(mp2);
			window->Height=SHORT2FROMMP(mp2);
			window->sized();
			break;
		case WM_CHAR :
			if (window->key(SHORT1FROMMP(mp1),SHORT1FROMMP(mp2),
				SHORT2FROMMP(mp2))) break;
			goto def;
		case WM_HSCROLL :
			if (window->hscrolled(SHORT2FROMMP(mp2),SHORT1FROMMP(mp2)))
				break;
			goto def;
		case WM_VSCROLL :
			if (window->vscrolled(SHORT2FROMMP(mp2),SHORT1FROMMP(mp2)))
				break;
			goto def;
		case WM_BUTTON1CLICK :
			click=button1; goto button;
		case WM_BUTTON2CLICK :
			click=button2; goto button;
		case WM_BUTTON3CLICK :
			click=button3; goto button;
		case WM_BUTTON1UP :
			click=button1up; goto button;
		case WM_BUTTON2UP :
			click=button2up; goto button;
		case WM_BUTTON3UP :
			click=button3up; goto button;
		case WM_BUTTON1DOWN :
			click=button1down; goto button;
		case WM_BUTTON2DOWN :
			click=button2down; goto button;
		case WM_BUTTON3DOWN :
			click=button3down; goto button;
		case WM_BUTTON1DBLCLK :
			click=button1double; goto button;
		case WM_BUTTON2DBLCLK :
			click=button2double; goto button;
		case WM_MOUSEMOVE :
			WinSetPointer(HWND_DESKTOP,window->New);
			click=mousemove; goto button;
		case WM_BUTTON3DBLCLK :
			click=button3double;
			button: points=(POINTS *)&mp1;
			window->clicked(points->x,points->y,click);
			break;
		case WM_COMMAND :
			if (window->Windowmenu)
				if (window->Windowmenu->call(SHORT1FROMMP(mp1)))
					break;
			goto def;
		case WM_SHOW :
			window->Visible=SHORT1FROMMP(mp1);
			goto def;
		case WM_CLOSE :
			if (window->close()) goto def;
			break;
		case WM_TIMER :
			window->timer(SHORT1FROMMP(mp1));
			break;
		default :
			def: return WinDefWindowProc(hwnd,msg,mp1,mp2);
	}
	return (MRESULT)FALSE;
}

Program::Program ()
{ 	Hab=WinInitialize(0);
	Hmq=WinCreateMsgQueue(Hab,0);
	WinRegisterClass(Hab,"EasyMainWindow",MainWindowProc,
		CS_SIZEREDRAW,sizeof(StandardWindow *));
}

Program::~Program ()
{	WinDestroyMsgQueue(Hmq);
	WinTerminate(Hab);
}

inline int Program::getmessage ()
{	return WinGetMsg(Hab,&Qmsg,0L,0,0);
}

inline void Program::dispatch ()
{	WinDispatchMsg(Hab,&Qmsg);
}

void Program::loop ()
{	while(getmessage())
		dispatch();
}

// ************** Window ******************************

Window::Window ()
{	New=Old=WinQuerySysPointer(HWND_DESKTOP,pointer_arrow,TRUE);
}

void Window::size (long w, long h)
{	WinSetWindowPos(Handle,NULLHANDLE,0,0,
		w,h,SWP_SIZE);
}

void Window::pos (long x, long y)
{	WinSetWindowPos(Handle,NULLHANDLE,x,y,
		0,0,SWP_MOVE);
}

void Window::setpointer (pointertype p)
{	New=WinQuerySysPointer(HWND_DESKTOP,p,FALSE);
	WinSetPointer(HWND_DESKTOP,New);
}

void Window::resetpointer ()
{	WinSetPointer(HWND_DESKTOP,Old);
	New=Old;
}

void Window::showpointer ()
{	WinShowPointer(HWND_DESKTOP,TRUE);
}

void Window::hidepointer ()
{	WinShowPointer(HWND_DESKTOP,FALSE);
}

void Window::pos (long &x, long &y, long &w, long &h)
{   SWP p;
	WinQueryWindowPos(Handle,&p);
	x=p.x; y=p.y; w=p.cx; h=p.cy;
}

// ************* Standard Window ***********************

void StandardWindow::size (LONG w, LONG h)
{   RECTL r;
	r.xLeft=r.yBottom=0; r.xRight=w; r.yTop=h;
	WinCalcFrameRect(FrameHandle,&r,FALSE);
	WinSetWindowPos(FrameHandle,NULLHANDLE,0,0,
		r.xRight-r.xLeft,r.yTop-r.yBottom,SWP_SIZE);
}

void StandardWindow::init ()
{   window_invalid=1;
	actwindow=this;
	FrameHandle=WinCreateStdWindow(HWND_DESKTOP,
		WS_VISIBLE,&Flags,"EasyMainWindow",
		(char *)Name,WS_VISIBLE,(HMODULE)0,Id,&Handle);
	WinSetWindowPtr(Handle,QWL_USER,this);
	window_invalid=0;
	Windowmenu=NULL;
	Visible=1;
}

StandardWindow::~StandardWindow ()
{	WinDestroyWindow(FrameHandle);
}

StandardWindow::StandardWindow (int id, char *name, ULONG flags)
	: Name(name)
{	Id=id; Flags=flags;
}

void StandardWindow::top ()
{	WinSetWindowPos(FrameHandle,
		HWND_TOP,0,0,0,0,SWP_ZORDER);
	WinSetFocus(HWND_DESKTOP,Handle);
}

int StandardWindow::rubberbox (LONG x, LONG y, clicktype click,
	Rectangle &R, LONG wmin, LONG hmin, double wscale, double hscale)
{   WindowPS ps(*this);
	ps.mix(FM_XOR);
	switch (click)
	{	case button1down :
			capture();
			Rubber=1;
			R=Rectangle(x,y,wmin,hmin);
			ps.frame(R,0,CLR_GREEN);
			return RUBBER_START;
		case mousemove :
			if (!Rubber) break;
			ps.frame(R,0,CLR_GREEN);
			R.resize(x-R.x(),y-R.y());
			R.minsize(hmin,wmin);
			if (wscale>0) R.wrescale(wscale);
			if (hscale>0) R.hrescale(hscale);
			ps.frame(R,0,CLR_GREEN);
			Rubber=2;
			break;
		case button1up :
			capture(0);
			if (!Rubber || Rubber==1) return RUBBER_CANCEL;
			ps.frame(R,0,CLR_GREEN);
			Rubber=0;
			if (abs(y-R.y())>=hmin) return RUBBER_DONE;
			else return RUBBER_CANCEL;
	}
	return RUBBER_ZERO;
}

void StandardWindow::vscroll (int pos, int min, int max)
{	HWND h=WinWindowFromID(FrameHandle,FID_VERTSCROLL);
	WinSendMsg(h,SBM_SETSCROLLBAR,MPFROMSHORT(pos),
		MPFROM2SHORT(min,max));
}

void StandardWindow::hscroll (int pos, int min, int max)
{	HWND h=WinWindowFromID(FrameHandle,FID_HORZSCROLL);
	WinSendMsg(h,SBM_SETSCROLLBAR,MPFROMSHORT(pos),
		MPFROM2SHORT(min,max));
}

void StandardWindow::title (char *s)
{	HWND h=WinWindowFromID(FrameHandle,FID_TITLEBAR);
	WinSetWindowText(h,s);
}

// ************* Menus ************************************

Menu::~Menu ()
{	while (Mp)
	{	delete Mp;
		Mp=Mp->next();
	}
}

int Menu::call (int command)
{	Menuentry *m=Mp;
	Command=command;
	while (m)
		if (m->command()==command)
		{	m->call(command);
			return 1;
		}
		else m=m->next();
	return 0;
}

void Menu::enable (int id, int flag)
{	HWND handle=WinWindowFromID(W->framehandle(),FID_MENU);
	WinSendMsg(handle,MM_SETITEMATTR,
		MPFROM2SHORT(id,TRUE),MPFROM2SHORT(MIA_DISABLED,
		flag?0:MIA_DISABLED));
}

void Menu::check (int id, int flag)
{	HWND handle=WinWindowFromID(W->framehandle(),FID_MENU);
	WinSendMsg(handle,MM_SETITEMATTR,
		MPFROM2SHORT(id,TRUE),MPFROM2SHORT(MIA_CHECKED,
		flag?MIA_CHECKED:0));
}

// ************* Presentation Space class (PS) *************

void PS::clip (long x, long y, long w, long h)
{	RECTL r;
	r.xLeft=x+X; r.xRight=x+X+w-1; r.yBottom=y+Y; r.yTop=y+Y+h-1;
	GpiIntersectClipRectangle(Handle,&r);
	S.cx=w; S.cy=h;
}

void PS::setcolor (int index, Rgb rgb, int pure)
{	LONG table[1];
	table[0]=rgb;
	GpiCreateLogColorTable(Handle,pure?LCOL_PURECOLOR:0,
				LCOLF_CONSECRGB,index,1,(PLONG)table);
}

void PS::directcolor (int pure)
{	GpiCreateLogColorTable(Handle,pure?LCOL_PURECOLOR:0,
				LCOLF_RGB,0,0,NULL);
}

void PS::defaultcolors ()
{	GpiCreateLogColorTable(Handle,LCOL_RESET,0,0,0,NULL);
}

void PS::move (LONG c, LONG r, ULONG col)
{   color(col);
	P.x=c+X; P.y=r+Y; GpiMove(Handle,&P);
}

void PS::linerel (LONG w, LONG h, ULONG col)
{   color(col);
	P.x+=w; P.y+=h; GpiLine(Handle,&P);
}

void PS::lineto (LONG c, LONG r, ULONG col)
{   color(col);
	P.x=c+X; P.y=r+Y; GpiLine(Handle,&P);
}

void PS::point (LONG c, LONG r, ULONG col)
{   color(col);
	P.x=c+X; P.y=r+Y; GpiSetPel(Handle,&P);
}

void PS::text (char *s, ULONG col, ULONG al, ULONG val)
{	color(col);
	GpiSetTextAlignment(Handle,al,val);
	GpiCharString(Handle,strlen(s),s);
}

void PS::textbox (char *text, long &width, long &height)
{   POINTL pts[4];
	GpiQueryTextBox(Handle,strlen(text),text,4,pts);
	width=pts[2].x-pts[0].x;
	height=pts[2].y-pts[0].y;
}

double PS::textextend (char *text, double vx, double vy)
{   POINTL pts[4];
	int i;
	double emin,emax,r,h;
	GpiQueryTextBox(Handle,strlen(text),text,4,pts);
	r=sqrt(vx*vx+vy*vy); vx/=r; vy/=r;
	pts[0].x--; pts[1].x--;
	pts[2].x++; pts[2].x++;
	emin=emax=vx*pts[0].x+vy*pts[0].y;
	for (i=1; i<4; i++)
	{	h=vx*pts[i].x+vy*pts[i].y;
		if (h<emin) emin=h;
		else if (h>emax) emax=h;
	}
	return emax-emin;
}

void PS::frame (LONG w, LONG h, int r, ULONG col)
{	color(col);
	P.x+=w+1; P.y+=h+1; GpiBox(Handle,DRO_OUTLINE,&P,r,r);
}

void PS::area (LONG w, LONG h, int r, ULONG col)
{	color(col);
	GpiSetBackMix(Handle,BM_OVERPAINT);
	GpiSetPattern(Handle,PATSYM_SOLID);
	P.x+=w+1; P.y+=h+1; GpiBox(Handle,DRO_FILL,&P,r,r);
}

void PS::framedarea (LONG w, LONG h, int r, ULONG col)
{	color(col);
	GpiSetBackMix(Handle,BM_OVERPAINT);
	GpiSetPattern(Handle,PATSYM_BLANK);
	P.x+=w+1; P.y+=h+1; GpiBox(Handle,DRO_OUTLINEFILL,&P,r,r);
}

void PS::frame (Rectangle &R, int r, ULONG col)
{	move(R.x1(),R.y1());
	frame(abs(R.w()),abs(R.h()),r,col);
}

void PS::area (Rectangle &R, int r, ULONG col)
{	move(R.x1(),R.y1());
	area(abs(R.w()),abs(R.h()),r,col);
}

void PS::framedarea (Rectangle &R, int r, ULONG col)
{	move(R.x1(),R.y1());
	framedarea(abs(R.w()),abs(R.h()),r,col);
}

void PS::mark (LONG c, LONG r, ULONG type, ULONG col)
{   color(col);
	GpiSetMarker(Handle,type);
	P.x=c+X; P.y=r+Y; GpiMarker(Handle,&P);
}

void PS::circle (LONG c, LONG r, LONG rad, double factor, ULONG col)
{   ARCPARAMS p;
	p.lP=rad; p.lS=0;
	p.lQ=rad*factor; p.lR=0;
	GpiSetArcParams(Handle,&p);
	move(c,r);
	color(col);
	GpiFullArc(Handle,DRO_OUTLINE,65536l);
}

void PS::arc (LONG c, LONG r, LONG rad, double factor,
	double phi1, double phi2, ULONG col)
{   ARCPARAMS p;
	p.lP=rad; p.lS=0;
	p.lQ=rad*factor; p.lR=0;
	GpiSetArcParams(Handle,&p);
	move(c,r);
	color(col);
	double delta;
	if (phi1<0) phi1+=360;
	if (phi2<0) phi2+=360;
	if (phi2>=phi1) delta=phi2-phi1;
	else delta=phi2+360-phi1;
	GpiSetLineType(Handle,LINETYPE_INVISIBLE);
	GpiPartialArc(Handle,&P,65536l,phi1*65536l,0);
	GpiSetLineType(Handle,LINETYPE_SOLID);
	GpiPartialArc(Handle,&P,65536l,
		phi1*65536l,delta*65536l);
}

void PS::setfont (Font &font, int id)
{   SIZEF s;
	double f;
	GpiCreateLogFont(Handle,NULL,id,font.fat());
	GpiSetCharSet(Handle,id);
	if (font.pointsize()!=font.nominalsize())
	{	GpiQueryCharBox(Handle,&s);
		f=font.pointsize()/font.nominalsize();
		s.cx*=f; s.cy*=f;
		GpiSetCharBox(Handle,&s);
	}
}

void PS::image (long w, long h, unsigned char *data)
{   SIZEL s;
	s.cx=w; s.cy=h;
	GpiImage(Handle,0,&s,((w-1)/8+1)*h,data);
}

//************ Bitmaps Presentation Spaces *****************

BitmapPS::BitmapPS (Window &window)
{	PSZ pszData[4]={"Display",NULL,NULL,NULL};
	BITMAPINFOHEADER2 bmp;
	LONG alData[2],size;
	DeviceHandle=DevOpenDC(program.hab(),OD_MEMORY,"*",4,
		(PDEVOPENDATA)pszData,NULLHANDLE);
	S.cx=window.width(); S.cy=window.height();
	Handle=GpiCreatePS(program.hab(),DeviceHandle,&S,
		PU_PELS|GPIA_ASSOC|GPIT_MICRO);
	GpiQueryDeviceBitmapFormats(Handle,2,(PLONG)alData);
	bmp.cbFix=(ULONG)sizeof(BITMAPINFOHEADER2);
	bmp.cx=S.cx; bmp.cy=S.cy;
	Planes=bmp.cPlanes=alData[0];
	Colorbits=bmp.cBitCount=alData[1];
	bmp.ulCompression=BCA_UNCOMP;
	bmp.cbImage=(((S.cx*(1<<bmp.cPlanes)*(1<<bmp.cBitCount))+31)
		/32)*S.cy;
	bmp.cxResolution=70; bmp.cyResolution=70;
	bmp.cclrUsed=2; bmp.cclrImportant=0;
	bmp.usUnits=BRU_METRIC; bmp.usReserved=0;
	bmp.usRecording=BRA_BOTTOMUP; bmp.usRendering=BRH_NOTHALFTONED;
	bmp.cSize1=0; bmp.cSize2=0;
	bmp.ulColorEncoding=BCE_RGB; bmp.ulIdentifier=0;
	size=sizeof(BITMAPINFO2)+
		(sizeof(RGB2)*(1<<bmp.cPlanes)*(1<<bmp.cBitCount));
	if (DosAllocMem((PVOID *)&Info,size,PAG_COMMIT|PAG_READ|PAG_WRITE))
	{	Valid=0;
		GpiDestroyPS(Handle);
		DevCloseDC(DeviceHandle);
	}
	else Valid=1;
	Info->cbFix=bmp.cbFix;
	Info->cx=bmp.cx; Info->cy=bmp.cy;
	Info->cPlanes=bmp.cPlanes; Info->cBitCount=bmp.cBitCount;
	Info->ulCompression=BCA_UNCOMP;
	Info->cbImage=((S.cx+31)/32)*S.cy;
	Info->cxResolution=70; Info->cyResolution=70;
	Info->cclrUsed=2; Info->cclrImportant=0;
	Info->usUnits=BRU_METRIC;
	Info->usReserved=0;
	Info->usRecording=BRA_BOTTOMUP;
	Info->usRendering=BRH_NOTHALFTONED;
	Info->cSize1=0; Info->cSize2=0;
	Info->ulColorEncoding=BCE_RGB; Info->ulIdentifier=0;
	BitmapHandle=GpiCreateBitmap(Handle,&bmp,FALSE,NULL,Info);
	GpiSetBitmap(Handle,BitmapHandle);
	GpiErase(Handle);
}

BitmapPS::BitmapPS (long w, long h)
{	PSZ pszData[4]={"Display",NULL,NULL,NULL};
	BITMAPINFOHEADER2 bmp;
	LONG alData[2],size;
	DeviceHandle=DevOpenDC(program.hab(),OD_MEMORY,"*",4,
		(PDEVOPENDATA)pszData,NULLHANDLE);
	S.cx=w; S.cy=h;
	Handle=GpiCreatePS(program.hab(),DeviceHandle,&S,
		PU_PELS|GPIA_ASSOC|GPIT_MICRO);
	GpiQueryDeviceBitmapFormats(Handle,2,(PLONG)alData);
	bmp.cbFix=(ULONG)sizeof(BITMAPINFOHEADER2);
	bmp.cx=S.cx; bmp.cy=S.cy;
	Planes=bmp.cPlanes=alData[0];
	Colorbits=bmp.cBitCount=alData[1];
	bmp.ulCompression=BCA_UNCOMP;
	bmp.cbImage=(((S.cx*(1<<bmp.cPlanes)*(1<<bmp.cBitCount))+31)
		/32)*S.cy;
	bmp.cxResolution=70; bmp.cyResolution=70;
	bmp.cclrUsed=2; bmp.cclrImportant=0;
	bmp.usUnits=BRU_METRIC; bmp.usReserved=0;
	bmp.usRecording=BRA_BOTTOMUP; bmp.usRendering=BRH_NOTHALFTONED;
	bmp.cSize1=0; bmp.cSize2=0;
	bmp.ulColorEncoding=BCE_RGB; bmp.ulIdentifier=0;
	size=sizeof(BITMAPINFO2)+
		(sizeof(RGB2)*(1<<bmp.cPlanes)*(1<<bmp.cBitCount));
	if (DosAllocMem((PVOID *)&Info,size,PAG_COMMIT|PAG_READ|PAG_WRITE))
	{	Valid=0;
		GpiDestroyPS(Handle);
		DevCloseDC(DeviceHandle);
	}
	else Valid=1;
	Info->cbFix=bmp.cbFix;
	Info->cx=bmp.cx; Info->cy=bmp.cy;
	Info->cPlanes=bmp.cPlanes; Info->cBitCount=bmp.cBitCount;
	Info->ulCompression=BCA_UNCOMP;
	Info->cbImage=((S.cx+31)/32)*S.cy;
	Info->cxResolution=70; Info->cyResolution=70;
	Info->cclrUsed=2; Info->cclrImportant=0;
	Info->usUnits=BRU_METRIC;
	Info->usReserved=0;
	Info->usRecording=BRA_BOTTOMUP;
	Info->usRendering=BRH_NOTHALFTONED;
	Info->cSize1=0; Info->cSize2=0;
	Info->ulColorEncoding=BCE_RGB; Info->ulIdentifier=0;
	BitmapHandle=GpiCreateBitmap(Handle,&bmp,FALSE,NULL,Info);
	GpiSetBitmap(Handle,BitmapHandle);
	GpiErase(Handle);
}

BitmapPS::~BitmapPS ()
{   if (!Valid) return;
	GpiDeleteBitmap(BitmapHandle);
	DosFreeMem(Info);
	GpiDestroyPS(Handle);
	DevCloseDC(DeviceHandle);
}

void BitmapPS::copy (PS &ps, int mode, long x, long y)
{	POINTL a[4];
	RECTL r;
	r.xLeft=a[0].x=x; r.yBottom=a[0].y=y;
	r.xRight=a[1].x=x+S.cx; r.yTop=a[1].y=y+S.cy;
	a[2].x=0; a[2].y=0;
	if (!GpiRectVisible(ps.handle(),&r)) return;
	GpiBitBlt(ps.handle(),Handle,3,a,mode,BBO_IGNORE);
}

void BitmapPS::save (char *filename)
{	FILE *f;
	BITMAPFILEHEADER2 bfh;
	char *buffer;
	PBITMAPINFO2 pbmi;
	long size,colorsize,hdsize=20;
	ULONG compression=BCA_UNCOMP;
	f=fopen(filename,"wb");
	if (!f)
	{   Warning("Could not open\nthat file.","Save Error");
		return;
	}
	bfh.usType=BFT_BMAP;
	colorsize=((Colorbits<=8)?(1<<Colorbits):256)*sizeof(RGB2);
	size=((width()*Colorbits+31)/32)*4*height();
	bfh.offBits=14+hdsize+colorsize;
	bfh.cbSize=14+hdsize+colorsize+size;
	buffer=(char *)malloc(size);
	pbmi=(PBITMAPINFO2)malloc(16+colorsize);
	memset(pbmi,0,sizeof(BITMAPINFOHEADER2));
	pbmi->cbFix=hdsize;
	pbmi->cPlanes=1;
	pbmi->cBitCount=Colorbits;
	pbmi->ulCompression=compression;
	if (GpiQueryBitmapBits(handle(),0,height(),buffer,pbmi)!=height())
	{   Warning("Bitmap Save Error!","Error");
		goto end;
	}
	fwrite((char *)&bfh,14,1,f);
	fwrite((char *)pbmi,hdsize+colorsize,1,f);
	fwrite(buffer,1,size,f);
	end: free(pbmi);
	free(buffer);
	fclose(f);
}

// ************ Printer ***********************

Queues::Queues (char *name) : Queues(0),NQueues(0)
{   int Default=0,i;
	all();
	if (NQueues==0) return;
	if (name[0])
	{	for (i=0; i<NQueues; i++)
			if (!strcmp(name,Queues[i].pszName))
			{	Default=i;
				break;
			}
	}
	memmove(&ChosenQueue,Queues+Default,sizeof(PRQINFO3));
	delete Queues;
	Queues=0;
}

PRQINFO3 *Queues::all ()
{   ULONG ret,size;
	if (Queues) delete Queues;
	SplEnumQueue(NULL,3,Queues,0,
		&ret,&NQueues,&size,NULL);
	if (NQueues==0) return 0;
	Queues=(PRQINFO3 *)(new char[size]);
	SplEnumQueue(NULL,3,Queues,size,
		&ret,&NQueues,&size,NULL);
	return Queues;
}

PrinterPS::PrinterPS (Queues &q, char *name, int op)
	: Valid(0),Myname(name),Open(0)
{	memmove(&Queue,q.chosen(),sizeof(PRQINFO3));
	Valid.set();
	if (op) open();
}

PrinterPS::PrinterPS (char *name, int op)
	: Valid(0),Myname(name),Open(0)
{	Queues queues;
	if (queues.number()<1) return;
	Valid.set();
	memmove(&Queue,queues.chosen(),sizeof(PRQINFO3));
	if (op) open();
}

void PrinterPS::open ()
{   if (Open) return;
	Dos.pszLogAddress=Queue.pszName;
	Dos.pszDriverName=Queue.pszDriverName;
	*strchr(Dos.pszDriverName,'.')=0;
	Dos.pdriv=Queue.pDriverData;
	Dos.pszDataType="PM_Q_STD";
	Dos.pszComment=Myname;
	Dos.pszQueueProcName=Queue.pszPrProc;
	Dos.pszQueueProcParams=NULL;
	Dos.pszSpoolerParams=NULL;
	Dos.pszNetworkParams=NULL;
	HandlePrinter=DevOpenDC(program.hab(),OD_QUEUED,"*",9,
		(PDEVOPENDATA)&Dos,(HDC)0);
	if (HandlePrinter==DEV_ERROR)
	{   Valid.clear(); return;
	}
	Handle=GpiCreatePS(program.hab(),HandlePrinter,&S,
		PU_PELS|GPIA_ASSOC);
	if (DevEscape(HandlePrinter,DEVESC_STARTDOC,0,0,0,0)!=DEV_OK)
	{	Valid.clear(); return;
	}
	LONG alArray [CAPS_WIDTH_IN_CHARS];
	DevQueryCaps(HandlePrinter,CAPS_FAMILY,CAPS_WIDTH_IN_CHARS,
		(PLONG)alArray);
	S.cx=alArray[CAPS_WIDTH];
	S.cy=alArray[CAPS_HEIGHT];
	Open.set();
}

void PrinterPS::close ()
{   if (!Open) return;
	DevEscape(HandlePrinter,DEVESC_ENDDOC,0,0,0,0);
	DevCloseDC(HandlePrinter);
	GpiDestroyPS(Handle);
	Open.clear();
}

// ************* Metafiles **********************

MetafilePS::MetafilePS (Window &window)
{   DEVOPENSTRUC dos;
	DEVOPENSTRUC dop;
	char *p;
	dop.pszLogAddress=(PSZ)NULL;
	dop.pszDriverName="DISPLAY";
	Hdc=DevOpenDC(program.hab(),
		OD_METAFILE,"*",2L,(PDEVOPENDATA)&dop,0);
	S.cx=window.width(); S.cy=window.height();
	Handle=GpiCreatePS(program.hab(),Hdc,&S,PU_PELS|GPIA_ASSOC);
}

MetafilePS::~MetafilePS ()
{	close();
}

void MetafilePS::close ()
{   if (Handle==NULLHANDLE) return;
	GpiAssociate(Handle,NULLHANDLE);
	GpiDestroyPS(Handle);
	Metafilehandle=DevCloseDC(Hdc);
	Handle=NULLHANDLE;
}

//***************** Bitmap Resources ***********************

Bitmap::Bitmap (int id, int width, int height)
{	PSZ pszData[4]={"Display",NULL,NULL,NULL};
	DeviceHandle=DevOpenDC(program.hab(),OD_MEMORY,"*",4,
		(PDEVOPENDATA)pszData,NULLHANDLE);
	S.cx=width; S.cy=height;
	PsHandle=GpiCreatePS(program.hab(),DeviceHandle,&S,
		PU_PELS|GPIA_ASSOC|GPIT_MICRO);
	Handle=GpiLoadBitmap(PsHandle,0,id,0,0);
}

Bitmap::~Bitmap ()
{   GpiDestroyPS(PsHandle);
	DevCloseDC(DeviceHandle);
	GpiDeleteBitmap(Handle);
}

//******************* Help ******************************

Help::Help (StandardWindow &window,
	int id, char *filename, char *title)
{	HELPINIT hini;
	hini.cb=sizeof(HELPINIT);
	hini.ulReturnCode=0L;
	hini.pszTutorialName=(PSZ)NULL;
	hini.phtHelpTable=
		(PHELPTABLE)MAKELONG(id,0xFFFF);
	hini.hmodHelpTableModule=(HMODULE)0;
	hini.hmodAccelActionBarModule=(HMODULE)0;
	hini.idAccelTable=0;
	hini.idActionBar=0;
	hini.pszHelpWindowTitle=title;
	hini.fShowPanelId = CMIC_HIDE_PANEL_ID;
	hini.pszHelpLibraryName=filename;
	Handle=WinCreateHelpInstance(program.hab(),&hini);
	if (!Handle) { Valid=0; return; }
	Valid=1;
	WinAssociateHelpInstance(Handle,window.framehandle());
}

// ****************** Dialogs *************************

static Dialog *activedlg,*dlgs=0;

static MRESULT EXPENTRY dialogproc (HWND hwnd, ULONG msg,
	MPARAM mp1, MPARAM mp2)
{   Dialogitem *item;
	Dialog *dlg,*dlg1;
	int result;
	if (!activedlg) // search dialog using window handle
	{	dlg=dlgs;
		while (dlg)
		{	if (dlg->Handle==hwnd) break;
			dlg=dlg->Next;
		}
		if (!dlg) // should not happen
			return WinDefDlgProc(hwnd,msg,mp1,mp2);
	}
	else dlg=activedlg;
	switch (msg)
	{   case WM_INITDLG : // initialize activedlg
			activedlg->Handle=hwnd;
			item=activedlg->Items;
			while (item)
			{	item->init();
				item=item->next();
			}
			activedlg->start();
			// put dialog into dialog chain
			activedlg->Next=dlgs;
			dlgs=activedlg;
			activedlg=0; // activedlg unusable
			break;
		case WM_CHAR :
			if (dlg->key(SHORT1FROMMP(mp1),SHORT1FROMMP(mp2),
				SHORT2FROMMP(mp2))) break;
			goto def;
		case WM_HELP :
			if (dlg->H) dlg->H->display(dlg->Hid);
			break;
		case WM_CONTROL :
			result=dlg->Result=(SHORT1FROMMP(mp1));
			item=dlg->Items;
			while (item)
			{	if (item->id()==result)
				{	item->command(mp1,mp2);
					return 0;
				}
				item=item->next();
			}
			break;
		case WM_DESTROY :
			result=-1;
			goto finish;
		case WM_COMMAND :
			result=dlg->Result=(SHORT1FROMMP(mp1));
			item=dlg->Items;
			while (item)
			{	if (item->id()==result)
				{	item->command(mp1,mp2);
					return 0;
				}
				item=item->next();
			}
			if (dlg->handler(result))
			{   finish :
				item=dlg->Items;
				while (item)
				{	item->exit();
					item=item->next();
				}
				dlg->stop();
				// remove dialog from dialog chain
				if (dlgs==dlg) dlgs=dlg->Next;
				else
				{	dlg1=dlgs;
					while (dlg1)
					{	if (dlg1->Next==dlg) break;
						dlg1=dlg1->Next;
					}
					if (dlg1) dlg1->Next=dlg->Next;
				}
				// close dialog
				WinDismissDlg(hwnd,SHORT1FROMMP(mp1));
			}
			break;
		default :
			def: return WinDefDlgProc(hwnd,msg,mp1,mp2);
	}
	return 0;
}

void Dialog::init (Window &window, int id)
{	W=&window;
	Id=id;
	Items=0;
}

Dialog::Dialog (Window &window, int id, Help &h, int hid) : S()
{	init(window,id);
	H=&h; Hid=hid; Next=0;
}

Dialog::Dialog (Window &window, int id) : S()
{	init(window,id);
	H=0; Next=0;
}

Dialogitem *Dialog::entry (Dialogitem *item)
{	Dialogitem *note=Items;
	Items=item;
	return note;
}

void Dialog::carryout ()
{   activedlg=this;
	Handle=WinDlgBox(HWND_DESKTOP,W->handle(),dialogproc,
		(HMODULE)0,Id,NULL);
}

void Dialog::show ()
{   activedlg=this;
	Handle=WinLoadDlg(HWND_DESKTOP,W->handle(),dialogproc,
		(HMODULE)0,Id,NULL);
}

char *Dialog::gettext (int id, char *text, long size)
{   HWND handle=WinWindowFromID(Handle,id);
	WinQueryWindowText(handle,size,text);
	return text;
}

char *Dialog::gettext (int id)
{   HWND handle=WinWindowFromID(Handle,id);
	WinQueryWindowText(handle,S.size(),(PSZ)S.text());
	return S;
}

void Dialog::settext (int id, char *text)
{	HWND handle=WinWindowFromID(Handle,id);
	WinSetWindowText(handle,text);
}

MRESULT Dialog::message (int id, int msg,
	Parameter mp1, Parameter mp2)
{	HWND h;
	h=WinWindowFromID(Handle,id);
	return WinSendMsg(h,msg,mp1,mp2);
}

//*************** Dialog Items ******************

Dialogitem::Dialogitem (int id, Dialog &dialog)
{	Id=id;
	D=&dialog;
	Next=dialog.entry(this);
}

void StringItem::init ()
{   int size=strlen(S)+1;
	if (Length>size) size=Length;
	limit(size);
	D->settext(Id,S);
	D->message(Id,EM_SETREADONLY,Readonly);
}

void StringItem::exit ()
{	S.copy(D->gettext(Id));
}

void StringItem::limit (int length)
{	D->message(Id,EM_SETTEXTLIMIT,Parameter(length));
}

void CheckItem::init ()
{   D->message(Id,BM_SETCHECK,F);
}

void CheckItem::exit ()
{	F=(long)D->message(Id,BM_QUERYCHECK);
}

void RadioItem::init ()
{   D->message(Ids[I],BM_CLICK,1);
}

void RadioItem::exit ()
{	int i;
	for (i=0; i<N; i++)
	{	if (D->message(Ids[i],BM_SETCHECK))
		{	I=i; return;
		}
	}
}

void DoubleItem::init ()
{	sprintf(S,"%-0.10g",X);
	D->settext(Id,S);
	D->message(Id,EM_SETREADONLY,Readonly);
}

void DoubleItem::exit ()
{ 	S.copy(D->gettext(Id),64);
	sscanf(S,"%lg",&X);
}

void LongItem::init ()
{	sprintf(S,"%-ld",N);
	D->settext(Id,S);
	D->message(Id,EM_SETREADONLY,Readonly);
}

void LongItem::exit ()
{ 	S.copy(D->gettext(Id),64);
	sscanf(S,"%ld",&N);
}

void SpinItem::init ()
{	D->message(Id,SPBM_SETLIMITS,Upper,Lower);
	D->message(Id,SPBM_SETCURRENTVALUE,N);
}

void SpinItem::exit ()
{	D->message(Id,SPBM_QUERYVALUE,&N,Parameter(0,SPBQ_ALWAYSUPDATE));
}

void SliderItem::init ()
{	D->message(Id,SLM_SETSLIDERINFO,
		Parameter(SMA_SLIDERARMPOSITION,SMA_INCREMENTVALUE),
		Parameter(N));
}

void SliderItem::exit ()
{	N=(long)D->message(Id,SLM_QUERYSLIDERINFO,
		Parameter(SMA_SLIDERARMPOSITION,SMA_INCREMENTVALUE));
}

void SliderItem::tick (int n, int size)
{	D->message(Id,SLM_SETTICKSIZE,Parameter(n,size));
}

void SliderItem::label (int n, char *text)
{	D->message(Id,SLM_SETSCALETEXT,Parameter(n),Parameter(text));
}

void ValuesetItem::select (int row, int col)
{	D->message(Id,VM_SELECTITEM,Parameter(row,col));
}

void ValuesetItem::setbitmap (int row, int col, Bitmap &b)
{	D->message(Id,VM_SETITEMATTR,Parameter(row,col),
		Parameter(VIA_BITMAP,1));
	D->message(Id,VM_SETITEM,Parameter(row,col),
		Parameter((long)b.handle()));
}

void ValuesetItem::setcolor (int row, int col, long color)
{	D->message(Id,VM_SETITEMATTR,Parameter(row,col),
		Parameter(VIA_RGB,1));
	WindowPS ps(*D->w());
	color=GpiQueryRGBColor(ps.handle(),0,color);
	D->message(Id,VM_SETITEM,Parameter(row,col),
		Parameter((long)color));
}

void ValuesetItem::exit ()
{	Parameter p(D->message(Id,VM_QUERYSELECTEDITEM,0));
	Col=p.higher(); Row=p.lower();
}

void ValuesetItem::command (Parameter mp1, Parameter mp2)
{	if (mp1.higher()==VN_SELECT)
	{	Row=mp2.lower(); Col=mp2.higher();
		notify();
	}
}

void ListItem::insert (char *text)
{	D->message(Id,LM_INSERTITEM,Parameter(LIT_END),Parameter(text));
}

void ListItem::select (int sel)
{	D->message(Id,LM_SELECTITEM,Parameter(sel),Parameter(1));
	Selection=sel;
}

void ListItem::exit ()
{	Selection=Parameter(D->message(Id,LM_QUERYSELECTION)).lower();
}

void ListItem::command (Parameter mp1, Parameter mp2)
{	if (mp1.higher()==LN_ENTER)
	{   Selection=Parameter(D->message(Id,LM_QUERYSELECTION)).lower();
		notify();
	}
}

void MultilineItem::init ()
{	limit(S.size());
	D->settext(Id,S);
	D->message(Id,MLM_SETREADONLY,Readonly);
}

void MultilineItem::limit (int length)
{	D->message(Id,MLM_SETTEXTLIMIT,Parameter(length));
}

void MultilineItem::exit ()
{	D->gettext(Id,S,S.size());
}

//******************* File Selector ********************

FileSelector::FileSelector (Window &window,
		char *filter, int saving,
		char *title, char *ok) : Filter(filter),Title(title)
{	memset(&Fd,0,sizeof(FILEDLG));
	Fd.cbSize=sizeof(FILEDLG);
	Fd.fl=FDS_CENTER|FDS_ENABLEFILELB
		|(saving?FDS_SAVEAS_DIALOG:FDS_OPEN_DIALOG);
	Fd.pszTitle=(char *)Title;
	if (!ok)
	{	if (saving) Ok.copy("Save");
		else Ok.copy("Load");
	}
	else Ok.copy(ok);
	Fd.pszOKButton=(char *)Ok;
	W=&window;
}

char *FileSelector::select ()
{   char *p=Filter.filename();
	String s(Fd.szFullFile,256);
	s.stripfilename();
	strcpy(Fd.szFullFile,s); strcat(Fd.szFullFile,p);
	Freturn=WinFileDlg(HWND_DESKTOP,W->handle(),&Fd);
	if (!Freturn || Fd.lReturn!=DID_OK) return 0;
	return Fd.szFullFile;
}

// ************ Font Selector *********

int FontSelector::select (Window &window)
{   HPS hps;
	hps=WinGetPS(window.handle());
	memset(&Fd,0,sizeof(FONTDLG));
	Fd.cbSize=sizeof(FONTDLG);
	Fd.hpsScreen=hps;
	Fd.pszFamilyname=Facename;
	Fd.usFamilyBufLen=Facename.size();
	Fd.fxPointSize=long(Pointsize*65536);
	Fd.fl=FNTS_CENTER;
	Fd.clrFore=CLR_BLACK;
	Fd.clrBack=CLR_WHITE;
	Fd.fAttrs.usCodePage=Codepage;
	Result=0;
	WinFontDlg(HWND_DESKTOP,window.handle(),&Fd);
	Result=(Fd.lReturn==DID_OK);
	WinReleasePS(hps);
	return Result;
}

// *************** handling fonts **************************

Fonts::Fonts (PS &ps)
{	Count=0;
	Count=GpiQueryFonts(ps.handle(),
		QF_PUBLIC,0,&Count,sizeof(FONTMETRICS),0);
	AllFonts=new FONTMETRICS[Count];
	GpiQueryFonts(ps.handle(),
		QF_PUBLIC,0,&Count,sizeof(FONTMETRICS),AllFonts);
}

Fonts::~Fonts ()
{	delete AllFonts;
}

Font::Font (FontSelector &fs)
{	memcpy(&Fat,fs.fat(),sizeof(FATTRS));
	PointSize=fs.fd()->fxPointSize/65536.0;
	NominalPointSize=fs.fd()->sNominalPointSize/10.0;
}

// ************ Profile ***************

void Profile::writestring (char *k, char *i)
{	PrfWriteProfileString(P,S,k,i);
}

void Profile::writedouble (char *k, double x)
{	PrfWriteProfileData(P,S,k,&x,sizeof(double));
}

void Profile::writelong (char *k, long x)
{	PrfWriteProfileData(P,S,k,&x,sizeof(long));
}

void Profile::writeint (char *k, int x)
{	PrfWriteProfileData(P,S,k,&x,sizeof(int));
}

char *Profile::readstring (char *k, char *d, long size)
{	H.copy(d,size);
	PrfQueryProfileString(P,S,k,(char *)H,(char *)H,size);
	return H;
}

double Profile::readdouble (char *k, double x)
{   double res;
	unsigned long size=sizeof(double);
	if (PrfQueryProfileData(P,S,k,&res,&size)) return res;
	else return x;
}

long Profile::readlong (char *k, long x)
{   long res;
	unsigned long size=sizeof(long);
	if (PrfQueryProfileData(P,S,k,&res,&size)) return res;
	else return x;
}

int Profile::readint (char *k, int x)
{   int res;
	unsigned long size=sizeof(int);
	if (PrfQueryProfileData(P,S,k,&res,&size)) return res;
	else return x;
}

// ********** Clipboard ***************

void Clipboard::copy (MetafilePS &meta)
{   meta.close();
	if (!WinOpenClipbrd(program.hab())) return;
	WinEmptyClipbrd(program.hab());
	WinSetClipbrdData(program.hab(),
		meta.metafilehandle(),CF_METAFILE,CFI_HANDLE);
	WinCloseClipbrd(program.hab());
}

