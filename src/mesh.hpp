#include <optional>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include <functional>

#include <Fade_2D.h>
#include <boost/mpi.hpp>
#include <boost/bimap.hpp>
#include <boost/serialization/optional.hpp>

using namespace GEOM_FADE2D;

#include <main.hpp>

std::vector<Triangle2 *> trianglesInBbox(SerializableMesh &mesh, Bbox2 &bbox)
{
}

struct LocalMesh
{
    // Triangulation
    SerializableMesh mesh;

    // the bounding box that defines the area of the localMesh
    // the actual mesh will extend beyond this due to the overlapping needed by the algorithm
    Bbox2 *bbox;

    // Neighboring meshes
    std::unordered_map<Neighbor, std::optional<size_t>> neighbors;

    // Parameters
    int maxCircumradius; // max circumradius in the entire mesh

    LocalMesh()
    {
        mesh = SerializableMesh();
    }

    /*
    Delete all the triangle in the provided Bbox.
    Then insert all the triangles in the provided serializedTriangulation in to mesh.
    A new Zone2 should be created, the corresponding field in the struct updated.
    */
    void updateBbox(Bbox2 *bbox, SerializableMesh *incomingMesh)
    {
        std::vector<Point2 *> vertices = mesh->getVertexPointers();
        for (Point2 vertex : vertices)
        {
            if (bbox->isInBox(vertex))
            {
                // remove
                mesh.remove(vertex);
            }
        }
        std::vector<Point2 *> vertices_in = incomingMesh->getVertexPointers();
        for (Point2 vertex : vertices_in)
        {
            mesh.insert(vertices_in)
        }
    }

    /*
    Refine the provided list of zones as one zone (not sequentially)
    */
    void refineBbox(Bbox2 *bbox)
    { // currently redoing
        Bbox2 unioned = Bbox2();
        for (Bbox2 box : bbox)
        {
            unioned = unioned.add(box)
        }
    }

    /*
    Serializer for boost::serialization
    */
    template <class Archive>
    void serialize(Archive &archive, const unsigned version)
    {
        archive & mesh;
        archive & neighbors;
        archive & maxCircumradius;
    }
};

struct GlobalMesh
{
    Fade_2D mesh;

    MeshGenParams initMeshGenParams; // params for initial sequential refinement
    std::string inFilePath, outFilePath;
    int numProcessors;

    GlobalMesh(RuntimeParameters params)
    {
        inFilePath = params.inFilePath;
        outFilePath = params.outFilePath;
        numProcessors = params.numProcessors;
    }

    /*
    Sequentially refine the entire mesh
    */
    Fade_2D refineMesh()
    {
        mesh.refine(); // TODO
    }

    /*
    Given the number of processors, split the mesh and create a localMesh object.
    Populates all the fields (depending on if initZones is true, populate the zones)
    Return a vector of localMeshes of length nproc.
    */
    std::vector<LocalMesh> splitMesh(int nproc, bool initZones)
    {
        std::vector<LocalMesh *> meshes(nproc); // not done
    }

    /*
    Delete the current mesh and reconstruct one by combining a list of localMeshes.
    Used to combine results as the end of computation.
    */
    void loadFromLocalMeshes(std::vector<LocalMesh> localMeshes)
    {
        SerializableMesh recombine;
        for (LocalMesh local : localMeshes)
        {
            recombine
        }
    }

    /*
    Save mesh to .ply file (file name specified by runtimeParameters)
    using writePointsPLY().
    */
    void saveToPLY()
    {
        writePointsPLY(inFilePath, mesh, outFilePath)
    }
};
