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
#include <Fade_2D.h>
#include <mpi>

#include "main.h"

int main(int argc, char** argv) {
    // Read inputs
    //RuntimeParameters params = parseArgs(argc, argv);

    // Computation
    //const double init_time = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - init_start).count();
    //std::cout << "Initialization time (sec): " << std::fixed << std::setprecision(10) << init_time << '\n';
	//const auto compute_start = std::chrono::steady_clock::now();

    // Preprocessing
    std::cout << "Preprocessing..." << std::endl;
    // run initial sequential refinement
    //Fade_2D mesh = sequentialRefineMesh(mesh);

    // divide up the mesh (re-zone) and create a list of (zones)
    //std::vector<Zone2*> zones = createZones(mesh, UPPER_LEFT); // calculate metrics that determine grid size, create a zone for each refinement area, put them in a list
    
    int world_size;
    int pid;

    // initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);

    if (pid == 0) {
        // load mesh file and perform initial sequential refinement
        GlobalMesh globalMesh = GlobalMesh(RuntimeParameters(argc, argv));
        globalMesh.refineMesh();

        // split globalMesh into localMeshes and send them to threads
        globalMesh.splitMesh()
    }

    MPI_Finalize();
    return 0;
}
