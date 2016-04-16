#include <sys/stat.h>
#include <stdio.h>
int main(){
	struct stat statbuf;
	int i=145431;
	i=stat("/usr/",&statbuf);
	printf("%d\n", i);
	printf("%d",statbuf.st_size);
	return 0;
}