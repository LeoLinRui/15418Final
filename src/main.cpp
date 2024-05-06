#include "main.hpp"
#include <thread>

int main(int argc, char** argv) {
    // initialize MPI
    mpi::environment env(argc, argv);
    mpi::communicator world;

    // important variables
    Timer timer;
    LocalMesh localMesh;
    RuntimeParameters runtimeParameters(argc, argv);
    GlobalMesh globalMesh = GlobalMesh(runtimeParameters);
    std::vector<MeshUpdate> incomingUpdates;
    std::vector<MeshUpdate> outgoingUpdates;
    std::vector<TaskGroup> taskGroups = initializeTaskGroups();

    setGlobalNumCPU(2);

    // print PID with rank
    //std::cout << "Rank " << world.rank() << " PID " << getpid() << std::endl;

    // start timer for overall duration
    if (world.rank() == 0) timer.start("Total Time");

    // load and preprocess mesh sequentially, scatter localMeshes to workers
    if (world.rank() == 0) {
        // load mesh file and perform initial sequential refinement
        std::cout << "main process using " << setGlobalNumCPU(runtimeParameters.numProcessors * 2) <<" threads" << std::endl;
        timer.start("Loading Input");
        globalMesh.loadFromRandom();
        timer.stop("Loading Input");

        timer.start("Global Sequential Refine");
        globalMesh.roughRefineMesh();
        timer.stop("Global Sequential Refine");

        setGlobalNumCPU(2);

        // split globalMesh into localMeshes and send them to threads
        std::vector<LocalMesh> localMeshes = globalMesh.splitMesh(world.size(), false);
        std::cout << "max circumradius: " << localMeshes[0].maxCircumradius << std::endl;

        // disseminates local meshes
        timer.start("Scatter Mesh");
        mpi::scatter(world, localMeshes, localMesh, 0);
        timer.stop("Scatter Mesh");
    } else {
        // receive local mesh
        mpi::scatter(world, localMesh, 0);
    }

    // Start of Computation
    if (world.rank() == 0) timer.start("Parallel Compute Region");

    // print out rank with local mesh bbox
    //std::cout << "[Thread " << world.rank() << "] Local mesh bbox: " << localMesh.bbox << std::endl;

    // loop through each taskGroup (phase)
    int taskGroupIndex = 0;
    for (auto& taskGroup : taskGroups) {
        if (taskGroupIndex == -1) { // early stop to see intermediate results
            taskGroupIndex++;
            break;
        }

        // post all async receives
        for (auto& task : taskGroup.receiveTasks) {
            // check if that neighbor exists first (meshes on the edges has fewer neighbors)
            //assert(task.target.has_value() && "Tasks on the receive list must contain target");
            std::optional<size_t> requestSource = localMesh.neighbors[task.target.value()];
            if (requestSource.has_value()) {
                std::shared_ptr<SerializableMesh> buffer = std::make_shared<SerializableMesh>();
                mpi::request request = world.irecv(requestSource.value(), 0, *buffer);
                MeshUpdate update{request, task.bbox(&localMesh.bbox, localMesh.maxCircumradius), buffer};
                incomingUpdates.push_back(update);
            }
        }

        // do refinement, if any
        if (taskGroup.refineTask.has_value()) {
            //std::cout << "[Thread " << world.rank() << "] Starting local refinement with " << 
            //    localMesh.mesh.getMesh()->numberOfTriangles() << " triangles" << std::endl;
            localMesh.refineBbox(taskGroup.refineTask.value().bbox(&localMesh.bbox, localMesh.maxCircumradius));
            //std::cout << "[Thread " << world.rank() << "] Local refinement complete, it now has " <<
            //    localMesh.mesh.getMesh()->numberOfTriangles() << " triangles" << std::endl;
        }

        // post all async sends
        for (auto& task : taskGroup.sendTasks) {
            // check if that neighbor exists first (meshes on the edges has fewer neighbors)
            std::optional<size_t> requestDestination = localMesh.neighbors[task.target.value()];
            if (requestDestination.has_value()) {
                // populate send buffer with points to send
                Bbox2 sendBbox = task.bbox(&localMesh.bbox, localMesh.maxCircumradius);
                std::shared_ptr<SerializableMesh> buffer = std::make_shared<SerializableMesh>();
                std::vector<Point2*> pointsToSend = pointsInBbox(localMesh.mesh.getMesh(), sendBbox);
                for (auto& point : pointsToSend) {
                    buffer->getMesh()->insert(*point);
                }

                //std::cout << "[Thread " << world.rank() << "] sending " << buffer->getMesh()->numberOfPoints() 
                //    << " points in " << sendBbox << " to " << requestDestination.value() << std::endl;

                mpi::request request = world.isend(requestDestination.value(), 0, *buffer);
                MeshUpdate update{request, sendBbox, buffer};
                outgoingUpdates.push_back(update);
            }
        }

        // wait on all updates to complete
        while (!incomingUpdates.empty() || !outgoingUpdates.empty()) {
            // Iterate through outgoing updates
            for (auto it = outgoingUpdates.begin(); it != outgoingUpdates.end();) {
                if (it->request.test()) {
                    it = outgoingUpdates.erase(it);
                } else {
                    ++it;
                }
            }

            // Iterate through incoming updates
            for (auto it = incomingUpdates.begin(); it != incomingUpdates.end();) {
                if (it->request.test()) {
                    /*
                    std::cout << "[Thread " << world.rank() << "] Received " << 
                        it->buffer->getMesh()->numberOfPoints() << " points for box " << 
                        it->targetBox << ". localMesh bbox is " << localMesh.bbox << std::endl;
                    */
                    localMesh.updateBbox(it->targetBox, *it->buffer);
                    it = incomingUpdates.erase(it);
                } else {
                    ++it;
                }
            }
        }
        taskGroupIndex++;
    }

    // End of parallel compute
    if (world.rank() == 0) {
        timer.stop("Parallel Compute Region");

        timer.start("Gather Result Mesh");
        std::vector<std::vector<std::pair<double, double>>> resultPoints;
        mpi::gather(world, localMesh.getSerializablePoints(), resultPoints, 0);
        timer.stop("Gather Result Mesh");

        timer.start("Combine Result Mesh");
        std::cout << "main process using " << setGlobalNumCPU(runtimeParameters.numProcessors * 2) <<" threads" << std::endl;
        globalMesh.loadFromLocalMeshes(resultPoints);
        timer.stop("Combine Result Mesh");

        /*
        globalMesh.initializeVisualizer();
        globalMesh.visualizePoints();
        globalMesh.visualizeTriangles();
        globalMesh.saveVisualization();
        */

        timer.stop("Total Time");
    } else {
        // worker send local mesh
        mpi::gather(world, localMesh.getSerializablePoints(), 0);
    }

    return 0;
}
