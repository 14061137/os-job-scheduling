#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "job.h"
#include <time.h>
//#define DEBUG
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
	int jjid[100]={0};
	char timebuf[BUFLEN];
	int count=0;
	int mark = 0;
	struct jobinfo job;
	int  i=0;
	int  hit=0;
	if(argc!=1)
	{
		usage();
		return 1;
	}

	statcmd.type=STAT;
	statcmd.defpri=0;
	statcmd.owner=getuid();
	statcmd.argnum=0;
#ifdef DEBUG
	printf("statcmd cmdtype\t%d  (-1 means enq , -2 means deq ,-3means stat)\n"
			"statcmd owner\t%d\n"
			"statcmd defpri\t%d\n"
			//"enqcmd data\t%s\n"
			"statcmd argnum\t%d\n",
			statcmd.type,statcmd.owner,statcmd.defpri,statcmd.argnum);

#endif 

	if((fd=open("/tmp/server",O_WRONLY))<0)
		error_sys("stat open fifo failed");

	if(write(fd,&statcmd,DATALEN)<0)
		error_sys("stat write failed");

	close(fd);
	//sleep(3);

	if((fd=open("/tmp/mypipe",O_RDONLY|O_NONBLOCK))<0)
		error_sys("stat open fifo failed");

	
	count=0;
	for(i=0;i<100;i++){
		jjid[i]==0;
	}
	/*for(i=0;i<100;i++){
		printf("%d\n",jjid[i]);

	}*/
	while((read(fd,&job,sizeof(struct jobinfo)))<=0);
	do
	
		/* code */
	{


		//if(((read(fd,&job,DATALEN))<0)){
		//	break;
		//}
		/*for(i=0;i<100;i++){
			printf("%d\n",jjid[i]);
		}*/
		count++;
		strcpy(timebuf,ctime(&(job.create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		/*for(i=0;i<100;i++){
			if(jjid[i]==job.jid) mark=1;
		}*/
		//	printf("%d\n",job.jid );
		/*if(mark==0){
		//	printf("223333\n");
			for(i=0;i<100;i++){
				if(jjid[i]==0) {
					jjid[i]=job.jid;
					break;
				//	printf("wwwwwwww\n");
				}
			}*/
		if(count==1){
			printf("JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\n");
			printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			job.jid,
			job.pid,
			job.ownerid,
			job.run_time,
			job.wait_time,
			timebuf,"RUNNING");
			}
		else{
			printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			job.jid,
			job.pid,
			job.ownerid,
			job.run_time,
			job.wait_time,
			timebuf,"READY");
			}
	//}
	/*else{
		hit ++;
		if(hit==100) break;
	}*/
	}while((read(fd,&job,sizeof(struct jobinfo)))>0);
	close(fd);
	return 0;
}
