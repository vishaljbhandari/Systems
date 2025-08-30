#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
main (int argc, char *argv[])
{
  int fd, i;
  if (argc != 3)
    {
      printf ("oops, need the file name and the string to add to the file...\n");
      exit (1);
    }
  if ((fd = open (argv[1], O_RDWR)) == -1)
    {
      perror ("Opening file to share");
      exit (2);
    }
  for (i = 0; i < 200; i++)
    {
      write (fd, argv[2], strlen (argv[2]));
      fsync(fd);
      sleep (1);
    }
}
