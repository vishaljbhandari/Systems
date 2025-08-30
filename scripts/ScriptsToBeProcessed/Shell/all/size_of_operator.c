#include<stdio.h>
struct abc{
int x, y;
int h[10];
};
void main()
{
  char *ptr1,*ptr2;
  struct abc fl[2];   
  ptr1 = &fl[0];
  ptr2 = &fl[1];
  printf("%u",ptr2-ptr1);
}
