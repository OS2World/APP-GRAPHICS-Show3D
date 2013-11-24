#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

class Format
{   char *Buffer;
	void set (char *format, ...);
	public :
	Format (char *format, ...);
	Format (long size, char *format, ...);
	~Format ();
	operator char * () { return Buffer; }
};

class Queue
{	PPRQINFO3 Queues;
	ULONG NQueues;
	PPRQINFO3 Queue;
	int Open;
	DEVOPENSTRUC Data;
	HSPL Handle;
	char *Myname,*Buffer;
	long Buffersize;
	public :
	Queue (char *name="", char *myname="Application",
		long buffersize=4096);
	~Queue ();
	void search ();
	PPRQINFO3 active () { return Queue; }
	int open ();
	void close ();
	char *name () { return Queue->pszName; }
	void start ();
	void stop ();
	void write (char *s, long length=0);
	operator int () { return Open; }
	HSPL handle () { return Handle; }
	char *buffer () { return Buffer; }
	long  buffersize () { return Buffersize; }
};

Queue & operator << (Queue &q, char *s);
