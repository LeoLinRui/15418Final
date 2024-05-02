#include "main.hpp"
#include "task.hpp"
#include "mesh.hpp"

int main(int argc, char** argv) {
    // initialize MPI
    mpi::environment env(argc, argv);
    mpi::communicator world;

    // important variables
    Timer timer;
    LocalMesh localMesh;
    std::vector<MeshUpdate> incomingUpdates;
    std::vector<MeshUpdate> outgoingUpdates;
    std::vector<TaskGroup> taskGroups = initializeTaskGroups();

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

    // loop through each taskGroup (phase)
    for (auto& taskGroup : taskGroups) {
        // post all async receives
        for (auto& task : taskGroup.receiveTasks) {
            SerializableMesh* buffer = new SerializableMesh();

            // check if that neighbor exists first (meshes on the edges has fewer neighbors)
            std::optional<size_t> requestSource = localMesh.neighbors[task.target.value()];
            if (requestSource.has_value()) {
                mpi::request request = world.irecv(requestSource.value(), 0, buffer);
                MeshUpdate update{request, &task.bbox(localMesh.bbox, localMesh.maxCircumradius), buffer};
                incomingUpdates.push_back(update);
            }
        }

        // do refinement, if any
        if (taskGroup.refineTask.has_value()) {
            localMesh.refineBbox(&taskGroup.refineTask.value().bbox(localMesh.bbox, localMesh.maxCircumradius));
        }

        // post all async sends
        for (auto& task : taskGroup.sendTasks) {
            SerializableMesh* buffer = new SerializableMesh();

            // check if that neighbor exists first (meshes on the edges has fewer neighbors)
            std::optional<size_t> requestDestination = localMesh.neighbors[task.target.value()];
            if (requestDestination.has_value()) {
                mpi::request request = world.isend(requestDestination.value(), 0, buffer);
                MeshUpdate update{request, NULL, buffer};
                outgoingUpdates.push_back(update);
            }
        }

        // wait on all updates to complete
        while (!incomingUpdates.empty() || !outgoingUpdates.empty()) {
            // Iterate through outgoing updates
            for (auto it = outgoingUpdates.begin(); it != outgoingUpdates.end();) {
                if (it->request.test()) {
                    delete it->buffer;
                    it = outgoingUpdates.erase(it);
                } else {
                    ++it;
                }
            }

            // Iterate through incoming updates
            for (auto it = incomingUpdates.begin(); it != incomingUpdates.end();) {
                if (it->request.test()) {
                    localMesh.updateBbox(it->targetBox, it->buffer);
                    delete it->buffer;
                    it = incomingUpdates.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    return 0;
}
