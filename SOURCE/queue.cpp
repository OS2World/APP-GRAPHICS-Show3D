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

#include "queue.h"

Format::Format (char *format, ...)
{   va_list v;
	Buffer=new char[1024];
	va_start(v,format);
	vsprintf(Buffer,format,v);
	va_end(v);
}

void Format::set (char *format, ...)
{   va_list v;
	Buffer=new char[32];
	va_start(v,format);
	vsprintf(Buffer,format,v);
	va_end(v);
}

Format::Format (long size, char *format, ...)
{   va_list v;
	Buffer=new char[size];
	va_start(v,format);
	vsprintf(Buffer,format,v);
	va_end(v);
}

Format::~Format ()
{	delete Buffer;
}

Queue::Queue (char *name, char *myname, long buffersize)
{   ULONG n;
	Queues=0;
	Myname=myname;
	Buffer=new char[buffersize];
	Buffersize=buffersize;
	search();
	Queue=Queues;
	for (n=0; n<NQueues; n++)
		if (!strcmp(name,Queues[n].pszName))
		{	Queue=Queues;
			break;
		}
	Open=0;
	open(); start();
}

Queue::~Queue ()
{	stop(); close();
	if (Queues) delete Queues;
}

int Queue::open ()
{   if (!Queue) return 0;
	Data.pszLogAddress		=Queue->pszName;
	Data.pszDriverName		=Queue->pszDriverName;
	*strchr(Data.pszDriverName,'.')=0;
	Data.pdriv              = Queue->pDriverData;
	Data.pszDataType        = "PM_Q_RAW";
	Data.pszComment         = Myname;
	Data.pszQueueProcName   = Queue->pszPrProc;
	Data.pszQueueProcParams = NULL;
	Data.pszSpoolerParams   = NULL;
	Data.pszNetworkParams   = NULL;
	Handle=SplQmOpen("*",9L,(PQMOPENDATA)&Data);
	if (Handle) Open=1;
	return (Handle!=0);
}

void Queue::close ()
{	if (Open) SplQmClose(Handle);
	Open=0;
}

void Queue::search ()
/* Search all queues and devices */
{   ULONG ret,n;
	if (!Queues) delete Queues;
	Queues=0;
	SplEnumQueue(NULL,3,Queues,0,
		&ret,&NQueues,&n,NULL);
	if (NQueues==0) return;
	Queues=(PPRQINFO3)(new char[n]);
	SplEnumQueue(NULL,3,Queues,n,
		&ret,&NQueues,&n,NULL);
}

void Queue::start ()
{	if (Open) SplQmStartDoc(Handle,Myname);
}

void Queue::stop ()
{	if (Open) SplQmEndDoc(Handle);
}

void Queue::write (char *s, long l)
{	SplQmWrite(Handle,(l>0)?l:(strlen(s)+1),s);
}

Queue & operator << (Queue &q, char *s)
{	char *p=q.buffer(),*end=q.buffer()+q.buffersize();
	while (*s)
	{	switch(*s)
		{	case '\n' : *p++=13; *p++=10; break;
			default : *p++=*s;
		}
		s++;
		if (p>=end-1) break;
	}
	*p=0;
	if (q) SplQmWrite(q.handle(),p-q.buffer(),q.buffer());
	return q;
}

inline Queue & operator << (Queue &q, long n)
{	return q << (char *)Format(32,"%0ld",n);
}

