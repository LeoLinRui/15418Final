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

#include "main.h"

int main(int argc, char** argv) {
    // Read inputs
    typedef CGAL::Simple_cartesian<double> Kernel;
    typedef Kernel::Point_2 Point_2;

    Point_2 p(1, 1), q(10, 10);
    std::cout << "Distance from " << p << " to " << q << " is " << sqrt(CGAL::squared_distance(p, q)) << std::endl;
    return 0;

    // Computation
    const double init_time = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - init_start).count();
    std::cout << "Initialization time (sec): " << std::fixed << std::setprecision(10) << init_time << '\n';
	const auto compute_start = std::chrono::steady_clock::now();

    // Preprocessing
    std::cout << "Preprocessing..." << std::endl;
    // run initial sequential refinement

    // divide up the mesh

    #pragma omp parallel for

    {

    }

    // receive local mesh from process 0

    // send right edge to process on the right

    // send bottom edge to process below

    // send bottom right corner to process on the lower diagonal

    // receive 

    // Finalize the MPI environment.
    MPI_Finalize();
}
