#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "job.h"
#include <time.h>
/* 
 * √¸¡Ó”Ô∑®∏Ò Ω
 *     stat
 */
void usage()
{
	printf("Usage: stat\n");		
}

int main(int argc,char *argv[])
{
	struct jobcmd statcmd;
	int fd;
	char timebuf[BUFLEN];
	struct jobinfo job;
	int count=0;
	if(argc!=1)
	{
		usage();
		return 1;
	}

	statcmd.type=STAT;
	statcmd.defpri=0;
	statcmd.owner=getuid();
	statcmd.argnum=0;

	if((fd=open("/tmp/server",O_WRONLY))<0)
		error_sys("stat open fifo failed");

	if(write(fd,&statcmd,DATALEN)<0)
		error_sys("stat write failed");

	close(fd);
	if((fd=open("/tmp/mypipe",O_RDONLY|O_NONBLOCK))<0)
		error_sys("stat open fifo failed");
	count=0;
	while((read(fd,&job,sizeof(struct jobinfo)))<=0);
	do
	
		/* code */
	{


		count++;
		strcpy(timebuf,ctime(&(job.create_time)));
		timebuf[strlen(timebuf)-1]='\0';
	
		if(count==1){
			printf("JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tPRI\tSTATE\n");
			printf("%d\t%d\t%d\t%d\t%d\t%s\t%d\t%s\n",
			job.jid,
			job.pid,
			job.ownerid,
			job.run_time,
			job.wait_time,
			
			timebuf,job.curpri,"RUNNING");
		
			}
		else{
			printf("%d\t%d\t%d\t%d\t%d\t%s\t%d\t%s\n",
			job.jid,
			job.pid,
			job.ownerid,
			job.run_time,
			job.wait_time,
			
			timebuf,job.curpri,"READY");
		
		}	
	}while((read(fd,&job,sizeof(struct jobinfo)))>0);
	close(fd);
	return 0;
}
