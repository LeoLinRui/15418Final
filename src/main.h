#include <Fade_2D.h>
#include <stdio.h>
#include <optional>

using namespace GEOM_FADE2D;

struct RuntimeParameters {
    std::string filePath;
    int numProcessors;
    int numNodes;
    MeshGenParams meshGenParams;
    Fade_2D mesh;
};

struct LocalMesh {
    // Triangulation
    Fade_2D mesh;

    // Zones - Main
    Zone2* Main_TL;
    Zone2* Main_TR;
    Zone2* Main_BL;
    Zone2* Main_BR;

    // Zones - Inner Edges
    Zone2* Inner_Top;
    Zone2* Inner_Bottom;
    Zone2* Inner_Right;
    Zone2* Inner_Left;

    // Zones - Outer Edges
    Zone2* Outer_Top;
    Zone2* Outer_Top_L;
    Zone2* Outer_Top_R;

    Zone2* Outer_Bottom;
    Zone2* Outer_Bottom_L;
    Zone2* Outer_Bottom_R;

    Zone2* Outer_Right;
    Zone2* Outer_Right_T;
    Zone2* Outer_Right_B;

    Zone2* Outer_Left;
    Zone2* Outer_Left_T;
    Zone2* Outer_Left_B;

    // Zones - Corners
    Zone2* Inner_TL;
    Zone2* Inner_TR;
    Zone2* Inner_BL;
    Zone2* Inner_BR; 

    Zone2* Outer_TL;
    Zone2* Outer_TR;
    Zone2* Outer_BL;
    Zone2* Outer_BR; 

    // Neighboring meshes
    std::optional<int> Neighbor_Left;
    std::optional<int> Neighbor_Right;
    std::optional<int> Neighbor_Top;
    std::optional<int> Neighbor_Bottom;

    std::optional<int> Neighbor_TL;
    std::optional<int> Neighbor_TR;
    std::optional<int> Neighbor_BL;
    std::optional<int> Neighbor_BR;

};

enum Phase {
    UPPER_LEFT,
    UPPER_RIGHT,
    BOTTOM_RIGHT,
    BOTTOM_LEFT
};

RuntimeParameters parseArgs(int argc, char** argv) {
    return RuntimeParameters();
}

Fade_2D sequentialRefineMesh(Fade_2D mesh) {

}

std::vector<Zone2*> createZones(Fade_2D mesh) {

}

void refineZone(Zone2* zone) {

}

