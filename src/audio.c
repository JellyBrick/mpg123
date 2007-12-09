/*
	audio: audio output interface

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123app.h"
#include "common.h"
#include "buffer.h"

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

static int file_write(struct audio_output_struct* ao, unsigned char *bytes, int count)
{
	return (int)write(ao->fn, bytes, count);
}
static int wave_write(struct audio_output_struct* ao, unsigned char *bytes, int count)
{
	return wav_write(bytes, count);
}
static int builtin_get_formats(struct audio_output_struct *ao)
{
	if(param.outmode == DECODE_CDR)
	{
		if(ao->rate == 44100 && ao->channels == 2)
		return MPG123_ENC_SIGNED_16;
		else
		return 0;
	}
	else if(param.outmode == DECODE_AU) return MPG123_ENC_SIGNED_16|MPG123_ENC_UNSIGNED_8|MPG123_ENC_ULAW_8;
	else return MPG123_ENC_ANY;
}
static int builtin_close(struct audio_output_struct *ao)
{
	switch(param.outmode)
	{
		case DECODE_WAV:
		wav_close();
		break;
		case DECODE_AU:
		au_close();
		break;
		case DECODE_CDR:
		cdr_close();
		break;
	}
	return 0;
}
static int  builtin_nothingint(struct audio_output_struct *ao){ return 0; }
static void builtin_nothing(struct audio_output_struct *ao){}

audio_output_t* open_fake_module(void)
{
	audio_output_t *ao = NULL;
	ao = alloc_audio_output();
	if(ao == NULL)
	{
		error("Cannot allocate memory for audio output data.");
		return NULL;
	}
	ao->module = NULL;
	ao->open   = builtin_nothingint;
	ao->flush  = builtin_nothing;
	ao->get_formats = builtin_get_formats;
	ao->write  = wave_write;
	ao->close  = builtin_close;
	ao->device = param.filename;
	switch(param.outmode)
	{
		case DECODE_FILE:
			ao->fn    = OutputDescriptor;
			ao->write = file_write;
		break;
		case DECODE_WAV:
			ao->open  = wav_open;
		break;
		case DECODE_CDR:
			ao->open  = cdr_open;
		break;
		case DECODE_AU:
			ao->open  = au_open;
		break;
		case DECODE_TEST:
		break;
	}

	return ao;
}

/* Open an audio output module */
audio_output_t* open_output_module( const char* name )
{
	mpg123_module_t *module = NULL;
	audio_output_t *ao = NULL;
	int result = 0;

	if(param.usebuffer) return NULL;

	/* Use internal code. */
	if(param.outmode != DECODE_AUDIO) return open_fake_module();

	/* Open the module */
	module = open_module( "output", name );
	if (module == NULL) return NULL;

	/* Check module supports output */
	if (module->init_output == NULL) {
		error1("Module '%s' does not support audio output.", name);
		close_module( module );
		return NULL;
	}
	
	/* Allocation memory for audio output type */
	ao = alloc_audio_output();
	if (ao==NULL) {
		error( "Failed to allocate audio output structure." );
		return NULL;
	}
	
	/* Call the init function */
	ao->device = param.output_device;
	ao->flags  = param.output_flags;
	ao->is_open = FALSE;
	result = module->init_output(ao);
	if (result) {
		error1( "Module's init function failed: %d", result );
		close_module( module );
		return NULL;
	}
	
	/* Store the pointer to the module (so we can close it later) */
	ao->module = module;

	return ao;
}



/* Close the audio output and close the module */
void close_output_module( audio_output_t* ao ) 
{
	if (!ao) return; /* That covers buffer mode, too (ao == NULL there). */
	
	debug("closing output module");
	/* Close the audio output */
	if(ao->is_open && ao->close != NULL) ao->close(ao);

	/* Deinitialise the audio output */
	if (ao->deinit) ao->deinit( ao );
	
	/* Unload the module */
	if (ao->module) close_module( ao->module );

	/* Free up memory */
	free( ao );
}



/* allocate and initialise memory */
audio_output_t* alloc_audio_output()
{
	audio_output_t* ao = malloc( sizeof( audio_output_t ) );
	if (ao==NULL) error( "Failed to allocate memory for audio_output_t." );

	/* Initialise variables */
	ao->fn = -1;
	ao->rate = -1;
	ao->gain = -1;
	ao->userptr = NULL;
	ao->device = NULL;
	ao->channels = -1;
	ao->format = -1;
	ao->flags = 0;
	
	/*ao->module = NULL;*/

	/* Set the callbacks to NULL */
	ao->open = NULL;
	ao->get_formats = NULL;
	ao->write = NULL;
	ao->flush = NULL;
	ao->close = NULL;
	ao->deinit = NULL;
	
	return ao;
}

/*
static void audio_output_dump(audio_output_t *ao)
{
	fprintf(stderr, "ao->fn=%d\n", ao->fn);
	fprintf(stderr, "ao->userptr=%p\n", ao->userptr);
	fprintf(stderr, "ao->rate=%ld\n", ao->rate);
	fprintf(stderr, "ao->gain=%ld\n", ao->gain);
	fprintf(stderr, "ao->device='%s'\n", ao->device);
	fprintf(stderr, "ao->channels=%d\n", ao->channels);
	fprintf(stderr, "ao->format=%d\n", ao->format);
}
*/


/* Safer as function... */
const char* audio_encoding_name(const int encoding, const int longer)
{
	const char *name = longer ? "unknown" : "???";
	switch(encoding)
	{
		case MPG123_ENC_SIGNED_16:   name = longer ? "signed 16 bit"   : "s16 ";  break;
		case MPG123_ENC_UNSIGNED_16: name = longer ? "unsigned 16 bit" : "u16 ";  break;
		case MPG123_ENC_UNSIGNED_8:  name = longer ? "unsigned 8 bit"  : "u8  ";   break;
		case MPG123_ENC_SIGNED_8:    name = longer ? "signed 8 bit"    : "s8  ";   break;
		case MPG123_ENC_ULAW_8:      name = longer ? "mu-law (8 bit)"  : "ulaw "; break;
		case MPG123_ENC_ALAW_8:      name = longer ? "a-law (8 bit)"   : "alaw "; break;
	}
	return name;
}

static void capline(mpg123_handle *mh, long rate)
{
	int enci;
	const int  *encs;
	size_t      num_encs;
	mpg123_encodings(&encs, &num_encs);
	fprintf(stderr," %5ld  |", rate);
	for(enci=0; enci<num_encs; ++enci)
	{
		switch(mpg123_format_support(mh, rate, encs[enci]))
		{
			case MPG123_MONO:               fprintf(stderr, "   M   |"); break;
			case MPG123_STEREO:             fprintf(stderr, "   S   |"); break;
			case MPG123_MONO|MPG123_STEREO: fprintf(stderr, "  M/S  |"); break;
			default:                        fprintf(stderr, "       |");
		}
	}
	fprintf(stderr, "\n");
}

void print_capabilities(audio_output_t *ao, mpg123_handle *mh)
{
	int r,e;
	const long *rates;
	size_t      num_rates;
	const int  *encs;
	size_t      num_encs;
	const char *name = "<buffer>";
	const char *dev  = "<none>";
	if(!param.usebuffer)
	{
		name = ao->module ? ao->module->name : "file/raw/test";
		if(ao->device != NULL) dev = ao->device;
	}
	mpg123_rates(&rates, &num_rates);
	mpg123_encodings(&encs, &num_encs);
	fprintf(stderr,"\nAudio driver: %s\nAudio device: %s\nAudio capabilities:\n(matrix of [S]tereo or [M]ono support for sample format and rate in Hz)\n        |", name, dev);
	for(e=0;e<num_encs;e++) fprintf(stderr," %5s |",audio_encoding_name(encs[e], 0));
	fprintf(stderr,"\n --------------------------------------------------------\n");
	for(r=0; r<num_rates; ++r) capline(mh, rates[r]);

	if(param.force_rate) capline(mh, param.force_rate);

	fprintf(stderr,"\n");
}

/* This uses the currently opened audio device, queries its caps.
   In case of buffered playback, this works _once_ by querying the buffer for the caps before entering the main loop. */
void audio_capabilities(audio_output_t *ao, mpg123_handle *mh)
{
	int fmts;
	size_t ri;
	long rate;
	int channels;
	const long *rates;
	size_t      num_rates, rlimit;
	debug("audio_capabilities");
	mpg123_rates(&rates, &num_rates);
	mpg123_format_none(mh); /* Start with nothing. */
	rlimit = param.force_rate > 0 ? num_rates+1 : num_rates;
	for(channels=1; channels<=2; channels++)
	for(ri = 0;ri<rlimit;ri++)
	{
		rate = ri < num_rates ? rates[ri] : param.force_rate;
		if(param.verbose > 2) fprintf(stderr, "Note: checking support for %liHz/%ich.\n", rate, channels);
#ifndef NOXFERMEM
		if(param.usebuffer)
		{ /* Ask the buffer process. It is waiting for this. */
			buffermem->rate     = rate; 
			buffermem->channels = channels;
			buffermem->format   = 0; /* Just have it initialized safely. */
			debug2("asking for formats for %liHz/%ich", rate, channels);
			xfermem_putcmd(buffermem->fd[XF_WRITER], XF_CMD_AUDIOCAP);
			xfermem_getcmd(buffermem->fd[XF_WRITER], TRUE);
			fmts = buffermem->format;
		}
		else
#endif
		{ /* Check myself. */
			ao->rate     = rate;
			ao->channels = channels;
			fmts = ao->get_formats(ao);
		}
		if(param.verbose > 2) fprintf(stderr, "Note: result 0x%x", fmts);

		if(fmts < 0) continue;
		else mpg123_format(mh, rate, channels, fmts);
	}

#ifndef NOXFERMEM
	/* Buffer loop shall start normal operation now. */
	if(param.usebuffer) xfermem_putcmd(buffermem->fd[XF_WRITER], XF_CMD_WAKEUP);
#endif

	if(param.verbose > 1) print_capabilities(ao, mh);
}

#if !defined(WIN32) && !defined(GENERIC)
#ifndef NOXFERMEM
static void catch_child(void)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);
}
#endif
#endif


/* FIXME: Old output initialization code that needs updating */

int init_output(audio_output_t **ao)
{
	static int init_done = FALSE;
	
	if (init_done) return 1;
	init_done = TRUE;
  
#ifndef NOXFERMEM
	/*
	* Only DECODE_AUDIO and DECODE_FILE are sanely handled by the
	* buffer process. For now, we just ignore the request
	* to buffer the output. [dk]
	*/
	if (param.usebuffer && (param.outmode != DECODE_AUDIO) &&
	(param.outmode != DECODE_FILE)) {
	fprintf(stderr, "Sorry, won't buffer output unless writing plain audio.\n");
	param.usebuffer = 0;
	} 

	if (param.usebuffer)
	{
		unsigned int bufferbytes;
		sigset_t newsigset, oldsigset;
		bufferbytes = (param.usebuffer * 1024);
		if (bufferbytes < bufferblock)
		{
			bufferbytes = 2*bufferblock;
			if(!param.quiet) fprintf(stderr, "Note: raising buffer to minimal size %liKiB\n", (unsigned long) bufferbytes>>10);
		}
		bufferbytes -= bufferbytes % bufferblock;
		/* No +1024 for NtoM rounding problems anymore! */
		xfermem_init (&buffermem, bufferbytes ,0,0);
		sigemptyset (&newsigset);
		sigaddset (&newsigset, SIGUSR1);
		sigprocmask (SIG_BLOCK, &newsigset, &oldsigset);
#if !defined(WIN32) && !defined(GENERIC)
		catchsignal (SIGCHLD, catch_child);
#endif
		switch ((buffer_pid = fork()))
		{
			case -1: /* error */
			error("cannot fork!");
			return -1;
			case 0: /* child */
			{
				/* Buffer process handles all audio stuff itself. */
				audio_output_t *bao = NULL; /* To be clear: That's the buffer's pointer. */
				param.usebuffer = 0; /* The buffer doesn't use the buffer. */
				/* Open audio output module */
				bao = open_output_module(param.output_module);
				if(!bao)
				{
					error("Failed to open audio output module.");
					exit(1); /* communicate failure? */
				}
				if(open_output(bao) < 0)
				{
					error("Unable to open audio output.");
					close_output_module(bao);
					exit(2);
				}
				xfermem_init_reader (buffermem);
				buffer_loop(bao, &oldsigset); /* Here the work happens. */
				xfermem_done_reader (buffermem);
				xfermem_done (buffermem);
				close_output(bao);
				close_output_module(bao);
				exit(0);
			}
			default: /* parent */
			xfermem_init_writer (buffermem);
		}
	}
#else
	if(param.usebuffer)
	{
		error("Buffer not available in this build!");
		return -1;
	}
#endif
	if(!param.usebuffer)
	{ /* Only if I handle audio device output: Get that module. */
		*ao = open_output_module(param.output_module);
		if(!ao)
		{
			error("Failed to open audio output module");
			return -1;
		}
	}
	else *ao = NULL; /* That ensures we won't try to free it later... */
#ifndef NOXFERMEM
	if(param.usebuffer)
	{ /* Check if buffer is alive. */
		int res = xfermem_getcmd(buffermem->fd[XF_WRITER], TRUE);
		if(res < 0)
		{
			error("Buffer process didn't initialize!");
			return -1;
		}
	}
#endif
	/* This has internal protection for buffer mode. */
	if(open_output(*ao) < 0) return -1;

	return 0;
}

void exit_output(audio_output_t *ao, int rude)
{
	debug("exit output");
#ifndef NOXFERMEM
	if (param.usebuffer)
	{
		debug("ending buffer");
		buffer_stop(); /* Puts buffer into waiting-for-command mode. */
		buffer_end(rude);  /* Gives command to end operation. */
		xfermem_done_writer(buffermem);
		waitpid (buffer_pid, NULL, 0);
		xfermem_done (buffermem);
	}
#endif
	/* Close the output... doesn't matter if buffer handled it, that's taken care of. */
	close_output(ao);
	close_output_module(ao);
}

void output_pause(audio_output_t *ao)
{
	if(param.usebuffer) buffer_stop();
	else ao->flush(ao);
}

void output_unpause(audio_output_t *ao)
{
	if(param.usebuffer) buffer_start();
}

int flush_output(audio_output_t *ao, unsigned char *bytes, size_t count)
{
	if(count)
	{
		/* Error checks? */
#ifndef NOXFERMEM
		if(param.usebuffer){ if(xfermem_write(buffermem, bytes, count)) return -1; }
		else
#endif
		if(param.outmode != DECODE_TEST)
		return ao->write(ao, bytes, (int)count);
	}
	return (int)count; /* That is for DECODE_TEST */
}

int open_output(audio_output_t *ao)
{
	if(param.usebuffer) return 0;

	if(ao == NULL)
	{
		error("ao should not be NULL here!");
		exit(110);
	}

	switch(param.outmode)
	{
		case DECODE_AUDIO:
		case DECODE_WAV:
		case DECODE_AU:
		case DECODE_CDR:
		case DECODE_FILE:
			debug("opening normal audio/file");
			ao->is_open = ao->open(ao) < 0 ? FALSE : TRUE;
			if(!ao->is_open)
			{
				error("failed to open audio device");
				return -1;
			}
			else return 0;
		break;
		case DECODE_TEST:
			debug("decoding to nowhere");
			return 0;
		break;
	}
	debug("nothing");
	return -1; /* That's an error ... unknown outmode? */
}

/* is this used? */
void close_output(audio_output_t *ao)
{
	if(param.usebuffer) return;

	debug("closing output");
	switch(param.outmode)
	{
		case DECODE_AUDIO:
		case DECODE_WAV:
		case DECODE_AU:
		case DECODE_CDR:
		/* Guard that close call; could be nasty. */
		if(ao->is_open)
		{
			ao->is_open = FALSE;
			if(ao->close != NULL) ao->close(ao);
		}
		break;
	}
}

/* Also for WAV decoding? */
int reset_output(audio_output_t *ao)
{
	if(!param.usebuffer)
	{
		close_output(ao);
		return open_output(ao);
	}
	else return 0;
}
