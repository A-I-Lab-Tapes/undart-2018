#if 0
  NAME="remix-combo"
  SRC="/data/DARTremix."
  echo -n self_compiling $SRC/$NAME
  echo '  CopyrightⒸ2016-2018.Bruce_Guenther_Baumgart.Software_License:GPL-v3.'
  echo gcc -g -m64 -Wall -Werror -o /usr/local/bin/$NAME $SRC/$NAME.c $SRC/md5.c
  gcc -g -m64 -Wall -Werror -o /usr/local/bin/$NAME $SRC/$NAME.c $SRC/md5.c
  return
#endif

typedef unsigned long long uint64;
typedef          long long  int64;
typedef unsigned long long    u64;
typedef          long long    i64;
typedef unsigned char uchar;

/* Read the 229 reels of concatenated 9-track DART tapes
 *                 from the 85GB file named: /large/flat_DART_data8
 *     lookup blob hash serial numbers from: /large/sn-hash8-accession-by-sn
 *      partition into read buffer segments: /large/csv/seek_46_segments.csv
 * ------------------------------------------------------------------------------------ *
 *      D       A       R       T       -       R       E       M       I       X       *
 *                                                                                      *
 *  C       O       M       B       I       N       A       T       I       O       N   *
 * ------------------------------------------------------------------------------------ *
 echo -n 'copyrightⒸ2016-2018 Bruce_Guenther_Baumgart software_license:GPL-v3.'

 INPUT file pathnames and MD5 values are given in the next paragraph.
 Setup the OUTPUT absolute directory pathnames like so,

        mkdir -p /large/{csv,log,unix}  /large/{data8,text}/sn

 Typical execution is:

        time remix-combo

cd /large/csv
cat nametag.[0-9][0-9]    >cat_nametag 
cat blobsnhash.[0-9][0-9] >cat_blobsnhash
--------------------------------------------------------------------------------------- *
INPUT blob md5 serial numbers are used as "accession numbers" from
               /large/sn-hash8-accession-by-sn
        md5sum /large/sn-hash8-accession-by-sn
        b9663bbd331646607452485b6e7d8f52  /large/sn-hash8-accession-by-sn

INPUT flat DART tape file from its pathname
                /large/flat_DART_data8
        md5sum  /large/flat_DART_data8 # takes about 14 minutes
        3adbff17fd7f9f6eb9107755594ae0b9  /large/flat_DART_data8

 which contains a concatenated image of all the reels of DART tape
 in a format called "data8" for "8 bytes per PDP10 word"
 where the 36-bit PDP10 words are right justified in 64-bits.
--------------------------------------------------------------------------------------- *
PROCESSING

 de-frag         concatenate DART-7track-record data-payloads into SAIL-blobs
 de-dup          hash digest SAIL-blob-data8-format to get serial numbered unique blob content
 de-damage       Mark files with Previous-Media-Error or defective headings,
 de-flate        omit excessive record padding and redundancy
        then in later processing
 de-tox          omit ephemera

 This remix "knows" in advance that the input 7track record statistics are:
 case  6: //     5,486. tape reel BOT and EOT markers HEAD and TAIL records
 case -3: // 1,886,472. file start records
 case  0: // 1,045,270. file continue records
 case -9: //        63. gap records

cut -c9-12 /large/log/rib|sort|uniq -c > rib-record-type-counts
1045268 /  0
1886474 / -3
   5486 /  6
     63 / -9 GAPs from 1998 tape reading defects
2937291 Total
 and that the maximum dart 7track record size is a GAP at 382438. words
 and that the maximum dart 7track record size not-a-GAP is 10240. words
 and that the minimum dart 7track record size       GAP is    27. words
 and that the minimum dart 7track record size not-a-GAP is    12. words
 and that the maximum file         blob  size is
 and that the 1990 MCOPY *ERROR.ERR records number 6621.
 and that the 1998 gaps                     number   63.

 Farb=0 indicates the highest level of authencity with a bit-exact equivalence to
 the data read from the 229 reels of physical DART 9track tape in 1998.

 This Remix program generates farb=1 FILES for the BOT and EOT tape type=6 records, 
 the ERROR.ERR[ERR,OR␣] records, and for the -9,,Gap records.
--------------------------------------------------------------------------------------- *
*/
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "md5.h"

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
//
// Magnetic Tape defects include bit-drop outs and missing frames,
// that is failures to detect magnetic "one" markings when reading the tape.
// Given two nearly identical files, the file with the highest bit count
// may be more likely to be the accurate copy.
//
// Initialization, at top of main:
// for(n=16;n<4095;n++){
// number_of_bits[n] = number_of_bits[n/256]+number_of_bits[(n/16)%16]+number_of_bits[n%16];
// number_of_bits[n] = number_of_bits[n>>8 ]+number_of_bits[(n>>4)&15]+number_of_bits[n&15];
// }
//
// Missing (or extra) zero mag-tape-data frames are detectable in DMP executibles
// when the static analysis opcode pattern of a well known DMP image goes bananas.
// The beacon landmark PUSHJ positions go AWOL on the damaged DMP files.
//
//                         0 1 2 3  4 5 6 7  8 9 A B  C D E F
int number_of_bits[4096]={ 0,1,1,2, 1,2,2,3, 1,2,2,3, 2,3,3,4, };

// OUTPUT database tables
FILE *blobsnhash;
FILE *nametag;
/*
// OUTPUT log text "bamboo" for appreciating
// the mostly empty (or unknown) DART header content.
FILE *rib; // SAIL disk file System SYS: Retreival-Info-Block
FILE *air; // empty space padding added during MCOPY conversion from 7 to 9 track tape.
FILE *prm; // Previous Media errors
*/
// For reading the flat_DART_data8 file,
// use largest possible buffer size, see READ man page.
//              2^31 = 2147483648    = INT_MAX + 1
size_t sizeof_buffer = 2147479552LL; //nearly 2 GByte will read 43 buffers from flat_DART_data8
pdp10_word *buffer; // malloc'd later
#define BILLION 1000000000LL
#define MILLION 1000000LL

// The so called Tape Record is a pointer into the big buffer
pdp10_word *Tape;
pdp10_word Template[36]; // Copy from first 36. words of a -3 START_FILE tape record

// SIXBIT/*HEAD*/ SIXBIT/*TAIL*/ constants, Land Mark values
#define HEAD 0125045414412LL
#define TAIL 0126441515412LL

#if 0 // NOT implemented. NOT needed (yet) in light of the CSV representation. 2018.
// BINARY data8 remix record for a Tag-and-Blob representation for the WAITS-File-System.
typedef struct {
  uint64 record_length:32,:32, // word0
    zero_padding:32,      :32, // word1 PAD zero count words AFTER final non-zero word.
  //
    prj:18,prg:18,        :28, // word2
    filnam:36,            :28, // word3
    ext:18,typ:18,        :28, // word4
    mdate:18,mtime:18,    :28, // word5
  //
    wrtppn:36,            :28, // word6
    wrtool:36,            :28, // word7
  //    
    sn:20,damage:6,       :38, // word8
    drn:23,prot:9,mode:4, :28; // word9
} tag;
tag Tag;
#endif

// Blob ( data portion of a file accumulated from
//              -3,,leng        File Start with all its
//               0,,leng        File Continued records.
//              -9,,0           gap record
//                  leng        gap length
//               6,,11.         BOT and EOT markers are always 12. words long
//
//                      7727384 // PDP10 words in largest ERRBLOB      sn=992389
//                      3020800 // PDP10 words in largest "legal" blob sn=886393
//
#define MAX_BYTE_COUNT 65000000LL // Larger then any blob on the flat tape file
#define MAX_WORD_COUNT ((MAX_BYTE_COUNT+4)/5)

// pdp10_word blob[ MAX_WORD_COUNT ]; // 64-bit words.
// char utf8[ MAX_BYTE_COUNT ];
size_t sizeof_blob = MAX_BYTE_COUNT;
size_t sizeof_utf8 = MAX_BYTE_COUNT;
pdp10_word *blob;
char *utf8;

int seen, blob_word_count, zcount;

//      DART tape file records (type -3 or 0) Headers, here DART-Header is named 'darth'.
//      In data8 format,  the sizeof(darth) is 288. bytes which contains 36. PDP10 words.
//      Naturally, the Virtual Address pointer is named Vaddr.
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
  uint64 otnum:36,:28; // old tape number .. old DAEMON tape numbering 0 to 3526 seen at word24
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
//
//      MD5 digest values of a file data BLOB
//
struct md5_ctx ctx;
unsigned char digest[32];
char hashhex[32+1]; // hexadecimal string
char hashm64[32+1]; // modulo 60 xx encoded string
//
//      Globals
//
int verbose=0;
int name_count=0, blob_count=0; // Name Tags and Blobs, model of File System
// int record=0;
int dart7track_record_number=0;
int reel_number=3000;
int tape_number=1; 
int length=0, type=0;   // DART head prefix
int drn=0;              // dart "file" record number, for types -3, 6, or -9.
int serial_number=0;
int bad_text_bit_count=0;
int otnum; // word#24 in Darth ( possibly "other tape number" )

// seek position of most recent DART 7Track File-Start type==0 record,
// used to reswind after "end-of-buffer" or
// used for one-pass "start-of-buffer" know size pre-positioning.
off_t seekaddr=0; // Updated for each DART 7Track record.
struct seekstate {
  int record, reel, tape; off_t seek; size_t bufsize;
} At[90];

// content counters
struct {int  bits, bytes, nul, newline, ff, pd, space, alnum, other;} ConCount;

// content letter frequency
// Assumed letter rank frequency per classic ETA OIN SRHLD CUM FPG WYB VKX JQZ for English text.
const char* urank=" ETAOINSRHLDCUMFPGWYBVKXJQZ"; // E is rank# 1, Z is rank# 26th in frequency.
char vrank[32]={}; // sampled document's letter frequency ranking
int64 histogram[128]={};
double spearman=0;

const char* taxon_name[]={  "BINARY","ETEXT","OTEXT",  "ERRBLOB","GAP","XOT","BOT7","EOT7","BOT9","EOT9"};
enum taxon_value{  BINARY,ETEXT,OTEXT,  ERRBLOB,GAP,XOT,BOT7,EOT7,BOT9,EOT9};
enum taxon_value taxon;

// FILE damage code bits, Reported as hexadecimal value "0xFF" for MYSQL, or empty ""
int damaged=0;
#define logdam(X) damaged|=X
char damrep[16];
void
damage_report(){
  damrep[0]=0;
  if(damaged)sprintf(damrep,"0x%03X",damaged);
}
//
//      DART tape nametag per DART data record
//
char prg[4], prj[4], ppn[8], filnam[8], ext[4];
char ppname[32];
char pathname[32];
int mode, prot, hidate;
char write_program[16];
char write_ppn[16];
//
//      Date Time stamps ( iso_mdatetime will become the most relevant )
//      created, modified, accessed, dumped
//
int cdate, mdate, adate, ddate;
int        mtime, atime;
//
// Remix Prescience
//
// 1972-11-05 11:59     Tape #0001 BOT is at Noon 5 November 1972, nearly 18 years later
// 1990-08-17 17:08     Tape #3228 the final file ALLDIR.DAT[DMP,SYS] is written.
//                      so the DART records span ten million minutes, since
//                      timestampdiff( minute, '1972-11-05 11:59',' 1990-08-17 17:08' )
//                      is 9,351,669.
//
// De-damage remixing enforces FILE date consistency using the tape reel BOT dates.
// When a FILE mdate is out of range, it defaults to the BOT date for its track7 tape number,
// since that would be the maximum possible date for the file.
//
char iso_mcopy_bot_datetime[32]; // BOT9 date from MCOPY track9 BOT marker
char iso_bdatetime[32]; // BOT7 date on prior BOT7 marker of track7 tape
char iso_cdatetime[32]; // file system RIB alleged "creation" date
char iso_mdatetime[32]; // usually seen file "modification" / "write" date
char iso_adatetime[32]; // "access" date
char iso_ddatetime[32]; // "dump" date
//
// DART record validation
//
uint64 xor_check_tally, xor_check_word;  char *xor_check_result;
uint64 zip_check,kword;
//
// DART record internal markers
//
char word_dsk[8];       // sixbit'DSK   '
char word18[8];         // expect sixbit'DART  ' 444162640000
char word19[8];         //
char wordpend[8];       // expect sixbit'$PEND$' 046045,,564404 at Tape[m+length=2]
char word_mcsys[8];
//
// Text editor file from TV and E (almost always) have a page table of contents.
// COMMENT ⊗   VALID 00006 PAGES
// C REC  PAGE   DESCRIPTION
//

// PDP-10 sixbit character decoding tables (and the mod64 xx encoding table)
// SIXBIT tables
//           1         2         3         4         5         6     Decimal Ruler
//  1234567890123456789012345678901234567890123456789012345678901234
//  0       1       2       3       4       5       6       7      7 Octal Ruler
//  0123456701234567012345670123456701234567012345670123456701234567
//   !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_ SIXBIT character values
//                              omit comma for bare csv fields
//                                   ↓ omit dot for FILNAM fields
//           Backwhack ↓             ↓ ↓                                       Backwhack ↓
char *sixbit_table= " !\"" "#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ" "[\\]^_"; // actual SIXBIT decode
char *sixbit_fname= " ! "  "#$%&'()*+_-_/0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ" "___^_"; // SAIL filenames sanity
char *sixbit_uname= " B "  "#$%APLRX+Y-DZ0123456789CSN=MW@abcdefghijklmnopqrstuvwxyz" "IQOV_"; // unix filenames sanity
char *sixbit_ppn  = " __"  "_____________0123456789_______ABCDEFGHIJKLMNOPQRSTUVWXYZ" "_____"; // Project Programmer SAFE
char *sixbit_sail = "_ab"  "cdefghijklmno0123456789pqrstuvABCDEFGHIJKLMNOPQRSTUVWXYZ" "wxyz-"; // base64 for SAIL bit exact CSV

char *sixbit_mod64= "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_"; // modulo 64 naive URL safe
char *sixbit_xx   = "+-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
char *sixbit_rfc  = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_"; // rfc3548 filename URL safe
char *sixbit4648  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; // rfc4648 pad
char *sixURL4648  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"; // rfc4648 pad with = URL safe

// 2 lines from undart.c
char *ugly =   ";'" "#" "&" ":" "!" "\"" "()" "[]" "^" "_" "*" "," "/" "<>" "?" "\\" ".";
char *pretty = "SP" "H" "A" "C" "B" "Q"  "LR" "IO" "V" "U" "X" "Y" "Z" "NM" "W" "K"  "D";

// Ugly looking SIXBIT filenames are often magnetic-tape failures which DART source code calls "Prior-Media-Errors",
// with the exception of the Print Spooler and News Wire temporary file names.
// 'C' as well as SQL string notation must backwhack double quote and backslash
// SAIL filename sanity omits dot and square brackets
// Unix filename sanity omits bash shell special characters

/* SAIL SYS: Ralph File System SIXBIT character set
---------------------------------------------------------------------------------------------------
00 ␣ space
01 !            ugly    B       bang
02 "            ugly    Q       quote
03 #            ugly    H       hash
04 $ tolerated
05 % tolerated
06 &            ugly    A       Ampersand       &amp;
07 '            ugly    P       'Postrophe
10 (            ugly    L       Left paren
11 )            ugly    R       Right paren
12 *            ugly    X       Xplat
13 + plus
14 ,            ugly    Y       y,comma-shape
15 - minus
16 .            ugly    D       Dot
17 /            ugly    Z       Z/ebra-shape

20 0
21 1
22 2
23 3
24 4
25 5
26 6
27 7
30 8
31 9

32 :            ugly    C       Colon
33 ;            ugly    S       Semicolon
34 <            ugly    N       less thaN       &lt;
35 = equal
36 >            ugly    M       More than       &gt;
37 ?            ugly    W       What WTF

40 @ atsign
41 A
42 B
43 C
44 D
45 E
46 F
47 G
50 H
51 I
52 J
53 K
54 L
55 M
56 N
57 O
60 P
61 Q
62 R
63 S
64 T
65 U
66 V
67 W
70 X
71 Y
72 Z

73 [            ugly    I       in
74 \            ugly    Q       back-Quack
75 ]            ugly    O       out
76 ^            ugly    V       vee-shaped
77 _ underbar
---------------------------------------------------------------------------------------------------
 */

/* ascii SIXBIT decoding functions
 * -------------------------------------------------------------------- *
 * sixbit word into ASCII string 
 * sixbit halfword into ASCII string
 */
char *
sixbit_word_into_ascii(char *p0,pdp10_word w,char *tbl)
{
  char *p=p0;
  // SIXBIT into ASCII by arithmetic would by + 040
  *p++ = tbl[w.sixbit.c1];
  *p++ = tbl[w.sixbit.c2];
  *p++ = tbl[w.sixbit.c3];
  *p++ = tbl[w.sixbit.c4];
  *p++ = tbl[w.sixbit.c5];
  *p++ = tbl[w.sixbit.c6];
  *p++= 0;
  return p0;
}
char *
sixbit_uint_into_ascii(char *p0, uint64 x, char *tbl)
{
  pdp10_word w=(pdp10_word)x;
  char *p=p0;
  // SIXBIT into ASCII by arithmetic would by + 040
  *p++ = tbl[w.sixbit.c1];
  *p++ = tbl[w.sixbit.c2];
  *p++ = tbl[w.sixbit.c3];
  *p++ = tbl[w.sixbit.c4];
  *p++ = tbl[w.sixbit.c5];
  *p++ = tbl[w.sixbit.c6];
  *p++= 0;
  return p0;
}
char *
sixbit_halfword_into_ascii(char *p0,int halfword,char *tbl)
{
  char *p=p0;
  *p++ = tbl[(halfword >> 12) & 077];
  *p++ = tbl[(halfword >>  6) & 077];
  *p++ = tbl[ halfword        & 077];
  *p++ = 0;
  return p0;
}
void
omit_spaces(char *q)
{
  char *t,*p;
  for (p= t= q; *p; p++)
    if (*p != ' ')
      *t++ = *p;
  *t++ = 0;
}

void
force_to_lowercase(char *p)
{
    for (;*p;p++)    
        if ('A'<=*p && *p<='Z')
            *p |= 040;
}

/* Replace ugly SIXBIT filename chararacters
 * -----------------------------------------
 * In filename SIXBIT there are 26 + 10 alphanumeric characters,
 * plus nine more characters that may appear in a SAILDART filename:
 *	hash		#       dollar		$
 *      percent		%       ampersand       &
 *      plus            +       minus           -
 *      equal sign      =
 *
 * Use capital letters in unix version of PDP-10 filenames to replace characters
 * that are ugly to use on unix GNU/Linux file systems.
 *      CHARACTER NAME		UGLY	Replacement
 *	--------------		----	-----------
 *	ampersand		'&'	A
 *	colon			':'	C
 *	space		        ' '	S
 *	bang			'!'	B
 *	double quote		'"'	Q
 *	left parens		'('	L left
 *	right parens		')'	R right
 *	left square bracket	'['	I like In
 *	right square bracket	']'	O like Out
 *	caret			'^'	V
 *	underbar		'_'	U
 *	asterisk(aka star)	'*'	X 
 *	comma			','	Y
 *      slash                   '/'     Z
 *      less-than               '<'     N thaN
 *      greater-than            '>'     M More
 *      question-mark           '?'     W Who-What-Where-When WTF
 *      back-slash              '\\'    K bacK
 *      dot                     '\.'    D
 *      semicolon               ';'     S
 *      apostrophe              "'"     P aPPostroPPhe dang FILNAM . LPT [SPL,SYS]
 */

void
replace_ugly_characters(char *p)
{
    char *q;
    char *ugly =   ";'" "#" "&" ":" "!" "\"" "()" "[]" "^" "_" "*" "," "/" "<>" "?" "\\" ".";
    char *pretty = "SP" "H" "A" "C" "B" "Q"  "LR" "IO" "V" "U" "X" "Y" "Z" "NM" "W" "K"  "D";
    assert( strlen(ugly) == strlen(pretty)); /* so check the character count */
    while ((q= strpbrk(p,ugly)))
    {
      *q = pretty[index(ugly,*q)-ugly]; /* Beautification by table lookup */
    }
}
void
unixname(char* flat, char* filnam, char* ext, char* prj, char* prg)
{
  char clean_filnam[16]={},clean_ext[16]={},clean_prj[16]={},clean_prg[16]={};
  bzero(clean_filnam,16);
  bzero(clean_ext,16);
  bzero(clean_prj,16);
  bzero(clean_prg,16);

    /* Convert UPPER to lower and replace ugly punctuation with UPPER codes */
    strncpy( clean_filnam, filnam, 6 );
    strncpy( clean_ext, ext, 3 );
    strncpy( clean_prj, prj, 3 );
    strncpy( clean_prg, prg, 3 );

    omit_spaces( clean_filnam );
    omit_spaces( clean_ext );
    omit_spaces( clean_prj );
    omit_spaces( clean_prg );

    force_to_lowercase( clean_filnam );
    force_to_lowercase( clean_ext );
    force_to_lowercase( clean_prj );
    force_to_lowercase( clean_prg );

    replace_ugly_characters( clean_filnam );
    replace_ugly_characters( clean_ext );

    /* UNIX like order: Programmer, Project, Filename, dot, Extension */
    sprintf( flat, "%.3s/%.3s/%.6s.%.3s", clean_prg, clean_prj, clean_filnam, clean_ext );

    /* Remove trailing dot, if extension is blank */    
    if ( flat[strlen(flat)-1] == '.' ){
      flat[strlen(flat)-1] = 0;
    }
}

// associative array using hsearch from the 'C' Library.
#define MAXSN 999999 // The early SAILDART collection was at 886465, 2015 numbered 886781 with 154 blob holes.
uchar BIT[]={1,2,4,8,16,32,64,128}; // boolean bit-vector
uchar snseen[1+(MAXSN/8)]={}; // boolean array (used by de-dup to determine when a blob value reappears)
long snmax=1; // next sn ( there is no serial#0 )
long snmax_data=1; // next sn ( there is no serial#0 )
long snmax_xot=980000; //    16. xots and up to 3228. bots and eots.
long snmax_err=990000; // 8,318. error blobs
long snmax_gap=999900; //    61. gaps
long check_sn(char *hash){
  ENTRY e,*f;                   // see the 'hsearch' man 3 page
  long value;
  e.key = hash;                 // NAME-key is the digest-hash
  f= hsearch(e,FIND);
  value = ( f ? (long)(f->data) : 0 );
  // if(value==0)fprintf(stderr,"check_sn HASH8/%s/ NOT found !\n",hash);
  return value;
}
long fetch_sn(char *hash){
  ENTRY e,*f;                   // see the 'hsearch' man 3 page
  e.key = hash;                 // NAME-key is the digest-hash
  if(!(f= hsearch(e,FIND))){    // is this blob's hash key in the table ?
    e.key = strdup(hash);       // place copy of hash-key into malloc space
    switch(taxon){
    case ERRBLOB: e.data = (void *)snmax_err++; break;
    case GAP: e.data = (void *)snmax_gap++; break;
    case XOT: e.data = (void *)snmax_xot++; break;
    default: e.data = (void *)snmax_data++; break;   // coin new VALUE next serial number
    }
    f = hsearch(e,ENTER);
  }
  assert(f);
  return((long)(f->data));
}

// The snhash8 have become the "Accession-Numbers" for this "Digital-Archive".
void load_old_snhash8(){
  ENTRY e,*f;
  FILE *old;
  char *old_snhash8="/large/sn-hash8-accession-by-sn";
  long sn,cnt=0;
  // char altkey[24];
  errno = 0;
  old = fopen(old_snhash8,"r");
  if (!(old)){
        fprintf(stderr,"Open failed path old_snhash8='%s'\n", old_snhash8);
        exit(1);
  }
  while(EOF != fscanf(old,"%ld,%32ms\n",&sn,&e.key/*,altkey*/)){
    e.data= (void *)sn;
    //    fprintf(stderr,"%ld,/%s/\n",sn,e.key);
    f = hsearch(e,ENTER);
    assert((f=&e));
    cnt++;
    if(sn>snmax)snmax=sn;
    if(sn>snmax_data && sn<980000)snmax_data=sn;
  }
  snmax_data++; // next available sn
  fprintf(stderr,"DONE load_old_snhash8 cnt=%ld next available sn=%ld\n",cnt,snmax_data);
}

void
load_seek_segmentation(){
    FILE *o;
    int bufcnt;
    errno=0; o=fopen("/large/csv/seek_46_segments.csv","r"); assert(errno==0);
    for(bufcnt=1;bufcnt<=46;bufcnt++){
        fscanf(o,"%3d,%12d,%04d,%04d,%12ld,%12ld\n",
              &bufcnt, 
              &At[bufcnt].record,
              &At[bufcnt].reel,
              &At[bufcnt].tape,
              &At[bufcnt].seek,
              &At[bufcnt].bufsize );
    }
    fclose(o);
    fprintf(stderr,"DONE load_seek_segmentation\n");    
}


void
iso_date(char *dest,int sysdate,int systime){
  int day,month,year,hour,minute,second;
  int dpm[]={0,31,28,31, 30,31,30, 31,31,30, 31,30,31}; // days per month
  // 123456789.1234
  // 00-00-00 00:00
  sprintf(dest,"%-20s","null");
  sprintf(dest,"_%06d_%06d_",sysdate,systime);

  // preview
  day   =  sysdate % 31 + 1;
  month = (sysdate / 31) % 12 + 1;  
  year  = (sysdate / 31) / 12 + 1964;
  hour  = systime / 60;
  minute= systime % 60;
  /*
    if(sysdate>0 && ((sysdate < (1968-1964)*372) ||  (sysdate > (1991-1964)*372) || (systime > 1440 ))){
    fprintf(stderr,"%7d / sysdate=%d systime=%d ",record,sysdate,systime);
    fprintf(stderr,"iso_date defect year %d month %d day %d hour %d minute %d\n",year,month,day,hour,minute);
    }  
  */
  if(sysdate < (1968-1964)*372) return; // below floor
  if(sysdate > (1991-1964)*372) return; // above ceiling
  if(systime > 1440 ) return;           // over the wall
  
  // Note 31 times 12 is 372.
  //    sysdate = (year - 1964)*372 + (month-1)*31 + (day-1)
  //    systime = hour*60 + minute
  day   =  sysdate % 31 + 1;
  month = (sysdate / 31) % 12 + 1;  
  year  = (sysdate / 31) / 12 + 1964;

  // Day of month too big ?
  if( day > ((month!=2) ? dpm[month] : (year%4 == 0) ? 29 : 28)) return; // bad day-of-month

  hour = systime / 60;
  minute = systime % 60;
  second = 0;

  // Hour two big ?
  if(hour >= 24){ hour=23; minute=59; } // fake it
  
  // Check for the 23-hour Day Light Savings Spring Forward dates
  // increment one hour for times between 02:00 AM and 02:59 AM inclusive
  switch(sysdate){
  case   920: //  24-Apr-1966
  case  1310: //  30-Apr-1967
  case  1692: //  28-Apr-1968
  case  2075: //  27-Apr-1969
  case  2458: //  26-Apr-1970
  case  2841: //  25-Apr-1971
  case  3230: //  30-Apr-1972
  case  3613: //  29-Apr-1973
  case  3878: //   6-Jan-1974 Nixon oil crisis Daylight-Savings-Time
  case  4311: //  23-Feb-1975 ditto (but Ford was president in 1975)
  case  4761: //  25-Apr-1976
  case  5144: //  24-Apr-1977
  case  5534: //  30-Apr-1978
  case  5917: //  29-Apr-1979
  case  6299: //  27-Apr-1980
  case  6682: //  26-Apr-1981
  case  7065: //  25-Apr-1982
  case  7448: //  24-Apr-1983
  case  7837: //  29-Apr-1984
  case  8220: //  28-Apr-1985
  case  8603: //  27-Apr-1986
  case  8965: //   5-Apr-1987
  case  9347: //   3-Apr-1988
  case  9730: //   2-Apr-1989
  case 10113: //   1-Apr-1990
    if(hour==2){ // 2 AM clocks SPRING forward ... for GNU/linux that hour is INVALID.
      hour++; // work around for the impossible datetime stamp values
    }
    break;
  default: break;
  }
  // No adjustment for leap seconds.
  // ================================================================================
  // From 1972 to 1991 there were seventeen positive LEAP SECONDS
  // which occured at GMT 00:00:00 on 1 January 1972 to 1980, 1988, 1990, 1991.
  //           and at GMT 00:00:00 on 1 July 1972, 1982, 1983 and 1985.
  // There were no LEAP SECOND in 1984, 1986, 1987 and 1989.
  //    On 1 January 1972 the difference (TAI-UTC) was 10 seconds, and
  //    on 1 January 1991 the difference (TAI-UTC) was 26 seconds.
  // Although NTP Network-Time-Protocol overlaps the SAILDART epoch,
  // the computer date time was normally set from somebody's wristwatch
  // after a building power failure. The PDP10 clock interrupt was driven
  // at 60 hertz by powerline cycles, the Petit electronic clock likely
  // had a crystal oscillator but was set by software based on a wristwatch reading.
  // =================================================================================
  // No adjustment for leap seconds.
  sprintf( dest,"%4d-%02d-%02dT%02d:%02d:%02d",year,month,day,hour,minute,second );
}

void
output_blob_sn(){
  int n,bytcnt,sno;  // Serial-Numbered Output.
  char sn_path[256];
  // DATA8
  // Write blob to Serial Numbered path as eight bytes per PDP10 word.
  errno=0;
  sprintf(sn_path,"/large/data8/sn/%06d",serial_number);
  if(access(sn_path,F_OK)==-1){// Test for Non-Existence
    sno = open( sn_path, O_CREAT | O_WRONLY | O_TRUNC, 0600 );
    if (!(sno>0)){perror("WTF:");fprintf(stderr,"Open failed filename sn_path='%s'\n", sn_path);exit(1);}
    bytcnt = seen * 8;
    n = write( sno, blob, bytcnt );
    assert( n == bytcnt );
    assert(!close( sno ));
  }
  // TEXT UTF8 (former data7) 
  // Write blob to Serial Numbered path as UTF8 text.
  errno=0;
  sprintf(sn_path,"/large/text/sn/%06d",serial_number);
  if( taxon==ETEXT || taxon==OTEXT ){
    if(access(sn_path,F_OK)==-1){// Test for Non-Existence
      sno = open( sn_path, O_CREAT | O_WRONLY | O_TRUNC, 0600 );
      if (!(sno>0)){perror("WTF:");fprintf(stderr,"Open failed filename sn_path='%s'\n", sn_path);exit(1);}
      bytcnt = strlen(utf8);
      n = write( sno, utf8, bytcnt );
      assert( n == bytcnt );
      assert(!close( sno ));
    }
  }
}
void
reset_globals(){
  hashhex[0]=hashm64[0]=0;
  seen= 0;
  drn= 0;
  serial_number= 0;
  blob_word_count= 0;
  zcount = 0;
  prot= mode= 0;
  ppname[0]= iso_mdatetime[0]= 0;
  pathname[0]= 0;
  write_ppn[0]= write_program[0]= 0;
  damaged=0;
  taxon=BINARY; // default
  bad_text_bit_count=0;
  otnum=0;
  // memset(&Tag,0,sizeof(Tag));
  memset(histogram,0,sizeof(histogram));
  spearman=0;
}

void
decode(){
  // 5486. tape BOT and EOT type=6 records
  if(type==6){
    taxon = XOT;
    drn = dart7track_record_number;
    U = (ebot *)Vaddr;
    // 16. defective type==6 records (so XOT because it is not a good EOT or BOT).
    if(!(U->class == 2)){
      int markcnt=0; // No change
      drn = dart7track_record_number;
      sprintf(filnam,"MARK%02d", markcnt );
      strcpy( ext, "ERR");
      strcpy( prj, "XOT");
      strcpy( prg, "ERR");
      sprintf( ppn,    "%3.3s%3.3s",             prj,prg);
      sprintf( ppname, "%3.3s %3.3s %6.6s %3.3s",prj,prg,filnam,ext);
      unixname(pathname,filnam,ext,prj,prg);
      strcpy(write_program,"-REMIX");
      strcpy(write_ppn,    "ERRBGB");
      strcpy( iso_mdatetime, iso_bdatetime ); // use BOT date
      return;
    }
    sprintf(filnam,"P%05d",U->tapeno);
    strcpy( ext,
            U->headtail == HEAD ? "BOT" :
            U->headtail == TAIL ? "EOT" : "???" );
    sixbit_halfword_into_ascii( prj, U->prj, sixbit_ppn );
    sixbit_halfword_into_ascii( prg, U->prg, sixbit_ppn );
    sprintf( ppn,       "%3.3s%3.3s",             prj,prg);
    sprintf( ppname,    "%3.3s %3.3s %6.6s %3.3s",prj,prg,filnam,ext);
    unixname(pathname,filnam,ext,prj,prg);
    mdate = U->date_lo | (U->date_hi << 12);
    mtime = U->time;
    iso_date(iso_mdatetime,mdate,mtime); iso_mdatetime[10]=' '; iso_mdatetime[16]=0;
    if( U->tapeno<3000 && (U->headtail == HEAD)){
      strcpy( iso_bdatetime, iso_mdatetime ); // BOT date of pre-MCOPY tape
      tape_number = U->tapeno;
    }
    if( U->tapeno>=3000 && (U->headtail == HEAD)){
      strcpy( iso_mcopy_bot_datetime, iso_mdatetime ); // BOT date of MCOPY tape
      reel_number = U->tapeno;
    }
    return;
  }
  // 61. gap records type=-9
  if(type==-9){
    int gapcnt=0; // No change
    taxon = GAP;
    drn = dart7track_record_number;
    sprintf(filnam,"GAP%03d", gapcnt );
    strcpy( ext, "ERR");
    strcpy( prj, "GAP");
    strcpy( prg, "ERR");
    sprintf( ppn,    "%3.3s%3.3s",             prj,prg);
    sprintf( ppname, "%3.3s %3.3s %6.6s %3.3s",prj,prg,filnam,ext);
    unixname(pathname,filnam,ext,prj,prg);
    strcpy(write_program,"-REMIX");
    strcpy(write_ppn,    "ERRBGB");
    strcpy( iso_mdatetime, iso_bdatetime ); // use BOT date
    return;
  }
  // File-Start records ONLY past here.
  if(!(type==-3)){
    return;
  }
  drn = dart7track_record_number; // File Start record number
  seen = 0;
  damaged = 0;

  // File size word count ( which has proven to be misleading )
  blob_word_count = Vaddr->count;

  // Save a copy of the DART record header into a Template
  // for validating the File-Continuation Records
  memcpy(Template,Tape,8*35);

  // Metadata for the SAIL File System (aka Ralph's File System),
  // which I shall name 'RALFS' pronounced "Ralph's"
  // eponymous for Ralph Gorin, REG, who contributed to its maintenance and preservation.
  // This file system was likely created by accretion from DEC digital equipment software,
  // portions of which were modified by Saunders, Moorer, Poole, Petit and others prior to Ralph Gorin.
  sixbit_uint_into_ascii(       filnam,         Vaddr->filnam ,             sixbit_fname );
  sixbit_uint_into_ascii(       write_program,  Vaddr->write_program ,      sixbit_fname );
  sixbit_uint_into_ascii(       write_ppn,      Vaddr->write_ppn,           sixbit_ppn   );
  sixbit_halfword_into_ascii(   ext,            Vaddr->ext ,                sixbit_fname );
  sixbit_halfword_into_ascii(   prj,            Vaddr->prj ,                sixbit_ppn   );
  sixbit_halfword_into_ascii(   prg,            Vaddr->prg ,                sixbit_ppn   );
  
  sprintf( ppn,         "%3.3s%3.3s",             prj,prg);
  sprintf( ppname,      "%3.3s %3.3s %6.6s %3.3s",prj,prg,filnam,ext);
  unixname(pathname,filnam,ext,prj,prg);
  mode = Vaddr->mode;
  prot = Vaddr->prot;

  cdate = Vaddr->c_date;                     // 18-bits
  mdate = Vaddr->date_lo | (cdate & 070000); // hi order 3-bits from cdate, lo order 12-bits
  adate = Vaddr->refdate;
  ddate = Vaddr->dumpdate;
  atime = Vaddr->reftime;
  mtime = Vaddr->time;
  otnum = Vaddr->otnum;

  iso_date(iso_cdatetime,cdate,0);      iso_cdatetime[10]=0;
  iso_date(iso_mdatetime,mdate,mtime);  iso_mdatetime[10]=' ';      iso_mdatetime[16]=0;
  iso_date(iso_adatetime,adate,atime);  iso_adatetime[10]=' ';      iso_adatetime[16]=0;
  iso_date(iso_ddatetime,ddate,0);      iso_ddatetime[10]=0;
  
  // When M date is missing, promote date value from A, C, D or B in that order.
  if( iso_mdatetime[0]=='_' ) logdam(1); // M date defect
  if( iso_mdatetime[0]=='_' ) strcpy( iso_mdatetime, iso_adatetime ); // Access
  if( iso_mdatetime[0]=='_' ) strcpy( iso_mdatetime, iso_cdatetime ); // Create
  if( iso_mdatetime[0]=='_' ) strcpy( iso_mdatetime, iso_ddatetime ); // Dump
  if( iso_mdatetime[0]=='_' ) strcpy( iso_mdatetime, iso_bdatetime ); // BOT

  //  8,318. File Start    *ERROR.ERR
  // 10,052. File Continue *ERROR.ERR
  if(!strcmp(ppname,"ERR OR  *ERROR ERR")){
    int media_error=0; // rename PRMERR Previous-Media-Error file
    taxon = ERRBLOB;
    // Remix into FLAT SAIL the size of a (Previous Media Error) blob left by
    // the DART master tape-to-tape copy MCOPY command
    sprintf(filnam,"ME%04d",media_error);
    strcpy( ext, "ERR");
    strcpy( prj, "PRM");
    strcpy( prg, "ERR");
    sprintf( ppn,    "%3.3s%3.3s",             prj,prg);
    sprintf( ppname, "%3.3s %3.3s %6.6s %3.3s",prj,prg,filnam,ext);
    unixname(pathname,filnam,ext,prj,prg);
    strcpy( write_program,"-REMIX" );
    strcpy( write_ppn,    "ERRBGB");
    strcpy( iso_mdatetime, iso_bdatetime ); // use BOT date
  }
}

void
damage(){
  int j;
  if( type==-9 || type==6 ){
    return;
  }
  //
  // Verify DART Header record constants. Type 0 and -3.
  //
  sixbit_word_into_ascii(word_dsk,Tape[1],sixbit_ppn);            // sixbit'DSK   ' 044635 300000 or ' ERROR' 004562 625762
  sixbit_word_into_ascii(word18,Tape[18], sixbit_ppn);            // sixbit'DART  ' 444162 640000
  sixbit_word_into_ascii(word19,Tape[19], sixbit_fname);          // sixbit'*FILE*' 124651 544512 -3 records
  sixbit_word_into_ascii(word19,Tape[19], sixbit_fname);          // sixbit'CON  #' 435756 000003  0 records
  sixbit_word_into_ascii(wordpend,Tape[length-2], sixbit_fname);  // sixbit'$PEND$' 046045 564404
  sixbit_word_into_ascii(word_mcsys,Tape[21],     sixbit_ppn  );  // sixbit' MCSYS' 005543 637163
  //
  // DART Header, darth defects are in the file name tag, not in the data blob value.
  //
  if(strcmp( word_dsk,"DSK   "))          logdam(2);
  if(strcmp(   word18,"DART  "))          logdam(2);
  if(type==-3)if(strcmp(word19,"*FILE*")) logdam(2);
  if(type== 0)if(strcmp(word19,"CON  #")) logdam(2);
  if(strcmp(wordpend,  "$PEND$"))         logdam(2);
  if(strcmp(word_mcsys," MCSYS"))         logdam(2);
  //
  // Check the XOR checksum, includes blob data payload
  //
  xor_check_tally = 0;
  xor_check_word = Tape[length-1].fw;
  for( j=1; j<=(length-2); j++){
    xor_check_tally ^= Tape[j].fw;
  }
  xor_check_result = (xor_check_tally == xor_check_word) ? "" : "xorBAD";
  if (xor_check_tally != xor_check_word) logdam(0x100); // xorBAD
  //
  // check for DAMAGE report,
  // check whether "PRMERR" (previous tape media) errors exist
  //
  zip_check = 0;
  for(j=3;j<=23;j++){
    zip_check |= Tape[length-j].fw;
  }
  zip_check |= Tape[25].fw;
  zip_check |= Tape[27].fw;
  for(j=29;j<=35;j++){
    zip_check |= Tape[j].fw;
  }
  if (zip_check){
    logdam(0x200); // zipBAD indicates Previous Media Errors
  }  
  damage_report();
  
  // Only File-Continuation records get PAST here
  if(type!=0)return;
  //
  // compare File-Continue
  //  with   File-Start Template
  // except for words 0, 19, 20, 24 and 28 which are allowed to differ.
  //
  for(j=0;j<=35;j++){
    switch(j){
    case 0:
    case 19:
    case 20:
    case 24:
    case 28: break; // OK for these slots to be different
    default:
      if( Tape[j].fw != Template[j].fw ){
        logdam(0x020); // file-continuation header Mismatched.
      }
    }
  }
}

// Concatenate the File-Continuation blobs
void
de_frag(){
  // Append payload data words to file content blob
  // Ignore blob_word_count, size of payload determined by record length.
  switch(type){
  case -3:      // file start
  case 0:       // file continue
    memcpy( blob+seen, Tape+36, (length-61)*sizeof(pdp10_word) );
    seen += (length-61);
    break;
  case -9:      // gap
    memcpy( blob+seen, Tape+2, (length-2)*sizeof(pdp10_word) );      
    seen += (length-2);
    break;
  case 6:       // tape marker
    assert(length==12);
    memcpy( blob+seen, Tape, length*sizeof(pdp10_word));
    seen += length;
    break;
  }
}
// content counters
// struct {int  bits, bytes, nul, newline, ff, space, alnum, other;} ConCount;
void tally(int c){
  ConCount.bytes++;
  ConCount.bits += number_of_bits[c];
  switch(c){
  case 0:               ConCount.nul++;break;
  case '\n':            ConCount.newline++;break;
  case '\f':            ConCount.ff++;break;
  case '\017':          ConCount.pd++;break; // ∂ partial derivative as SAIL-ASCII character 017.
  case ' ':             ConCount.space++;break;
  case 'a'...'z':
  case 'A'...'Z':       histogram[c]++;
  case '0'...'9':       ConCount.alnum++;break;
  default:              ConCount.other++;break;
  }
}
void
convert_blob_into_utf8(){
  char *p=utf8;
  char *q=utf8+sizeof_utf8;
  int n,ma;
  int a1,a2,a3,a4,a5;
  int page_count;
  memset(utf8,0,sizeof_utf8);
  memset(&ConCount,0,sizeof(ConCount));
  for(ma=0;ma<seen;ma++)
    if( blob[ma].fw ){
      a1 = blob[ma].seven.a1;
      a2 = blob[ma].seven.a2;
      a3 = blob[ma].seven.a3;
      a4 = blob[ma].seven.a4;
      a5 = blob[ma].seven.a5;
      n = sprintf(p,"%s%s%s%s%s",
                  SAIL_ASCII[ a1 ],
                  SAIL_ASCII[ a2 ],
                  SAIL_ASCII[ a3 ],
                  SAIL_ASCII[ a4 ],
                  SAIL_ASCII[ a5 ]);
      // advance
      p += n;
      assert(p<q);
      // gather statistics
      tally(a1);tally(a2);tally(a3);tally(a4);tally(a5);
      if( blob[ma].seven.bit35 ){
        ConCount.bits++;
        if(!(('0'<=a5 && a5<='9')||a5==' '))    // When final character non-numeric non-space,
          bad_text_bit_count++;                 // not-text file
      }
    }
  // Judgement: Is this blob clean good-looking seven-bit text ?
  n = sscanf(utf8,"COMMENT ⊗   VALID %05d PAGES\n"
             "C REC  PAGE   DESCRIPTION\n"
             "C00001 00001",&page_count);
  if(n==1){
    taxon=ETEXT; // 'E' text table of contents found.
    if( page_count != 1+ConCount.ff ){
      if(0)fprintf(stderr,
              "sn=%06d "
              "defective COMMENT ⊗ VALID %05d PAGES vs %05d FormFeeds+1 '%s' %s\n",
              serial_number,
              page_count, 1+ConCount.ff, write_program, ppname );
    }
  }else if(!bad_text_bit_count){
    taxon=OTEXT; // other 'O' text
  }
}

// omit duplicate blob content from the remix
void
de_dup(){
  int i; char *q;
  unsigned char *p;
  uchar newblob=0;
  // This file blob is NOT yet ready.
  // Look ahead is the next record a CONTINUATION type==0 ?
  if( dart7track_record_number<2937291 &&       // NOT end-of-file, final EOF.
      (type==-3 || type==0) &&                  // YES inside a blob
      (0==Tape[length].half.left)){             // YES look-ahead next-record is type==0 Continue.
    return;
  }
  // Blob may be longer or shorter than expected ( log as damaged )
  if( (seen>0) && (seen != blob_word_count) ){
    if (seen < blob_word_count ){
      logdam( 0x040 ); // SHORTER than expected word count.
    }else{
      logdam( 0x080 ); // LONGER than expected word count.
    }
  }
  // Count trailing zero words
  for( zcount=0;
       zcount < seen  &&  blob[ seen-zcount-1 ].fw == 0;
       zcount++ ){
  }
  // Compute an MD5 digest hash and lookup (or assign) to it a serial number.
  bzero(digest,sizeof(digest));
  md5_init_ctx( &ctx );
  md5_process_bytes( blob, seen*8, &ctx );
  md5_finish_ctx( &ctx, digest );
  // Hexadecimal 32-digit character string of numerals 0 to 9 and letters a to f.
  p = digest;
  q = hashhex;
  bzero(hashhex,sizeof(hashhex));
  if(1){
    for (i=0; i<16; i++) // 8-bits each loop cycle
      {
        *q++ = "0123456789abcdef"[(*p & 0xF0 ) >> 4 ];
        *q++ = "0123456789abcdef"[(*p++) & 0xF ];
      }
    hashhex[32]=0;
  }
  newblob=0; // don't know yet
  serial_number = check_sn( hashhex );
  if(!( snseen[serial_number/8] & BIT[serial_number%8] )){
    convert_blob_into_utf8();
    newblob=1;
  }
  if(serial_number==0){
    serial_number = fetch_sn( hashhex );    // coin new serial number when needed
    assert( serial_number <= MAXSN );
    fprintf(stderr,"Gak!_new_SN_HASH8  %06d,%s\n", serial_number, hashhex );
  }
  // Set this blob serial number has been SEEN.
  snseen[serial_number/8] |= BIT[serial_number%8];
  // Write blob content once only
  if(newblob){
    output_blob_sn();
    blob_count++;
    fprintf( blobsnhash,
             "%06d,%s,%s,"
             "%d,%d,%d,"
             "%d,%d,%d,"
             "%d,%d,%d,"
             "%d,%d,%d,%d\n",
             serial_number, hashhex, taxon_name[taxon],
             seen,
             blob_word_count,
             zcount,
             ConCount.space,
             ConCount.alnum,
             ConCount.other,
             ConCount.bits,
             ConCount.bytes,
             ConCount.nul,
             bad_text_bit_count,
             ConCount.ff,
             ConCount.newline,
             ConCount.pd // partial derivative ∂ tokens seen, may indicate email date banners.
             );
    // fflush( blobsnhash );
  }
  // Write name tag meta data for each blob occurence (more tags than blobs)
  name_count++;
  fprintf( nametag,
           "%12ld,%7d,%04d,%04d,%4d,"
           "%06d,%6s,%-18.18s,%-16.16s,"
           "%6s,%d,%03o_%02o,%5s,%s\n",
           seekaddr,drn,reel_number,tape_number,otnum,
           serial_number,
           write_ppn,
           ppname,
           iso_mdatetime,
           write_program,
           (prot&022 ? 1:0), // No Read permitted. Read Protected for both local and world.
           prot, mode,
           damrep,
           pathname
           );
  // fflush( nametag );
  reset_globals(); // clear global values
}

int
main(int ac,char **av){
  int64 n,m=0;
  int64 wrdcnt;
  int bufcnt=0;
  int iflatdart;
  off_t seek_offset=0;
  
  // Input FILE format is a long "DART 9Track tape" eight bytes per PDP10 word.
  fprintf(stderr,"Begin remix-combo.\n");
  iflatdart = open("/large/flat_DART_data8",O_RDONLY);
  if (iflatdart<0){ fprintf(stderr,"ERROR: open file \"/large/flat_DART_data\" failed.\n"); return 1; }
  
  // initialize table for counting bits
  for(n=16;n<4095;n++){
    // number_of_bits[n] = number_of_bits[n/256]+number_of_bits[(n/16)%16]+number_of_bits[n%16];
    number_of_bits[n] = number_of_bits[n>>8]+number_of_bits[(n>>4)&15]+number_of_bits[n&15];
  }
  // verify bit counting table
  for(n=0;n<127;n++){
    assert(number_of_bits[n] == ((n&1?1:0)+(n&2?1:0)+(n&4?1:0)+(n&8?1:0)+(n&16?1:0)+(n&32?1:0)+(n&64?1:0)));
  }
  
  // Initialize an hsearch hash table
  // for looking up previously assigned ACCESSION NUMBERS, serial number, sn Values;
  // given the 32-character hexadecimal Key string for the MD5 of the data8 blob content.
  hcreate(2*MAXSN);
  load_old_snhash8();
  load_seek_segmentation(); 

  // clear off date BOT values
  iso_date(iso_mcopy_bot_datetime,0,0);
  iso_date(iso_bdatetime,0,0);
  iso_date(iso_cdatetime,0,0);
  iso_date(iso_mdatetime,0,0);
  iso_date(iso_adatetime,0,0);
  iso_date(iso_ddatetime,0,0);

  // Allocate the large buffers
  buffer = (pdp10_word *)malloc(sizeof_buffer);if(!buffer)handle_error("malloc buffer");
  blob   = (pdp10_word *)malloc(sizeof_blob);if(!buffer)handle_error("malloc blob");
  utf8   =       (char *)malloc(sizeof_utf8);if(!buffer)handle_error("malloc utf8");

  // Read 46 segments from flatdart tape 8-byte PDP10 words
  // ======================================================
  for(bufcnt=1;bufcnt<=46;bufcnt++){
    char fnam[64];

    // open output for Database-Tables in Comma-String-Value, CVS, format.
    sprintf(fnam,"/large/csv/blobsnhash.%02d",bufcnt);
    if( access(fnam,F_OK) == 0 ){ // Test previous Existence
      fprintf(stderr,"output #%02d, file %s exists already,\n",bufcnt,fnam);
      continue;
    }
    fprintf(stderr,"Open NEW output #%02d, file %s\n",bufcnt,fnam);
    errno=0;blobsnhash = fopen(fnam,"w");assert(errno==0);    
    sprintf(fnam,"/large/csv/nametag.%02d",bufcnt);
    errno=0;nametag = fopen(fnam,"w");assert(errno==0);
    
    dart7track_record_number    =       At[bufcnt].record;
    reel_number                 =       At[bufcnt].reel;
    tape_number                 =       At[bufcnt].tape;
    
    lseek( iflatdart, At[bufcnt].seek,SEEK_SET); assert(errno==0);
    n = read( iflatdart, buffer, At[bufcnt].bufsize );
    if(errno){
      char msg[128];
      sprintf(msg,"read");
      handle_error(msg);
    }
    wrdcnt = n/8;
    m=0; // offset of DART record within buffer.
    seek_offset += n;
    buffer[wrdcnt].half.left = -9; // sigh: fake lookahead type==-9 to force de_dup to finalize the last blob in buffer.    
    fprintf(stdout,"buffer #%2d, reel#%04d, tape#%04d, record#%06d \n",
            bufcnt, reel_number, tape_number, dart7track_record_number );
    fflush(stdout);

    // Process DART record at offset m within the buffer
    // =================================================
    while((m+1) < wrdcnt){

      // There are two pointers, Tape and Vaddr, into the DART 7track tape record.
      Tape  = buffer+m;                 // Tape is a typedef PDP10 word array format.
      Vaddr = (darth *)(buffer+m);      // Vaddr is a DART Header struct format.
      type  = Vaddr->type;              // same as "type = Tape[0].half.left;"
      seekaddr = At[bufcnt].seek + 8*m; // byte displacement within the /large/flat_DART_data8 file.
      dart7track_record_number++;

      // Verify that input file DART 7Track record word zero is always valid.
      // assert( type==0 || type==-3 || type==6 || type==-9 ); // a one line version
      // length = (type == -9) ? Tape[1].full.word + 2 :  Tape[0].half.right + ((type==6) ? 1 : 2);
      switch(type){
      case -9:          length = Tape[1].full.word + 2; assert(27<=length && length<=382438); break;
      case -3: case 0:  length = Tape[0].half.right+ 2; assert(61<=length && length<=10240); break;
      case  6:          length = Tape[0].half.right+ 1; assert(length==12); break; // BOT EOT markers always length==12.
      default: handle_error("BAD input file /large/flat_DART_data8.\n" // input file is off the reservation
                            "md5sum must be 3adbff17fd7f9f6eb9107755594ae0b9\n");
      }
      // check that the whole record is within the buffer
      if((m+length) > wrdcnt) break; // exit inner loop           

      // FIXUP: two defects where type==0 follows a type==6 record.
      if ( dart7track_record_number==22161 ||
           dart7track_record_number==2404328 ){
        type=-3; // mark as file START, salvage tail end of damaged file content.
      }      
      // Process one 7track record
      // Apply four D-steps to each 7track record
      decode(); // Decode external nametag, look inside the blob later.
      damage(); // Report Current and Previous-Media-Errors. DART code 'PRMERR'.
      de_frag();// Concatenate record payloads into a data blob.
      de_dup(); // Apply digest-hash (MD5 is good enough) to data blob. Unique data blobs into the REMIX once only.
      
      // Advance over the processed 7track record
      m += length;
      length = 0;
    }
    // final after EACH buffer
    fclose(blobsnhash);
    fclose(nametag);
  }  
  // Finalization after ALL buffers
  hdestroy(); // deallocate hash table
  close(iflatdart);
  fprintf(stderr,"DONE dart-combo\n");
  return 0;
}
