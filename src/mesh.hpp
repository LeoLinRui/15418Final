#include "main.hpp"

std::vector<Triangle2*> trianglesInBbox(SerializableMesh& mesh, Bbox2& bbox) {

}

struct LocalMesh {
    // Triangulation
    SerializableMesh mesh;

    // the bounding box that defines the area of the localMesh
    // the actual mesh will extend beyond this due to the overlapping needed by the algorithm
    Bbox2* bbox;

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

        std::vector<double> bboxData;
        if (Archive::is_saving::value) {
            // Serialization
            bboxData = {bbox->get_minX(), bbox->get_minY(), bbox->get_maxX(), bbox->get_maxY()};
        }
        archive & bboxData;
        if (Archive::is_loading::value) {
            // Deserialization
            bbox->setMinX(bboxData[0]);
            bbox->setMinY(bboxData[1]);
            bbox->setMaxX(bboxData[2]);
            bbox->setMaxY(bboxData[3]);
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

