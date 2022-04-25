outputName := main
n = 5


build:
	@mpic++ ${wildcard *.cpp} -o ${outputName}

run: build
	@echo Running ${outputName} with ${n} processes
	@mpirun -np ${n} --oversubscribe ${args} ${outputName}
clean:
	rm -f ${outputName}

