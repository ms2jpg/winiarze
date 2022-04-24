build:
	@mpic++ main.cpp -o main

run: build
	@mpirun -np 5 --oversubscribe main