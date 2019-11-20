#ifndef HEADERFILE_E
#define HEADERFILE_E

// Typedefs
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef short int16;

// Macros to help with bit extraction/insertion
#define SHIFT(size,startpos) ((startpos)-(size)+1)
#define MASK(size, startpos) (((2<<(size-1))-1) << SHIFT(size,startpos))
#define PUTBITS( dest, data, size, startpos) dest = ((dest & ~MASK(size, startpos)) | ((data << SHIFT(size, startpos)) & MASK(size,startpos)))
#define SHIFTHIGH(size, startpos) (((startpos)-32)-(size)+1)
#define MASKHIGH(size, startpos) (((1<<(size))-1) << SHIFTHIGH(size,startpos))
#define PUTBITSHIGH(dest, data, size, startpos) dest = ((dest & ~MASKHIGH(size, startpos)) | ((data << SHIFTHIGH(size, startpos)) & MASKHIGH(size,startpos)))
#define GETBITS(source, size, startpos)  (( (source) >> ((startpos)-(size)+1) ) & ((1<<(size)) -1))
#define GETBITSHIGH(source, size, startpos)  (( (source) >> (((startpos)-32)-(size)+1) ) & ((1<<(size)) -1))
#ifndef PGMOUT
#define PGMOUT 1
#endif
// Thumb macros and definitions
#define	R_BITS59T 4
#define G_BITS59T 4
#define	B_BITS59T 4
#define	R_BITS58H 4
#define G_BITS58H 4
#define	B_BITS58H 4
#define	MAXIMUM_ERROR (255*255*16*1000)
#define R 0
#define G 1
#define B 2
#define BLOCKHEIGHT 4
#define BLOCKWIDTH 4
#define BINPOW(power) (1<<(power))
#define	TABLE_BITS_59T 3
#define	TABLE_BITS_58H 3

// Helper Macros
#define CLAMP(ll,x,ul) (((x)<(ll)) ? (ll) : (((x)>(ul)) ? (ul) : (x)))
#define JAS_ROUND(x) (((x) < 0.0 ) ? ((int)((x)-0.5)) : ((int)((x)+0.5)))

#define RED_CHANNEL(img,width,x,y,channels)   img[channels*(y*width+x)+0]
#define GREEN_CHANNEL(img,width,x,y,channels) img[channels*(y*width+x)+1]
#define BLUE_CHANNEL(img,width,x,y,channels)  img[channels*(y*width+x)+2]
#define ALPHA_CHANNEL(img,width,x,y,channels)  img[channels*(y*width+x)+3]

unsigned int read_big_endian_4byte_word(FILE *f);
void setupAlphaTable();
void decompressBlockDiffFlipC(unsigned int block_part1, unsigned int block_part2, uint8 *img, int width, int height, int startx, int starty, int channels);
void decompressBlockETC2(unsigned int block_part1, unsigned int block_part2, uint8 *img, int width, int height, int startx, int starty);
void decompressBlockAlpha(uint8* data, uint8* img, int width, int height, int ix, int iy);

#endif