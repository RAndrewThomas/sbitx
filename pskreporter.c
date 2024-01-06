#define _XOPEN_SOURCE  // necessary to use strptime() from glibc
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "pskreporter.h"

// Like `mktime', but for TP represents Universal Time, not local time
extern time_t timegm (struct tm *__tp) __THROW;



// all loaded from user_settings.ini
char pskr_callsign[17] = "N5RGN";
char pskr_gridloc [ 7] = "EM16el";
char pskr_antenna [65] = "20m/40m attic fan dipole";

char pskr_server [50] = "report.pskreporter.info";
char pskr_appname[50] = "sbitx v3.0-n5rgn";

/*

  pskreporter.c - by n5rgn.  License: GPL 2 or newer
  
  Sends FT8 reports to pskreporter.info or compatible.
  
  Usage:
    Application should call pskr_open () when reporting is to be enabled.
    Application should call pskr_add_spot() to add spots one at a time.
    Application should call pskr_close () when reporting is no longer needed.

  In pskr_open() a thread is created to handle connecting and sending spots
  to the remote host.  The thread wakes at the specified interval to send
  any spots that have been accumulated. 

  ) define spot struct

  ) load configs from user_settings


  - pskr_open()


  - pskr_add_spot()

    ) determine if spot passed to pskr_add_spot() is valid

    ) add valid spot to linked list of spots


  - pskr_get_spot()

    ) check linked list for spots

    ) return ptr to spot if exists


  - pskr_del_spot()

    ) if spot valid, move pskr_spots to next spot

    ) free prev spot 


  - pskr_close()

    ) 

*/



int pskr_sock = 0;


// for a dynamic, linked list of spots
struct pskr_spot *pskr_spots = 0;

int pskr_num_spots = 0;


void pskr_purge_spots(void)
{
  struct pskr_spot *p = 0;
  struct pskr_spot *pn= 0;

  p = pn = pskr_spots;

  // free to the end of list of spots
  while (p != 0) {
    pn = p->next_spot;
    free (p);
    p = pn;
  }

  pskr_num_spots = 0;
  pskr_spots     = 0;
}


int pskr_add_spot(char *time, int16_t snr, float hz, char *msg)
{
  struct pskr_spot *p = 0;
  struct pskr_spot *pn= 0;
  int test = 0;
//  if (pskr_thread_busy) {
//      pskr_cache_spot ();
//  }

  pskr_num_spots = 0;

  if (0 == pskr_spots) {
    // allocate space for first spot
    pskr_spots = malloc (sizeof (struct pskr_spot));
    if (NULL == pskr_spots) {
      puts ("pskr_add_spot() - malloc 1 failed");
      return (-1);
    }
    p = pn = pskr_spots;
  } else {
    p = pskr_spots;

    // find end of list of spots
    test = 0;
    while (p->next_spot != 0) {
      p = p->next_spot;
      pskr_num_spots++;
      if (pskr_num_spots > 100) {
//        puts ("pskr_add_spot() - failed sanity check");
        puts ("pskr_add_spot() - too many pending spots, purging");
        pskr_purge_spots ();
        return (-2);
      }
      test++;
    }

    // should be at end of list of spots now
    // allocate space for this new spot
    pn = malloc (sizeof (struct pskr_spot));
    if (NULL == pn) {
      puts ("pskr_add_spot() - malloc 2 failed");
      return (-3);
    }
    p ->next_spot = pn;  // link new spot to the list of spots
  }

  pn->next_spot = 0;   // terminate the list of spots

  pn->snr = snr;
  pn->freq_hz = hz;

  strncpy(pn->callsign, msg,  16);  // !!!!
  strncpy(pn->gridloc,  msg,   6);  // !!!!
  strncpy(pn->time_str, time, 19);

  printf ("pskr_add_spot() - pskr_num_spots: %4d, msg: %s\n", pskr_num_spots, msg);
  return (pskr_num_spots);
}



char *pskr_get_spot(void)
{
  // !!!!
  if (0 == pskr_spots)
  {
  }    
  return (0);
}



int pskr_get_num_spots(void)
{
  // !!!!
  return (0);
}



int pskr_open(void)
{
  pskr_purge_spots ();
  // start send thread !!!!
  return (0);
}



int pskr_close(void)
{
  // kill send thread !!!!
  pskr_purge_spots ();
  return (0);
}



int32_t pskr_read_int(char **pointer, int32_t *value)
{
  char *start = *pointer;
  *value = strtol(start, pointer, 10);
  return start != *pointer;
}



int32_t pskr_read_dbl(char **pointer, double *value)
{
  char *start = *pointer;
  *value = strtod(start, pointer);
  return start != *pointer;
}



int32_t pskr_read_time(char **pointer, struct tm *value)
{
  *pointer = strptime(*pointer, "%y%m%d %H%M%S", value);
  return *pointer != NULL;
}



void pskr_copy_char(char **pointer, const char *value)
{
  int8_t size = strlen(value);
  memcpy(*pointer, &size, 1);
  *pointer += 1;
  memcpy(*pointer, value, size);
  *pointer += size;
}



void pskr_copy_int1(char **pointer, int8_t value)
{
  memcpy(*pointer, &value, 1);
  *pointer += 1;
}



void pskr_copy_int2(char **pointer, int16_t value)
{
  value = htons(value);
  memcpy(*pointer, &value, 2);
  *pointer += 2;
}



void pskr_copy_int4(char **pointer, int32_t value)
{
  value = htonl(value);
  memcpy(*pointer, &value, 4);
  *pointer += 4;
}



int pskr_send_spots(void)
{
  struct hostent *host;
  struct sockaddr_in addr;
  struct tm tm;
  struct timespec ts;

  double sync, dt;

  int32_t snr, freq, counter, rc, padding, sequence, size;

  char buffer[512],  call[17], grid[8], *src, *dst, *start;

  char header[] =
  {
    0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x03, 0x00, 0x2C, 0x99, 0x92, 0x00, 0x04,
    0x00, 0x00,
    0x80, 0x02, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F,
    0x80, 0x04, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F,
    0x80, 0x08, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F,
    0x80, 0x09, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F,
    0x00, 0x00,

    0x00, 0x02, 0x00, 0x3C, 0x99, 0x93, 0x00, 0x07,
    0x80, 0x01, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F,
    0x80, 0x03, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F,
    0x80, 0x05, 0x00, 0x04, 0x00, 0x00, 0x76, 0x8F,
    0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x76, 0x8F,
    0x80, 0x0A, 0xFF, 0xFF, 0x00, 0x00, 0x76, 0x8F,
    0x80, 0x0B, 0x00, 0x01, 0x00, 0x00, 0x76, 0x8F,
    0x00, 0x96, 0x00, 0x04
  };

  pskr_callsign[16] = 0;
  pskr_gridloc [ 6] = 0;
  pskr_antenna [64] = 0;

  if((pskr_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    fprintf(stderr, "Cannot open socket.\n");
    return EXIT_FAILURE;
  }

  if((host = gethostbyname(pskr_server)) == NULL)
  {
    fprintf(stderr, "Cannot find remote host address.\n");
    return EXIT_FAILURE;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  memcpy(&addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);
  addr.sin_port = htons(4739);

  clock_gettime(CLOCK_REALTIME, &ts);
  srand(ts.tv_nsec / 1000);

  dst = header + 12;
  pskr_copy_int4(&dst, rand());

  memcpy(buffer, header, sizeof(header));

  start = buffer + sizeof(header);

  dst = start + 4;
  pskr_copy_char(&dst, pskr_callsign);
  pskr_copy_char(&dst, pskr_gridloc );
  pskr_copy_char(&dst, pskr_appname );
  pskr_copy_char(&dst, pskr_antenna );

  size = dst - start;
  padding = (4 - size % 4) % 4;
  size += padding;
  memset(dst, 0, padding);

  dst = start;
  pskr_copy_int2(&dst, 0x9992);
  pskr_copy_int2(&dst, size);

  start += size;

  counter = 0;
  sequence = 0;
  dst = start + 4;
  for(;;)
  {
    src = pskr_get_spot();
    if (src == NULL) {
      sleep(1);
      continue;
    }

    call[0] = 0;
    grid[0] = 0;
    rc = pskr_read_time(&src, &tm)
      && pskr_read_dbl (&src, &sync)
      && pskr_read_int (&src, &snr)
      && pskr_read_dbl (&src, &dt)
      && pskr_read_int (&src, &freq)
      && sscanf(src, "%13s %4s", call, grid);

    if(!rc) {
      continue;
    }

    pskr_copy_char(&dst, call);
    pskr_copy_char(&dst, grid);
    pskr_copy_int4(&dst, freq);
    pskr_copy_int1(&dst, snr);
    pskr_copy_char(&dst, "FT8");
    pskr_copy_int1(&dst, 1);
    pskr_copy_int4(&dst, timegm(&tm) + 15);

    ++counter;

    if(counter < 10) {
      continue;
    }

    // send the accumulated spots
    size = dst - start;
    padding = (4 - size % 4) % 4;
    size += padding;
    memset(dst, 0, padding);

    dst = start;
    pskr_copy_int2(&dst, 0x9993);
    pskr_copy_int2(&dst, size);

    dst = buffer + 2;
    size += start - buffer;
    pskr_copy_int2(&dst, size);
    pskr_copy_int4(&dst, time(NULL));
    pskr_copy_int4(&dst, sequence);

    sendto(pskr_sock, buffer, size, 0, (struct sockaddr *)&addr, sizeof(addr));

    counter = 0;
    ++sequence;
    dst = start + 4;
  }
}
