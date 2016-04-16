job: job.c job.h error.c enq deq stat o o1 o2 o3
	cc -o job job.c job.h error.c
enq: enq.c job.h error.c
	cc -o enq enq.c job.h error.c
deq: deq.c job.h error.c
	cc -o deq deq.c job.h error.c
stat: stat.c job.h error.c
	cc -o stat stat.c job.h error.c
o: Demo.c
	cc -o o Demo.c
o1:test.c
	cc -o o1 test.c
o2:test2.c
	cc -o o2 test2.c
o3:test3.c
	cc -o o3 test3.c
clean:
	rm job enq deq stat o o1 o2 o3 
