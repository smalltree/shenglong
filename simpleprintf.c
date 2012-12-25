#include "simpleprintf.h"

#ifdef DebugPrint

void printstr(char *s)
{
   unsigned long port = PORT3f8;
   while (*s) {
     while (((*(volatile unsigned char*)(PORT3f8+5)) & 0x20)==0);
     *(unsigned char*)port = *s;
     s++;
   } 
   while (((*(volatile unsigned char*)(PORT3f8+5)) & 0x20)==0);
}   

void printstr1(char *s)
{
   unsigned long port = PORT3f8;

     while (*s) {
     *(unsigned char*)port = *s;
     s++;
   } 
}  

void printnum(unsigned long long n)
{
  int i,j;
  unsigned char a[40];
  unsigned long port = PORT3f8;
  i = 0;
  do {
   a[i] = n % 16;
   n = n / 16;
   i++;
  }while(n);
 
  for (j=i-1;j>=0;j--) {
   if (a[j]>=10) {
     while (((*(volatile unsigned char*)(PORT3f8+5)) & 0x20)==0);
     *(unsigned char*)port = 'a' + a[j] - 10;
   }else{
     while (((*(volatile unsigned char*)(PORT3f8+5)) & 0x20)==0);
     *(unsigned char*)port = '0' + a[j];
   }
  }
  /*printstr("\r\n");*/
}

void printnum1(unsigned long long n)
{
  int i,j;
  unsigned char a[16];
  unsigned long port = PORT3f8;

   i = 0;
  do {
   a[i] = n % 16;
   n = n / 16;
   i++;
  }while(n);
 
  for (j=i-1;j>=0;j--) {
   if (a[j]>=10) {
     *(volatile unsigned char*)port = 'a' + a[j] - 10;
   }else{
     *(volatile unsigned char*)port = '0' + a[j];
   }
  }
  printstr1("\r\n");
}



#endif

