#include <Fade_2D.h>
#include <stdio.h>

using namespace GEOM_FADE2D;

struct RuntimeParameters {
    std::string filePath;
    int numProcessors;
    int numNodes;
    MeshGenParams meshGenParams;
    Fade_2D mesh;
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

