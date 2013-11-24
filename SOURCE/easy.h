#define INCL_WIN
#define INCL_GPI
#define INCL_DOSMISC
#define INCL_DOSPROCESS
#define INCL_DEV
#define INCL_SPL
#define INCL_SPLDOSPRINT
#define INCL_BASE
#define INCL_GPIERRORS


#include <os2.h>

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

class Answers
{   public :
	enum { yes=MBID_YES, no=MBID_NO, abort=MBID_CANCEL };
};

void dumplong (long n);
void dump (char *s);
void Warning (char *s, char *title);
void Message (char *s, char *title);
int Question (char *s, char *title);
int QuestionAbort (char *s, char *title);
void Beep (int frequency, double seconds);

enum {RUBBER_ZERO,RUBBER_START,RUBBER_CANCEL,RUBBER_DONE};

class Parameter
{	MPARAM M;
	public :
	Parameter(long m) { M=(MPARAM)m; }
	Parameter(int m1, int m2) { M=MPFROM2SHORT(m1,m2); }
	Parameter(int m) { M=MPFROMSHORT(m); }
	Parameter(void *p) { M=(MPARAM)p; }
	operator MPARAM () { return M; }
	operator LONG () { return (long)M; }
	int lower () { return SHORT1FROMMP(M); }
	int higher () { return SHORT2FROMMP(M); }
};

class Time
{	double Seconds;
	public :
	void set ();
	Time () { set(); }
	double seconds () { return Seconds; }
	operator double () { set(); return Seconds; }
	void sleep (double t) { DosSleep(1000*t); }
};

class String
{	char *P;
	long Size;
	public :
	static long defaultsize;
	String ();
	String (char *text, long size=defaultsize);
	String (int id); // id from Resource
	~String ();
	char *text () { return P; }
	long size () { return Size-1; }
	void copy (char *text, long size);
	void copy (char *text);
	char *filename ();
	void stripfilename ();
	operator char * () { return P; }
	operator PSZ () { return P; }
	int todouble (double &x);
	int tolong (long &n);
};

class ConvertString : public String
{	public :
	ConvertString (long n) : String("",32) { ltoa(n,*this,10); }
	ConvertString (double x) : String("",32)
		{ sprintf(*this,"%-0.10g",x); }
};

class Flag
{	int F,Fix;
	public :
	Flag (int f=0) { F=f; Fix=0; }
	void set () { if (Fix) return; F=1; }
	void clear () { if (Fix) return; F=0; }
	void toggle () { if (Fix) return; F=!F; }
	operator int () { return F; }
	void fix () { Fix=1; }
	void unfix () { Fix=0; }
	int operator = (int flag) { if (Fix) return flag; return F=flag; }
};

class Rectangle
{	LONG X,Y,W,H;
	public :
	Rectangle (LONG x=0, LONG y=0, LONG w=1, LONG h=1)
	{	X=x; Y=y; W=w; H=h;
	}
	LONG x1 ()
	{	if (W<0) return X+W+1;
		else return X;
	}
	LONG x2 ()
	{	if (W>0) return X+W-1;
		else return X;
	}
	LONG y1 ()
	{	if (H<0) return Y+H+1;
		else return Y;
	}
	LONG y2 ()
	{	if (H>0) return Y+H-1;
		else return Y;
	}
	LONG x () { return X; }
	LONG y () { return Y; }
	LONG w () { return W; }
	LONG h () { return H; }
	void resize (LONG w, LONG h)
	{	W=w; H=h;
	}
	void hrescale (double scale);
	void wrescale (double scale);
	void minsize (LONG wmin, LONG hmin);
};

class Program
{	HAB Hab;
	HMQ Hmq;
	QMSG Qmsg;
	public :
	Program();
	~Program();
	int getmessage();
	void dispatch();
	void loop();
	HAB hab() { return Hab; }
	HMQ hmq() { return Hmq; }
};

extern Program program;

class PS;

const int FCF_NORMAL=
	FCF_TITLEBAR|FCF_SYSMENU|FCF_SIZEBORDER|FCF_MINMAX|
	FCF_SHELLPOSITION|FCF_TASKLIST|FCF_ICON;

class Menu;

enum clicktype
{ 	button1,button2,button3,
	button1double,button2double,button3double,
	button1up,button1down,
	button2up,button2down,
	button3up,button3down,
	mousemove
};

enum pointertype
{	pointer_wait=SPTR_WAIT,pointer_arrow=SPTR_ARROW,
	pointer_text=SPTR_TEXT,pointer_move=SPTR_MOVE
};

class Window
{   protected :
	HWND Handle;
	int Width,Height;
	Flag Visible;
	HPOINTER Old,New;
	public :
	Window ();
	HWND handle () { return Handle; }
	int width () { return Width; }
	int height () { return Height; }
	void update () { WinInvalidateRect(Handle,NULL,TRUE); }
	void size (long w, long h);
	void pos (long x, long y);
	void validate () { WinValidateRect(Handle,NULL,TRUE); }
	int visible () { return Visible; }
	void setpointer (pointertype p);
	void resetpointer ();
	void showpointer ();
	void hidepointer ();
	void message (int msg, Parameter mp1=0, Parameter mp2=0)
	{	WinPostMsg(Handle,msg,mp1,mp2);
	}
	void capture (int flag=1)
	{	WinSetCapture(HWND_DESKTOP,flag?Handle:NULLHANDLE);
	}
	void pos (long &x, long &y, long &w, long &h);
	void quit () { message(WM_CLOSE); }
};

class Scroll
{	public :
	enum {
		left=SB_LINELEFT,right=SB_LINERIGHT,
		pageleft=SB_PAGELEFT,pageright=SB_PAGERIGHT,
		position=SB_SLIDERPOSITION,
		up=SB_LINEUP,down=SB_LINEDOWN,
		pageup=SB_PAGEUP,pagedown=SB_PAGEDOWN};
};

class Alignment
{	public :
	enum {
		left=TA_LEFT,center=TA_CENTER,right=TA_RIGHT,
		top=TA_TOP,middle=TA_HALF,bottom=TA_BOTTOM};
};

class Keycode
{	public :
	enum {
		up=KC_KEYUP,virtualkey=KC_VIRTUALKEY,
		charkey=KC_CHAR };
};

class StandardWindow : public Window
{	HWND FrameHandle;
	Menu *Windowmenu;
	int Id;
	int Rubber;
	String Name;
	ULONG Flags;
	public :
	enum {normal=FCF_NORMAL,menu=FCF_MENU,keys=FCF_ACCELTABLE,
		vscrollbar=FCF_VERTSCROLL,hscrollbar=FCF_HORZSCROLL};
	StandardWindow (int id,
		char *name="",
		ULONG flags=FCF_NORMAL);
	~StandardWindow ();
	void init ();
	void setmenu (Menu *m) { Windowmenu=m; }
	HWND framehandle () { return FrameHandle; }
	void top ();
	int rubberbox (LONG x, LONG y, clicktype click,
		Rectangle &R, LONG wmin=0, LONG hmin=0,
		double wscale=0, double hscale=0);
	virtual void redraw (PS &ps);
	virtual void sized () {}
	virtual void clicked (LONG x, LONG y, clicktype click) {}
	virtual int key (int flags, int code, int scan) { return 0; }
	void size (LONG w, LONG h);
	virtual int vscrolled (int scroll, int pos) { return 0; }
	virtual int hscrolled (int scroll, int pos) { return 0; }
	virtual void pres_changed () { update(); }
	void vscroll (int pos, int min=0, int max=100);
	void hscroll (int pos, int min=0, int max=100);
	friend MRESULT EXPENTRY MainWindowProc (HWND hwnd,
			ULONG msg,MPARAM mp1, MPARAM mp2);
	void title (char *s);
	virtual int close () { return 1; }
		// user tries to close the window
	void starttimer (double interval, int i=1)
	{	WinStartTimer(program.hab(),Handle,i,interval*1000);
	}
	void stoptimer (int i=1)
	{	WinStopTimer(program.hab(),Handle,i);
	}
	virtual void timer (int i) {}
};

typedef void Menuproc ();

class Menuentry
{   int Command;
	Menuproc *Proc;
	Menuentry *Next;
	public :
	Menuentry (int command, Menuproc *proc,
		Menuentry *next=NULL)
	{	Command=command;
		Proc=proc;
		Next=next;
	}
	Menuentry *next () { return Next; }
	void call (int command) { Proc(); }
	int command () { return Command; }
};

class Menu
{   int Command;
	Menuentry *Mp;
	StandardWindow *W;
	public :
	Menu (StandardWindow &window)
	{	Mp=NULL;
		W=&window;
		W->setmenu(this);
	}
	~Menu ();
	void add (ULONG command, Menuproc *proc)
	{   Mp=new Menuentry(command,proc,Mp);
	}
	int call (int command);
	int command () { return Command; }
	void enable (int id, int flag);
	void check (int id, int flag);
};

class Rgb
{	LONG Value;
	public :
	Rgb (int red, int green, int blue)
	{	Value=((unsigned long)red<<16)+((unsigned long)green<<8)+blue;
	}
	Rgb (LONG value) { Value=value; }
	operator LONG () { return Value; }
};

class Marker
{   public :
	enum
	{	def=MARKSYM_DEFAULT,cross=MARKSYM_CROSS,
		circle=MARKSYM_SMALLCIRCLE,diamond=MARKSYM_DIAMOND,
		dot=MARKSYM_DOT,square=MARKSYM_SQUARE,
		plus=MARKSYM_PLUS,star=MARKSYM_EIGHTPOINTSTAR
	};
};

class Colors
{   public :
	enum
	{	def=CLR_DEFAULT,white=CLR_WHITE,black=CLR_BLACK,
		blue=CLR_BLUE,red=CLR_RED,pink=CLR_PINK,green=CLR_GREEN,
		cyan=CLR_CYAN,yellow=CLR_YELLOW,darkgray=CLR_DARKGRAY,
		darkblue=CLR_DARKBLUE,darkred=CLR_DARKRED,
		darkpink=CLR_DARKPINK,darkgreen=CLR_DARKGREEN,
		darkcyan=CLR_DARKCYAN,brown=CLR_BROWN,palegray=CLR_PALEGRAY,
		gray=CLR_PALEGRAY,neutral=CLR_NEUTRAL
	};
};

class Markers
{	public :
	enum
	{	def=MARKSYM_DEFAULT,cross=MARKSYM_CROSS,plus=MARKSYM_PLUS,
		diamond=MARKSYM_DIAMOND,star=MARKSYM_SIXPOINTSTAR,
		square=MARKSYM_SQUARE,solidsquare=MARKSYM_SOLIDSQUARE,
		soliddiamond=MARKSYM_SOLIDDIAMOND,
		sixpointstar=MARKSYM_SIXPOINTSTAR,
		eightpointstart=MARKSYM_EIGHTPOINTSTAR,
		dot=MARKSYM_DOT,circle=MARKSYM_SMALLCIRCLE,
		blank=MARKSYM_BLANK
	};
};

class Font;
class PS
{   POINTL P;
	ULONG Color,Alignment;
	protected :
	HPS Handle;
	SIZEL S;
	long X,Y;
	public :
	PS () : X(0),Y(0)
	{	Handle=NULLHANDLE;
		Color=CLR_DEFAULT; Alignment=TA_LEFT;
	}
	PS (HPS hps)
	{	PS(); Handle=hps; GpiQueryPS(Handle,&S);
	}
	void clip (long x, long y, long w, long h);
	void offset (long x, long y) { X=x; Y=y; }
	long xoffset () { return X; }
	long yoffset () { return Y; }
	HPS handle () { return Handle; }
	void erase () { GpiErase(Handle); }
	LONG width () { return S.cx; }
	LONG height () { return S.cy; }
	void color (ULONG color)
	{	if (Color!=color)
		{	GpiSetColor(Handle,color);
			Color=color;
		}
	}
	void directcolor (int pure=0);
	void setcolor (int index, Rgb rgb, int pure=0);
	void defaultcolors ();
	void mix (int mode) { GpiSetMix(Handle,mode); }
	void move (LONG c, LONG r, ULONG color=CLR_DEFAULT);
	void lineto (LONG c, LONG r, ULONG color=CLR_DEFAULT);
	void linerel (LONG w, LONG h, ULONG color=CLR_DEFAULT);
	void point (LONG w, LONG h, ULONG color=CLR_DEFAULT);
	void text (char *s, ULONG color=CLR_DEFAULT,
		ULONG alignment=TA_LEFT, ULONG valignment=TA_BASE);
	void textbox (char *s, long &width, long &height);
	double textextend (char *s, double vx, double vy);
	void framedarea (Rectangle &R, int r, ULONG col=CLR_DEFAULT);
	void frame (Rectangle &R, int r=0, ULONG color=CLR_DEFAULT);
	void area (Rectangle &R, int r=0, ULONG color=CLR_DEFAULT);
	void framedarea (LONG w, LONG h, int r, ULONG col=CLR_DEFAULT);
	void frame (LONG w, LONG h, int r=0, ULONG color=CLR_DEFAULT);
	void area (LONG w, LONG h, int r=0, ULONG color=CLR_DEFAULT);
	void mark (LONG w, LONG h, ULONG type=MARKSYM_DEFAULT,
		ULONG color=CLR_DEFAULT);
	void circle (LONG c, LONG r, LONG rad, double factor,
		ULONG col=CLR_DEFAULT);
	void arc (LONG c, LONG r, LONG rad, double factor,
		double phi1, double phi2,
		ULONG col=CLR_DEFAULT);
	void setfont (Font &font, int id=1);
	void image (long w, long h, unsigned char *data);
};

inline void StandardWindow::redraw (PS &ps)
{	ps.erase();
}

class WindowPS : public PS
{   Window *W;
	int getps;
	void set (HPS handle, Window &window)
	{	W=&window;
		Handle=handle;
		S.cx=window.width(); S.cy=window.height();
	}
	public :
	WindowPS (HPS handle, Window &window)
	{	set(handle,window);
		getps=0;
	}
	WindowPS (Window &window)
	{	set(WinGetPS(window.handle()),window);
		getps=1;
	}
	~WindowPS () { if (getps) WinReleasePS(handle()); }
};

class RedrawPS : public WindowPS
{	public :
	RedrawPS (HWND hwnd, Window &window) :
		WindowPS(WinBeginPaint(hwnd,NULLHANDLE,NULL),window) {}
	~RedrawPS () { WinEndPaint(handle()); }
};

class MetafilePS : public PS
{   HMF Metafilehandle;
	HDC Hdc;
	public :
	MetafilePS (Window &window);
	~MetafilePS ();
	HMF metafilehandle () { return Metafilehandle; }
	void close ();
};

class CopyMode
{  	public :
	enum {
		copy=ROP_SRCCOPY, or=ROP_SRCPAINT, xor=ROP_SRCINVERT,
	};
};

class BitmapPS : public PS
{	HDC DeviceHandle;
	HBITMAP BitmapHandle;
	PBITMAPINFO2 Info;
	int Valid,Planes,Colorbits;
	public :
	BitmapPS (Window &window);
	BitmapPS (long w, long h);
	~BitmapPS ();
	void copy (PS &ps, int mode=CopyMode::copy, long x=0, long y=0);
	void save (char *filename);
	HBITMAP bitmaphandle () { return BitmapHandle; }
};

class Queues
{	ULONG NQueues;
	PRQINFO3 *Queues;
	PRQINFO3 ChosenQueue;
	public :
	Queues (char *name="");
	~Queues () { if (Queues) delete Queues; }
	ULONG number () { return NQueues; }
	PRQINFO3 *chosen () { return &ChosenQueue; }
	PRQINFO3 *all ();
};

class PrinterPS : public PS
{   HDC HandlePrinter;
	DEVOPENSTRUC Dos;
	String Myname;
	PRQINFO3 Queue;
	public :
	Flag Valid,Open;
	PrinterPS (char *name="Print", int op=1);
	PrinterPS (Queues &q, char *name="Print", int op=1);
	~PrinterPS () { if (Open) close(); }
	char *queuename () { return Dos.pszLogAddress; }
	char *drivername () { return Dos.pszDriverName; }
	void open ();
	void close ();
};

class Bitmap
{	HBITMAP Handle;
	HDC DeviceHandle;
	HPS PsHandle;
	SIZEL S;
	public :
	Bitmap (int id, int width=0, int height=0);
	~Bitmap ();
	HBITMAP handle () { return Handle; }
};

class Help
{   HWND Handle;
	Help *H;
	Flag Valid;
	public :
	Help (StandardWindow &window,
		int id, char *filename, char *title="");
	void Help::general (void)
	{	if (Valid) WinSendMsg(Handle,HM_EXT_HELP,NULL,NULL);
	}
	void Help::index (void)
	{	if (Valid) WinSendMsg(Handle,HM_HELP_INDEX,NULL,NULL);
	}
	void Help::content (void)
	{   if (Valid) WinSendMsg(Handle,HM_HELP_CONTENTS,NULL,NULL);
	}
	void Help::display (int id)
	{	if (Valid) WinSendMsg(Handle,HM_DISPLAY_HELP,
			MPFROMSHORT(id),HM_RESOURCEID);
	}
	int valid () { return Valid; }
};

class Thread
{	int (*F) (Parameter);
	TID Tid;
	int Started,Expected;
	long Stacksize;
	Parameter P;
	public :
	Thread (int (*f) (Parameter), long stacksize=4096) : P(0)
	{	F=f;
		Started=0; Stacksize=stacksize;
		Expected=0;
	}
	void start (Parameter p=0);
	void stop ();
	void suspend ();
	void resume ();
	void wait ();
	int expected () { return Expected; }
	Parameter p () { return P; }
	int call () { return F(P); }
	int started () { return Started; }
	void finished () { Started=0; }
};

class Dialogitem;

class Dialog
{	HWND Handle;
	int Result,Id;
	Window *W;
	String S;
	Dialogitem *Items;
	Help *H;
	int Hid;
	void init (Window &window, int id);
	Dialog *Next;
	public :
	enum { ok=DID_OK,cancel=DID_CANCEL };
	Dialog (Window &window, int id);
	Dialog (Window &window, int id, Help &h, int hid);
	Dialogitem *entry (Dialogitem *item);
	void carryout ();
	void show ();
	virtual void start () {}
	virtual void stop () {}
	virtual int handler (int command) { return 1; }
	int result () { return Result; }
	char *gettext (int id, char *text, long size=String::defaultsize);
	char *gettext (int id);
	void settext (int id, char *text);
	MRESULT message (int id, int msg,
		Parameter mp1=NULL, Parameter mp2=NULL);
	HWND handle () { return Handle; }
	friend MRESULT EXPENTRY dialogproc (HWND hwnd, ULONG msg,
		MPARAM mp1, MPARAM mp2);
	void finish ()
	{	WinSendMsg(Handle,WM_COMMAND,MPARAM(DID_OK),0);
	}
	virtual int key (int flags, int code, int scan) { return 0; }
	Window *w () { return W; }
};

class Dialogitem
{	Dialogitem *Next;
	protected :
	int Id;
	Dialog *D;
	public :
	Dialogitem (int id, Dialog &dialog);
	Dialogitem *next () { return Next; }
	int id () { return Id; }
	virtual void init () {}
	virtual void exit () {}
	virtual void command (Parameter mp1,Parameter mp2) {}
	virtual void notify () {}
	void finish () { D->finish(); }
};

class CheckItem : public Dialogitem
{	int F;
	public :
	CheckItem (int id, Dialog &dialog, int flag)
		: Dialogitem(id,dialog) { F=flag; }
	virtual void init ();
	virtual void exit ();
	void set (int f) { F=f; }
	void reinit (int f) { set(f); init(); }
	operator int () { return F; }
};

class RadioItem : public Dialogitem
{	int I,N,*Ids;
	public :
	RadioItem (int *ids, int n, Dialog &dialog, int i=1)
		: Dialogitem(0,dialog),N(n),I(i),Ids(ids) {}
	virtual void init();
	virtual void exit ();
	void set (int i) { I=i; }
	void reinit (int i) { set(i); init(); }
	operator int () { return I; }
};

class StringItem : public Dialogitem
{   String S;
	int Length;
	Flag Readonly;
	public :
	StringItem (int id, Dialog &dialog, char *string, int length=64)
		: Dialogitem(id,dialog),S(string) { Length=length; }
	virtual void init ();
	virtual void exit ();
	void set (char *text) { S.copy(text); }
	void reinit (char *text) { set(text); init(); }
	void limit (int length);
	void readonly (int flag=1) { Readonly=flag; }
	operator char * () { return S; }
};


class DoubleItem : public Dialogitem
{   double X;
	String S;
	Flag Readonly;
	public :
	DoubleItem (int id, Dialog &dialog, double x)
		: Dialogitem(id,dialog),S("",64) { X=x; }
	virtual void init ();
	virtual void exit ();
	void set (double x) { X=x; }
	void reinit (double x) { set(x); init(); }
	void readonly (int flag=1) { Readonly=flag; }
	operator double () { return X; }
};

class LongItem : public Dialogitem
{   long N;
	String S;
	Flag Readonly;
	public :
	LongItem (int id, Dialog &dialog, long n)
		: Dialogitem(id,dialog),S("",64) { N=n; }
	virtual void init ();
	virtual void exit ();
	void set (long n) { N=n; }
	void reinit (long n) { set(n); init(); }
	void readonly (int flag=1) { Readonly=flag; }
	operator long () { return N; }
};

class SpinItem : public Dialogitem
{   long N,Lower,Upper;
	String S;
	public :
	SpinItem (int id, Dialog &dialog, long n,
		long lower, long upper)
		: Dialogitem(id,dialog),S("",64)
	{	N=n; Lower=lower; Upper=upper;
	}
	virtual void init ();
	virtual void exit ();
	void set (long n, long lower, long upper)
	{	N=n; Lower=lower; Upper=upper; }
	void reinit (long n, long lower, long upper)
	{	set(n,lower,upper); init();
	}
	void set (long n) { N=n; }
	void reinit (long n) { set(n); init(); }
	operator long () { return N; }
};

class SliderItem : public Dialogitem
{   long N;
	String S;
	public :
	SliderItem (int id, Dialog &dialog, long n)
		: Dialogitem(id,dialog),S("",64)
	{	N=n;
	}
	virtual void init ();
	virtual void exit ();
	void set (long n)
	{	N=n; }
	void reinit (long n)
	{	set(n); init();
	}
	void tick (int n, int size=3);
	void label (int n, char *text);
	operator long () { return N; }
};

class ValuesetItem : public Dialogitem
{	int Col,Row;
	public :
	ValuesetItem (int id, Dialog &dialog, int r=1, int c=1)
		: Dialogitem(id,dialog),Col(c),Row(r) {}
	void select (int row, int col);
	void setbitmap (int row, int col, Bitmap &b);
	void setcolor (int row, int col, long color);
	virtual void init () { select(Row,Col); }
	virtual void exit ();
	virtual void command (Parameter mp1, Parameter mp2);
	int col () { return Col; }
	int row () { return Row; }
};

class ListItem : public Dialogitem
{   int Selection;
	public :
	ListItem (int id, Dialog &dialog)
		: Dialogitem(id,dialog) {}
	void select (int sel);
	virtual void init () { select(0); }
	virtual void exit ();
	virtual void command (Parameter mp1, Parameter mp2);
	void insert (char *text);
	operator int () { return Selection; }
};

class MultilineItem : public Dialogitem
{   String S;
	Flag Readonly;
	public :
	MultilineItem (int id, Dialog &dialog, char *string,
			int length=1024)
		: Dialogitem(id,dialog),S(string,length) {}
	virtual void init ();
	virtual void exit ();
	void set (char *text) { S.copy(text); }
	void reinit (char *text) { set(text); init(); }
	void limit (int length);
	void readonly (int flag=1) { Readonly=flag; }
	operator char * () { return S; }
};


class FileSelector
{	int Freturn;
	Window *W;
	FILEDLG Fd;
	String Filter,Title,Ok;
	public :
	FileSelector(Window &window,
		char *filter, int saving,
		char *title="", char *ok=0);
	char *select ();
};

class FontSelector
{   FONTDLG Fd;
	String Facename;
	int Codepage,Type,Result;
	double Pointsize;
	public :
	enum { screen, printer };
	FontSelector (int type=screen,
		char *name="", double pointsize=10, int codepage=0)
		: Type(type),Facename(name,66),Pointsize(pointsize),
			Codepage(codepage),Result(DID_CANCEL) {}
	int select (Window &window);
	FATTRS *fat () { return &Fd.fAttrs; }
	FONTDLG *fd () { return &Fd; }
	operator int () { return Result; }
};

class Fonts
{	long Count;
	FONTMETRICS *AllFonts;
	public :
	Fonts (PS &ps);
	~Fonts ();
	long count () { return Count; }
	FONTMETRICS *fonts () { return AllFonts; }
};

class Font
{	FATTRS Fat;
	double NominalPointSize,PointSize;
	public :
	Font (FontSelector &fs);
	FATTRS *fat () { return &Fat; }
	char *name () { return Fat.szFacename; }
	double nominalsize () { return NominalPointSize; }
	double pointsize () { return PointSize; }
};

class Profile
{	int P;
	String S,H;
	public :
	enum { user=HINI_USERPROFILE,system=HINI_SYSTEMPROFILE };
	Profile (char *prog, int p=user) : P(p),S(prog),H("") {}
	void write (char *k, void *p, unsigned long size)
	{	PrfWriteProfileData(P,S,k,p,size);
	}
	void writestring (char *k, char *i);
	void writedouble (char *k, double x);
	void writelong (char *k, long x);
	void writeint (char *k, int x);
	int read (char *k, void *p, unsigned long size)
	{	return PrfQueryProfileData(P,S,k,p,&size);
	}
	char *readstring (char *k, char *d="", long size=String::defaultsize);
	double readdouble (char *k, double x=0);
	long readlong (char *k, long x=0);
	int readint (char *k, int x=0);
};

class Clipboard
{	public :
	void copy (MetafilePS &meta);
};

