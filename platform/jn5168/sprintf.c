/*
File: printf.c

Copyright (C) 2004,2008  Kustaa Nyholm

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

copied from http://www.sparetimelabs.com/printfrevisited/index.html
modified by Philipp Scholl <scholl@teco.edu>

*/

#include "contiki-conf.h"
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dev/uart0.h"

static char*  bf, buf[14], uc, zs;
static unsigned int num;

static void out(char c) {
  *bf++ = c;
}

static void outDgt(char dgt) {
  out(dgt+(dgt<10 ? '0' : (uc ? 'A' : 'a')-10));
  zs=1;
}

static void divOut(unsigned int div) {
  unsigned char dgt=0;
  //num &= 0xffff; // just for testing the code  with 32 bit ints
  while (num>=div) {
    num -= div;
    dgt++;
  }
  if (zs || dgt>0) outDgt(dgt);
}

int vsnprintf(char *str, size_t n, const char *fmt, __VALIST va)
{
  char ch, *p, *str_orig = str;
  char next_ch;

  while ((ch=*fmt++) && str-str_orig < n) {
    if (ch!='%') {
      *str++ = ch;
    } else {
      char lz=0;
      char w=0;
      ch=*(fmt++);
      if (ch=='0') {
        ch=*(fmt++);
        lz=1;
      }
      if (ch>='0' && ch<='9') {
        w=0;
        while (ch>='0' && ch<='9') {
          w=(((w<<2)+w)<<1)+ch-'0';
          ch=*fmt++;
        }
      }
      bf=buf;
      p=bf;
      zs=0;
start_format:
      next_ch = *fmt;
      switch (ch) {
        case 0:
          goto abort;
        case 'l':
        if(next_ch == 'x'
            || next_ch == 'X'
            || next_ch == 'u'
            || next_ch == 'd') {
          ch = *(fmt++);
          goto start_format;
        }
        case 'u':
        case 'd':
          num=va_arg(va, unsigned int);
          if (ch=='d' && (int)num<0) {
            num = -(int)num;
            out('-');
          }
          divOut(1000000000);
          divOut(100000000);
          divOut(10000000);
          divOut(1000000);
          divOut(100000);
          divOut(10000);
          divOut(1000);
          divOut(100);
          divOut(10);
          outDgt(num);
          break;
        case 'p':
        case 'x':
        case 'X':
          uc= ch=='X';
          num=va_arg(va, unsigned int);
          //divOut(0x100000000UL);
          divOut(0x10000000);
          divOut(0x1000000);
          divOut(0x100000);
          divOut(0x10000);
          divOut(0x1000);
          divOut(0x100);
          divOut(0x10);
          outDgt(num);
          break;
        case 'c':
          out((char) (va_arg(va, int)));
          break;
        case 's':
          p=va_arg(va, char*);
          break;
        case '%':
          out('%');
        default:
          break;
      }
      *bf=0;
      bf=p;

      while (*bf++ && w > 0)
        w--;
      while (w-- > 0)
        if (str-str_orig < n)
          *str++ = lz ? '0' : ' ';
        else
          goto abort;
      while ((ch= *p++))
        if (str-str_orig < n)
          *str++ = ch;
        else
          goto abort;
    }
  }

abort:
  if(str-str_orig < n) {
    *str = '\0';
  } else {
    *(--str) = '\0';
  }
  return str - str_orig;
}

int sprintf(char *str, const char *fmt, ...)
{
  int m;
  __VALIST va;
  va_start(va,fmt);
  m = vsnprintf(str, 0xffffffff, fmt, va);
  va_end(va);
  return m;
}

int snprintf(char *str, size_t n, const char *fmt, ...)
{
  int m;
  __VALIST va;
  va_start(va,fmt);
  m = vsnprintf(str, n, fmt, va);
  va_end(va);
  return m;
}

int printf(const char *fmt, ...)
{
  int m,i;
  char str[256];
  __VALIST va;
  va_start(va,fmt);
  m = vsnprintf(str, sizeof(str), fmt, va);
  va_end(va);
  for (i=0;i<m;i++)
	  putchar(str[i]);
  return m;
}

int puts(const char *s)
{
  char c;

  while (c=*s++)
	  putchar(c);
  putchar('\n');

  return strlen(s);
}

#if SLIP_BRIDGE_CONF_NO_PUTCHAR
int putchar(int c)
{
  uart0_writeb(c);
  return 1;
}
#endif
