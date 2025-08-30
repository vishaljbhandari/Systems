#include <unistd.h>
main ()
{
execl("/bin/head","head","/etc/shadow",0);
}


