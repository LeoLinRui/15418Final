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

public:
    // Triangulation
    Fade_2D mesh;

    // Zones
    std::unordered_map<std::string, Zone2*> zones;

    // Neighboring meshes
    std::optional<int> NeighborLeft;
    std::optional<int> NeighborRight;
    std::optional<int> NeighborTop;
    std::optional<int> NeighborBottom;

    std::optional<int> NeighborTL;
    std::optional<int> NeighborTR;
    std::optional<int> NeighborBL;
    std::optional<int> NeighborBR;

    // Parameters
    int maxCircumradius; // max circumradius in the entire mesh

    LocalMesh() {
        // populate zones map with placeholder values
    }
    
    /*
    Creates a local mesh struct with a mesh and the max circumradius.
    Likely called on the main thread.
    */
    LocalMesh(Fade_2D mesh, int r) {
        // populate zones map with placeholder values
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
    void refineZones(std::vector<Zone2*> zone) {

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
        archive & NeighborLeft;
        archive & NeighborRight;
        archive & NeighborTop;
        archive & NeighborBottom;

        archive & NeighborTL;
        archive & NeighborTR;
        archive & NeighborBL;
        archive & NeighborBR;

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


enum Phase {
    UPPER_LEFT,
    UPPER_RIGHT,
    BOTTOM_RIGHT,
    BOTTOM_LEFT
};

