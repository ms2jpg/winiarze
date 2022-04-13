build:
	@mpic++ main.cpp -o main

run: build
	@mpirun -np 8 --oversubscribe main