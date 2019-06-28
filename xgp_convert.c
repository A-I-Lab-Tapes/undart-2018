#if 0
NAME="xgp_convert"
  SRC="/data/home/font"
  echo -n self_compiling $SRC/$NAME
  echo '  Copyright Ⓒ 2018.Bruce_Guenther_Baumgart.Software_License:GPL-v3.'
  echo gcc -g -m64 -Wall -Werror -o /usr/local/bin/$NAME $SRC/$NAME.c
       gcc -g -m64 -Wall -Werror -o /usr/local/bin/$NAME $SRC/$NAME.c
  return
#endif
    
  typedef unsigned long long uint64;
typedef          long long  int64;
typedef unsigned long long    u64;
typedef          long long    i64;
typedef unsigned char uchar;

/* /data/home/font/xgpfntp1.c
   -*- coding: utf-8 -*-

   Very early XGP raster generation
   BgBaumgart 1972 code at XIP, XAP, DDFONT and EDFONT

   PUB and POX generated XGP files

   TeX generated XGP files

   The UUO manual XGP documentation

   The WAITS system source code is at XGPSER[S,SYS] one hundred versions 1973 to 1986.
   * =======================================================================================
   cut-and-paste from XAP.FAI[1,BGB] page 8:

   COMMENT ⊗ STANFORD FONT FILE FORMAT.---------------------------------

   WORDS 0-177:
   XWD CHARACTER_WIDTH,CHARACTER_ADDRESS
   WORDS 200-237:
   CHARACTER_SET_NUMBER
   HEIGHT
   MAX_WIDTH (IN BITS)
   BASE LINE (BITS FROM TOP OF CHARACTER)
   WORDS 240-377:
   ASCIZ/FONT DESCRIPTION/
   REMAINDER OF FILE:
   EACH CHARACTER:
   CHARACTER_CODE,,WORD_COUNT+2
   ROWS_FROM_TOP,,DATA_ROW_COUNT
   BLOCK WORD_COUNT
   --------------------------------------------------------------------⊗
   * =======================================================================================
   cut-and-paste from TEXOUT.SAI[1,DEK]7 dated 1978-05-24
   * =======================================================================================
   comment initialization: initout,declareofil;

   internal string ofilext # filename extension for output;
   internal string deviceext # extension to use in font information files;
   internal string ofilname # output file name, set by first \input;
   internal string libraryarea # default system area for fonts;
   integer ochan # output channel number;
   boolean no_output_yet # no pages shipped out yet;

   internal procedure initout # get TEXOUT started properly;
   begin ofilname←null; ofilext←".XGP"; deviceext←".TFX"; libraryarea←"[XGP,SYS]";
   ochan←-1; no_output_yet←true;
   arrclr(strings);
   end;

   internal procedure declareofil(string s) # initializes the output on file s;
   begin comment This procedure is called when the name of the output file is
   first known. It opens the file and gets things started;
   integer i;
   ofilname←s;
   open(ochan←getchan,"DSK",0,0,19,0,0,eof);
   while true do
   begin enter(ochan,ofilname,eof);
   if eof then
   begin print(nextline,"I can't write on file ",ofilname,
   newline,"Output file = ");
   ofilname←inchwl;
   end
   else done;
   end;
   for i←1 thru '200 do out(ochan,"TRASH") # preamble block will overwrite this later;
   end;
   comment Output codes for the XGP.

   Here is a summary of the XGP file output format, for the files produced
   by this module.

   First comes an almost-fixed preamble block of 128x5 characters, a block
   which tells TEX's conventions to the XGP server:
   /LMAR=50/TMAR=50/RMAR=4095/BMAR=1/PMAR=0/XLINE=0/FONT#0=NGR13/USETI=00
   followed by nnnnn, followed by 113 appearances of "*TEX*".
   The only variable part of this block is the value of nnnnn, which is set
   to the (decimal) block number of the "postamble". (The postamble location
   is computed by using tricky system code after the pages have all been
   output. Therefore the preamble isn't actually written out until the
   page output is all complete, a dummy block comes out first -- see the
   declareofil procedure. Font NGR13, which is declared as "font number 0"
   in this preamble, will be used for headings and error messages that are
   not part of the "real" TEX output. This font was chosen mainly because it
   takes up less space than any other font in the XGP repertoire.)

   Then come the specifications of individual pages, which are composed of
   7-bit code sequences understood by the XGP server as follows:

   c, where c≠'000,'011,'012,'014,'015, or '177
   Output character c in the current font, then advance the
   appropriate number of columns
   '177&c,	where c='000,'011,'012,'014,'015, or '177
   Output character c in the current font, then advance the
   appropriate number of columns
   '177&'001&'040&x1&x2, where x1&x2=x is a 14-bit binary number, x < 4096
   Move to column x
   '177&'002&d, where d is a 7-bit two's complement number
   Move to the current column plus d
   '012&'177&'003&y1&y2, where y1&y2=y is a 14-bit binary number
   The top row of the next characters should be row y
   '015&'014       Cut the paper at the current row and (if spooled with the
   /head option) put page heading 1/4 inch from top of new page
   (Cutting the paper starts rows counting at 0 again.)
   '177&'006&(f+1), where f is a font number in TEX (0 ≤ f < 32)
   The next characters should be in font f until further notice
   '177&'004&y1&y2&x1&x2&'000&'000&'000&h1&h2&w1&w2
   Output a black rectangle with top row y and left column x,
   where the width is w and the height is h

   These commands occur in order of increasing values of y (i.e., increasing
   values of the tops of the characters and rules being output). Actually y
   is called y0 in the program documentation, since it stands for the
   minimum y value of the character. (Coordinates are chosen so that
   increasing x means going to the right, while increasing y means going
   downward. This convention is slightly non-Cartesian, but it is a natural
   thing to do given the XGP hardware.)

   Finally, the postamble appears. (A bunch of '000 characters is first
   written out if necessary to ensure that the postamble begins on a
   block boundary.) The postamble consists simply of specifications like
   /FONT#<decimal value of f+1>=<name of font f>=<list of chars used>
   where the list of characters used is terminated by repeating the final character.
   For example, if the TEX user wrote /a:←CMR10[1,DEK] and if only characters
   xyz of this font were ever generated, the postamble would include the string
   /FONT#2=CMR10[1,DEK]=xyzz
   among its font specifications. The postamble concludes with as many '000
   characters as necessary to fill a block. (Note: A project-programmer
   specification like "[1,DEK]" in a font file name can be omitted if and only if
   is "[XGP,SYS]".);

*/
#define _GNU_SOURCE
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

// input
// input
FILE *fancy; // output listing of XGP device text and device commands.

// This union defines PDP10 bit-field names within one Big Endian 36-bit PDP10 word.
// The programming language 'C' x86 words are Little Endian, so Right comes before Left.
typedef union {
  uint64 fw;
  struct {  int64  word:36,        :28; } full; //    signed
  struct {  int64 right:18,left:18,:28; } half; //    signed
  struct { uint64  word:36,        :28; } u36;  // Un-signed
  struct { uint64 right:18,left:18,:28; } u18;  // Un-signed halves
  struct { uint64 q3:9,q2:9,q1:9,q0:9,: 28; } u9; // Un-signed quarter words
  struct { uint64 hi:12,mid:12,lo:12,:28; } third;
  struct { uint64 c6:6,c5:6,c4:6,c3:6,c2:6,c1:6,:28;}    sixbit; // sixbit+040
  struct { uint64 bit35:1,a5:7,a4:7,a3:7,a2:7,a1:7,:28;	} seven; // 'seven bit ASCII' or SAIL-ASCII
} pdp10_word;

// Table of pointers to SAIL font file DATA8 content
#define FONTMAX 300
pdp10_word *font_atlas[FONTMAX];
pdp10_word *font_map[FONTMAX];
char *font_atlas_name[FONTMAX];
char *font_map_name[FONTMAX];

char *page[1024];
char *line[1024];

// Doctored seven-bit SAIL-ASCII into UTF-8 string table.
//   ======================================================================
//    ↓αβ∧¬επλ\t\n\v\f\r∞∂⊂⊃∩∪∀∃⊗↔_→~≠≤≥≡∨ !\"#$%&'()*+,-./0123456789:;<=>?
//   @ABCDEFGH I J K L MNOPQRSTUVWXYZ[\\]↑←'abcdefghijklmnopqrstuvwxyz{|⎇}␈
//   ======================================================================
// The remix TEXT conversion omits NUL,  VT, RETURN,   ALT and RUBOUT !
//                          codes \000 \013    \015  \0175    \0177
//
// Since 2018, the Stanford-ALT-character in SAILDART is represented in unicode as § U00A7,
// which resembles the dollar-sign $ in old SAIL documentation,
// and is not quite as ugly as ⎇ or Ⓢ used formerly in SAILDART material.
char *SAIL_ASCII[]={
  // 00      01      02      03          04      05      06      07
  "",       "↓",    "α",    "β",         "∧",    "¬",    "ε",    "π",  //  000
  "λ",     "\t",   "\n",     "",        "\f",   "",      "∞",    "∂",  //  010
  "⊂",      "⊃",    "∩",    "∪",         "∀",    "∃",    "⊗",    "↔",  //  020
  "_",      "→",    "~",    "≠",         "≤",    "≥",    "≡",    "∨",  //  030
  " ",      "!",   "\"",    "#",         "$",    "%",    "&",    "'",  //  040
  "(",      ")",    "*",    "+",         ",",    "-",    ".",    "/",  //  050
  "0",      "1",    "2",    "3",         "4",    "5",    "6",    "7",  //  060
  "8",      "9",    ":",    ";",         "<",    "=",    ">",    "?",  //  070
  "@",      "A",    "B",    "C",         "D",    "E",    "F",    "G",  // 0100
  "H",      "I",    "J",    "K",         "L",    "M",    "N",    "O",  // 0110
  "P",      "Q",    "R",    "S",         "T",    "U",    "V",    "W",  // 0120
  "X",      "Y",    "Z",    "[",         "\\",   "]",    "↑",    "←",  // 0130
  "'",      "a",    "b",    "c",         "d",    "e",    "f",    "g",  // 0140
  "h",      "i",    "j",    "k",         "l",    "m",    "n",    "o",  // 0150
  "p",      "q",    "r",    "s",         "t",    "u",    "v",    "w",  // 0160
  "x",      "y",    "z",    "{",         "|",    "",     "}",    ""    // 0170
};
// ----------------------------------------------------------------------------------------------100.-------------118.
u64 leftmask[]={
  0xFFFFFFFFFLL,
  0x800000000LL,0xC00000000LL,0xE00000000LL,0xF00000000LL,    0xF80000000LL,0xFC0000000LL,0xFE0000000LL,0xFF0000000LL,
  0xFF8000000LL,0xFFC000000LL,0xFFE000000LL,0xFFF000000LL,    0xFFF800000LL,0xFFFC00000LL,0xFFFE00000LL,0xFFFF00000LL,
  0xFFFF80000LL,0xFFFFC0000LL,0xFFFFE0000LL,0xFFFFF0000LL,    0xFFFFF8000LL,0xFFFFFC000LL,0xFFFFFE000LL,0xFFFFFF000LL,
  0xFFFFFF800LL,0xFFFFFFC00LL,0xFFFFFFE00LL,0xFFFFFFF00LL,    0xFFFFFFF80LL,0xFFFFFFFC0LL,0xFFFFFFFE0LL,0xFFFFFFFF0LL,
  0xFFFFFFFF8LL,0xFFFFFFFFCLL,0xFFFFFFFFELL,0xFFFFFFFFFLL,};

int verbose=0;

extern int errno;
unsigned long size5, size8;
int ubyte_cnt, uchar_cnt;

#define MAX_WORD_COUNT  3020800
#define MAX_BYTE_COUNT 15104000
unsigned char     obuf5[   MAX_BYTE_COUNT ]; // 5 seven-bit characters to PDP10 word.
unsigned char obuf_utf8[ 3*MAX_BYTE_COUNT ]; // unlikely UTF8 population
u64 buf8[ 3020800 ]; // The maximum SAIL file size in PDP10 words is DART.ARC[MC,SYS]2

// XGP page buffer, 2200 rows of 1728 columns, 200 dots per inch
// 8.5 by 11 letter size media.
// 8.5 by 14 legtal size media.
// u64 raster[ 2800*27 ];
int anyink=0;
u64 raster[ 2200*27 ];

void
omit_spaces(char *q)
{
  char *t,*p;
  for (p= t= q; *p; p++)
    if (*p != ' ')
      *t++ = *p;
  *t++ = 0;
}

char *
string_to_upper(char *str){
  char *s=str;
  while(*s){
    *s = toupper((unsigned char) *s);
    s++;
  }
  return str;
}

void
ink_dot( int r, int c ){
  if( r<0 || c<0 ) return;
  //if( r>=2800 || c>=1728 ) return;
  if( r>=2200 || c>=1728 ) return;
  raster[r*27 + (c/64)] |= (1LL << (63 - (c%64)));
  anyink=1;
}
// Place ink-dots RIGHT-to-LEFT for 36-bit PDP10 word.
void
ink_word36( int row, int column, u64 bits){
  int c=36;
  for( c=36; (c>0) && (bits!=0); c-- , bits>>=1 ){
    if(bits & 1){
      ink_dot( row, column+c );
    }
  }
}
// Place ink-dots LEFT-to-RIGHT
// advance column thru bits of a variable-length-"bytes"
// advance row byte to byte, ILDB
void
ink_bytes( int row,int column, u64 bits, int width, int rows){
  int c=0,r=0;
  for(r=0;r<rows;r++)
    for( c=0; c<width; c++ ){
      if(bits & 0x800000000LL){
        ink_dot( row+r, column+c );
      }
      bits &= 0x7FFFFFFFFLL;
      bits <<= 1;
      if(!bits)return;
    }
}
/* cut-paste from 1974-10-18 XIP.FAI
   ====================================
   SUBR(PRINT)	PLACE A GLYPH INTO XGP BUFFER AT ROW,COL.
   COMMENT .---------------------------------------------------------------------.
   ;Implicit Arguments to PRINT are ROW, COL, CHAR,
   ;FONT, FONTAB, ORGXGP, ENDXGP, TJMODE.
   ACCUMULATORS{G,B,B2,M,N,I,X16}
   SKIPN CHAR↔POP0J	;IGNORE NULL CHARACTERS.
   LAC 1,FONT		;CURRENT FONT NUMBER.
   SKIPN 2,FONTAB(1)↔POP0J	;FONT BASE ADDRESS.
   LAC I,203(2)		;ROWS BETWEEN TOP AND BASE LINE.
   ADD 2,CHAR		;POINTER INTO FONT'S CHARACTER TABLE.
   CAR N,(2)		;COLS WIDE OF THE GLYPH.
   CDR G,(2)↔SKIPN G↔POP0J ;EXIT WHEN NO CHARACTER.
   ADD G,FONTAB(1)↔AOS G	;CHARACTER'S GLYPH POINTER.
   CDR M,(G)		;ROWS HIGH OF THE GLYPH.
   CAR 0,(G)		;ROWS FROM TOP TO FIRST ROW OF GLYPH.
   SUB 0,I			;ROWS ABOVE CURRENT XGP PEN POSITION.
   ADD 0,ROW
   IMULI WWIDTH
   ADD ORGXGP↔HRRZM B	;WORD POINTER INTO XGP BUFFER.
   LAC 0,COL
   SKIPE TJMODE↔GO .+3	;CLIP LINE OVERFLOW IF TJMODE=0
   CAML 0,COLMAX↔POP0J
   IDIVI 0,=36		;REMAINDER IN AC-1 !
   AOS↔ADD B,0↔DAC B,B2	;WORD POINTER INTO XGP BUFFER.
   ADDM N,COL		;UPDATE XGP PEN COLUMN POSITION.
   TLO G,444400↔AOS G	;SETUP GLYPH BYTE POINTER.
   CAILE N,=36↔GO[
   IDIVI N,=36↔AOJA N,L0]	;WHEN CHARACTER WIDTH ≥ =36.
   DPB N,[POINT 6,G,11]	;SIZE OF BYTE.
   ADD 1,N↔SUBI 1,=36	; =36 - CHRWID - REMAINDER
   MOVEI N,1
   L0:	MOVNS 1↔DAP 1,L3	;BYTE POSITION WITH RESPECT TO WORD BOUNDARYS.

   ;INCLUSIVE OR GLYPH BITS INTO THE XGP BUFFER.

   L1:	LAC I,N
   L2:	ILDB 0,G↔SETZ 1,
   L3:	LSHC 0,0
   CAML B,ORGXGP↔CAMLE B,ENDXGP↔SKIPA↔IORM 0,(B)
   AOS B↔JUMPE 1,L4
   CAML B,ORGXGP↔CAMLE B,ENDXGP↔SKIPA↔IORM 1,(B)
   L4:	SOJG I,L2↔LAC B,B2
   ADDI B,WWIDTH↔DAC B,B2
   SOJG M,L1
   POP0J
   ENDR PRINT;BGB 23 MAY 1973 ----------------------------------------------------
   ====================================
*/

// Inclusive OR glyph bit map into raster at row,col
// `2018-11-10 bgbaumgart@mac.com' 45 years later.
int xgp_page_number = 1;
int xgp_obuf5_i0 = 0; // index of byte +1 past the XGP header formfeed.
int xgpfont=0;
int xgprow=200, xgpcol=100;
int xgp_font_height=0;
int xgp_font_maxwid=0;
int xgp_font_baseline=0;
int xgp_lmar=0, xgp_rmar=0, xgp_tmar=0, xgp_bmar=0, xgp_pmar=0, xgp_xline=3;
int xgp_nowraparound=0;
int xgp_useti=0;

void
ink_glyph( int font, int code, int row, int col ){
  int r,c,w;  
  pdp10_word *fnt = font_map[font];
  int chr_width = fnt[ code ].u18.left;
  if( code==' ' ){
    xgpcol += chr_width; return;
  }
  pdp10_word *glyph = fnt + fnt[ code ].u18.right;

  xgp_font_height = font_map[font][0201].full.word;
  xgp_font_maxwid = font_map[font][0202].full.word;
  xgp_font_baseline = font_map[font][0203].full.word;

  int bit_width = glyph[0].u9.q0;
  int ccode = glyph[0].u9.q1;
  int wordcount = glyph[0].u18.right;
  
  int left_kern = glyph[1].u9.q0;
  int rows_from_top = glyph[1].u9.q1;
  int row_count = glyph[1].u18.right;
  if (!row_count) return; // nothing to do.
  
  // Adjust word count and data pointer by two words
  wordcount     -= 2;
  glyph         += 2;
  
  int words_per_row = wordcount / row_count;
  if (!bit_width) bit_width = chr_width;
  if (!bit_width) return;
  int rows_per_word = 36 / bit_width;
  /*  
      if( font>0 && (code=='F' || code=='d' )){
      printf("INK_GLYPH( font=%d code=%d row=%d col=%d\n", font, code, row, col);
      printf("INK_GLYPH chr_width=%d bit_width=%d ccode=%d='%c' wordcount=%d\n",chr_width, bit_width, ccode,ccode, wordcount);
      printf("INK_GLYPH  kern=%d   top=%d row_count=%d\n",left_kern,rows_from_top,row_count);
      printf("INK_GLYPH words_per_row=%d\n",words_per_row);
      printf("INK_GLYPH rows_per_word=%d\n",rows_per_word);
      }

      verbose = (font>0 && code=='F');
  */  
  assert(code == ccode);
  if (!words_per_row){
    // rows per word
    for(r=0; r<row_count; r+=rows_per_word){
      int rows =  rows_per_word > (row_count-r) ? (row_count-r) : rows_per_word;
      u64 bits = (*glyph++).u36.word & leftmask[ rows*bit_width ];
      ink_bytes(rows_from_top+ row+r -xgp_font_baseline, col-left_kern, bits, bit_width, rows );
    }
  }else{
    // words per row NON-ZERO
    for(r=0;r<row_count;r++){
      int wide = bit_width<36 ? bit_width : 36;
      for(w=1;w<=words_per_row;w++){
        if ((w!=1)&&(w==words_per_row)){
          wide=(bit_width%36);
        }
        u64 bits = (*glyph++).u36.word & leftmask[ wide ];
        c = (w-1)*36 - left_kern;
        if(verbose)printf("r=%d w=%d wide=%d bits=%012llo\n",r,w,wide,bits);
        ink_word36(rows_from_top+ row+r -xgp_font_baseline, col+c, bits );
      }
    }
  }
  xgpcol += chr_width;
}
void ink_string(char *str){
  char c;
  while((c=*str++)){
    // if(c=='.')c=':';
    ink_glyph( xgpfont, c, xgprow, xgpcol);
  }
}
char *output_directory=NULL;
void
pbm_output(){
  int i;
  int n,o;
  char *header="P4 1728 2200\n";
  uchar *p = (uchar *)raster;
  char pbm_filename[128];
  
  if(0){ /** Bamboo Test patterns **/
    int j;

    // horizontal lines
    for(i=0;i<2800;i+=200){
      for(j=0;j<27;j++){
        raster[i*27+j] = 0xFFFFFFFFFFFFFFFF;
      }}
    // diagonal upper-left to lower-right
    for(i=0;i<1728;i++){
      raster[i*27 + (i/64)] |= (1LL << (63 - (i%64)));
    }

    ink_glyph( 2, 83, 400, 400 ); // place Stanford SEAL at 400 400
    xgpcol = 100;

    ink_glyph( 0, 'B', 800, xgpcol );  ink_glyph( 1, 'B', 800, xgpcol );
  }
  if(0){ /**  Bamboo Test **/
    
    ink_glyph( 0, 'a', 800, xgpcol );
    ink_glyph( 0, 'u', 800, xgpcol );
    ink_glyph( 0, 'm', 800, xgpcol );
    ink_glyph( 0, 'g', 800, xgpcol );
    ink_glyph( 0, 'a', 800, xgpcol );
    ink_glyph( 0, 'r', 800, xgpcol );
    ink_glyph( 0, 't', 800, xgpcol );

    xgprow = 40;
    xgpcol = 100;
    xgpfont= 0;
    ink_string("Find Stanford University");

    xgprow = 100;  xgpcol = 400;  xgpfont= 0;
    ink_string("STANFORD : !  .  1  9  : UNIVERSITY");
    xgprow = 200;  xgpcol = 400;  xgpfont= 0;
    ink_string("ABCDEFGHIJKLMNOPQRSTUVWXYZ []{}<>?,./");
    xgprow = 250;  xgpcol = 400;  xgpfont= 0;
    ink_string("\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017"
               "\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037");
    xgprow = 300;  xgpcol = 400;  xgpfont= 0;
    ink_string("abcdefghijklmnopqrstuvwxyz !@#$%^&*()");
  }
  
  // flip u64 endian
  for(i=0;i<2200*27;i++){
    u64 fetch = raster[i];
    *p++ = 0xFF & (fetch >> 56 );
    *p++ = 0xFF & (fetch >> 48 );
    *p++ = 0xFF & (fetch >> 40 );
    *p++ = 0xFF & (fetch >> 32 );
    *p++ = 0xFF & (fetch >> 24 );
    *p++ = 0xFF & (fetch >> 16 );
    *p++ = 0xFF & (fetch >>  8 );
    *p++ = 0xFF & (fetch       );
  }
  sprintf(pbm_filename,"%s/%03d.pbm", output_directory, xgp_page_number);
  printf("output to path: '%s'\n",pbm_filename);
  o = open(pbm_filename,O_CREAT|O_TRUNC|O_WRONLY,0644);
  if(o<0)handle_error(pbm_filename);
  n = write(o,header,strlen(header));
  printf("n=%d strlen(header)=%ld\n",n,strlen(header));
  assert(n==strlen(header));
  n = write(o,raster,sizeof(raster));
  assert(n==sizeof(raster));
  assert(!close(o));
  //  exit(0); // quit early: one page for test patterns.
  bzero(raster,sizeof(raster));
  anyink=0;xgpfont=0, xgprow=200, xgpcol=100;
}

/*
 * unicode 16-bit value for each SAIL 7-bit character
 * further remarks about the UTF8 conversion
 *
 *      NUL zero bytes will be omitted from TEXT files
 *              consider using unicode Zero-Width-Space U+200B
 *              if there is a need for reversibility
 *      DEL /177 is mapped into a unicode BS backspace glyph ␈u2408 or now ⌫ u232B
 */
unsigned short sail_unicode[128]={
  0,
  0x2193, // ↓ down arrow
  0x03b1, // α alpha
  0x03b2, // β beta
  0x2227, // ∧ boolean AND
  0x00ac, // ¬ boolean NOT
  0x03b5, // ε epsilon
  0x03c0, // π pi
  0x03bb, // λ lambda
  011,    012,    013,    014,    015,   // TAB, LF, VT, FF, CR as white space
  0x221e, // ∞ infinity
  0x2202, // ∂ partial differential

  0x2282, 0x2283, 0x2229, 0x222a, // ⊂ ⊃ ∩ ∪
  0x2200, 0x2203, 0x2297, 0x2194, // ∀ ∃ ⊗ ↔
  0x005f, 0x2192, 0x007e, 0x2260, // _ → ~ ≠
  0x2264, 0x2265, 0x2261, 0x2228, // ≤ ≥ ≡ ∨

  040,041,042,043,044,045,046,047, // ␣ ! 
  050,051,052,053,054,055,056,057,
  060,061,062,063,064,065,066,067,
  070,071,072,073,074,075,076,077,

  0100,0101,0102,0103,0104,0105,0106,0107,
  0110,0111,0112,0113,0114,0115,0116,0117,
  0120,0121,0122,0123,0124,0125,0126,0127,
  0130,0131,0132,0133,0134,0135,
         
  0x2191, // ↑ up arrow
  0x2190, // ← left arrow

  0140,0141,0142,0143,0144,0145,0146,0147,
  0150,0151,0152,0153,0154,0155,0156,0157,
  0160,0161,0162,0163,0164,0165,0166,0167,
  0170,0171,0172,
  
  0x7b,    // { left curly bracket
  0x7c,    // | vertical bar
  0xA7,    // § Stanford ALT character. ⎇ altmode
  0x7d,    // } right curly bracket ASCII octal 0175, but SAIL code 0176 !
  0x232B   // Backspace ⌫ glyph ⌫
};

/*
 * Initialize the UTF8 tables using the 16-bit unicode characters
 * defined in sail_unicode[128] corresponding to the 7-bit SAIL incompatible ASCII codes.
 */
char utf8_[128*4];      // content of utf strings
char *utf8[128];        // string pointers
int utf8size[128];      // string length
#define PUTCHAR(U)*p++=(U)
void
initialize_utf8_tables(){
  int i,u;
  char *p=utf8_;
  /**/
  for (i=0;i<128;i++){
    utf8[i] = p;
    u = sail_unicode[i];
    if ( 0x0001<=u && u<=0x007f ){
      utf8size[i]=1;
      PUTCHAR( u );
      PUTCHAR(0);
      PUTCHAR(0);
      PUTCHAR(0);
    } else
      if ( 0x00a0<=u && u<=0x07ff ){
        utf8size[i]=2;
        PUTCHAR(0xC0 | ((u>>6) & 0x1F));
        PUTCHAR(0x80 | ( u     & 0x3F));
        PUTCHAR(0);
        PUTCHAR(0);
      } else
	if ( 0x0800<=u && u<=0xffff ){
          utf8size[i]=3;
          PUTCHAR(0xE0 | ((u>>12) & 0x0F));
          PUTCHAR(0x80 | ((u>>6 ) & 0x3F));
          PUTCHAR(0x80 | ( u      & 0x3F));
          PUTCHAR(0);
        } else {
          utf8size[i]=0;
          PUTCHAR(0);
          PUTCHAR(0);
          PUTCHAR(0);
          PUTCHAR(0);          
        }
  }
}
#undef PUTCHAR

/*
  convert SAIL-ASCII data5 7-bit bytes into 3 streams: plain, fancy, ink bits.
  convert the SAIL 'binary' XGP format into:
  1. 'plain' no-markup utf8 text.
  2. 'fancy' with binary XGP escape commands expanded into readable text.
  3. 'ink' place the bit appropriate bit-raster-glyph into XGP bit raster page buffer.
*/
void
convert_data5_into_utf8(void){
  unsigned char c;
  unsigned char c1,c2,c3,c4;
  int i;
  
  for ( i=xgp_obuf5_i0; i < size5; i++ ){
    
    c = obuf5[i] & 0x7F;

    switch(c){
    case 011: continue;
    case 012: { // newline LineFeed
      if( obuf5[i+1]==0177 &&  obuf5[i+2]==3){
        fprintf(fancy,"<LF row=%d TeX>",xgprow);
        continue; // TeX linefeed+setscanline;
      }
      int maxhigh = xgp_font_height; // current font
        
      // Look ahead into the next line for font-select commands of greatest height.
      void *bol = (void *)(obuf5+i+1);
      void *eol = memchr( bol, '\n', size5-i );
      void *needle = "\177\1";
      if(eol){
        size_t linelength = eol - bol;
        void *hit = memmem( bol, linelength, needle, 2);
        while( hit ){
          int newfont = *(unsigned char *)(hit + 2);
          if(newfont<040){ // select-font command sub-code
            int newhigh = font_map[newfont][0201].full.word;
            // fprintf(stderr,"hit old %d new %d\n",xgpfont,newfont);
            if( maxhigh < newhigh ){
              // fprintf( stderr, "xgp_page#%d %d %d\n",xgp_page_number,maxhigh,newhigh);
              maxhigh = newhigh;
            }
          }
          // next
          bol = (hit+2);
          linelength = eol - bol;
          hit = memmem( bol, linelength, needle, 2);
        }
      }
      
      // increment XGP row position
      xgprow += maxhigh + xgp_xline;
      fprintf(fancy,"<LF row=%d>",xgprow);
      continue;
    }
    case 013: continue;
    case 014: // newpage FormFeed
      fprintf(fancy,"<FF>");
      pbm_output();
      printf("xgp_page_number #%d\n", xgp_page_number);
      xgp_page_number++;
      continue;
      
    case 015: // Carriage Return
      xgpcol = xgp_lmar;
      fprintf(fancy,"<CR col=%d>",xgpcol);
      continue;
      
      // XGP escape character is RUBOUT octal 0177.
    case 0177:
      c1 = obuf5[i+1] & 0x7F;
      c2 = obuf5[i+2] & 0x7F;
      c3 = obuf5[i+3] & 0x7F;
      c4 = obuf5[i+4] & 0x7F;
      switch(c1){
        // escape hidden - print glyph
      case 0:
      case 0177:
      case 011:
      case 012:
      case 014:
      case 015:
        ink_glyph( xgpfont, c1, xgprow, xgpcol );
        fprintf(fancy,"%s",(char *)utf8[c1]);
        i += 2;
        continue;        

        // TeX font select
      case 6:
        fprintf(fancy,"<Esc6 TeXfont=%d>\n",c2);
        // printf("\nTeXfont=%d>\n",c2);
        if(!( font_map[c2] )){
          fprintf(fancy,"\nfont_select(%d) FAILED. No such font.",c2);
          xgpfont = 0;
        }else{
          xgpfont = c2;
          //  xgpfont = 0;
        }
        xgp_font_height = font_map[xgpfont][0201].full.word;
        xgp_font_maxwid = font_map[xgpfont][0202].full.word;
        xgp_font_baseline = font_map[xgpfont][0203].full.word;
        i += 2;
        continue;
        
        // Font select AND sub-command Mickey-Mouse
      case 1:
        if(c2<040){
          fprintf(fancy,"\nfont_select(%d) ",c2);
          if(!( font_map[c2] )){
            fprintf(fancy,"\nfont_select(%d) FAILED. No such font.",c2);
            xgpfont = 0;
          }else{
            xgpfont = c2;
          }
          xgp_font_height = font_map[xgpfont][0201].full.word;
          xgp_font_maxwid = font_map[xgpfont][0202].full.word;
          xgp_font_baseline = font_map[xgpfont][0203].full.word;
          i += 2;
          continue;
        }
        switch(c2){
        case 040:
          xgpcol = (c3<<7)+c4;
          fprintf(fancy,"<Esc040 col=%d>", xgpcol );
          i += 4;
          continue;
        case 042:
          i += 3;
          xgprow += xgp_font_height +3 + c3; // linefeed PLUS the given # of rows
          fprintf(fancy,"\n<Esc042 row+(%d)=%d>", c3, xgprow );
          continue;
        case 043:
          fprintf(fancy,"\n<Esc043 baseline_adjust(%d)>", c3 );
          i += 3;
          continue;
        case 046:
          fprintf(fancy,"\nstart_underline() " );
          i += 2;
          continue;
        case 047:
          fprintf(fancy,"\nstop_underline(%d) ", c3 );
          i += 3;
          continue;
        case 051:
          fprintf(fancy,"\nstop_underline_thick(%d,%d) ", c3,c4 );
          i += 4;
          continue;
        default:
          fprintf(fancy,"Esc1-case-failure %03o\n",c2);
          fprintf(stderr, " XGP-ESC1-case#%d-failure page#%d %s\n",c2,xgp_page_number,output_directory);
          return;
        }
      case 2:
        if(c2 < 0100){
          xgpcol += c2;
          fprintf(fancy,"<Esc2 col+%d=%d>",c2,xgpcol);
        }else{
          int d2 = c2 - 128; // 7-bit twos complement
          xgpcol += d2;
          fprintf(fancy,"<Esc2 col%d=%d>",d2,xgpcol);
        }
        i += 2;
        continue;
      case 3:
        fprintf(fancy,"<Esc3 row=%d>",(c2<<7)+c3);
        xgprow = (c2<<7)+c3;
        i += 3; // advance buffer past command
        continue;
      case 4:
        {
          int x0,y0,dx,n,w;
          x0= y0= dx= n= w= 0;
          fprintf(fancy,"vector(%d,%d,%d,%d,%d)\n",x0,y0,dx,n,w);
        }
        continue;
      default:
        fprintf(stderr, " XGP-0177-case#%d-failure page#%d %s\n",c1,xgp_page_number,output_directory);
        i +=2 ;
        continue;
      } // end of switch 0177 (c1)
      continue;
      // normal (NON-whitespace and NON-rubout) characters all go through here as default
    default: break;
    } // end of switch(c)
    ink_glyph( xgpfont, c, xgprow, xgpcol );
    fprintf(fancy,"%s",(char *)utf8[c]);
  }
  if(anyink)
    pbm_output();
}

void
read_font_file(char* fontfile ){
  int fd, word_count;
  int height, width, baseline;
  size_t  byte_count;
  pdp10_word* p;
  int a;
  char prj[4],prg[4],filnam[8],fontname[32];
  char description[256],*q=description;
  bzero(description,sizeof(description));
  
  // Read file in data8 format
  fd = open(fontfile,O_RDONLY);
  if (fd<0) handle_error(fontfile);
  byte_count = read(fd,buf8,sizeof(buf8));
  close(fd);
  word_count = byte_count/8;

  a = sscanf(fontfile,"/8/%3[a-z]/%3[a-z0-9]/%6[a-z0-9].fnt",prg,prj,filnam);
  if( a && strcmp(prj,"xgp")!=0 && strcmp(prg,"sys")!=0){
    // printf("a=%d  %d   %d ", a, strcmp(prj,"xgp"), strcmp(prg,"sys"));
    // printf("a=%d %s[%3.3s,%3.3s] ",a,filnam,prj,prg);
    sprintf(fontname,"%s[%.3s,%.3s]",filnam,prj,prg);
  } else {
    sprintf(fontname,"%s",filnam);
  }
  string_to_upper(fontname);
  // printf("fontname='%s'\n",fontname);
  verbose=1;
  if(verbose)
    printf("READ %8ld bytes = %8d words from data8 font file %s\n", byte_count, word_count, fontfile );
  p = malloc(byte_count);
  memcpy(p,buf8,byte_count);
  height = p[0201].u18.right;
  width = p[0202].u18.right;
  baseline = p[0203].u18.right;
  
  for (a=0240;a<0400;a++){
    if( p[a].fw ){
      // printf("%06o / %06o,,%06o \n",a, p[a].u18.left, p[a].u18.right);
      strcat(q,SAIL_ASCII[ p[a].seven.a1 ]);
      strcat(q,SAIL_ASCII[ p[a].seven.a2 ]);
      strcat(q,SAIL_ASCII[ p[a].seven.a3 ]);
      strcat(q,SAIL_ASCII[ p[a].seven.a4 ]);
      strcat(q,SAIL_ASCII[ p[a].seven.a5 ]);
    }
  }
  if(verbose)
    printf(" xgpfont#%d font file %s font name '%s'\nwidth=%d height=%d baseline=%d\ndescription: %s\n\n",
           xgpfont,fontfile,fontname,width,height,baseline,description);
  verbose=0;
  
  font_atlas[xgpfont]= p;
  font_atlas_name[xgpfont]= strdup(fontname);
  // pre XGP testing
  font_map[xgpfont]= p;
  font_map_name[xgpfont]= strdup(fontname);

  xgpfont++;
}
int
read_xgp_file(char *path_xgp){
  // Read an XGP file in data8 format
  int byte_count;
  int word_count;
  int w;
  //
  int i = open(path_xgp,O_RDONLY);
  if (i<0){
    fprintf(stderr,"ERROR: input source open file \"%s\" failed.\n",path_xgp);
    return 0;
  }   
  byte_count = read(i,buf8,sizeof(buf8));
  close(i);
  word_count = byte_count/8;
  size5 = word_count*5;
  printf("READ from data8  %8d bytes = %7d words.\n",byte_count,word_count);

  // convert buf8 into buf5
  for ( w=0; w<word_count; w++){
    // data8 word is 64 bits with the 36 bits of PDP-10 right adjusted.
    pdp10_word word;
    unsigned char *dst = obuf5 + w*5;
    word.fw = buf8[ w ];
    // data5 packing of 5 x 7 bits with the final 36th bit at 0x80 of 5th byte.
    *dst++ = word.seven.a1;
    *dst++ = word.seven.a2;
    *dst++ = word.seven.a3;
    *dst++ = word.seven.a4;
    *dst++ = word.seven.a5 | (word.seven.bit35 << 7);
  }

  // parse the XGP header line
  {
    char *str;
    char *str0 = (char *)obuf5;
    char *first_crlf = strstr(str0, "\r\n\f" );
    char *first_star_TEX_star = strstr(str0, "*TEX*" );
    char *token;
    int val;
    char namlong[32];
    char nam[32];

    if(first_star_TEX_star){
      int n = (first_star_TEX_star - str0);
      str = strndup(str0,n);
      printf("This is a *TEX* header !\n");
      printf("%d bytes, first_line='%s'\n", n, str);
      strcpy( first_star_TEX_star, "\r\n\f");
      first_crlf = first_star_TEX_star;      
    }
    
    if(first_crlf){
      int n = (first_crlf - str0);
      str = strndup(str0,n);
      printf("%d bytes, first_line='%s'\n", n, str);

      // place the "read-head" at the "start-of-data"
      // PDP10 disk block 128. words containing 640 characters.
      xgp_obuf5_i0 = n+3; // <header-string> CR LF FF <the-real-stuff>
      assert( obuf5[n] == '\r' );
      assert( obuf5[n+1] == '\n' );
      assert( obuf5[n+2] == '\f' );
      
      token = strtok(str,"/");
      while( token != NULL ){
        if( !strcmp( token, "NOWRAPAROUND")){
          xgp_nowraparound = 1;
          printf("No wrap around ! token %s\n",token);
        }
        if(2 == sscanf( token, "FONT#%d=%[][ A-Z0-9,]", &val, namlong )){
          int u;
          omit_spaces(namlong);
          sscanf( token, "FONT#%d=%[A-Z0-9]", &val, nam );
          printf("  lookup font namlong='%s' nam='%s'\n", namlong,nam );
          for(u=0;u<FONTMAX;u++){
            if( font_atlas_name[u] &&
                ( !strcmp( font_atlas_name[u] , namlong ) ||
                  !strcmp( font_atlas_name[u] , nam )
                  )){
              font_map[val] = font_atlas[u];
              font_map_name[val] = strdup(nam);
              printf("  Font#%d atlas lookup hit %s in atlas at #%d\n",val,nam,u);
              break;
            }
          }
          if(u==FONTMAX){
            printf("          FONT MISSING: %s %s\n",namlong, nam);
          }
        }
        
        n =
          1 == sscanf( token, "XLINE=%d",&xgp_xline) ||
          1 == sscanf( token, "LMAR=%d", &xgp_lmar ) ||
          1 == sscanf( token, "RMAR=%d", &xgp_rmar ) ||
          1 == sscanf( token, "TMAR=%d", &xgp_tmar ) ||
          1 == sscanf( token, "BMAR=%d", &xgp_bmar ) ||
          1 == sscanf( token, "PMAR=%d", &xgp_pmar ) ||
          1 == sscanf( token, "USETI=%d", &xgp_useti); // DECIMAL number with zero-padding.
        
        printf("%d %s\n",n,token);        
        token = strtok(NULL,"/");
      }
    }
  }
  // parse the TeX postamble
  if(xgp_useti){
    char *token;
    char *str0 = (char *)obuf5;
    char *postam = str0 + (xgp_useti-1)*5*128; // seek offset into XGP file by PDP10 block size 128. words
    int val;
    char nam[32];
    int i;

    xgp_obuf5_i0 = 01200;         // <TeX header-PDP10-disk-block>  <the-real-stuff>  <TeX postamble>
    int eof=size5-1; // real EOF includes preamble, index of final byte in obuf5
    size5 = (xgp_useti-1)*5*128;  // new EOF. Omit the TeX postamble from the body of text.
    printf("TeX postamble useti %d.  size5=%ld\n",xgp_useti,size5);
    for (i=size5;i<eof;i++){
      if(obuf5[i]==0){
        obuf5[i] = 1; // "overwrite" occassional NULs following "=" signs so strtok works.
      }
    }
    token = strtok(postam,"/");
    while( token != NULL ){
      printf("postamble token seen '%s'\n",token);
      char *p=strstr(token,"[XGP,SYS]");
      if(p){
        *p = 0;
        printf("postam token truncated '%s'\n",token);
      }
      if(2 == sscanf( token, "FONT#%d=%[^=]", &val, nam )){
        int u;
        string_to_upper( nam );
        for(u=0;u<FONTMAX;u++){
          if( font_atlas_name[u] && !strcmp( font_atlas_name[u] , nam )){
            font_map[val] = font_atlas[u];
            font_map_name[val] = strdup(nam);
            printf("  Font#%d atlas lookup hit XGP %s in atlas at #%d\n",val,nam,u);
            break;
          }
        }
        if(u==FONTMAX){
          printf("          TeX FONT MISSING: %s\n",nam);
        }
      }
      token = strtok(NULL,"/");
    }
  }
  return 1;
}

void
read_fnt_batch(){
  char fnt_path[128];
  FILE *batch = fopen("/data/home/font/batch_fnt","r");
  xgpfont=0;
  while(fgets(fnt_path,128,batch)){
    *index(fnt_path,'\n') = 0;
    if(! *fnt_path)break;
    // printf("Font %-24s ",fnt_path);
    read_font_file(fnt_path);
  }
  xgpfont=0;
}

void
read_xgp_batch(){
  char xgp_path[128];
  FILE *batch = fopen("/data/home/font/batch_xgp","r");
  while(fgets(xgp_path,128,batch)){
    *index(xgp_path,'\n') = 0;

    // empty line or # Comment
    if(!strlen(xgp_path) || xgp_path[0]=='#' ){
      continue;
    }
    
    if(!strcmp( xgp_path, "quit" )){
      printf("QUIT batch_xgp\n");
      break;
    }
    if(!strncmp( xgp_path, "mkdir ",6 )){
      xgp_page_number = 1;
      bzero(raster,sizeof(raster));
      anyink=0; xgpfont=0, xgprow=200, xgpcol=100;

      // printf("MKDIR %-24s \n",xgp_path);      
      output_directory = strdup(rindex(xgp_path,' ')+1);
      printf("change output directory %-24s\n", output_directory );
      continue;
    }
    printf("XGP convert input %-24s \n",xgp_path);
    fflush(stdout);
    xgprow=200; xgpcol=100;
    if(read_xgp_file(xgp_path)){
      convert_data5_into_utf8();
    }
  }
}

/* MAIN xgp convert 2018

   Input:
   read batch file of SAIL data8 *.FNT bit-raster font path names
   read batch file of SAIL data8 *.XGP device-binary text path names

   Processing:
   Load all the bit-raster fonts filnam.FNT into a font atlas.
   For each filnam.XGP
   0. Parse header setting and font map names.
   1. repack data8 into data5 characters (consider starting from data5)
   2. parse data5 as 8-bit ASCII with escape 0177 character prefix into binary multi-byte XGP commands.
   0177 character prefix for a multi-byte XGP command.
*/
int
main(int ac,char **av){
  fancy = fopen("/data/home/font/fancy","w");
  if(!fancy)handle_error("fancy");
  initialize_utf8_tables();
  bzero(raster,sizeof(raster));anyink=0;
  read_fnt_batch();
  read_xgp_batch();
  fclose(fancy);
  return(0);
}
