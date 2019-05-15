/*
	syn123_int: internal header for libsyn123

	copyright 2018 by the mpg123 project,
	licensed under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	initially written by Thomas Orgis
*/

#ifndef _MPG123_SYN123_INT_H_
#define _MPG123_SYN123_INT_H_

#define _ISOC99_SOURCE

#include "config.h"
#include "intsym.h"
#include "compat.h"
#include "abi_align.h"
#include "syn123.h"

// Generally, a number of samples we work on in one go to
// allow the compiler to know our loops.
// An enum is the best integer constant you can define in plain C.
enum { bufblock = 512 };

struct syn123_wave
{
	enum syn123_wave_id id;
	int backwards; /* TRUE or FALSE */
	double freq; /* actual frequency */
	double phase; /* current phase */
};

struct syn123_sweep
{
	struct syn123_wave wave;
	double f1, f2; // begin/end frequencies (or logarithms of the same)
	enum syn123_sweep_id id;
	size_t i; // position counter
	size_t d; // duration
	size_t post; // amount of samples after sweep to finish period
	double endphase; // phase for continuing, just after sweep end
};

struct syn123_struct
{
	// Temporary storage in internal precision.
	// This is a set of two to accomodate x and y=function(x, y).
	// Working in blocks reduces function call overhead and gives
	// chance of vectorization.
	// This may also be used as buffer for data with output encoding,
	// exploiting the fact that double is the biggest data type we
	// handle, also with the biggest alignment.
	double workbuf[2][bufblock];
	struct mpg123_fmt fmt;
	// Pointer to a generator function that writes a bit of samples
	// into workbuf[1], possibly using workbuf[0] internally.
	// Given count of samples <= bufblock!
	void (*generator)(syn123_handle*, int);
	// Generator configuration.
	// wave generator
	size_t wave_count;
	struct syn123_wave* waves;
	// pink noise, maybe others: simple structs that can be
	// simply free()d
	void* handle;
	uint32_t seed; // random seed for some RNGs
	// Extraction of initially-computed waveform from buffer.
	void *buf;      // period buffer
	size_t bufs;    // allocated size of buffer in bytes
	size_t maxbuf;  // maximum period buffer size in bytes
	size_t samples; // samples (PCM frames) in period buffer
	size_t offset;  // offset in buffer for extraction helper
};

#ifndef NO_SMIN
static size_t smin(size_t a, size_t b)
{
	return a < b ? a : b;
}
#endif

#ifndef NO_SMAX
static size_t smax(size_t a, size_t b)
{
	return a > b ? a : b;
}
#endif

#ifndef NO_GROW_BUF
// Grow period buffer to at least given size.
// Content is not preserved.
static void grow_buf(syn123_handle *sh, size_t bytes)
{
	if(sh->bufs >= bytes)
		return;
	if(sh->buf)
		free(sh->buf);
	sh->buf = NULL;
	if(bytes && bytes <= sh->maxbuf)
		sh->buf = malloc(bytes);
	sh->bufs = sh->buf ? bytes : 0;
}
#endif

#ifdef FILL_PERIOD
static int fill_period(syn123_handle *sh)
{
	sh->samples = 0;
	if(!sh->maxbuf)
		return SYN123_OK;
	size_t samplesize = MPG123_SAMPLESIZE(sh->fmt.encoding);
	size_t buffer_samples = sh->maxbuf/samplesize;
	grow_buf(sh, buffer_samples*samplesize);
	if(buffer_samples > sh->bufs/samplesize)
		return SYN123_DOOM;
	int outchannels = sh->fmt.channels;
	sh->fmt.channels = 1;
	size_t buffer_bytes = syn123_read(sh, sh->buf, buffer_samples*samplesize);
	sh->fmt.channels = outchannels;
	if(buffer_bytes != buffer_samples*samplesize)
		return SYN123_WEIRD;
	sh->samples = buffer_samples;
	return SYN123_OK;
}
#endif

#endif
