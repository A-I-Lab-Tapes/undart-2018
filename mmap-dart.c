#if 0
  NAME="mmap-dart"
  SRC="/data/DARTremix."
  echo -n self_compiling $SRC/$NAME
  echo '  CopyrightⒸ2016-2018.Bruce_Guenther_Baumgart.Software_License:GPL-v3.'
  echo gcc -g -m64 -Wall -Werror -o /usr/local/bin/$NAME $SRC/$NAME.c $SRC/md5.c
  gcc -g -m64 -Wall -Werror -o /usr/local/bin/$NAME $SRC/$NAME.c $SRC/md5.c
  return
#endif

/* Read the 229 reels of concatenated 9-track DART tapes
 *                 from the 85GB file named: /large/flat_DART_data8
 *     lookup blob hash serial numbers from: /large/sn-hash8-accession-by-sn
 * ------------------------------------------------------------------------------------ *
 *                            M  M  A  P  -  D  A  R  T                                 *
 * ------------------------------------------------------------------------------------ *
 */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include "md5.h"
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define handle_error(msg)                               \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef unsigned long long uint64;
typedef          long long  int64;
typedef unsigned long long u64;
typedef          long long i64;
typedef unsigned char uchar;

// This union defines PDP10 bit-field names within one Big Endian 36-bit PDP10 word.
// The programming language 'C' x86 words are Little Endian, so Right comes before Left.
typedef union {
  uint64 fw;
  struct {  int64  word:36,        :28; } full; //    signed
  struct {  int64 right:18,left:18,:28; } half; //    signed
  struct { uint64  word:36,        :28; } u36;  // Un-signed
  struct { uint64 right:18,left:18,:28; } u18;  // Un-signed
  struct { uint64 hi:12,mid:12,lo:12,:28; } third;
  struct { uint64 c6:6,c5:6,c4:6,c3:6,c2:6,c1:6,:28;}    sixbit; // sixbit+040
  struct { uint64 bit35:1,a5:7,a4:7,a3:7,a2:7,a1:7,:28;	} seven; // 'seven bit ASCII' or SAIL-ASCII
} pdp10_word;

// The so called Tape Record is a pointer into the big buffer
pdp10_word *Tape;
pdp10_word Template[36]; // Copy from first 36. words of a -3 START_FILE tape record

// Doctored seven-bit SAIL-ASCII into UTF-8 string table.
//   ======================================================================
//    ↓αβ∧¬επλ\t\n\v\f\r∞∂⊂⊃∩∪∀∃⊗↔_→~≠≤≥≡∨ !\"#$%&'()*+,-./0123456789:;<=>?
//   @ABCDEFGH I J K L MNOPQRSTUVWXYZ[\\]↑←'abcdefghijklmnopqrstuvwxyz{|⎇}␈
//   ======================================================================
// TEXT conversion omits NUL, VT, RETURN, ALT and RUBOUT ! codes \000 \015    \0175   \0177
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

//      DART tape file records (type -3 or 0) Headers, here DART-Header is named 'darth'.
//      In data8 format,  the sizeof(darth) is 288. bytes which contains 36. PDP10 words.
//      Naturally the Virtual Address point is named Vaddr.
//
//      RIGHT halfword ,, LEFT halfword  ; 28. bits unused.
typedef struct {
  int64 leng:18,type:18,:28;            // word  0. Type,,Leng
  uint64 DSK___:36,:28;                 // word  1. 'DSK␣␣␣' marker
  // RIB = Retrieval-Information-Block for the SAIL Ralph File System
  uint64 filnam:36,:28;                 // word  2. FILNAM      DDNAM
  uint64 c_date:18,ext:18,:28;          // word  3. EXT         DDEXT
  uint64 date_lo:12,time:11,mode:4,prot:9,:28; // word  4.      DDPRO
  uint64 prg:18,prj:18,:28;             // word  5.             DDPPN
  uint64 track:36,:28;                  // word  6.             DDLOC
  uint64 count:22,:42;                  // word  7.             DDLNG           file size word count
  uint64 refdate:12, reftime:11, :41;   // word  8.             DREFTM          access date time
  uint64 dumpdate:15, dumptape:12, :2,  // word  9.             DDMPTM           dump  date time
    never:1, reap:1, P_invalid:1, P_count:3, TP:1;  // dart bookkeeping inside the disk file system
  uint64 word10:36,:28; // DGRP1R
  uint64 word11:36,:28; // DNXTGP
  uint64 word12:36,:28; // DSATID
  // DQINFO five words
  uint64 word13:36,:28;
  uint64 word14:36,:28;
  uint64 write_program:36,:28;  // word15. WPROGRM              WRTool
  uint64 write_ppn:36,:28;      // word16. WPPN                 WRTppn
  uint64 word17:36,:28;
  // Thin AIR: six words from master copy tape-to-tape, MCOPY Leader.
  uint64 DART__:36,:28; // word 18. 'DART  ' marker
  uint64 _FILE_:36,:28; // word 19. '*FILE*' or 'CON  #' marker
  uint64 word20:36,:28;
  uint64 word21:36,:28;
  uint64 word22:36,:28; // MCOPY class==2,,reel#3000 and up
  uint64 word23:36,:28;
  // Landmark AIR: five words of (sometimes unreliable / unnecessary) redundant information
  uint64 otnum:36,:28; // old tape number .. Temporary tape numbering 0 to 3526 seen
  uint64 word25:36,:28; //  0
  uint64 word26:36,:28; // -1
  uint64 word27:36,:28; //  0
  uint64 word28:36,:28; // words remaining
  // Clear AIR: seven word block of zeroes.
  uint64 word29:36,:28; // 0
  uint64 word30:36,:28; // 0
  uint64 word31:36,:28; // 0
  uint64 word32:36,:28; // 0
  uint64 word33:36,:28; // 0
  uint64 word34:36,:28; // 0
  uint64 word35:36,:28; // 0
} darth;
darth *Vaddr;

//
//      DART records 6,,000013 for BOT and EOT tape events
//
/* The HEADER AND TRAILER BLOCKS:
   Word  0: version,,length
   "version" is the positive version number of the DART that created this tape (VERSION);
   "length" is always octal 013, decimal 11. the length of the data following, includes the rotated checksum word below.
   Word  1: 'DART  ' sixbit DART
   Word  2: '*HEAD*' or '*TAIL*' sixbit, indicates Header or Trailer block,
   Word  3: time,date	in file system format
           Bits 13:23 are time in mins, bits 24:35 are date.
           Bits 4:12 are unused (considered high minutes).
           DART Version 5: Bits 0-2 are high date.
           DART Version 6: Bit 3 means this is time from prev media
   Word  4: ppn 		the name of the person running Dart.
   Word  5: class,,tapno	Tape number of this tape

   Dump class of this dump
   The top bits of tapno indicate which
   sequence of tapes this nbr is in.

   Word  6: relative dump,,absolute dump	(relative is within this tape)
   Word  7: tape position in feet from load point

   Word  8:	 0		reserved for future use
   Word  9:	-1		all bits on
   Word 10:	 0		all bits off
   Word 11.	rotated checksum of words 1 through 12 above.
*/
typedef struct {
  int64 leng:18,type:18,:28;            // word#0.
  uint64 DART__:36,:28;                 // word#1.
  uint64 headtail:36,:28;               // word#2.
  uint64 date_lo:12,time:11+9,dateflag:1,date_hi:3,:28; // word#3
  uint64 prg:18,prj:18,:28;             // word#4. PPN
  uint64 tapeno:18,class:18,:28;        // word#5.
  uint64 word6:36,:28;                  // word#6.
  uint64 feet:36,:28;                   // word#7.
  uint64 zip8;                          // word#8 is always zero.
  int64 neg1:36,:28;                    // word#9 is always -1 == octal 777777,,777777
  uint64 zip10;                         // word#10 is always zero.
  uint64 word11:36,:28; // Rotated checksum of words #1. through #10. above.
} ebot;
ebot *U;

int
main(int argc, char *argv[])
{
  char *addr;
  int fd;
  struct stat sb;
  off_t offset, pa_offset;
  size_t length;
  int type;
  int iflatdart;
  char *infile="/large/flat_DART_data8";
  int dartrn=1;
  long histo[10];
  
  // Input FILE format is a long "DART 9Track tape" eight bytes per PDP10 word.
  fd= iflatdart= open( infile, O_RDONLY );
  if (fd == -1)
    handle_error("open");

  if (fstat(fd, &sb) == -1)           /* To obtain file size */
    handle_error("fstat");

  // offset = atoi(argv[2]);
  offset = 0;
  pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
  /* offset for mmap() must be page aligned */
  if (offset >= sb.st_size) {
    fprintf(stderr, "offset is past end of file\n");
    exit(EXIT_FAILURE);
  }
  length = sb.st_size - offset;
  
  addr= mmap(NULL, length + offset - pa_offset, PROT_READ, MAP_PRIVATE, fd, pa_offset);
  Vaddr= (darth*)addr;
  Tape = (pdp10_word *)Vaddr;
  if (addr == MAP_FAILED)
    handle_error("mmap");
  fprintf(stderr,"%s mmap OK!\n",infile);
  
  // Buzz through counting DART records by type.
  // Takes 14 minutes with or without the fprintf.
  bzero(histo,sizeof(histo));
  for(dartrn=1;dartrn<=2937291;dartrn++){
  // for (dartrn=1;dartrn<=9999;dartrn++){
    Vaddr= (darth*)Tape;
    type = Vaddr->type;
    assert( type==0 || type==-3 || type==6 || type==-9 );
    length = (type == -9) ? Tape[1].full.word + 2 : Tape[0].half.right + ((type==6) ? 1 : 2);
    histo[abs(type)]++;
    // fprintf(stderr,"%07d,%3d,%6lu\n",dartrn,type,length);
    Tape += length;
  }
  fprintf(stderr,"histo[0]= %8ld =? 1045270 \n",histo[0]);
  fprintf(stderr,"histo[3]= %8ld =? 1886472 \n",histo[3]);
  fprintf(stderr,"histo[6]= %8ld =?    5486 \n",histo[6]);
  fprintf(stderr,"histo[9]= %8ld =?      63 \n",histo[9]);
  fprintf(stderr,"  Total   %8ld records    \n",histo[0]+histo[3]+histo[6]+histo[9]);
}
