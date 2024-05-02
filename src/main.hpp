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
    std::string filePath;
    int numProcessors;
    int numNodes;
    // MeshGenParams meshGenParams;

    RuntimeParameters(int argc, char** argv) {
    
    }
};


struct LocalMesh {
    // Triangulation
    SerializableMesh mesh; // TODO use SerializableMesh

    // the bounding box that defines the area of the localMesh
    // the actual mesh will extend beyond this due to the overlapping needed by the algorithm
    Bbox2* bbox;

    // TODO // Neighboring meshes
    std::unordered_map<Neighbor, std::optional<size_t>> neighbors;

    // Parameters
    int maxCircumradius; // max circumradius in the entire mesh

    LocalMesh() {
        
    }
    
    /*
    Creates a local mesh struct with a mesh and the max circumradius.
    Likely called on the main thread.
    */
    LocalMesh(Fade_2D mesh, int r) {
        LocalMesh localMesh;
    }

    /*
    Based on the provided maximum circumradius, 
    calculates the bounds of each zone
    and use it to place each triangle of mesh into a zone.
    Populates all the Zone2* fields of this struct.

    Likely called on the worker thread.
    */
    void initZones() {
        
    }

    /*
    Delete all the triangle in the provided zone.
    Then insert all the triangles in the provided serializedTriangulation in to mesh.
    A new Zone2 should be created, the corresponding field in the struct updated.
    */
    void updateBbox(Bbox2* bbox, SerializableMesh* incomingMesh) {
        
    }

    /*
    Refine the provided list of zones as one zone (not sequentially)
    */
    void refineBbox(Bbox2* bbox) {
        
    }

    /*
    Serializer for boost::serialization
    */
    template<class Archive>
    void serialize(Archive & archive, const unsigned version) {
        archive & mesh;
        archive & neighbors;
        archive & maxCircumradius;
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
    /*
    Serializer for boost::serialization
    */
    template<class Archive>
    void serialize(Archive & archive, const unsigned version) {
        std::string meshData;
        if (Archive::is_saving::value) {
            // Serialization
            std::ostringstream stream;
            std::vector<Zone2*> zoneVector;
            for (auto& name : zoneNames) {
                zoneVector.push_back(zones[name]);
            }

            saveTriangulation(stream, zoneVector);
            meshData = stream.str();
        }
        archive & meshData;
        if (Archive::is_loading::value) {
            // Deserialization
            std::istringstream stream;
            stream << meshData;
            std::vector<Zone2*> zoneVector;
            load(stream, zoneVector);
            
            size_t i = 0;
            for (auto& zone : zoneVector) {
                zones[zoneNames[i]] = zone;
                i++;
            }
        }
    }
};

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

struct MeshUpdate {
    mpi::request request;
    Bbox2* targetBox;
    SerializableMesh* buffer;
};


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

