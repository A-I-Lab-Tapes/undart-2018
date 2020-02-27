#if 0
echo Self compiling source
NAME=dart-flatten
echo \
gcc -g -m64 -Wall -Werror -o /usr/local/bin/$NAME /home/sail/c/$NAME.c
gcc -g -m64 -Wall -Werror -o /usr/local/bin/$NAME /home/sail/c/$NAME.c
return
#endif

/* dart-flatten.c
 * =======================================================================================
 * Concatenate DART records from the 41594 files named p3000.000 to p3227.069
 * when a valid DART record header is not found generate a DART -9 gap record header,
 * inserts two words for the header and 1 to 4 zero bytes to restore the modulo 5 word phase.
 * After a gap record, dart-flatten resumes at the next valid -3,,length START FILE record.
 * In some of the gaps, I have seen 0,,length file-continuation-records,
 * which I shall leave for later to examine.
 *
 * Determine the offset and length of each DART record, and
 * Determine the offset and length of each gap when a DART record is missing,
 *      spans of bytes that lack a dart format interpretation.
 * =======================================================================================
 * copyright: (c)2001-2016 Bruce Guenther.Baumgart
 * software_license:  GPL license Version 2.
 * http://www.gnu.org/licenses/gpl.txt MD5=393a5ca445f6965873eca0259a17f833
 *
 * Derived from reducing versions of undart*.c and dart-segmentation.c
 * =======================================================================================
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <assert.h>

/* PDP-10 word formats (little endian for Intel and for AMD x86 architecture)
 * --------------------------------------------------------------------------
 * The PDP-6 and PDP-10 are BIG endian. The PDP-11 and Intel x86 are LITTLE endian.
 * Network order is BIG endian, per Dan Cohen, who I once met, see RFC-1700 now RFC3-232.
 * The programming language 'C' bit fields are endian dependent !
 * The following declaration is for 'C' on LITTLE endian Intel x86.
 */
typedef unsigned long long uint64;
typedef          long long  int64;
typedef union
{
  uint64 fw;
  struct { uint64 word:36,:28;				} full;
  struct {  int64 right:18,left:18,:28;         	} half;
  struct { uint64 nibble:4, byte4:8, byte3:8, byte2:8, byte1:8,:28; } tape; // repacking 9-Track tape bytes
  struct { uint64 c6:6,c5:6,c4:6,c3:6,c2:6,c1:6,:28;	} sixbit; // ASCII sixbit+040
  struct { uint64 bit35:1,a5:7,a4:7,a3:7,a2:7,a1:7,:28;	} seven; // seven bit ASCII
  // WAITS file RIB "Retrieval Information Block" format
  struct { uint64 :15,date_hi:3,:46; } word2;
  struct { uint64 date_lo:12,time:11,mode:4,prot:9,:28; } word3;
  struct { uint64 date_lo:12,time:11+9,flag:1,date_hi:3,:28; } word3tape; // DART tape HEAD/TAIL header.
  struct { uint64 count:22,:42; } word6;
} pdp10_word;

#define MAX_BYTE_COUNT 40000000 // Larger then any DART file
#define MAX_WORD_COUNT ((MAX_BYTE_COUNT+4)/5)
unsigned char data5[ MAX_BYTE_COUNT ];
pdp10_word    data8[ MAX_WORD_COUNT ]; // 64-bit words.
pdp10_word      gap[ 2 ]; // 2 word gap header: -9,,0; word_count.

extern int errno;
/*
 * globals 
 */
int segment;

        char saildartroot[120]="/data/2012";
        FILE *stdlog;
        FILE *stdcsv;
        char device[8];
        int reel,tapeno;

        char dartfile[32];
        char label1[32];
        char label2[32];

        int file_byte_count;
        int file_word_count;

        int data_word_count; /* Number of 36-bit words in current file */
        int record_type;
        int record_length;

        int waitsdate;	/* WAITS file date and time */
        int waitstime;	/* WAITS file date and time */
        int year, month, day, hour, minute, second;
        char iso_datetime[64];
        int deltatimeflag;

        int waitsdate;	/* WAITS file date and time */
        int waitstime;	/* WAITS file date and time */
        int waitsprot;	/* WAITS file protection */
        int waitsmode;	/* WAITS file data mode */
        int sailprot;	/* WAITS file protection === SAIL prot of SYSTEM.DMP[J17,SYS] */
int prot,mode; // unix like
char longname[32];
char filnam[7], ext[4], prj[4], prg[4];

/* Convert SAIL file WAITS protection bits into Unix file protection bits.
 * -----------------------------------------------------------------------
 */
int
waits2unixprot(unsigned int prot)
{
  prot &= ~0444;// Turn off bits without unix equivalence
  prot <<= 1;	// Shift the WAITS "rw" bits into the correct UNIX places
  prot ^= 0666;	// Change polarity
  prot |= 0400; // ENABLE read for owner
  return(prot);
}
void
pass(void){
}
#define SIXBIT_DSK    0446353000000L
#define SIXBIT_DART   0444162640000L
#define SIXBIT_ERROR  0004562625762L // SIXBIT/ ERROR/
#define SIXBIT_FILE   0124651544512L // SIXBIT/*FILE*/
#define SIXBIT_HEAD   0125045414412L // SIXBIT/*HEAD*/
#define SIXBIT_TAIL   0126441515412L // SIXBIT/*TAIL*/

/* ascii SIXBIT decoding functions
 * -------------------------------------------------------------------- *
 * sixbit word into ASCII string 
 * sixbit halfword into ASCII string
 */
char *
sixbit_word_into_ascii(char *p0,pdp10_word w)
{
  char *p=p0;
  *p++ = (w.sixbit.c1 + 040);
  *p++ = (w.sixbit.c2 + 040);
  *p++ = (w.sixbit.c3 + 040);
  *p++ = (w.sixbit.c4 + 040);
  *p++ = (w.sixbit.c5 + 040);
  *p++ = (w.sixbit.c6 + 040);
  *p++= 0;
  return p0;
}
char *
sixbit_halfword_into_ascii(char *p0,int halfword)
{
  char *p=p0;
  *p++ = ((halfword >> 12) & 077) + 040;
  *p++ = ((halfword >> 6) & 077) + 040;
  *p++ = (halfword & 077) + 040;
  *p++ = 0;
  return p0;
}

/* Convert 5-byte PDP-10 "tape-words" into 8-byte unsigned long long words. 
 * ------------------------------------------------------------------------------
 * The PDP-10 (and DEC System-20) tape words were 36-bits packed into bytes in
 * BIG Endian order with the final four bits in the low order of the fifth byte.
 * The 36-bit PDP-10 words are placed into the low order side of a 64-bit 
 * unsigned long long union so that bit field notation can be used for unpacking.
 * ------------------------------------------------------------------------------
 */
void
input_dartfile( char *dartfile ){
  int i,ma;
  /*
   * reset 
   */
  bzero(data5,sizeof(data5));
  bzero(data8,sizeof(data8));
  fflush(stdlog);
  fflush(stdcsv);
  /*
   * Input from source file 
   */
  errno = 0;
  i = open(dartfile,O_RDONLY);
  if (i<0){
    fprintf(stderr,"ERROR: input source open file \"%s\" failed.\n",dartfile);
    return;
  }
  bzero(data5,sizeof(data5));
  file_byte_count = read(i,data5,sizeof(data5));
  file_word_count = file_byte_count/5;
  //  assert( 0 == file_byte_count%5 ); // gak ugly dart file found
  if (file_byte_count%5 )
    fprintf(stderr,"%s GAK file_byte_count modulo 5 remainder %d\n",dartfile,file_byte_count%5);
  close(i);
  /*
   * Log input file read.
   */
  {
    char *fmt="%s FILE read %12d bytes %12d words";
    fprintf(stderr,fmt, dartfile, file_byte_count, file_word_count );
    fprintf(stdlog,fmt, dartfile, file_byte_count, file_word_count );
    fprintf(stdlog,"\n");
    fflush( stdlog );
  }
  /*
   * CONVERT 5-byte "tape-words" into 8-byte unsigned long long words 
   */
  assert( file_word_count < MAX_WORD_COUNT );
  for( ma=0; ma < file_word_count; ma++ ){
    int q = ma*5;
    data8[ma].tape.byte1 = data5[q];    // octet
    data8[ma].tape.byte2 = data5[q+1];  // octet
    data8[ma].tape.byte3 = data5[q+2];  // octet
    data8[ma].tape.byte4 = data5[q+3];  // octet
    data8[ma].tape.nibble = data5[q+4]; // only the four low order bits !
  }
}

/* Find next good record header 
 * -------------------------------------------------------------------- *
 */
int
advance_to_good_record_header(int ma)
{
  int ma2, p_length; short p_type;
  long long n, q, p_DSK;
  unsigned char *p;
  int ma_new;
  pdp10_word w,x;
  /**/
  assert( ma < MAX_WORD_COUNT );
  q = ma*5;

  // Scan for record marker \223 \72 \300 \0 \0 which is SIXBIT/DSK␣␣␣/
  fprintf(stderr," Scan for next record marker\n");
  p = data5 + q - 1;
  do {
    p++;
    n = file_byte_count - (p - data5);
    // Scan for the FIVE byte pattern  \223 \72 \300 \0 \0 
    p = (unsigned char *)memchr((void *)p,'\223', n );          // hit first byte
    if((uint64)p == 0)break; // marker NOT found
    if(p[1]!=072 || p[2]!=0300 || p[3]!=0 || p[4]!=0 )continue;       // test 2nd, 3rd, 4th and 5th bytes.
    // The SIXBIT/DSK␣␣␣/ landmark found, but contine checking
    x.tape.byte1 = p[0];
    x.tape.byte2 = p[1];
    x.tape.byte3 = p[2];
    x.tape.byte4 = p[3];
    x.tape.nibble= p[4];
    p_DSK = x.full.word; // SIXBIT_DSK
    // one word before /DSK␣␣␣/ should be type == -3 ,, and a length within bounds
    w.tape.byte1 = p[-5];
    w.tape.byte2 = p[-4];
    w.tape.byte3 = p[-3];
    w.tape.byte4 = p[-2];
    w.tape.nibble= p[-1];
    p_type = w.half.left;
    p_length = w.half.right;    
    fprintf(stderr,"...candidate at p=%ld 'DSK␣␣␣' %6o=%6d,,%6d.\n",(long)p,0777777&p_type,p_type,p_length);
  } while (p && !(      p_DSK == SIXBIT_DSK // sixbit 'DSK' with zero right pad
                        && p_type == -3
                        && p_length>=59
                        && p_length<=10238 ));
  // End-of-file ?
  // No further DART file start -3,,wrdcnt;'DSK␣␣␣' record marker has been found.
  if ((uint64)p == 0 ){
    fprintf(stderr,"exit p==0 EOF\n");
    return file_word_count; // Return ma_guess just beyond EOF.
  }
  p -= 5; // back one word from 'DSK␣␣␣' marker
  ma_new = (p - data5) / 5;
  if ((p-data5)%5==0 && (data8[ma_new+1].fw == SIXBIT_DSK)){
    fprintf(stderr,"found next record ma=%d byte alignment OK\n",ma_new);
    return ma_new;
  }

  fprintf(stderr,"append %ld zero bytes to gap and repack data8 words\n", 5-(p-data5)%5);
  // clear off final 1 to 4 bytes of the gap in order to re-align the data5 to the data8.
  switch(5-(p-data5)%5){
  case 5: data8[ma_new].tape.byte1 = 0;
  case 4: data8[ma_new].tape.byte2 = 0;
  case 3: data8[ma_new].tape.byte3 = 0;
  case 2: data8[ma_new].tape.byte4 = 0;
  case 1: data8[ma_new].tape.nibble= 0;
  default: break;
  }
  ma_new++; // step over the final word of the gap
    
  // repack into data8 words starting from the next good record
  for( ma2 = ma_new; ma2 < file_word_count; ma2++ ){
    data8[ma2].tape.byte1 = *p++;
    data8[ma2].tape.byte2 = *p++;
    data8[ma2].tape.byte3 = *p++;
    data8[ma2].tape.byte4 = *p++;
    data8[ma2].tape.nibble= *p++;
  }
  assert( data8[ma_new+1].fw == SIXBIT_DSK );
  return ma_new; // Continue record processing
}

void
process_data_record( int ma, char *dartfile ){
  int j, record_type, wrdcnt, record_length;
  uint64 check_sum_now, check_sum_word;
  char xorcheck[8];
  static int data_words_seen=0;
  /*
   * record header 
   */
  record_type = data8[ma].half.left;
  record_length = data8[ma].half.right;

  data_word_count = data8[ma+7].word6.count;
  assert( data_word_count < MAX_WORD_COUNT );
  /*
   * CHECK checksum using XOR per word.
   */
  check_sum_now = 0;
  check_sum_word = (uint64)data8[ma+1+record_length].full.word;
  for( j=0; j<record_length; j++){
    check_sum_now ^= data8[ma+1+j].full.word;
  }
  if ( check_sum_now == check_sum_word ){
    sprintf(xorcheck,"^^OK^^");
    if (record_type == -3){
      data_words_seen = 0;
    }
    wrdcnt = record_length - 59;
    data_words_seen += wrdcnt;
    if ( data_words_seen == data_word_count ){
      pass();
    }
    /*
     * Verify landmark constants
     */
    if ( data8[ma+1].fw == SIXBIT_DSK ){
      //    assert(data8[ma+10].fw == 1 );
      assert(data8[ma+18].fw == SIXBIT_DART );
      assert(data8[ma+26].fw == 0777777777777L );
    }else{
      assert( data8[ma+1].fw == SIXBIT_ERROR );
      assert(data8[ma+18].fw == SIXBIT_DART );
      assert(data8[ma+26].fw == 0777777777777L );
    }
  } else {
    fprintf(stderr,"checksum  now = %012llo\n", check_sum_now );
    fprintf(stderr,"checksum word = %012llo\n", check_sum_word );
    sprintf(xorcheck,"^FAIL^");
  }
}

int
process_dart_head_or_tail(int ma){
  int j;
  char prj[4],prg[4];
  unsigned long rotchk=0, word10, word11, word12, word13;
  int feet, version, length;
  /**/
  version = data8[ma].half.left; // always 6
  assert(version==6);
  length = data8[ma].half.right;
  sixbit_word_into_ascii( label1, data8[ma+1] ); // DART
  sixbit_word_into_ascii( label2, data8[ma+2] ); // *HEAD* or *TAIL*
  waitsdate= data8[ma+3].word3tape.date_hi <<12 | data8[ma+3].word3tape.date_lo; // flag,time,date
  waitstime= data8[ma+3].word3tape.time;
  deltatimeflag= data8[ma+3].word3tape.flag; // BOOLEAN FLAG indication delta time from previous media.
  day = waitsdate % 31 + 1;
  month = (waitsdate/31) % 12 + 1;
  year = ((waitsdate/31) / 12 + 64) + 1900;
  hour = waitstime / 60;
  minute = waitstime % 60;
  second = 0;
  sprintf( iso_datetime, "%d-%02d-%02dT%02d:%02d:%02d", year,month,day, hour,minute,second );

  sixbit_halfword_into_ascii(prj, data8[ma+4].half.left );
  sixbit_halfword_into_ascii(prg, data8[ma+4].half.right );

  word10= (ulong)data8[ma+010].full.word;
  word11= (ulong)data8[ma+011].full.word;
  word12= (ulong)data8[ma+012].full.word;
  word13= (ulong)data8[ma+013].full.word;

  for (j=1;j<=012;j++){
    rotchk <<= 1;
    if ( 0x1000000000 & rotchk ){
      rotchk |= 1;
      rotchk &= 0xFFFFFFFFF;
    }
    rotchk += (ulong)data8[ma+j].full.word;
    rotchk &= 0xFFFFFFFFF;
  }

  if (strncmp("DART  ",label1,6)==0 
      && strncmp("*HEAD*",label2,6)==0 
      && strncmp("DMP",prj,3)==0 
      && strncmp("SYS",prg,3)==0 ){
    tapeno = data8[ma+5].half.right;
  }
  if (strncmp("DART  ",label1,6)==0 
      && strncmp("*HEAD*",label2,6)==0 
      && strncmp(" MC",prj,3)==0 
      && strncmp("SYS",prg,3)==0 ){
    reel = data8[ma+5].half.right;
  }
  feet = (ulong)data8[ma+7].full.word;
  fprintf( stdlog,
          "%-9s TAPE marker:%s:%s: version#%d length=%d "
          "dumped by [%s,%s] %s %s class=%d reel=%d tapeno=%d  %6d feet ",
           dartfile, label1, label2, version, length, prj, prg,
           deltatimeflag ? "datetime......" : "datetime stamp",
           iso_datetime,
           data8[ma+5].half.left, reel, tapeno, feet );
           // data8[ma+6].half.left, data8[ma+6].half.right,    // 1,,1

  if ( /**/word10==0L 
       && word11==0xFFFFFFFFF 
       && word12==0 
       && word13==rotchk ){
    fprintf(stdlog,"rot-check OK\n");
  } else {
    fprintf(stdlog,"rot-check words %lo=?0, %lo=?-1, %lo=?0, %lo =? %lo FAILED.\n",
            word10,word11,word12,word13,rotchk);
  }
  if ( strncmp("DART  ",label1,6)==0
       && strncmp("*TAIL*",label2,6)==0 
       && strncmp(" MC",prj,3)==0 )
    return 0;
  else
    return ma;
}

/* Main 
 * -------------------------------------------------------------------- *
 */
int
main(int ac,char **av){
  int ma, arg, ma_guess;
  int malo, mahi, byte_offset, byte_length, pdp10words;
  char ok, segment_field[12];
  int flatfile, n, byte_count;
  if ( ac <= 1 ){ fprintf(stderr,"\nusage: flatten DART_FILE_NAMES\n"); return 1; }
  errno=0;stdlog = fopen("/data/log/flatten","w");assert(errno==0);
  errno=0;stdcsv = fopen("/data/csv/flatten","w");assert(errno==0);
  errno=0;flatfile = open("/data8/flatdart",O_CREAT | O_WRONLY, 0600);assert(errno==0);

  // There are 41594 dart filnames from p3000.000 to p3227.069
  for ( arg=1 ; arg<ac; arg++ ){
    strcpy( dartfile, av[arg] );

    input_dartfile( dartfile );
    /*
     * Scan for DART segments (tape records AND gaps).
     */
    ma= segment= 0;
    while ( ma < file_word_count && segment<1000){
      /*
       * Pickup a two word segment prefix (dart record header).
       */
      malo = ma; // start-of-segment
      segment++;
      record_type = data8[ma].half.left;
      record_length = data8[ma].half.right;
      sixbit_word_into_ascii( device, data8[ma+1] ); 
      bzero(label1,sizeof(label1));
      bzero(label2,sizeof(label2));
      /*
       * DETECT bad prefix AND skip ahead if necessary.
       * Test for Unacceptable record TYPE or LENGTH or DEVICE marker. 
       */
      if ( !( record_type == -3 || record_type == 0 || record_type == 6 )
           || !( 11 <= record_length && record_length < 10240 )
           || !( strncmp("DSK   ",device,6)==0
                 || strncmp("DART  ",device,6)==0 
                 || strncmp(" ERROR",device,6)==0 )
           ){
        fprintf(stderr,"#");
        record_type = -9; // FAKE record_type -9 for GAP
      }

      if( record_type==-3  || record_type==0 ){
        
        sixbit_word_into_ascii(  filnam, data8[ma+2] );
        sixbit_halfword_into_ascii( ext, data8[ma+3].half.left );
        sixbit_halfword_into_ascii( prj, data8[ma+5].half.left );
        sixbit_halfword_into_ascii( prg, data8[ma+5].half.right );

        waitsdate= data8[ma+3].word2.date_hi <<12 | data8[ma+4].word3.date_lo;
        waitstime= data8[ma+4].word3.time;
        waitsmode= data8[ma+4].word3.mode;
        waitsprot= data8[ma+4].word3.prot;
        sailprot= data8[ma+4].word3.prot; // sweeter by another name

        prot = waits2unixprot( waitsprot );
        mode = waitsmode;

        day = waitsdate % 31 + 1;
        month = (waitsdate/31) % 12 + 1;
        year = ((waitsdate/31) / 12 + 64) + 1900;
        hour = waitstime / 60;
        minute = waitstime % 60;
        second = 0;
        if( 1966<=year && year<=1991 && waitstime<1440 ){
          sprintf( iso_datetime, "%4d-%02d-%02dT%02d:%02d:%02d", year,month,day, hour,minute,second );
        }else{
          sprintf( iso_datetime, "%10d. %4d.    ", waitsdate, waitstime );
        }
        sprintf(longname,"%3s %3s %6s.%-3s",prg,prj,filnam,ext);
      }else{
        iso_datetime[0]=0;
        longname[0]=0;
      }

      /*
       * Dispatch processing for this segment of the file.
       * DART record types -3, 0 or 6 for the good segments.
       * ASSIGN record_type -9 to the unusable gaps between good records
       */
      switch ( record_type ){
      case -9: ok='N';                                               // gap
        ma_guess = advance_to_good_record_header(ma);
        record_length = ma_guess - malo - 1; // Reminder: "record_length" does not include the record type,,size word.
        gap[0].half.left = -9; // xwd -9,,0
        gap[0].half.right = 0;
        gap[1].full.word = record_length + 1; // gap word count which may exceed 10240, and for some gaps it exceeds 2^18.
        n = write( flatfile, gap, 8*2); // INSERT a gap record header
        fprintf(stderr,"!bingo! gap size %ld words\n",(long)gap[1].full.word);
        break;
      case -3: ok='Y';process_data_record( ma, av[arg] );ma++;break; //    start File
      case  0: ok='Y';process_data_record( ma, av[arg] );ma++;break; // continue File
      case  6: ok='Y';process_dart_head_or_tail( ma );        break; // Tape reel head-or-tail
      }
      ma += record_length + 1;
      mahi = ( ma - 1 ); // last-word belonging to this segment.
      assert( ma < MAX_WORD_COUNT );
      sprintf(segment_field,"%d",segment);
      byte_offset = 5*malo;
      byte_length = 5*(mahi-malo+1);
      pdp10words = (mahi-malo+1);
      fprintf(stderr,".");
      fprintf(stdlog,
              "%-12s segment #%-6s " \
              " %10d byte offset %10d byte length" \
              " %3d,,%6d :%s:%s:%s" \
              "tape#%d,%4d, " \
              "word#%8d %-3s ",
              dartfile, segment_field, byte_offset, byte_length,
              record_type, record_length,
              ok=='Y' ? device : "      ",
              label1, label2,
              reel,tapeno, ma, 
              ok=='Y' ? "ok" : "gap" );
      fprintf(stdlog,"%s %s",longname,iso_datetime);
      fprintf(stdlog,"\n");
      fflush(stdlog);
      fprintf(stdcsv,"%-9s,%3d,%8d,%4d,%d,%d\n",
              dartfile, segment, pdp10words, record_type, reel, tapeno );
      fflush(stdcsv);
      
      // write dart segment to flat file 8 bytes per PDP-10 word
      byte_count = 8*(mahi-malo+1);
      n = write( flatfile, data8+malo, 8*(mahi-malo+1));
      assert( n == byte_count );
    }
    byte_offset = 5*ma; // at EOF ?
    fprintf( stderr,"EOF %d segments\n",segment);
    fprintf( stdlog, "%-9s EOF                " \
             " %10d byte offset %10d file length\n",
             dartfile, byte_offset, file_byte_count );
    fflush(stdlog);
  }
  close(flatfile);
  return 0;
}
