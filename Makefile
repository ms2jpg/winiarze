build:
	@mpic++ main.cpp -o main

run: build
	@mpirun -np 4 --oversubscribe main