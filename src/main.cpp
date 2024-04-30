#include "main.h"

int main(int argc, char** argv) {
    // initialize MPI
    mpi::environment env(argc, argv);
    mpi::communicator world;

    // important variables
    LocalMesh localMesh;

    if (world.rank() == 0) {
        // load mesh file and perform initial sequential refinement
        GlobalMesh globalMesh = GlobalMesh(RuntimeParameters(argc, argv));
        globalMesh.refineMesh();

        // split globalMesh into localMeshes and send them to threads
        std::vector<LocalMesh> localMeshes = globalMesh.splitMesh(world.size(), false);

        // disseminates local meshes
        mpi::scatter(world, localMeshes, localMesh, 0);
    } else {
        // receive local mesh
        mpi::scatter(world, localMesh, 0);
    }

    return 0;
}
