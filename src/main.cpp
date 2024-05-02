#include "main.hpp"

int main(int argc, char** argv) {
    // initialize MPI
    mpi::environment env(argc, argv);
    mpi::communicator world;

    // important variables
    Timer timer;
    LocalMesh localMesh;
    std::vector<MeshUpdate> incomingUpdates;
    std::vector<mpi::request> outgoingUpdates;

    // start timer for overall duration
    if (world.rank() == 0) timer.start("Total Time");

    // load and preprocess mesh sequentially, scatter localMeshes to workers
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

    // Start of Computation
    if (world.rank() == 0) timer.start("Parallel Compute Region");
    localMesh.initZones();

    // Phase 1 (top left)
    // post all async receives
    for (auto& src : recvList[PhaseTopLeft]) {
        SerializableMesh buffer;
        Zone targetZone;
        // TODO
        mpi::request request = world.irecv(localMesh.NeighborRight.value(), localMesh.neighbors[src], buffer);
        
        MeshUpdate update{request, targetZone, buffer};
        incomingUpdates.push_back(update);
    }

    localMesh.refineZones({"InnerTL", "InnerTop", "innerLeft", "MainTL"});

    // post all async sends
    for (auto& src : sendList[PhaseTopLeft]) {
        SerializableMesh buffer;
        Zone targetZone;
        // TODO
        mpi::request request = world.isend(localMesh.NeighborRight.value(), localMesh.neighbors[src], buffer);
        
        outgoingUpdates.push_back(request);
    }

    // wait on all updates to complete
    while (!incomingUpdates.empty() || !outgoingUpdates.empty()) {
        // check on outgoing sends

        // check on incoming receives
    }

    return 0;
}
