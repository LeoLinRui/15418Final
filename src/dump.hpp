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
    TopLeft,
    TopRight,
    BottomRight,
    BottomLeft
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
        std::vector<Triangle2*> triangles;
        mesh.getTrianglePointers(triangles);

        std::vector<Bbox2*> bboxes;
        std::vector<Zone> zoneDescriptors;
        for (auto& zone : zoneDescriptors) {
            // TODO: calculate the coordinates for each zone

            // make Bbox
            Bbox2 bbox;
            bbox.setMaxX(.0);
            bbox.setMinX(.0);
            bbox.setMaxY(.0);
            bbox.setMinY(.0);

            bboxes.push_back(&bbox);
        }

        // go through the triangle list and populate zones
        std::vector<std::vector<Triangle2*>> triangleLists;
        for (auto& triangle : triangles) {
            Point2 center = triangle->getBarycenter();
            for (auto& bbox : bboxes) {
                if (bbox->isInBox(center)) {
                    // TODO: add to zone
                    triangleLists[0].push_back(triangle);
                }
            }
        }

        // create zones
        std::vector<Zone2*> zones;
        for (auto& list : triangleLists) {
            zones.push_back(mesh.createZone(list, true));
        }
    }

    /*
    Delete all the triangle in the provided zone.
    Then insert all the triangles in the provided serializedTriangulation in to mesh.
    A new Zone2 should be created, the corresponding field in the struct updated.
    */
    void updateZone(Zone2* zone, SerializableMesh* incomingMesh) {
        // remove the old zone
        std::vector<Point2*> oldVertices;
        zone->getVertices(oldVertices);
        mesh.remove(oldVertices);
        mesh.deleteZone(zone);

        // add the new zone
        std::vector<Point2*> newVertices;
        incomingMesh->getVertexPointers(newVertices);
        for (auto& vertexPointer : newVertices) {
            mesh.insert(*vertexPointer);
        }
    }

    /*
    Refine the provided list of zones as one zone (not sequentially)
    */
    void refineZones(std::vector<Zone2*> zones) {
        // union all the zones into combinedZones
        Zone2* combinedZone = zones.back();
        zones.pop_back();
        for (auto& zone : zones) {
            combinedZone = zoneUnion(combinedZone, zone);
        }

        // refine the mesh in the zone
        mesh.refine(combinedZone, 20, 1, 10, true);
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

