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

int jobid=0;
int flag = 0;
int siginfo=1;
int fifo;
int globalfd;
int time_count = 0;
int fd;
/*struct jobinfo uselessjob;
uselessjob.cmdarg = NULL;
uselessjob.create_time = -1;
uselessjob.curpri = -1;
uselessjob.defpri = -1;
uselessjob.jid = -1;
uselessjob.ownerid = -1;
uselessjob.wait_time = -1
uselessjob.pid = -1;
uselessjob.run_time = -1;
sleep
void jobclone(struct jobinfo j1, struct jobinfo j2){

}*/


struct waitqueue *head1 = NULL, *head2 = NULL,*head3 = NULL;

struct waitqueue *next = NULL,*current = NULL;


/* µ÷¶È³ÌÐò */
////////////////////
void removejob(struct waitqueue *head,struct waitqueue *p){
    struct waitqueue *now = head;
    while(now != NULL){
        if(now ->next == p){
            now->next = p ->next;
            p->next = NULL;
            return;
        }
        //pre = now;
        now = now->next;
    }

}

void addjob(struct waitqueue *head, struct waitqueue *p)
{
	/* data */
	struct waitqueue *now;
	for(now = head ;now->next!= NULL;now = now->next);
	now ->next = p;
	p->next = NULL;

}

//////////////////////////

void scheduler()
{
	struct jobinfo *newjob=NULL;
	struct waitqueue *now1 = NULL;
	struct jobcmd cmd;
	int  count = 0;
	bzero(&cmd,DATALEN);
	if((count=read(fifo,&cmd,DATALEN))<0)
		error_sys("read fifo failed");
#ifdef DEBUG

	if(count){
		printf("cmd cmdtype\t%d\ncmd defpri\t%d\ncmd data\t%s\n",cmd.type,cmd.defpri,cmd.data);
	}
	else
		printf("no data read\n");
#endif
	/* ¸üÐÂµÈ´ý¶ÓÁÐÖÐµÄ×÷Òµ */
	updateall();
	switch(cmd.type){
	case ENQ:
		do_enq(newjob,cmd);
		break;
	case DEQ:
		do_deq(cmd);
		break;
	case STAT:
		do_stat(cmd);
		break;
	default:
		break;
	}
	/* Ñ¡Ôñ¸ßÓÅÏÈ¼¶×÷Òµ */
	/* ×÷ÒµÇÐ»» */
	//printf("111111\n");
	if(current&&current->job->curpri == 2)
	{
		if(flag == 0){
			if(time_count < 0)
					time_count++;
				else{
					next=jobselect();
					jobswitch();
				}
		}
		else
			{
				flag = 0;
				time_count = -1; 
			}
	}
	else if(current&&current->job->curpri == 1 ){
			if(flag == 0){
				if(time_count == 0)
					time_count++;
				else{
					next=jobselect();
					jobswitch();
				}
			}
			else
			{
				flag = 0;
				time_count = -1; 
			}
	}
	else if(current&&current->job->curpri == 0 ){
			if(flag == 0){
				if(time_count < 4)
					time_count++;
				else{
					next=jobselect();
					jobswitch();
				}
			}
			else
			{
				flag = 0;
				time_count = -1; 
			}
	}
	else
	{
		if(flag == 0){
			next=jobselect();
			jobswitch();
		}
		else
			flag = 0;
	}
}

int allocjid()
{
	return ++jobid;
}

void updateall()
{
	struct waitqueue *p;
    struct waitqueue *q, *pre = head2;
	/* ¸üÐÂ×÷ÒµÔËÐÐÊ±¼ä */
	if(current)
		current->job->run_time += 1; /* ¼Ó1´ú±í1000ms */
	/* ¸üÐÂ×÷ÒµµÈ´ýÊ±¼ä¼°ÓÅÏÈ¼¶ */
	for(p = head1->next; p != NULL; p = p->next){
		p->job->wait_time += 1000;
		if(p->job->wait_time >= 10000){

			p->job->curpri++;
			p->job->wait_time = 0;            
            removejob(head1, p);
            addjob(head2, p);
           // printf("pri up ,remove");
		}
	}
	for(p = head2->next; p != NULL; p = p->next){
		p->job->wait_time += 1000;
		if(p->job->wait_time >= 10000){

			p->job->curpri++;
			p->job->wait_time = 0;
            //p->next = NULL;
            removejob(head2, p);
            addjob(head3, p);
           // printf("pri up ,remove");
		}
	}
	for(p = head3->next; p != NULL; p = p->next){
        p->job->wait_time += 1000;
	}

}

struct waitqueue* jobselect()
{
	struct waitqueue *p,*prev,*select,*selectprev;
	int highest = -1;

	select = NULL;
	
	if(head3->next){
		/* ±éÀúµÈ´ý¶ÓÁÐÖÐµÄ×÷Òµ£¬ÕÒµ½ÓÅÏÈ¼¶×î¸ßµÄ×÷Òµ */
		//printf("select 3\n");
        	select = head3->next;
        	head3->next = select->next;
        	select->next = NULL;
        //	printf("%d %d\n",select->job->pid, select->job->curpri);
	}
	else if(current && current->job->defpri == 2&&current->job->state != DONE){
		//removejob(head3,current);
	}
	else if(head2->next){

        //	printf("select 2\n");
        	select = head2->next;
        	head2->next = select->next;
        	select->next = NULL;
        //	printf("%d %d\n",select->job->pid, select->job->curpri);
	}
	else if(current && current->job->defpri == 1&&current->job->state != DONE){
		//printf("1111111\n\n\n\n\n\n\n");
		//removejob(head2,current);
		
	}
	else if(head1->next){
		//	printf("select 1\n");
        	select = head1->next;
        	head1->next = select->next;
        	select->next = NULL;
        //	printf("%d %d\n",select->job->pid, select->job->curpri);
	}
	else if(current && current->job->defpri == 0 &&current->job->state != DONE){
		//printf("0000000\n\n\n\n\n\n\n");
		//removejob(head1,current);
		//select = current;

	}

	return select;
}

void jobswitch()
{
	struct waitqueue *p;
	int i;
	time_count = 0;
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
		sleep(1);
		kill(current->job->pid,SIGCONT);
		//printf("%d %d\n",current->job->pid, current->job->curpri);
		return;
	}
	else if (next != NULL && current != NULL){ /* ÇÐ»»×÷Òµ */
		/*if(current->job->curpri == 2)
			removejob(head3,current);
		else if(current->job->curpri == 1)
			removejob(head2,current);
		else if(current->job->curpri == 0)
			removejob(head1,current);*/
		printf("switch to Pid: %d\n",next->job->pid);
		kill(current->job->pid,SIGSTOP);
		current->job->curpri = current->job->defpri;
		current->job->wait_time = 0;
		current->job->state = READY;
		if(current->job->curpri == 0){
            addjob(head1,current);
		}
		if(current->job->curpri == 1){
           addjob(head2,current);
		}
		if(current->job->curpri == 2){
            addjob(head3,current);
		}
		/* ·Å»ØµÈ´ý¶ÓÁÐ */
		/*if(head){
			for(p = head; p->next != NULL; p = p->next);
			p->next = current;
		}else{
			head = current; STat
		}*/
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		current->job->wait_time = 0;
		kill(current->job->pid,SIGCONT);
		return;
	}
	else{ /* next == NULLÇÒcurrent != NULL£¬²»ÇÐ»» */
		//printf("1212121212121212121212");
		//printf("%d %d\n",current->job->pid, current->job->curpri);
		return;
	}
}

void sig_handler(int sig,siginfo_t *info,void *notused)
{
	int status;
	int ret;

	switch (sig) {
case SIGALRM: /* µ½´ï¼ÆÊ±Æ÷ËùÉèÖÃµÄ¼ÆÊ±¼ä¸ô */
	
	/*if(current&&current->job->curpri == 2)
	{
		scheduler();
	}
	else if(current&&current->job->curpri == 1 ){
		if(time_count == 0)
			time_count++;
		else
		{
			scheduler();
		}
	}
	else if(current&&current->job->curpri == 0 ){
		if(time_count < 4)
			time_count++;
		else{
			scheduler();
		}
	}
	else
	{*/
	scheduler();
	//}
	return;
case SIGCHLD: /* ×Ó½ø³Ì½áÊøÊ±´«ËÍ¸ø¸¸½ø³ÌµÄÐÅºÅ */
	ret = waitpid(-1,&status,WNOHANG);
	if (ret == 0)
		return;
	if(WIFEXITED(status)){
		current->job->state = DONE;
		printf("normal termation, exit status = %d\n",WEXITSTATUS(status));
	}else if (WIFSIGNALED(status)){
		printf("abnormal termation, signal number = %d\n",WTERMSIG(status));
	}else if (WIFSTOPPED(status)){
		printf("child stopped, signal number = %d\n",WSTOPSIG(status));
	}
	return;
	default:
		return;
	}
}

void do_enq(struct jobinfo *newjob,struct jobcmd enqcmd)
{
	struct waitqueue *newnode,*p,*now1;
	int i=0,pid;
	char *offset,*argvec,*q;
	char **arglist;
	sigset_t zeromask;

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
	while (i < enqcmd.argnum){
		if(*offset == ':'){
			*offset++ = '\0';
			q = (char*)malloc(offset - argvec);
			strcpy(q,argvec);
			arglist[i++] = q;
			argvec = offset;
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
	if(current && newnode->job->curpri >= current->job->curpri){
			flag =1;////////////////////////////
	}
	else{
		if(newnode->job->curpri == 0)
		{
			addjob(head1,newnode);
			//printf("add in 1");
		}
		else if(newnode->job->curpri == 1)
		{
			addjob(head2,newnode);
			//printf("add in 2");
		}
		else if(newnode->job->curpri == 2)
		{
			addjob(head3,newnode);
			//printf("add in 3");
		}
	}

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

		/*¸´ÖÆÎÄ¼þÃèÊö·ûµ½±ê×¼Êä³ö*switch to Pid/
		dup2(globalfd,1);
		/* Ö´ÐÐÃüÁî */ 
		if(execv(arglist[0],arglist)<0)
			printf("exec failed\n");
		exit(1);
	}else{
		sleep(1);
		newjob->pid=pid;
	}
	if(flag == 1){
		struct waitqueue *now = NULL; 
		time_count = 0;
		if(current && current->job->state == DONE){ 		
			for(i = 0;(current->job->cmdarg)[i] != NULL; i++){
				free((current->job->cmdarg)[i]);
				(current->job->cmdarg)[i] = NULL;
			}
			free(current->job->cmdarg);
			free(current->job);
			free(current);

			current = newnode;
			next = NULL;
			current->job->state = RUNNING;
			current->job->wait_time = 0;
			kill(current->job->pid,SIGCONT);
			return;
		}
			/*if(current->job->curpri == 2)
			removejob(head3,current);
		else if(current->job->curpri == 1)
			removejob(head2,current);
		else if(current->job->curpri == 0)
			removejob(head1,current);*/
			printf("switch to Pid: %d\n",newnode->job->pid);
			kill(current->job->pid,SIGSTOP);
			current->job->curpri = current->job->defpri;
			current->job->wait_time = 0;
			current->job->state = READY;
			if(current->job->curpri == 0){
	            addjob(head1,current);
			}
			if(current->job->curpri == 1){
	           addjob(head2,current);
			}
			if(current->job->curpri == 2){
	           addjob(head3,current);
			}
			current = newnode;
			next = NULL;
			current->job->state = RUNNING;
			current->job->wait_time = 0;
			kill(current->job->pid,SIGCONT);
			return;
	}
}

void do_deq(struct jobcmd deqcmd)
{
	int deqid,i;
	struct waitqueue *p,*prev,*select,*selectprev;
	deqid=atoi(deqcmd.data);

#ifdef DEBUG
	printf("deq jid %d\n",deqid);
#endif

	/*current jodid==deqid,ÖÕÖ¹µ±Ç°×÷Òµ*/
	if (current && current->job->jid ==deqid){
		printf("teminate current job\n");
		if(current->job->curpri == 0){
            removejob(head1,current);
		}
		else if(current->job->curpri == 1){
            removejob(head2,current);
		}
		else if(current->job->curpri == 2){
            removejob(head3,current);
		}
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
		if(head3->next){
			for(prev=head3,p=head3->next;p!=NULL;prev=p,p=p->next)
				if(p->job->jid==deqid){
					select=p;
					selectprev=prev;
					break;
				}
				selectprev->next=select->next;
				select->next = NULL;
				if(select==selectprev)
					head3->next=NULL;
		}
		if(head2->next){
			for(prev=head2,p=head2->next;p!=NULL;prev=p,p=p->next)
				if(p->job->jid==deqid){
					select=p;
					selectprev=prev;
					break;
				}
				selectprev->next=select->next;
				select->next = NULL;
				if(select==selectprev)
					head2=NULL;
		}
		if(head1->next){
			for(prev=head1,p=head1->next;p!=NULL;prev=p,p=p->next)
				if(p->job->jid==deqid){
					select=p;
					selectprev=prev;
					break;
				}
				selectprev->next=select->next;
				select->next = NULL;
				if(select==selectprev)
					head1=NULL;
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
}

void do_stat(struct jobcmd statcmd)
{
	struct waitqueue *p;
	char timebuf[BUFLEN] = {'0'};
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
	if((fd=open("/tmp/mypipe",O_WRONLY))<0)
		error_sys("stat open fifo failed");
	/* ´òÓ¡ÐÅÏ¢Í·²¿ */
	printf("JOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tPRI\tSTATE\n");
	//printf("%d %d\n",current->job->pid, current->job->curpri);
	
	//printf("%d %d\n",current->job->pid, current->job->curpri);
	if(current){
		//printf("111111111\n");
		strcpy(timebuf,ctime(&(current->job->create_time)));
		//printf("111111111\n");
		timebuf[strlen(timebuf)-1]='\0';
		//printf("111111111\n");
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%d\t%s\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			
			timebuf,current->job->curpri,"RUNNING");
		if(write(fd,(current->job),sizeof(struct jobinfo))<0)
			error_sys("enq write failed");
		//printf("111111111\n");
	}
	for(p=head1->next;p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%d\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			//p->job->curpri,
			timebuf,
			p->job->curpri,
			"READY");
		if(write(fd,(p->job),sizeof(struct jobinfo))<0)
			error_sys("enq write failed");
	}
	for(p=head2->next;p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%d\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			
			timebuf, 
			p->job->curpri,
			"READY");
		if(write(fd,(p->job),sizeof(struct jobinfo))<0)
			error_sys("enq write failed");
	}
	for(p=head3->next;p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%d\t%s\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			//p->job->curpri,
			timebuf,
			p->job->curpri,
			"READY");
		if(write(fd,(p->job),sizeof(struct jobinfo))<0)
			error_sys("enq write failed");
	}
}

int main()
{
	//printf("1212212");
	struct timeval interval;
	struct itimerval new,old;
	struct stat statbuf;
	struct sigaction newact,oldact1,oldact2;
	//printf("222222");
	head1 = (struct waitqueue*)malloc(sizeof(struct waitqueue));
    head2 = (struct waitqueue*)malloc(sizeof(struct waitqueue));
    head3 = (struct waitqueue*)malloc(sizeof(struct waitqueue));
    //printf("111111");
    head1->job = (struct jobinfo*)malloc(sizeof(struct jobinfo));
    head2->job = (struct jobinfo*)malloc(sizeof(struct jobinfo));
    head3->job = (struct jobinfo*)malloc(sizeof(struct jobinfo));
    head1->job->cmdarg = NULL;
	head1->job->create_time = -1;
	head1->job->curpri = -1;
	head1->job->defpri = -1;
	head1->job->jid = -1;
	head1->job->ownerid = -1;
	head1->job->pid = -1;
	head1->job->run_time = -1;
	head1->job->wait_time = -1;
	head1->next = NULL;

	head2->job->cmdarg = NULL;
	head2->job->create_time = -1;
	head2->job->curpri = -1;
	head2->job->defpri = -1;
	head2->job->jid = -1;
	head2->job->ownerid = -1;
	head2->job->pid = -1;
	head2->job->run_time = -1;
	head2->job->wait_time = -1;
	head2->next = NULL;

	head3->job->cmdarg = NULL;
	head3->job->create_time = -1;
	head3->job->curpri = -1;
	head3->job->defpri = -1;
	head3->job->jid = -1;
	head3->job->ownerid = -1;
	head3->job->pid = -1;
	head3->job->run_time = -1;
	head3->job->wait_time = -1;
	head3->next = NULL;
	if(stat("/tmp/server",&statbuf)==0){
		/* Èç¹ûFIFOÎÄ¼þ´æÔÚ,É¾µô */
		if(remove("/tmp/server")<0)
			error_sys("remove failed");
	}
	if(stat("/tmp/mypipe",&statbuf)==0){
		if(remove("/tmp/mypipe")<0)
			error_sys("remove failed233");
	}
	if(mkfifo("/tmp/server",0666)<0)
		error_sys("mkfifo failed");
	if(mkfifo("/tmp/mypipe",0666)<0)
		error_sys("mkfifo failed");
	/* ÔÚ·Ç×èÈûÄ£Ê½ÏÂ´ò¿ªFIFO */
	if((fifo=open("/tmp/server",O_RDONLY|O_NONBLOCK))<0)
		error_sys("open fifo failed");

	if((fd=open("/tmp/mypipe",O_RDONLY|O_NONBLOCK))<0)
		error_sys("open fifo failed");
	/* ½¨Á¢ÐÅºÅ´¦Àíº¯Êý */
	newact.sa_sigaction=sig_handler;
	sigemptyset(&newact.sa_mask);
	newact.sa_flags=SA_SIGINFO;
	sigaction(SIGCHLD,&newact,&oldact1);
	sigaction(SIGALRM,&newact,&oldact2);

	/* ÉèÖÃÊ±¼ä¼ä¸ôÎª1000ºÁÃë */
	interval.tv_sec=1;
	interval.tv_usec=0;

	new.it_interval=interval;
	new.it_value=interval;
	setitimer(ITIMER_REAL,&new,&old);

	while(siginfo==1);

	close(fifo);
	close(fd);
	close(globalfd);
	return 0;
}
