build:
	@mpic++ -std=c++11 main.cpp -o main

run: build
	@mpirun -np 4 --oversubscribe main