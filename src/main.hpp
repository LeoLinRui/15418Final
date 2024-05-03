#include <optional>
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
#include <functional>

#include <Fade_2D.h>
#include <boost/mpi.hpp>
#include <boost/bimap.hpp>
#include <boost/serialization/optional.hpp>

using namespace GEOM_FADE2D;
namespace mpi = boost::mpi;

struct RuntimeParameters {
    std::string inFilePath;
    std::string outFilePath;
    int numProcessors;
    // MeshGenParams meshGenParams;

    RuntimeParameters(int argc, char** argv) {
    
    }
};

class Timer {
private:
    struct TimerInfo {
        std::chrono::high_resolution_clock::time_point startTime;
        std::string name;
    };
    
    std::unordered_map<std::string, TimerInfo> timers;

public:
    void start(const std::string& name) {
        timers[name] = {std::chrono::high_resolution_clock::now(), name};
    }

    void stop(const std::string& name) {
        auto it = timers.find(name);
        if (it != timers.end()) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - it->second.startTime).count();
            std::cout << "Timer [" << it->second.name << "] Duration: " << duration << " ms" << std::endl;
            timers.erase(it);
        }
    }

    void stop(const std::string& name, const std::string& message) {
        auto it = timers.find(name);
        if (it != timers.end()) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - it->second.startTime).count();
            std::cout << "Timer [" << it->second.name << "] Duration: " << duration << " ms - " << message << std::endl;
            timers.erase(it);
        }
    }

    ~Timer() {
        // End all remaining timers with a warning message
        for (auto& pair : timers) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - pair.second.startTime).count();
            std::cout << "Timer [" << pair.second.name << "] was still running. Duration: " << duration << " ms. Timer stopped at destruction." << std::endl;
        }
    }
};

class SerializableMesh : public Fade_2D {
public:
    /*
    Serializer for boost::serialization
    */
    template<class Archive>
    void serialize(Archive & archive, const unsigned version) {
        std::string meshData;
        if (Archive::is_saving::value) {
            // Serialization
            std::ostringstream stream;

            // create save zone with all triangles
            std::vector<Triangle2*> allTriangles;
            getTrianglePointers(allTriangles);
            Zone2* saveZone = createZone(allTriangles);

            // save mesh to string
            std::vector<Zone2*> zoneVector = { saveZone };
            saveTriangulation(stream, zoneVector);
            meshData = stream.str();
        }
        archive & meshData;
        if (Archive::is_loading::value) {
            // Deserialization
            std::istringstream stream;
            stream >> meshData;
            std::vector<Zone2*> zoneVector;
            load(stream, zoneVector);
        }
    }
};

struct MeshUpdate {
    mpi::request request;
    Bbox2 targetBox;
    SerializableMesh* buffer;
};




// MESH

enum class Neighbor {
    // Neighbors on sides
    Left,
    Right,
    Top,
    Bottom,

    // Neighbors on corners
    TL,
    TR,
    BL,
    BR
};

std::vector<Triangle2*> trianglesInBbox(SerializableMesh& mesh, Bbox2 bbox) {

}

std::vector<Point2> pointsInBbox(SerializableMesh& mesh, Bbox2 bbox) {

}

struct LocalMesh {
    // Triangulation
    SerializableMesh mesh;

    // the bounding box that defines the area of the localMesh
    // the actual mesh will extend beyond this due to the overlapping needed by the algorithm
    Bbox2 bbox;

    // Neighboring meshes
    std::unordered_map<Neighbor, std::optional<size_t>> neighbors;

    // Parameters
    int maxCircumradius; // max circumradius in the entire mesh

    LocalMesh() {
        mesh = SerializableMesh();
    }

    /*
    Delete all the triangle in the provided Bbox.
    Then insert all the triangles in the provided serializedTriangulation in to mesh.
    A new Zone2 should be created, the corresponding field in the struct updated.
    */
    void updateBbox(Bbox2 bbox, SerializableMesh* incomingMesh) {
        
    }

    /*
    Refine the provided list of zones as one zone (not sequentially)
    */
    void refineBbox(Bbox2 bbox) {
        
    }

    /*
    Serializer for boost::serialization
    */
    template<class Archive>
    void serialize(Archive & archive, const unsigned version) {
        archive & mesh;
        // TODO archive & neighbors;
        archive & maxCircumradius;

        std::vector<double> bboxData;
        if (Archive::is_saving::value) {
            // Serialization
            bboxData = {bbox.get_minX(), bbox.get_minY(), bbox.get_maxX(), bbox.get_maxY()};
        }
        // TODO archive & bboxData;
        if (Archive::is_loading::value) {
            // Deserialization
            bbox.setMinX(bboxData[0]);
            bbox.setMinY(bboxData[1]);
            bbox.setMaxX(bboxData[2]);
            bbox.setMaxY(bboxData[3]);
        }
    }
};


struct GlobalMesh {
    Fade_2D mesh;

    // MeshGenParams initMeshGenParams; // params for initial sequential refinement

    GlobalMesh(RuntimeParameters params) {

    }

    /*
    Sequentially refine the entire mesh.
    */
    Fade_2D refineMesh() {

    }

    /*
    Given the number of processors, split the mesh and create a localMesh object.
    Populates all the fields (depending on if initZones is true, populate the zones)
    Return a vector of localMeshes of length nproc.
    */
    std::vector<LocalMesh> splitMesh(int nproc, bool initZones) {

    }

    /*
    Delete the current mesh and reconstruct one by combining a list of localMeshes.
    Used to combine results as the end of computation.
    */
    void loadFromLocalMeshes(std::vector<LocalMesh> localMeshes) {

    }

    /*
    Save mesh to .ply file (file name specified by runtimeParameters)
    using writePointsPLY().
    */
   void saveToPLY() {

   }
};

// TASKS

enum class Operation {
    Send,
    Receive,
    Refine
};

struct Task {
    Operation operation;
    std::optional<Neighbor> target;

    // takes a bbox that defines the local mesh's zone and the max circumradius
    // returns a box that specifies the area where the operation shall be performed
    std::function<Bbox2(Bbox2*, double)> bbox; 

    Task(Operation operation, Neighbor target, std::function<Bbox2(Bbox2*, double)> bbox) {
        this->operation = operation;
        this->target = target;
        this->bbox = bbox;
    }

    Task(Operation operation, std::function<Bbox2(Bbox2*, double)> bbox) {
        this->operation = operation;
        this->target = std::nullopt;
        this->bbox = bbox;
    }
};

struct TaskGroup {
    std::vector<Task> sendTasks;
    std::vector<Task> receiveTasks;
    std::optional<Task> refineTask;
};

std::vector<TaskGroup> initializeTaskGroups() {
    TaskGroup phaseZeroTasks;
    phaseZeroTasks.sendTasks = {
        Task(Operation::Send, Neighbor::Right, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_maxX() - 2 * r);
            bbox.setMinY(b->get_minY());
            bbox.setMaxX(b->get_maxX());
            bbox.setMaxY(b->get_maxY() - 2 * r);
            return bbox;
        }),
        Task(Operation::Send, Neighbor::BR, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_maxX() - 2 * r);
            bbox.setMinY(b->get_maxY() - 2 * r);
            bbox.setMaxX(b->get_maxX());
            bbox.setMaxY(b->get_maxY());
            return bbox;
        }),
        Task(Operation::Send, Neighbor::Bottom, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX());
            bbox.setMinY(b->get_maxY() - 2 * r);
            bbox.setMaxX(b->get_maxX() - 2 * r);
            bbox.setMaxY(b->get_maxY());
            return bbox;
        })
    };
    phaseZeroTasks.receiveTasks = {
        Task(Operation::Receive, Neighbor::Left, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - 2 * r);
            bbox.setMinY(b->get_minY());
            bbox.setMaxX(b->get_minX());
            bbox.setMaxY(b->get_maxY() - 2 * r);
            return bbox;
        }),
        Task(Operation::Receive, Neighbor::TL, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_maxX() - 2 * r);
            bbox.setMinY(b->get_minY() - 2 * r);
            bbox.setMaxX(b->get_minX());
            bbox.setMaxY(b->get_minY());
            return bbox;
        }),
        Task(Operation::Receive, Neighbor::Top, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX());
            bbox.setMinY(b->get_minY() - 2 * r);
            bbox.setMaxX(b->get_maxX() - 2 * r);
            bbox.setMaxY(b->get_minY());
            return bbox;
        })
    };
    phaseZeroTasks.refineTask = std::nullopt;

    TaskGroup phaseOneTasks;
    phaseOneTasks.sendTasks = {
        Task(Operation::Send, Neighbor::Left, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - 2 * r);
            bbox.setMinY(b->get_minY() - 2 * r);
            bbox.setMaxX(b->get_minX() + 2 * r);
            bbox.setMaxY((b->get_minY() + b->get_maxY()) / 2 + 2 * r);
            return bbox;
        })
    };
    phaseOneTasks.receiveTasks = {
        Task(Operation::Receive, Neighbor::Right, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_maxX() - 2 * r);
            bbox.setMinY(b->get_minY() - 2 * r);
            bbox.setMaxX(b->get_maxX() + 2 * r);
            bbox.setMaxY((b->get_minY() + b->get_maxY()) / 2 + 2 * r);
            return bbox;
        })
    };
    phaseOneTasks.refineTask = 
        Task(Operation::Refine, [](Bbox2* b, double r) { 
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - r);
            bbox.setMinY(b->get_minY() - r);
            bbox.setMaxX((b->get_minX() + b->get_maxX()) / 2 + r);
            bbox.setMaxY((b->get_minY() + b->get_maxY()) / 2 + r);
            return bbox;
        });

    TaskGroup phaseTwoTasks;
    phaseTwoTasks.sendTasks = {
        Task(Operation::Send, Neighbor::Top, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() + 2 * r);
            bbox.setMinY(b->get_minY() - 2 * r);
            bbox.setMaxX(b->get_maxX() + 2 * r);
            bbox.setMaxY(b->get_minY() + 2 * r);
            return bbox;
        })
    };
    phaseTwoTasks.receiveTasks = {
        Task(Operation::Receive, Neighbor::Bottom, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() + 2 * r);
            bbox.setMinY(b->get_maxY() - 2 * r);
            bbox.setMaxX(b->get_maxX() + 2 * r);
            bbox.setMaxY(b->get_maxY() + 2 * r);
            return bbox;
        })
    };
    phaseTwoTasks.refineTask =
        Task(Operation::Refine, [](Bbox2* b, double r) { 
            Bbox2 bbox = Bbox2();
            bbox.setMinX((b->get_minX() + b->get_maxX()) / 2);
            bbox.setMinY(b->get_minY() - r);
            bbox.setMaxX(b->get_maxX());
            bbox.setMaxY((b->get_minY() + b->get_maxY()) / 2 + r);
            return bbox;
        });

    TaskGroup phaseThreeTasks;
    phaseThreeTasks.sendTasks = {
        Task(Operation::Send, Neighbor::Right, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_maxX() - 2 * r);
            bbox.setMinY(b->get_minY() + 2 * r);
            bbox.setMaxX(b->get_maxX() + 2 * r);
            bbox.setMaxY(b->get_maxY() + 2 * r);
            return bbox;
        })
    };
    phaseThreeTasks.receiveTasks = {
        Task(Operation::Receive, Neighbor::Left, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - 2 * r);
            bbox.setMinY(b->get_minY() + 2 * r);
            bbox.setMaxX(b->get_minX() + 2 * r);
            bbox.setMaxY(b->get_maxY() + 2 * r);
            return bbox;
        })
    };
    phaseThreeTasks.refineTask =
        Task(Operation::Refine, [](Bbox2* b, double r) { 
            Bbox2 bbox = Bbox2();
            bbox.setMinX((b->get_minX() + b->get_maxX()) / 2 - r);
            bbox.setMinY((b->get_minY() + b->get_maxY()) / 2);
            bbox.setMaxX(b->get_maxX() + r);
            bbox.setMaxY(b->get_maxY());
            return bbox;
        });

    TaskGroup phaseFourTasks;
    phaseFourTasks.sendTasks = {
        Task(Operation::Send, Neighbor::Left, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - 2 * r);
            bbox.setMinY(b->get_minY() + 2 * r);
            bbox.setMaxX(b->get_minX());
            bbox.setMaxY(b->get_maxY());
            return bbox;
        }),
        Task(Operation::Send, Neighbor::BL, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - 2 * r);
            bbox.setMinY(b->get_maxY());
            bbox.setMaxX(b->get_minX());
            bbox.setMaxY(b->get_maxY() + 2 * r);
            return bbox;
        }),
        Task(Operation::Send, Neighbor::Bottom, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX());
            bbox.setMinY(b->get_maxY());
            bbox.setMaxX(b->get_maxX() - 2 * r);
            bbox.setMaxY(b->get_maxY() + 2 * r);
            return bbox;
        })
    };
    phaseFourTasks.receiveTasks = {
        Task(Operation::Receive, Neighbor::Right, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_maxX() - 2 * r);
            bbox.setMinY(b->get_minY() + 2 * r);
            bbox.setMaxX(b->get_maxX());
            bbox.setMaxY(b->get_maxY());
            return bbox;
        }),
        Task(Operation::Receive, Neighbor::TR, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_maxX() - 2 * r);
            bbox.setMinY(b->get_minY());
            bbox.setMaxX(b->get_maxX());
            bbox.setMaxY(b->get_minY() + 2 * r);
            return bbox;
        }),
        Task(Operation::Receive, Neighbor::Top, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX());
            bbox.setMinY(b->get_minY());
            bbox.setMaxX(b->get_maxX() - 2 * r);
            bbox.setMaxY(b->get_minY() + 2 * r);
            return bbox;
        })
    };
    phaseFourTasks.refineTask =
        Task(Operation::Refine, [](Bbox2* b, double r) { 
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX());
            bbox.setMinY((b->get_minY() + b->get_maxY()) / 2);
            bbox.setMaxX((b->get_minX() + b->get_maxX()) / 2);
            bbox.setMaxY(b->get_maxY());
            return bbox;
        });

    std::vector<TaskGroup> taskGroups = {phaseZeroTasks, phaseOneTasks, phaseTwoTasks, phaseThreeTasks, phaseFourTasks};
    return taskGroups;
}
