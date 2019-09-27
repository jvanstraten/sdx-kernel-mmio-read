sdx-mmio: main.cpp
	g++ $^ -o $@ -I XRT/src/runtime_src/core/include -I XRT/src/runtime_src -std=c++11
