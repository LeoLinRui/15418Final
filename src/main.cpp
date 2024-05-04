#include "main.hpp"
#include <thread>

int main(int argc, char** argv) {
    // initialize MPI
    mpi::environment env(argc, argv);
    mpi::communicator world;

    // important variables
    Timer timer;
    LocalMesh localMesh;
    localMesh.mesh.initMesh();
    RuntimeParameters runtimeParameters(argc, argv);
    std::vector<MeshUpdate> incomingUpdates;
    std::vector<MeshUpdate> outgoingUpdates;
    std::vector<TaskGroup> taskGroups = initializeTaskGroups();

    // start timer for overall duration
    if (world.rank() == 0) timer.start("Total Time");

    // load and preprocess mesh sequentially, scatter localMeshes to workers
    if (world.rank() == 0) {
        // load mesh file and perform initial sequential refinement
        timer.start("Loading Input");
        GlobalMesh globalMesh = GlobalMesh(runtimeParameters);
        globalMesh.loadFromRandom();
        timer.stop("Loading Input");

        timer.start("Global Sequential Refine");
        // globalMesh.refineMesh();
        timer.stop("Global Sequential Refine");

        // split globalMesh into localMeshes and send them to threads
        std::vector<LocalMesh> localMeshes = globalMesh.splitMesh(world.size(), false);

        // disseminates local meshes
        timer.start("Scatter Mesh");
        mpi::scatter(world, localMeshes, localMesh, 0);
        timer.stop("Scatter Mesh");

        // sleep fpr 3 s
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // free local meshes
        int i = 0;
        for (auto& mesh : localMeshes) {
            // if (i != 0) mesh.mesh.freeMesh();
            i++;
        }
        std::cout << "Local meshes freed on main thread" << std::endl;
    } else {
        // receive local mesh
        mpi::scatter(world, localMesh, 0);
    }

    // Start of Computation
    if (world.rank() == 0) timer.start("Parallel Compute Region");

    // loop through each taskGroup (phase)
    for (auto& taskGroup : taskGroups) {
        std::cout << "[Thread " << world.rank() << "] Starting task group" << std::endl;
        // post all async receives
        for (auto& task : taskGroup.receiveTasks) {
            // check if that neighbor exists first (meshes on the edges has fewer neighbors)
            assert(task.target.has_value() && "Tasks on the receive list must contain target");
            std::optional<size_t> requestSource = localMesh.neighbors[task.target.value()];
            if (requestSource.has_value()) {
                SerializableMesh buffer;
                buffer.initMesh();
                mpi::request request = world.irecv(requestSource.value(), 0, buffer);
                MeshUpdate update{request, task.bbox(&localMesh.bbox, localMesh.maxCircumradius), buffer};
                incomingUpdates.push_back(update);
            }
        }

        // do refinement, if any
        if (taskGroup.refineTask.has_value()) {
            std::cout << "[Thread " << world.rank() << "] Starting local refinement" << std::endl;
            localMesh.refineBbox(taskGroup.refineTask.value().bbox(&localMesh.bbox, localMesh.maxCircumradius));
            std::cout << "[Thread " << world.rank() << "] Local refinement complete" << std::endl;
        }

        // sleep fpr 3 s
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // post all async sends
        for (auto& task : taskGroup.sendTasks) {
            // check if that neighbor exists first (meshes on the edges has fewer neighbors)
            std::optional<size_t> requestDestination = localMesh.neighbors[task.target.value()];
            if (requestDestination.has_value()) {
                printf("[Thread %d] Posting send request to %d\n", world.rank(), requestDestination.value());
                // populate send buffer with points to send
                Bbox2 sendBbox = task.bbox(&localMesh.bbox, localMesh.maxCircumradius);
                SerializableMesh buffer;
                buffer.initMesh();
                std::vector<Point2*> pointsToSend = pointsInBbox(localMesh.mesh.getMesh(), sendBbox);
                for (auto& point : pointsToSend) {
                    buffer.getMesh()->insert(*point);
                }

                printf("[Thread %d] Sending %d points to %d\n", world.rank(), buffer.getMesh()->numberOfPoints(), requestDestination.value());

                mpi::request request = world.isend(requestDestination.value(), 0, buffer);
                MeshUpdate update{request, sendBbox, buffer};
                outgoingUpdates.push_back(update);
            }
        }
        std::cout << "[Thread " << world.rank() << "] has posted all send requests" << std::endl;

        // wait on all updates to complete
        while (!incomingUpdates.empty() || !outgoingUpdates.empty()) {
            // Iterate through outgoing updates
            for (auto it = outgoingUpdates.begin(); it != outgoingUpdates.end();) {
                if (it->request.test()) {
                    //it->buffer.freeMesh();
                    it = outgoingUpdates.erase(it);
                } else {
                    ++it;
                }
            }

            // Iterate through incoming updates
            for (auto it = incomingUpdates.begin(); it != incomingUpdates.end();) {
                if (it->request.test()) {
                    localMesh.updateBbox(it->targetBox, it->buffer);
                    //it->buffer.freeMesh();
                    it = incomingUpdates.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    // End of parallel compute
    if (world.rank() == 0) {
        timer.stop("Parallel Compute Region");

        std::vector<LocalMesh> localMeshes;
        mpi::gather(world, localMesh, localMeshes, 0);

        GlobalMesh outputMesh(runtimeParameters);
        outputMesh.loadFromLocalMeshes(localMeshes);
        outputMesh.saveToPLY();

        timer.stop("Total Time");
    } else {
        // worker send local mesh
        mpi::gather(world, localMesh, 0);
    }

    return 0;
}
