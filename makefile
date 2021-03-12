cc = g++
prom = run
deps = $(shell find ./ -name "*.h")
src = aco.c
src_cpp = __test.cpp utils.cpp scheduler.cpp co_io.cpp machine.cpp  task.cpp uv_timer.cpp uv_fs.cpp  mlist.cpp
obj = $(src:%.c=%.o)
obj_s = acosw.o
obj_cpp = $(src_cpp:%.cpp=%.o)

$(prom): $(obj) $(obj_cpp) $(obj_s)
	$(cc) -o $(prom) $(obj) $(obj_cpp) $(obj_s) -luv -lpthread -Wall

acosw.o: acosw.S	
	$(cc) -c acosw.S -o acosw.o

%.o: %.c $(deps)
	$(cc) -c $< -o $@

%.o: %.cpp $(deps)
	$(cc) -c $< -o $@

clean:
	rm $(obj) && rm $(obj_s) && rm $(obj_cpp)