#include <Fade_2D.h>
#include <stdio.h>
#include <optional>

using namespace GEOM_FADE2D;


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
    Fade_2D mesh;

    // Zones - Main
    Zone2* MainTL;
    Zone2* MainTR;
    Zone2* MainBL;
    Zone2* MainBR;

    // Zones - Inner Edges
    Zone2* InnerTop;
    Zone2* InnerBottom;
    Zone2* InnerRight;
    Zone2* InnerLeft;

    // Zones - Outer Edges
    Zone2* OuterTop;
    Zone2* OuterTopL;
    Zone2* OuterTopR;

    Zone2* OuterBottom;
    Zone2* OuterBottomL;
    Zone2* OuterBottomR;

    Zone2* OuterRight;
    Zone2* OuterRightT;
    Zone2* OuterRightB;

    Zone2* OuterLeft;
    Zone2* OuterLeftT;
    Zone2* OuterLeftB;

    // Zones - Corners
    Zone2* InnerTL;
    Zone2* InnerTR;
    Zone2* InnerBL;
    Zone2* InnerBR; 

    Zone2* OuterTL;
    Zone2* OuterTR;
    Zone2* OuterBL;
    Zone2* OuterBR; 

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

    /*
    Creates a local mesh struct with a mesh and the max circumradius.
    Likely called on the main thread.
    */
    LocalMesh(Fade_2D mesh, int r) {

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


enum Phase {
    UPPER_LEFT,
    UPPER_RIGHT,
    BOTTOM_RIGHT,
    BOTTOM_LEFT
};

