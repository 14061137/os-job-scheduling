#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include "job.h"

//#define DEBUG
//#define UPDATE
//#define DENQ
//#define DDEQ
//#define DSTAT
//#define JOBS   //false
//#define JOBSW
//#define SIGH

int jobid=0;
int siginfo=1;
int fifo;
int globalfd;
int fd;

struct waitqueue *head=NULL;
struct waitqueue *next=NULL,*current =NULL;

/* µ÷¶È³ÌÐò */
void scheduler()
{
	struct jobinfo *newjob=NULL;
	struct jobcmd cmd;
	int  count = 0;
	bzero(&cmd,DATALEN);
	if((count=read(fifo,&cmd,DATALEN))<0)
		error_sys("read fifo failed");
#ifdef DEBUG
	printf("Reading while other process send command !!\n");
	if(count){
		printf("cmd cmdtype\t%d\ncmd defpri\t%d\ncmd data\t%s\n",cmd.type,cmd.defpri,cmd.data);
	}
	/*else
		printf("no data read\n");*/
#endif

	/* ¸üÐÂµÈ´ý¶ÓÁÐÖÐµÄ×÷Òµ */
#ifdef DEBUG
	printf("Update wait jobs in wait queue!!\n");
#endif
	updateall();

	switch(cmd.type){
	case ENQ:
		do_enq(newjob,cmd);
		break;
	case DEQ:
		do_deq(cmd);
		break;
	case STAT:
		do_stat();
		break;
	default:
		break;
	}

	/* Ñ¡Ôñ¸ßÓÅÏÈ¼¶×÷Òµ */
#ifdef DEBUG
	printf("select which job to run next !!\n");
#endif
	next=jobselect();
	/* ×÷ÒµÇÐ»» */
#ifdef DEBUG
	printf("switch to next job !\n");
#endif
	jobswitch();
}

int allocjid()
{
	return ++jobid;
}

void updateall()
{
	struct waitqueue *p;
#ifdef UPDATE
	printf("Before Update !!!\n");
	do_stat();
#endif
	/* ¸üÐÂ×÷ÒµÔËÐÐÊ±¼ä */
	if(current)
		current->job->run_time += 1; /* ¼Ó1´ú±í1000ms */

	/* ¸üÐÂ×÷ÒµµÈ´ýÊ±¼ä¼°ÓÅÏÈ¼¶ */
	for(p = head; p != NULL; p = p->next){
		p->job->wait_time += 1000;
		if(p->job->wait_time >= 5000 && p->job->curpri < 3){
			p->job->curpri++;
			p->job->wait_time = 0;
		}
	}
#ifdef UPDATE
	printf("After Update !!\n");
	do_stat();
#endif
}

struct waitqueue* jobselect()
{
	struct waitqueue *p,*prev,*select,*selectprev;
	int highest = -1;
	char timebuf[BUFLEN];

	select = NULL;
	selectprev = NULL;

	if(head){
		/* ±éÀúµÈ´ý¶ÓÁÐÖÐµÄ×÷Òµ£¬ÕÒµ½ÓÅÏÈ¼¶×î¸ßµÄ×÷Òµ */
		/*for(prev = head, p = head; p != NULL; prev = p,p = p->next)
			if(p->job->curpri > highest){
				select = p;
				selectprev = prev;
				highest = p->job->curpri;
			}


			
			selectprev->next = select->next;
			if (select == selectprev)
				head = NULL;*/
				//printf("%d\n",head->job->jid);
				select=head;
				//printf("%d\n",select->job->jid);
#ifdef JOBS
	if(select){
		printf("select\n");
	printf("JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\n");
	
		strcpy(timebuf,ctime(&(select->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%s\n",timebuf );
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			select->job->jid,
			select->job->pid,
			select->job->ownerid,
			select->job->run_time,
			select->job->wait_time,timebuf,
			"READY");
	}


#endif
				head=head->next;
				select->next=NULL;
	}



	return select;



}

void jobswitch()
{
	struct waitqueue *p;
	int i;

#ifdef JOBSW
	printf("before switch:\n");
	do_stat();
#endif

	if(current && current->job->state == DONE){ /* µ±Ç°×÷ÒµÍê³É */
		/* ×÷ÒµÍê³É£¬É¾³ýËü */
		for(i = 0;(current->job->cmdarg)[i] != NULL; i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i] = NULL;
		}
		/* ÊÍ·Å¿Õ¼ä */
		free(current->job->cmdarg);
		free(current->job);
		free(current);

		current = NULL;
	}

	if(next == NULL && current == NULL) /* Ã»ÓÐ×÷ÒµÒªÔËÐÐ */

		return;
	else if (next != NULL && current == NULL){ /* ¿ªÊ¼ÐÂµÄ×÷Òµ */

		printf("begin start new job\n");
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		sleep(3);
		kill(current->job->pid,SIGCONT);
#ifdef JOBSW
	printf("After switch:\n");
	do_stat();
#endif
		return;
	}
	else if (next != NULL && current != NULL){ /* ÇÐ»»×÷Òµ */

		printf("switch to Pid: %d\n",next->job->pid);
		sleep(3);
		kill(current->job->pid,SIGSTOP);
		current->job->curpri = current->job->defpri;
		current->job->wait_time = 0;
		current->job->state = READY;

		/* ·Å»ØµÈ´ý¶ÓÁÐ */
		if(head){
			for(p = head; p->next != NULL; p = p->next);
			p->next = current;
		}else{
			head = current;
		}
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		current->job->wait_time = 0;
		sleep(3);
		kill(current->job->pid,SIGCONT);
#ifdef JOBSW
	printf("After switch:\n");
	do_stat();
#endif
		return;
	}else{ /* next == NULLÇÒcurrent != NULL£¬²»ÇÐ»» */
#ifdef JOBSW
	printf("After switch:\n");
	do_stat();
#endif
		return;
	}

}

void sig_handler(int sig,siginfo_t *info,void *notused)
{
	int status;
	int ret;
	//printf ("hi\n");

	switch (sig) {
case SIGVTALRM: /* µ½´ï¼ÆÊ±Æ÷ËùÉèÖÃµÄ¼ÆÊ±¼ä¸ô */
	scheduler();
	#ifdef DEBUG
		printf("SIGVTALRM RECEIVED !\n" );
	#endif
	return;
case SIGCHLD: /* ×Ó½ø³Ì½áÊøÊ±´«ËÍ¸ø¸¸½ø³ÌµÄÐÅºÅ */
	ret = waitpid(-1,&status,WNOHANG);
	if (ret == 0){// printf("2333\n");
		return;}
	else{
		#ifdef DEBUG
			printf("SIGCHLD RECEIVED !\n");
		#endif
	}
	if(WIFEXITED(status)){
		current->job->state = DONE;
		printf("normal termation, exit status = %d\n",WEXITSTATUS(status));    //正常结束
	}else if (WIFSIGNALED(status)){
		printf("abnormal termation, signal number = %d\n",WTERMSIG(status));   //被杀死
	}else if (WIFSTOPPED(status)){ 
		printf("child stopped, signal number = %d\n",WSTOPSIG(status));        //暂停
	}
	printf("ssss\n");
	#ifdef SIGH
		printf("SIGCHLD GET\n");
		do_stat();
	#endif
	return;
	default:
		return;
	}
}

void do_enq(struct jobinfo *newjob,struct jobcmd enqcmd)
{
	struct waitqueue *newnode,*p;
	int i=0,pid;
	char *offset,*argvec,*q;
	char **arglist;
	sigset_t zeromask;

#ifdef DENQ
	printf("Before enq !!1\n");
	do_stat();
#endif
	sigemptyset(&zeromask);

	/* ·â×°jobinfoÊý¾Ý½á¹¹ */
	newjob = (struct jobinfo *)malloc(sizeof(struct jobinfo));
	newjob->jid = allocjid();
	newjob->defpri = enqcmd.defpri;
	newjob->curpri = enqcmd.defpri;
	newjob->ownerid = enqcmd.owner;
	newjob->state = READY;
	newjob->create_time = time(NULL);
	newjob->wait_time = 0;
	newjob->run_time = 0;
	arglist = (char**)malloc(sizeof(char*)*(enqcmd.argnum+1));
	newjob->cmdarg = arglist;
	offset = enqcmd.data;
	argvec = enqcmd.data;
	/*printf("%s233\n", offset);
	printf("%s\n", argvec);*/
	//printf("%d\n",enqcmd.argnum );
	while (i < enqcmd.argnum){
		if(*offset == ':'){
			*offset++ = '\0';
			q = (char*)malloc(offset - argvec);

			strcpy(q,argvec);
			//printf("%s\n",q );
			arglist[i++] = q;
			argvec = offset;
			//printf("%s\n",argvec );
			//printf("%s\n", offset);
		}else
			offset++;
	}

	arglist[i] = NULL;

#ifdef DEBUG

	printf("enqcmd argnum %d\n",enqcmd.argnum);
	for(i = 0;i < enqcmd.argnum; i++)
		printf("parse enqcmd:%s\n",arglist[i]);

#endif

	/*ÏòµÈ´ý¶ÓÁÐÖÐÔö¼ÓÐÂµÄ×÷Òµ*/
	newnode = (struct waitqueue*)malloc(sizeof(struct waitqueue));
	newnode->next =NULL;
	newnode->job=newjob;

	if(head)
	{
		for(p=head;p->next != NULL; p=p->next);
		p->next =newnode;
	}else
		head=newnode;

	/*Îª×÷Òµ´´½¨½ø³Ì*/
	if((pid=fork())<0)
		error_sys("enq fork failed");

	if(pid==0){
		newjob->pid =getpid();
		/*×èÈû×Ó½ø³Ì,µÈµÈÖ´ÐÐ*/
		raise(SIGSTOP);
#ifdef DEBUG

		printf("begin running\n");
		for(i=0;arglist[i]!=NULL;i++)
			printf("arglist %s\n",arglist[i]);
#endif

		/*¸´ÖÆÎÄ¼þÃèÊö·ûµ½±ê×¼Êä³ö*/
		dup2(globalfd,1);
		/* Ö´ÐÐÃüÁî */
		if(execv(arglist[0],arglist)<0)
			printf("exec failed\n");
		exit(1);
	}else{
		newjob->pid=pid;
	}
#ifdef DENQ
	printf("After enq !!1\n");
	do_stat();
#endif
}

void do_deq(struct jobcmd deqcmd)
{
	int deqid,i;
	struct waitqueue *p,*prev,*select,*selectprev;
	deqid=atoi(deqcmd.data);
#ifdef DDEQ
	printf("before deq !!\n");
	do_stat();
#endif
#ifdef DEBUG
	printf("deq jid %d\n",deqid);
#endif

	/*current jodid==deqid,ÖÕÖ¹µ±Ç°×÷Òµ*/
	if (current && current->job->jid ==deqid){
		printf("teminate current job\n");
		kill(current->job->pid,SIGKILL);
		for(i=0;(current->job->cmdarg)[i]!=NULL;i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i]=NULL;
		}
		free(current->job->cmdarg);
		free(current->job);
		free(current);
		current=NULL;
	}
	else{ /* »òÕßÔÚµÈ´ý¶ÓÁÐÖÐ²éÕÒdeqid */
		select=NULL;
		selectprev=NULL;
		if(head){
			for(prev=head,p=head;p!=NULL;prev=p,p=p->next)
				if(p->job->jid==deqid){
					select=p;
					selectprev=prev;
					break;
				}
				selectprev->next=select->next;
				if(select==selectprev)
					head=NULL;
		}
		if(select){
			for(i=0;(select->job->cmdarg)[i]!=NULL;i++){
				free((select->job->cmdarg)[i]);
				(select->job->cmdarg)[i]=NULL;
			}
			free(select->job->cmdarg);
			free(select->job);
			free(select);
			select=NULL;
		}
	}
#ifdef DDEQ
	printf("After deq !!\n");
	do_stat();
#endif
}

void do_stat()
{
	struct waitqueue *p;
	char timebuf[BUFLEN];
	struct stat statbuf;
	
	/*
	*´òÓ¡ËùÓÐ×÷ÒµµÄÍ³¼ÆÐÅÏ¢:
	*1.×÷ÒµID
	*2.½ø³ÌID
	*3.×÷ÒµËùÓÐÕß
	*4.×÷ÒµÔËÐÐÊ±¼ä
	*5.×÷ÒµµÈ´ýÊ±¼ä
	*6.×÷Òµ´´½¨Ê±¼ä
	*7.×÷Òµ×´Ì¬
	*/
	#ifdef DSTAT
		printf("before stat!!\n");
		printf("JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\n");
	if(current){
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			timebuf,"RUNNING");
		if((fd=open("/tmp/server",O_WRONLY))<0)
			error_sys("stat open fifo failed");

		if(write(fd,&(enqcmd->job),DATALEN)<0)
			error_sys("enq write failed");

		close(fd);
	}

	for(p=head;p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		/*printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY");*/
		if((fd=open("/tmp/server",O_WRONLY))<0)
			error_sys("stat open fifo failed");

		if(write(fd,(enqcmd->job),DATALEN)<0)
			error_sys("enq write failed");

		close(fd);
	}

	#endif

	
	/* ÔÚ·Ç×èÈûÄ£Ê½ÏÂ´ò¿ªFIFO */
	//if((fd=open("/tmp/mypipe",O_RDONLY|O_NONBLOCK))<0)
	//	error_sys("open fifo failed");
	if((fd=open("/tmp/mypipe",O_WRONLY))<0)
		error_sys("stat open fifo failed");
	/* ´òÓ¡ÐÅÏ¢Í·²¿ */
	printf("JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\n");
	if(current){
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			timebuf,"RUNNING");
		if(write(fd,(current->job),sizeof(struct jobinfo))<0)
			error_sys("enq write failed");

		
	}

	for(p=head;p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY");
		if(write(fd,(p->job),sizeof(struct jobinfo))<0)
			error_sys("enq write failed");

		
	}
	#ifdef DSTAT
		printf("after stat!!\n");
		printf("JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\n");
	if(current){
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			timebuf,"RUNNING");
	}

	for(p=head;p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY");
	}

	#endif
}

int main()
{
	struct timeval interval;
	struct itimerval new,old;
	struct stat statbuf;
	struct sigaction newact,oldact1,oldact2;

	#ifdef DEBUG
		printf("DEBUG IS OPEN ! \n");

	#endif
  //  printf("hi\n");
	if(stat("/tmp/mypipe",&statbuf)==0){
		if(remove("/tmp/mypipe")<0)
			error_sys("remove failed233");
	}
	if(stat("/tmp/server",&statbuf)==0){
		/* Èç¹ûFIFOÎÄ¼þ´æÔÚ,É¾µô */
		//printf("%d",statbuf);
		//printf("hi2\n");
		if(remove("/tmp/server")<0)
			error_sys("remove failed");
	}
	
	//printf("hi2\n");
//	sleep(50);
	if(mkfifo("/tmp/server",0666)<0)
		error_sys("mkfifo failed");
	if(mkfifo("/tmp/mypipe",0666)<0)
		error_sys("mkfifo failed");
	/* ÔÚ·Ç×èÈûÄ£Ê½ÏÂ´ò¿ªFIFO */
	if((fifo=open("/tmp/server",O_RDONLY|O_NONBLOCK))<0)
		error_sys("open fifo failed");

	if((fd=open("/tmp/mypipe",O_RDONLY|O_NONBLOCK))<0)
		error_sys("open fifo failed");
	
	//printf("hi2\n");
//	sleep(50);
	

	/* ½¨Á¢ÐÅºÅ´¦Àíº¯Êý */
	newact.sa_sigaction=sig_handler;
	//printf("en \n");
	sigemptyset(&newact.sa_mask);
	newact.sa_flags=SA_SIGINFO;
	sigaction(SIGCHLD,&newact,&oldact1);
	sigaction(SIGVTALRM,&newact,&oldact2);

	/* ÉèÖÃÊ±¼ä¼ä¸ôÎª1000ºÁÃë */
	interval.tv_sec=1;
	interval.tv_usec=0;

	new.it_interval=interval;
	new.it_value=interval;
	setitimer(ITIMER_VIRTUAL,&new,&old);

	while(siginfo==1);

	close(fifo);
	close(fd);
	close(globalfd);
	return 0;
}
