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
private:
    const std::vector<std::string> zoneNames = {
        // Main
        "MainTL", "MainTR", "MainBL", "MainBR",
        // Inner edges
        "InnerTop", "InnerBottom", "InnerRight", "InnerLeft",
        // Outer edges
        "OuterTop", "OuterTopL", "OuterTopR",
        "OuterBottom", "OuterBottomL", "OuterBottomR",
        "OuterRight", "OuterRightT", "OuterRightB",
        "OuterLeft", "OuterLeftT", "OuterLeftB",
        // Corners
        "InnerTL", "InnerTR", "InnerBL", "InnerBR",
        "OuterTL", "OuterTR", "OuterBL", "OuterBR"
    };

    boost::bimap<std::string, size_t> nameMap;

public:
    // Triangulation
    Fade_2D mesh; // TODO use SerializableMesh

    // Zones
    std::unordered_map<std::string, Zone2*> zones;

    // TODO // Neighboring meshes
    std::unordered_map<Neighbor, std::optional<size_t>> neighbors;

    // Parameters
    int maxCircumradius; // max circumradius in the entire mesh

    LocalMesh() {
        // initialize the bimap
        size_t i = 0;
        for (auto& name : zoneNames) {
            nameMap.insert(boost::bimap<std::string, size_t>::value_type(name, i));
            i++;
        }

        // populate zones map with placeholder values
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
    void updateZone(Zone2* zone, std::string serializedTriangulation) {

    }

    /*
    Serialize the zone into a string so that it can be sent over MPI.
    */
    std::string serializeZone(Zone2* zone) {

    }

    /*
    Refine the provided list of zones as one zone (not sequentially)
    */
    void refineZones(std::vector<std::string> zone) {

    }

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

            mesh.saveTriangulation(stream, zoneVector);
            meshData = stream.str();
        }
        archive & meshData;
        if (Archive::is_loading::value) {
            // Deserialization
            std::istringstream stream;
            stream << meshData;
            std::vector<Zone2*> zoneVector;
            mesh.load(stream, zoneVector);
            
            size_t i = 0;
            for (auto& zone : zoneVector) {
                zones[zoneNames[i]] = zone;
                i++;
            }
        }

        // (De)serialize the simple fields
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


class Communicator {
public:
    int pid;
    int nproc;

    Communicator(int& argc, char**& argv) {
        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &nproc);
        MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    }

    ~Communicator() {
        MPI_Finalize();
    }

    void send(const std::vector<char>& data, int dest, int tag) const {
        MPI_Send(data.data(), data.size(), MPI_CHAR, dest, tag, MPI_COMM_WORLD);
    }

    std::vector<char> receive(int source, int tag) const {
        MPI_Status status;
        MPI_Probe(source, tag, MPI_COMM_WORLD, &status);
        
        int count;
        MPI_Get_count(&status, MPI_CHAR, &count);
        std::vector<char> buffer(count);
        
        MPI_Recv(buffer.data(), count, MPI_CHAR, source, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        return buffer;
    }

    void asyncSend(const std::vector<char>& data, int dest, int tag, MPI_Request* request) const {
        MPI_Isend(data.data(), data.size(), MPI_CHAR, dest, tag, MPI_COMM_WORLD, request);
    }

    // Asynchronous receive, returns true if started receiving
    bool asyncReceive(std::vector<char>& buffer, int source, int tag, MPI_Request* request) const {
        MPI_Status status;
        int flag;
        MPI_Iprobe(source, tag, MPI_COMM_WORLD, &flag, &status);
        
        if (flag) {
            int count;
            MPI_Get_count(&status, MPI_CHAR, &count);
            buffer.resize(count);
            MPI_Irecv(buffer.data(), count, MPI_CHAR, source, tag, MPI_COMM_WORLD, request);
            return true;
        }
        return false;
    }

    void waitFor(MPI_Request* request) const {
        MPI_Wait(request, MPI_STATUS_IGNORE);
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

enum Phase {
    TopLeft,
    TopRight,
    BottomRight,
    BottomLeft
};

enum class Neighbor {
    Left,
    Right,
    Top,
    Bottom,

    TL,
    TR,
    BL,
    BR
};

// TODO
enum class Zone {
    // Main Zones
    MainTL, MainTR, MainBL, MainBR,
    
    // Inner Edges
    InnerTop, InnerBottom, InnerRight, InnerLeft,
    
    // Outer Edges
    OuterTop, OuterTopL, OuterTopR,
    OuterBottom, OuterBottomL, OuterBottomR,
    OuterRight, OuterRightT, OuterRightB,
    OuterLeft, OuterLeftT, OuterLeftB,
    
    // Corners
    InnerTL, InnerTR, InnerBL, InnerBR,
    OuterTL, OuterTR, OuterBL, OuterBR
};

struct MeshUpdate {
    mpi::request request;
    Zone targetZone;
    SerializableMesh buffer;
};

// TODO
std::unordered_map<Phase, std::vector<Zone>> sendList = 
{
    {
        PhaseTopLeft, 
    {
        "InnerTL", "OuterTL" "InnerLeft", "OuterLeft", "OuterLeftTop", "OuterLeftBottom"
    }
    }
};

std::unordered_map<Phase, std::vector<std::string>> recvList;
