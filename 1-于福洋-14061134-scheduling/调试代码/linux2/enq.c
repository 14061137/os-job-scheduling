#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include "job.h"

#define DEBUG
/* 
 * ÃüÁîÓï·¨¸ñÊ½
 *     enq [-p num] e_file args
 */
void usage()
{
	printf("Usage: enq[-p num] e_file args\n"
		"\t-p num\t\t specify the job priority\n"
		"\te_file\t\t the absolute path of the exefile\n"
		"\targs\t\t the args passed to the e_file\n");
}

int main(int argc,char *argv[])
{
	int p=0;
	int fd;
	char c,*offset;
	struct jobcmd enqcmd;

	if(argc==1)
	{
		usage();
		return 1;
	}
	//printf("%c\n",*(++argv)[0] );
	while(--argc>0 && (*++argv)[0]=='-')
	{
		//printf("%s\n",*(argv)[0]);
		while(c=*++argv[0])	{

			if(c=='p'){
				p=atoi(*(++argv));
				//printf("%d\n", p);
				argc--;
				
			}

			else {
				printf("Illegal option %c\n",c);
				return 1;
			}

		}	//	printf("c\n");
		/*	printf("%c\n", c);
			switch(c)
		{
			case 'p':{
				p=atoi(*(++argv));
				argc--;
				break;
		}
			default:
				printf("Illegal option %c\n",c);
				return 1;
		}*/

	}
	//printf("%d\n", p);
	if(p<0||p>3)
	{
		printf("invalid priority:must between 0 and 3\n");
		return 1;
	}

	enqcmd.type=ENQ;
	enqcmd.defpri=p;
	enqcmd.owner=getuid();
	enqcmd.argnum=argc;
	offset=enqcmd.data;

	while (argc-->0)
	{
		strcpy(offset,*argv);
		strcat(offset,":");
		offset=offset+strlen(*argv)+1;
		argv++;
	}

    #ifdef DEBUG
		printf("enqcmd cmdtype\t%d  (-1 means enq , -2 means deq ,-3means stat)\n"
			"enqcmd owner\t%d\n"
			"enqcmd defpri\t%d\n"
			"enqcmd data\t%s\n"
			"enqcmd argnum\t%d\n",
			enqcmd.type,enqcmd.owner,enqcmd.defpri,enqcmd.data,enqcmd.argnum);

    #endif 

		if((fd=open("/tmp/server",O_WRONLY))<0)
			error_sys("enq open fifo failed");

		if(write(fd,&enqcmd,DATALEN)<0)
			error_sys("enq write failed");

		close(fd);
		return 0;
}

