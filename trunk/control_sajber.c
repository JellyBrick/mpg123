
/*
 * Control interface to front ends.
 * written/copyrights 1997 by Michael Hipp
 */

#include <stdio.h>

#if !defined(WIN32) && !defined(GENERIC)
#include <sys/time.h>
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/uio.h>

#if !defined(WIN32) && !defined(GENERIC)
#include <sys/socket.h>
#endif

#if !defined(WIN32) && !defined(GENERIC)
#include "jukebox/controldata.h"
#endif
#include "mpg123.h"

#define MODE_STOPPED 0
#define MODE_PLAYING 1
#define MODE_PAUSED 2

extern FILE *filept;
extern int tabsel_123[2][3][16];

#if !defined(WIN32) && !defined(GENERIC)

int sajber_sendmsg(int type,int data)
{
	TControlMsg msg;
	msg.type = type;
	msg.data = data;
	return write(1,&msg,sizeof(TControlMsg));
}

void control_sajber(struct frame *fr) 
{
	struct timeval timeout;
	fd_set readfds;
	int n;
	int mode = MODE_STOPPED;
	int mode_rev;
	TControlMsg smsg,rmsg;
	struct msghdr msghdr;
	struct m_cmsghdr cmsghdr;
	struct iovec iovec[1];
	char buf[2];
	int init = 0;
	int framecnt = 0;

	while(1) {
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_SET(0,&readfds);

		if(mode == MODE_PLAYING) {
			n = select(32,&readfds,NULL,NULL,&timeout);
			if(n == 0) {
				if(!read_frame(fr)) {
					mode = MODE_STOPPED;
					rd->close(rd);
					sajber_sendmsg(MSG_NEXT,0);
					continue;
				}
				play_frame(init,fr);
				if(init) {
					AudioInfo sai;
					sai.bitrate = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index] * 1000;
					sai.frequency = freqs[fr->sampling_frequency];
					sai.stereo = fr->stereo;
					sai.type = fr->lay;
					sai.sample = 16;
					sajber_sendmsg(MSG_INFO,TRUE);
					write(1,&sai,sizeof(AudioInfo));
					init = 0;
					
				}
				framecnt++;
				if(!(framecnt & 0xf)) {
					sajber_sendmsg(MSG_FRAMES,framecnt);
					sajber_sendmsg(MSG_POSITION,rd->tell(rd));
				}
			}
		}
		else {
			while(1) {
				n = select(32,&readfds,NULL,NULL,NULL);
				if(n > 0)
					break;
			}
		}
		if(n < 0) {
			exit(1);
		}
		if(n > 0) {
			int len;
			len = read(0,&rmsg,sizeof(TControlMsg));
			if(len != sizeof(TControlMsg)) {
				fprintf(stderr,"Error reading control message!\n");
				exit(1);
			}
#if 0
fprintf(stderr,"%d.%d\n",rmsg.type,rmsg.data);
#endif
			switch(rmsg.type) {
				case MSG_CTRL:
					switch(rmsg.data) {
						case FORWARD_STEP:
							if(mode != MODE_STOPPED) {
								int i;
								for(i=0;i<16;i++) {
									read_frame(fr);
	
									if(framecnt && fr->lay == 3)
										set_pointer(512);
									framecnt++;
									sajber_sendmsg(MSG_FRAMES,framecnt);
									sajber_sendmsg(MSG_POSITION,rd->tell(rd));
								}
							}
							sajber_sendmsg(MSG_RESPONSE,FORWARD_STEP);
							break;
						case FORWARD_BEGIN:
							mode_rev = mode;
							if(mode != MODE_STOPPED) {
								mode = MODE_PAUSED;
								sajber_sendmsg(MSG_RESPONSE,FORWARD_BEGIN);
							}
							break;
						case FORWARD_END:
							mode = mode_rev;
							sajber_sendmsg(MSG_RESPONSE,FORWARD_END);
							break;
						case REWIND_BEGIN:
							mode_rev = mode;
							if(mode != MODE_STOPPED) {
								mode = MODE_STOPPED;
								sajber_sendmsg(MSG_RESPONSE,REWIND_BEGIN);
							}
							break;
						case REWIND_STEP:
							if(mode != MODE_STOPPED) {
								if(!rd->back_frame)
									break;
								if(rd->back_frame(rd,fr,16) == 0)
									framecnt -= 16;
								else
									framecnt = 0;
								sajber_sendmsg(MSG_RESPONSE,REWIND_STEP);
								sajber_sendmsg(MSG_FRAMES,framecnt);
							}
							sajber_sendmsg(MSG_POSITION,rd->tell(rd));
							break;
						case REWIND_END:
							mode = mode_rev;
							sajber_sendmsg(MSG_RESPONSE,REWIND_END);
							break;
						case PLAY_STOP:
							mode = MODE_STOPPED;
							rd->close(rd);
							break;
						case PLAY_PAUSE:
							if (mode == MODE_PAUSED)
								mode = MODE_PLAYING;
							else
								mode = MODE_PAUSED;
							break;
					}
					break;
				case MSG_BUFFER:
					break;
				case MSG_SONG:
					if(mode == MODE_PLAYING) {
						rd->close(rd);
						mode = MODE_STOPPED;
					}

					iovec[0].iov_base = buf;
					iovec[0].iov_len = 2;
					/* 
					 * this control message 'delivers' a given file
					 * descriptor to another process .. 
					 * the sent descriptor can then be accesed by the
					 * child process.
					 */
					cmsghdr.cmsg_len = sizeof(cmsghdr);
					cmsghdr.cmsg_level = SOL_SOCKET;
					cmsghdr.cmsg_type = SCM_RIGHTS;

					msghdr.msg_name = NULL;
					msghdr.msg_namelen = 0;
					msghdr.msg_iov = iovec;
					msghdr.msg_iovlen = 1;
					msghdr.msg_control = &cmsghdr;
					msghdr.msg_controllen = sizeof(cmsghdr);

					if(recvmsg(0,&msghdr,0) < 0) {
						perror("recvmsg");
						exit(1);
					}

					open_stream(NULL,cmsghdr.fd);
					mode = MODE_PLAYING;
					init = 1;
					framecnt = 0;
					read_frame_init();
					break;
				case MSG_QUIT:
					exit(0);
					break;
				case MSG_QUERY:
					smsg.type = MSG_RESPONSE;
					smsg.data = FALSE;
					switch(rmsg.data) {
						case QUERY_PLAYING:
							if(mode == MODE_PLAYING)
								smsg.data = TRUE;
							break;
						case QUERY_PAUSED:
							if(mode == MODE_PAUSED) 
								smsg.data = TRUE;
							break;
					}
					write(1,&smsg,sizeof(TControlMsg));
					break;
				case MSG_BUFAHEAD:
					break;
				case MSG_SEEK:
					if (mode == MODE_STOPPED) break;
					if (rmsg.data > rd->tell(rd)) {
						while (rmsg.data > rd->tell(rd)) {
							if(!read_frame(fr)) {
								sajber_sendmsg(MSG_FRAMES,framecnt);
								sajber_sendmsg(MSG_POSITION,rd->tell(rd));
								mode = MODE_STOPPED;
								rd->close(rd);
								sajber_sendmsg(MSG_NEXT,0);
								continue;
							}
							framecnt++;
							if(framecnt && fr->lay == 3)
								set_pointer(512);
						/*
							rd->back_frame(rd,fr,-16);
							framecnt += 16;
						*/
						}
					}
					else {
						if(!rd->back_frame)
							break;
						while (rmsg.data < rd->tell(rd)) {
							if(rd->back_frame(rd,fr,16) == 0)
								framecnt -= 16;
							else
								framecnt = 0;
						}
					}
					sajber_sendmsg(MSG_FRAMES,framecnt);
					sajber_sendmsg(MSG_POSITION,rd->tell(rd));
					break;
				case MSG_PRIORITY:
					break;
				case MSG_RELEASE:
					break;
			}
			
		}
	}
}
#endif



