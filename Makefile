FLAGS = -lGL -lGLU -lglut -lIL -lILU -O3
all:
	$(CXX) $(FLAGS) -Iinclude/ -std=c++11 -g -o emu src/main.cpp

test:
	$(CXX) $(FLAGS) -Iinclude/ -std=c++11 -g -o test1 src/test.cpp

clean:
	find . | grep "~" | xargs rm -f
	find . | grep "#" | xargs rm -f
	find . | grep ".pyc" | xargs rm -f
	find . | grep ".gch" | xargs rm -f # remove precompiled headers
	rm -rf build/


