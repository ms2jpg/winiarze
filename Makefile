build:
	@mpic++ -std=c++11 main.cpp -o main

run: build
	@mpirun -np 8 --oversubscribe main