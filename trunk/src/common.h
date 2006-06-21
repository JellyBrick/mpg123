/*
 * common.h
 */

/* max = 1728 */
#define MAXFRAMESIZE 3456
#define HDRCMPMASK 0xfffffd00

extern void print_id3_tag(unsigned char *buf);
extern unsigned long firsthead;
extern int tabsel_123[2][3][16];
extern double compute_tpf(struct frame *fr);
extern double compute_bpf(struct frame *fr);
extern long compute_buffer_offset(struct frame *fr);

struct bitstream_info {
  int bitindex;
  unsigned char *wordpointer;
};

extern struct bitstream_info bsi;

/* well, I take that one for granted... at least layer3 */
#define DECODER_DELAY 529
extern int skipbegin;
extern int skipend;

/* for control_generic */
extern const char* remote_header_help;
void make_remote_header(struct frame* fr, char *target);
