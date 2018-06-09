gz1:
	./create_gz 1 2>/dev/null
gz2:
	./create_gz 2 2>/dev/null
gz3:
	./create_gz 3 2>/dev/null
gz4:
	./create_gz 4 2>/dev/null
gz5:
	./create_gz 5 2>/dev/null

moss1:
	./create_gz 1 2>/dev/null
	./run_moss 1 | awk 'NR==0; END{print}' | xargs ./percent_checker.py | xargs echo
moss2:
	./create_gz 2 2>/dev/null
	./run_moss 2 | awk 'NR==0; END{print}' | xargs ./percent_checker.py | xargs echo
moss3:
	./create_gz 3 2>/dev/null
	./run_moss 3 | awk 'NR==0; END{print}' | xargs ./percent_checker.py | xargs echo
moss4:
	./create_gz 4 2>/dev/null
	./run_moss 4 | awk 'NR==0; END{print}' | xargs ./percent_checker.py | xargs echo
moss5:
	./create_gz 5 2>/dev/null
	./run_moss 5 | awk 'NR==0; END{print}' | xargs ./percent_checker.py | xargs echo

hint:
	@echo 'export LD_LIBRARY_PATH=/home/alexander/Documents/distributed-computing/libs/pa2345_starter_code/lib64'
	@echo 'LD_PRELOAD=/home/alexander/Documents/distributed-computing/libs/pa2345_starter_code/lib64'

clean:
	rm -f */a.out
	rm -f */events.log
	rm -f */pipes.log
