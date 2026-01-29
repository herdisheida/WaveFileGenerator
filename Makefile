CC=clang
CXX=clang++
CFLAGS=-std=c++98 -Wall -Wextra -o program


build: wav_file_generation.cpp
	$(CXX) $(CFLAGS) wav_file_generation.cpp

