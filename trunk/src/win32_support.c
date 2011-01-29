#include "config.h"
#include "mpg123app.h"
#include "debug.h"

#ifdef WANT_WIN32_UNICODE
int win32_cmdline_utf8(int * argc, char *** argv)
{
	wchar_t **argv_wide;
	char *argvptr;
	int argcounter;

	/* That's too lame. */
	if(argv == NULL || argc == NULL) return -1;

	argv_wide = CommandLineToArgvW(GetCommandLineW(), argc);
	if(argv_wide == NULL){ error("Cannot get wide command line."); return -1; }

	*argv = (char **)calloc(sizeof (char *), *argc);
	if(*argv == NULL){ error("Cannot allocate memory for command line."); return -1; }

	for(argcounter = 0; argcounter < *argc; argcounter++)
	{
		win32_wide_utf8(argv_wide[argcounter], &argvptr, NULL);
		(*argv)[argcounter] = argvptr;
	}
	LocalFree(argv_wide); /* We don't need it anymore */

	return 0;
}

void win32_cmdline_free(int argc, char **argv)
{
	int i;

	if(argv == NULL) return;

	for(i=0; i<argc; ++i) free(argv[i]);
}
#endif /* WANT_WIN32_UNICODE */

void win32_set_priority (const int arg)
{
	DWORD proc_result = 0;
	HANDLE current_proc = NULL;
	if (arg) {
	  if ((current_proc = GetCurrentProcess()))
	  {
	    switch (arg) {
	      case -2: proc_result = SetPriorityClass(current_proc, IDLE_PRIORITY_CLASS); break;  
	      case -1: proc_result = SetPriorityClass(current_proc, BELOW_NORMAL_PRIORITY_CLASS); break;  
	      case 0: proc_result = SetPriorityClass(current_proc, NORMAL_PRIORITY_CLASS); break; /*default priority*/
	      case 1: proc_result = SetPriorityClass(current_proc, ABOVE_NORMAL_PRIORITY_CLASS); break;
	      case 2: proc_result = SetPriorityClass(current_proc, HIGH_PRIORITY_CLASS); break;
	      case 3: proc_result = SetPriorityClass(current_proc, REALTIME_PRIORITY_CLASS); break;
	      default: fprintf(stderr,"Unknown priority class specified\n");
	    }
	    if(!proc_result) {
	      fprintf(stderr,"SetPriorityClass failed\n");
	    }
	  }
	  else {
	    fprintf(stderr,"GetCurrentProcess failed\n");
	  }
	}
}

#ifdef WANT_WIN32_FIFO
static HANDLE fifohandle;
static OVERLAPPED ov1;

VOID CALLBACK ReadComplete(
  DWORD dwErrorCode,
  DWORD dwNumberOfBytesTransfered,
  LPOVERLAPPED lpOverlapped
)
{
  /* Reset OVERLAPPED structure */
  debug("ReadFileEx completed");
  memset(lpOverlapped,0,sizeof(OVERLAPPED));
  return;
}

ssize_t win32_fifo_read(void *buf, size_t nbyte){
  int check;
  DWORD re;
  DWORD readbuff;
  DWORD available;
  debug1("Reading pipe handle %p", fifohandle);
  if (!fifohandle) return 0;
  available = win32_fifo_read_peek(NULL);
  if (!available) return 0;
  /* This looks more like a hack than a proper check */
  readbuff = (nbyte > available) ? available : nbyte;
  debug("Starting ReadFileEx");
  check = ReadFileEx(fifohandle,buf,readbuff,&ov1,&ReadComplete);
  WaitForSingleObjectEx(fifohandle,INFINITE,TRUE);
  debug1("Read %ld bytes from pipe", readbuff);
  return (!check) ? 0 : readbuff;
}

/* function should be able to tell if bytes are
   available and return immediately on overlapped
   asynchrounous pipes, like unix select() */
DWORD win32_fifo_read_peek(struct timeval *tv){
  DWORD ret = 0;
  DWORD err, timer;

  timer = (tv) ? tv -> tv_sec * 1000 : INFINITE;

  debug1("Peeking on pipe handle %p", fifohandle);

  SetLastError(0);
  if(!fifohandle) return 0;
    PeekNamedPipe(fifohandle, NULL, 0, NULL, &ret, NULL);
  err =  GetLastError();
  if (err == ERROR_BROKEN_PIPE) {
    debug("Broken pipe, disconnecting");
    DisconnectNamedPipe(fifohandle);
    ConnectNamedPipe(fifohandle,&ov1);
  } else if (err == ERROR_BAD_PIPE) {
      debug("Bad pipe, Waiting for connect");
      DisconnectNamedPipe(fifohandle);
      ConnectNamedPipe(fifohandle,&ov1);
  }

  debug1("Waiting %d msec for pipe to be ready", timer);
  debug1("GetLastError was %ld", err);
  WaitForSingleObjectEx(fifohandle,timer,TRUE);
  debug2("peek %d bytes, error %d",ret, err);
  return ret;
}

void win32_fifo_close(void){
  debug1("Attempting to close handle %p", fifohandle);
  if (fifohandle) {
    DisconnectNamedPipe(fifohandle);
    CloseHandle(fifohandle);
  }
  fifohandle = NULL;
}

int win32_fifo_mkfifo(const char *path){
  HANDLE ret;
  win32_fifo_close();
#ifdef WANT_WIN32_UNICODE
  wchar_t *str;
  if (win32_utf8_wide(path,&str,NULL) == 0){
    fprintf(stderr,"Cannot get FIFO name, likely out of memory\n");
    return -1;
  }
#if (DEBUG == 1)
  fwprintf(stderr,L"CreateNamedPipeW %ws\n", str);
#endif
  ret = CreateNamedPipeW(str,PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED,PIPE_TYPE_BYTE,1,255,255,0,NULL);
  free(str);
#else
#if (DEBUG == 1)
  fprintf(stderr,"CreateNamedPipeA %s\n", path);
#endif
  ret = CreateNamedPipeA(path,PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED,PIPE_TYPE_BYTE,1,255,255,0,NULL);
#endif /* WANT_WIN32_UNICODE */
  if (ret == INVALID_HANDLE_VALUE) return -1;
  fifohandle = ret;
  /* Wait for client */
  ConnectNamedPipe(fifohandle,&ov1);
  WaitForSingleObjectEx(fifohandle,INFINITE,TRUE);
  return 0;
}
#endif /* WANT_WIN32_FIFO */
