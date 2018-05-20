gz1:
	./create_gz 1
gz2:
	./create_gz 2
gz3:
	./create_gz 3
gz4:
	./create_gz 4
gz5:
	./create_gz 5

moss1:
	./run_moss 1
moss2:
	./run_moss 2
moss3:
	./run_moss 3
moss4:
	./run_moss 4
moss5:
	./run_moss 5

hint:
	@echo 'export LD_LIBRARY_PATH=/home/alexander/Documents/distributed-computing/libs/pa2345_starter_code/lib64'
	@echo 'LD_PRELOAD=/home/alexander/Documents/distributed-computing/libs/pa2345_starter_code/lib64'

clean:
	rm -f */a.out
	rm -f */events.log
	rm -f */pipes.log
