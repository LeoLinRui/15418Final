#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <string>
#include <vector>
#include <random>
#include <thread>
#include <cassert>
#include <CGAL/Simple_cartesian.h>

#include <mpi.h>

#include "main.h"

int main(int argc, char** argv) {
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int pid;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    // Print a hello world message
    printf("Hello world from processor %s, rank %d out of %d processors\n",
           processor_name, pid, world_size);

    // Initialization
    if (pid == 0) {
        typedef CGAL::Simple_cartesian<double> Kernel;
        typedef Kernel::Point_2 Point_2;

        Point_2 p(1, 1), q(10, 10);
        std::cout << "Distance from " << p << " to " << q << " is " << sqrt(CGAL::squared_distance(p, q)) << std::endl;
        return 0;
    }

    // Computation
    if (pid == 0) {
		const double init_time = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - init_start).count();
		std::cout << "Initialization time (sec): " << std::fixed << std::setprecision(10) << init_time << '\n';
	}
	const auto compute_start = std::chrono::steady_clock::now();

    // Finalize the MPI environment.
    MPI_Finalize();
}
