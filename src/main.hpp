#include <optional>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <string>
#include <vector>
#include <random>
#include <cassert>
#include <functional>

#include <Fade_2D.h>
#include <boost/mpi.hpp>
#include <boost/program_options.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

using namespace GEOM_FADE2D;
namespace mpi = boost::mpi;
namespace po = boost::program_options;

struct RuntimeParameters {
    std::string inFilePath;
    std::string outFilePath;
    size_t numRandomPoints;
    int numProcessors;
    double minAngle;
    double minEdgeLength;
    double maxEdgeLength;

    double MIN_ANGLE = 25.0;
    double MIN_EDGE_LENGTH = 0.0;
    double MAX_EDGE_LENGTH = 10.0;

    RuntimeParameters() : numProcessors(1), minAngle(MIN_ANGLE), minEdgeLength(MIN_EDGE_LENGTH), maxEdgeLength(MAX_EDGE_LENGTH) {
    }

    RuntimeParameters(int argc, char** argv) {
        parseCommandLine(argc, argv);
    }

    void parseCommandLine(int argc, char** argv) {
        // Declare the supported options.
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            //("input,i", po::value<std::string>(&inFilePath)->required(), "input file path")
            //("output,o", po::value<std::string>(&outFilePath)->required(), "output file path")
            ("num-random-points,n", po::value<size_t>(&numRandomPoints)->default_value(10000), "number of randomly generated points")
            ("processors,p", po::value<int>(&numProcessors)->default_value(1), "number of processors")
            ("min-angle,a", po::value<double>(&minAngle)->default_value(MIN_ANGLE), "minimum angle")
            ("min-edge-length,m", po::value<double>(&minEdgeLength)->default_value(MIN_EDGE_LENGTH), "minimum edge length")
            ("max-edge-length,M", po::value<double>(&maxEdgeLength)->default_value(MAX_EDGE_LENGTH), "maximum edge length");

        po::variables_map vm;
        try {
            po::store(po::parse_command_line(argc, argv, desc), vm);

            if (vm.count("help")) {
                std::cout << desc << "\n";
                return;
            }

            po::notify(vm);
        } catch (po::error& e) {
            std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
            std::cerr << desc << std::endl;
            exit(EXIT_FAILURE);
        }
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
        std::cout << "Timer [" << name << "] started" << std::endl;
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

struct SerializableMesh {
private:
    std::shared_ptr<Fade_2D> mesh;

public:
    SerializableMesh() : mesh(std::make_shared<Fade_2D>()) {}

    std::shared_ptr<Fade_2D> getMesh() const {
        if (!mesh) {
            throw std::runtime_error("Mesh not initialized.");
        }
        return mesh;
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

            // create save zone with all triangles
            std::vector<Triangle2*> allTriangles;
            getMesh()->getTrianglePointers(allTriangles);
            Zone2* saveZone = mesh->createZone(allTriangles);

            // save mesh to string
            std::vector<Zone2*> zoneVector = { saveZone };
            getMesh()->saveTriangulation(stream, zoneVector);
            meshData = stream.str();
        }
        archive & meshData;
        if (Archive::is_loading::value) {
            // Deserialization
            if (getMesh()->numberOfTriangles() != 0 || getMesh()->numberOfPoints() != 0) {
                std::cout << "Mesh has " << getMesh()->numberOfPoints() << " points and " 
                    << getMesh()->numberOfTriangles() << " triangles\n";
                throw std::runtime_error("Deserializing into a non-empty mesh");
            }
            std::stringstream stream(meshData);
            std::vector<Zone2*> zoneVector;
            getMesh()->load(stream, zoneVector);
        }
    }
};

struct MeshUpdate {
    mpi::request request;
    Bbox2 targetBox;
    std::shared_ptr<SerializableMesh> buffer;
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

/*
Custom boost serialization for enum class Neighbor and std::optional<size_t>
*/
namespace boost {
    namespace serialization {

        template<class Archive>
        void serialize(Archive& ar, Neighbor& n, const unsigned int version) {
            typedef typename std::underlying_type<Neighbor>::type EnumType;
            EnumType val = static_cast<EnumType>(n);
            ar & val;
            if (Archive::is_loading::value) {
                n = static_cast<Neighbor>(val);
            }
        }

        template<class Archive>
        void serialize(Archive& ar, std::optional<long unsigned int>& opt, const unsigned int version) {
            bool is_present = opt.has_value();
            ar & is_present;
            
            if (is_present) {
                if constexpr (!Archive::is_loading::value) {
                    long unsigned int value = opt.value();
                    ar & value;
                } else {
                    long unsigned int value;
                    ar & value;
                    opt = value;
                }
            } else {
                if constexpr (Archive::is_loading::value) {
                    opt.reset();
                }
            }
        }

    } // namespace serialization
} // namespace boost

template<typename SmartPtr>
std::vector<Point2*> pointsInBbox(SmartPtr mesh, const Bbox2& bbox) {
    std::vector<Point2*> validPoints;
    std::vector<Point2*> allPoints;
    mesh->getVertexPointers(allPoints);
    for (auto& point : allPoints) {
        if (bbox.isInBox(*point)) { // && !mesh->isConstraint(point)
            validPoints.push_back(point);
        }
    }
    return validPoints;
}

template<typename SmartPtr>
std::vector<Triangle2*> trianglesInBbox(SmartPtr mesh, const Bbox2& bbox) {
    std::vector<Triangle2*> validTriangles;
    std::vector<Triangle2*> allTriangles;
    mesh->getTrianglePointers(allTriangles);
    for (auto& triangle : allTriangles) {
        if (bbox.isInBox(triangle->getBarycenter())) {
            validTriangles.push_back(triangle);
        }
    }
    return validTriangles;
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
    RuntimeParameters runtimeParameters;
    double maxCircumradius; // max circumradius in the entire mesh

    /*
    Delete all the vertices in the provided Bbox.
    Then insert all the vertices in the provided serializedTriangulation in to mesh.
    */
    void updateBbox(const Bbox2& bbox, SerializableMesh& incomingMesh) {  
        std::vector<Point2*> pointsToRemove = pointsInBbox(mesh.getMesh(), bbox);
        mesh.getMesh()->remove(pointsToRemove);
        std::cout << "Removed " << pointsToRemove.size() << " points from local mesh" << std::endl;
        
        std::vector<Point2*> pointsToInsert;
        incomingMesh.getMesh()->getVertexPointers(pointsToInsert);
        
        for (auto& point : pointsToInsert) {
            if (!bbox.isInBox(*point)) {
                // print point and bbox
                std::cout << "Point: " << *point << std::endl;
                std::cout << "Bbox: " << bbox << std::endl;
                throw std::runtime_error("Point in incoming mesh is not in bbox");
            } 
            mesh.getMesh()->insert(*point);
        }
    }

    /*
    Refine the triangles in the provided bbox
    */
    void refineBbox(const Bbox2& bbox) {
        std::vector<Triangle2*> validTriangles = trianglesInBbox(mesh.getMesh(), bbox);
        std::cout << "Refining local mesh with " << mesh.getMesh()->numberOfTriangles() << 
            " triangles" << std::endl;
        
        Zone2* refineZone = mesh.getMesh()->createZone(validTriangles);
        refineZone = refineZone->convertToBoundedZone();
        assert(refineZone != NULL);

        if (mesh.getMesh()->numberOfTriangles() == 0) {
            std::cout << "No triangles to refine in local mesh" << std::endl;
            exit(0);
            return;
        }
        
        mesh.getMesh()->refine(refineZone, runtimeParameters.minAngle, 
            runtimeParameters.minEdgeLength, runtimeParameters.maxEdgeLength, false);

        mesh.getMesh()->deleteZone(refineZone);
    }

    /*
    Serializer for boost::serialization
    */
    template<class Archive>
    void serialize(Archive & archive, const unsigned version) {
        archive & mesh;
        archive & neighbors;
        archive & maxCircumradius;
        //archive & runtimeParameters;

        std::vector<double> bboxData;
        if (Archive::is_saving::value) {
            // Serialization
            bboxData = {bbox.get_minX(), bbox.get_minY(), bbox.get_maxX(), bbox.get_maxY()};
        }
        archive & bboxData;
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
    std::shared_ptr<Fade_2D> mesh;
    RuntimeParameters runtimeParameters;
    std::unique_ptr<Visualizer2> visualizer;

    // MeshGenParams initMeshGenParams; // params for initial sequential refinement

    GlobalMesh() {}

    GlobalMesh(RuntimeParameters params) {
        mesh = std::make_unique<Fade_2D>();
        runtimeParameters = params;
    }

    // Delete the copy constructor and copy assignment operator
    GlobalMesh(const GlobalMesh&) = delete;
    GlobalMesh& operator=(const GlobalMesh&) = delete;

    // Delete the move constructor and move assignment operator
    GlobalMesh(GlobalMesh&&) = delete;
    GlobalMesh& operator=(GlobalMesh&&) = delete;

    /*
    Sequentially refine the entire mesh with a much lower standard.
    */
    void roughRefineMesh() {
        // create a global zone for refinement
        std::vector<Triangle2*> allTriangles;
        mesh->getTrianglePointers(allTriangles);
        Zone2* refineZone = mesh->createZone(allTriangles);
        refineZone = refineZone->convertToBoundedZone();
        assert(refineZone != NULL);

        // refine the global zone
        std::cout << "Global refinement zone created. Global refinement starting..." << std::endl;
        mesh->refine(refineZone, 10, 10, 500, true);
        std::cout << "Global refinement complete. Mesh has " << mesh->numberOfPoints() <<
            " points after refinement" << std::endl;

        mesh->deleteZone(refineZone);
    }

    /*
    Sequentially refine the entire mesh.
    */
    void refineMesh() {
        // create a global zone for refinement
        std::vector<Triangle2*> allTriangles;
        mesh->getTrianglePointers(allTriangles);
        Zone2* refineZone = mesh->createZone(allTriangles);
        refineZone = refineZone->convertToBoundedZone();
        assert(refineZone != NULL);

        // refine the global zone
        std::cout << "Global refinement zone created. Global refinement starting..." << std::endl;
        mesh->refine(refineZone, runtimeParameters.minAngle, 
            runtimeParameters.minEdgeLength, runtimeParameters.maxEdgeLength, true);
        std::cout << "Global refinement complete. Mesh has " << mesh->numberOfPoints() <<
            " points after refinement" << std::endl;

        mesh->deleteZone(refineZone);
    }

    /*
    Given the number of processors, split the mesh and create a localMesh object.
    Populates all the fields (depending on if initZones is true, populate the zones)
    Return a vector of localMeshes of length nproc.
    */
    std::vector<LocalMesh> splitMesh(const size_t nproc, const bool initZones) {
        size_t numCols = std::sqrt(nproc);
        size_t numRows = std::sqrt(nproc);

        // get global bbox
        Bbox2 bboxAll = mesh->computeBoundingBox();
        double boxWidth = (bboxAll.get_maxX() - bboxAll.get_minX()) / numCols;
        double boxHeight = (bboxAll.get_maxY() - bboxAll.get_minY()) / numRows;

        // calculate maximum circumradius
        double maxCircumradius= .0;
        std::vector<Triangle2*> allTriangles;
        mesh->getTrianglePointers(allTriangles);
        for (auto& triangle : allTriangles) {
            CircumcenterQuality quality;
            double circumRadius = (triangle->getCircumcenter(quality) - *triangle->getCorner(0)).length();
            if (quality == CCQ_OUT_OF_BOUNDS) {
                throw std::runtime_error("Circumradius calculation returned out-of-bounds in splitMesh.\n");
            }
            maxCircumradius = std::max(maxCircumradius, circumRadius);
        }

        std::vector<LocalMesh> localMeshes;
        size_t index = 0;
        for (size_t row = 0; row < numRows; row++) {
            for (size_t col = 0; col < numCols; col++) {
                LocalMesh localMesh;

                // initialize bbox
                localMesh.bbox.setMinX(bboxAll.get_minX() + col * boxWidth);
                localMesh.bbox.setMaxX(bboxAll.get_minX() + (col + 1) * boxWidth);
                localMesh.bbox.setMinY(bboxAll.get_minY() + row * boxHeight);
                localMesh.bbox.setMaxY(bboxAll.get_minY() + (row + 1) * boxHeight);

                // insert points to localMesh
                std::vector<Point2*> pointsToAdd = pointsInBbox(mesh, localMesh.bbox);
                for (auto& point : pointsToAdd) {
                    localMesh.mesh.getMesh()->insert(*point);
                    assert(localMesh.bbox.isInBox(*point));
                }

                // assign neighbors
                localMesh.neighbors[Neighbor::Top] = row < numRows - 1 ?
                    std::make_optional(index + numCols) : std::nullopt;
                localMesh.neighbors[Neighbor::Bottom] = row > 0 ? 
                    std::make_optional(index - numCols) : std::nullopt;
                localMesh.neighbors[Neighbor::Left] = col > 0 ? 
                    std::make_optional(index - 1) : std::nullopt;
                localMesh.neighbors[Neighbor::Right] = col < numCols - 1 ? 
                    std::make_optional(index + 1) : std::nullopt;
                localMesh.neighbors[Neighbor::TL] = localMesh.neighbors[Neighbor::Top].has_value() && 
                    localMesh.neighbors[Neighbor::Left].has_value() ? 
                    std::make_optional(localMesh.neighbors[Neighbor::Top].value() - 1) : std::nullopt;
                localMesh.neighbors[Neighbor::TR] = localMesh.neighbors[Neighbor::Top].has_value() && 
                    localMesh.neighbors[Neighbor::Right].has_value() ? 
                    std::make_optional(localMesh.neighbors[Neighbor::Top].value() + 1) : std::nullopt;
                localMesh.neighbors[Neighbor::BL] = localMesh.neighbors[Neighbor::Bottom].has_value() && 
                    localMesh.neighbors[Neighbor::Left].has_value() ? 
                    std::make_optional(localMesh.neighbors[Neighbor::Bottom].value() - 1) : std::nullopt;
                localMesh.neighbors[Neighbor::BR] = localMesh.neighbors[Neighbor::Bottom].has_value() && 
                    localMesh.neighbors[Neighbor::Right].has_value() ? 
                    std::make_optional(localMesh.neighbors[Neighbor::Bottom].value() + 1) : std::nullopt;

                // set parameters (max circumradius and runtime parameters)
                localMesh.maxCircumradius = maxCircumradius;
                localMesh.runtimeParameters = this->runtimeParameters;

                localMeshes.push_back(localMesh);
                index++;
            }
        }
        std::cout << "Splitted global mesh into " << index << " local meshes" << std::endl;
        return localMeshes;
    }

    /*
    Delete the current mesh and reconstruct one by combining a list of localMeshes.
    Used to combine results as the end of computation.
    */
    void loadFromLocalMeshes(std::vector<LocalMesh>& localMeshes) {
        mesh->reset();

        for (auto& localMesh : localMeshes) {
            std::vector<Point2*> points;
            localMesh.mesh.getMesh()->getVertexPointers(points);
            // exclude the boarder overlap points
            std::vector<Point2*> pointsToAdd = pointsInBbox(localMesh.mesh.getMesh(), localMesh.bbox);
            for (auto& point : pointsToAdd) {
                this->mesh->insert(*point);
            }
        }
    }

    /*
    Load mesh from .ply file (file name specified by runtimeParameters)
    using readPointsPLY().
    */
    void loadFromPLY() {
        std::vector<Point2> loadedPoints;
        //readPointsPLY(runtimeParameters.inFilePath.c_str(), true, loadedPoints, NULL);
        mesh->insert(loadedPoints);
    }
    
    /*
    Save mesh to .ply file (file name specified by runtimeParameters)
    using writePointsPLY().
    */
    void saveToPLY() {
        std::vector<Point2*> pointsToSave;
        mesh->getVertexPointers(pointsToSave);
        //writePointsPLY(runtimeParameters.outFilePath.c_str(), pointsToSave, true);
    }

    /*
    Load randomly generated points
    */
    void loadFromRandom() {
        std::vector<Point2> generatedPoints;
        generateRandomPoints(runtimeParameters.numRandomPoints, 0.0, 10000.0, generatedPoints, 15418);
        std::cout << "Generated " << generatedPoints.size() << " random points" << std::endl;
        
        mesh->insert(generatedPoints);
        std::cout << "Mesh has " << mesh->numberOfPoints() << 
            " vertices after insertion of random points" << std::endl;
    }

    void initializeVisualizer() {
        visualizer = std::make_unique<Visualizer2>("visualization.ps");
    }

    void visualizePoints() {
        std::vector<Point2*> points;
        mesh->getVertexPointers(points);
        Color color = Color(255, 0, 0, 0.5);
        visualizer->addObject(points, color);

        Color highlightColor = Color(0, 255, 0, 5);
        visualizer->addObject(Point2(0, 0), highlightColor);
    }

    void visualizeTriangles() {
        std::vector<Triangle2*> triangles;
        mesh->getTrianglePointers(triangles);
        Color color = Color(0, 0, 255, 0.2);
        visualizer->addObject(triangles, color);
    }

    void saveVisualization() {
        visualizer->writeFile();
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
            bbox.setMinY(b->get_minY() + 2 * r);
            bbox.setMaxX(b->get_maxX());
            bbox.setMaxY(b->get_maxY());
            return bbox;
        }),
        Task(Operation::Send, Neighbor::BR, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_maxX() - 2 * r);
            bbox.setMinY(b->get_minY());
            bbox.setMaxX(b->get_maxX());
            bbox.setMaxY(b->get_minY() + 2 * r);
            return bbox;
        }),
        Task(Operation::Send, Neighbor::Bottom, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX());
            bbox.setMinY(b->get_minY());
            bbox.setMaxX(b->get_maxX() - 2 * r);
            bbox.setMaxY(b->get_minY() + 2 * r);
            return bbox;
        })
    };
    phaseZeroTasks.receiveTasks = {
        Task(Operation::Receive, Neighbor::Left, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - 2 * r);
            bbox.setMinY(b->get_minY() + 2 * r);
            bbox.setMaxX(b->get_minX());
            bbox.setMaxY(b->get_maxY());
            return bbox;
        }),
        Task(Operation::Receive, Neighbor::TL, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - 2 * r);
            bbox.setMinY(b->get_maxY());
            bbox.setMaxX(b->get_minX());
            bbox.setMaxY(b->get_maxY() + 2 * r);
            return bbox;
        }),
        Task(Operation::Receive, Neighbor::Top, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX());
            bbox.setMinY(b->get_maxY());
            bbox.setMaxX(b->get_maxX() - 2 * r);
            bbox.setMaxY(b->get_maxY() + 2 * r);
            return bbox;
        })
    };
    phaseZeroTasks.refineTask = std::nullopt;

    TaskGroup phaseOneTasks;
    phaseOneTasks.sendTasks = {
        Task(Operation::Send, Neighbor::Left, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - 2 * r);
            bbox.setMinY(b->get_minY() + 2 * r);
            bbox.setMaxX(b->get_minX() + 2 * r);
            bbox.setMaxY(b->get_maxY() + 2 * r);
            return bbox;
        })
    };
    phaseOneTasks.receiveTasks = {
        Task(Operation::Receive, Neighbor::Right, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_maxX() - 2 * r);
            bbox.setMinY(b->get_minY() + 2 * r);
            bbox.setMaxX(b->get_maxX() + 2 * r);
            bbox.setMaxY(b->get_maxY() + 2 * r);
            return bbox;
        })
    };
    phaseOneTasks.refineTask = 
        Task(Operation::Refine, [](Bbox2* b, double r) { 
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - r);
            bbox.setMinY((b->get_minY() + b->get_maxY()) / 2 - r);
            bbox.setMaxX((b->get_minX() + b->get_maxX()) / 2 + r);
            bbox.setMaxY(b->get_maxY() + r);
            return bbox;
        });

    TaskGroup phaseTwoTasks;
    phaseTwoTasks.sendTasks = {
        Task(Operation::Send, Neighbor::Top, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() + 2 * r);
            bbox.setMinY(b->get_maxY() - 2 * r);
            bbox.setMaxX(b->get_maxX() + 2 * r);
            bbox.setMaxY(b->get_maxY() + 2 * r);
            return bbox;
        })
    };
    phaseTwoTasks.receiveTasks = {
        Task(Operation::Receive, Neighbor::Bottom, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() + 2 * r);
            bbox.setMinY(b->get_minY() - 2 * r);
            bbox.setMaxX(b->get_maxX() + 2 * r);
            bbox.setMaxY(b->get_minY() + 2 * r);
            return bbox;
        })
    };
    phaseTwoTasks.refineTask =
        Task(Operation::Refine, [](Bbox2* b, double r) { 
            Bbox2 bbox = Bbox2();
            bbox.setMinX((b->get_minX() + b->get_maxX()) / 2);
            bbox.setMinY((b->get_minY() + b->get_maxY()) / 2 - r);
            bbox.setMaxX(b->get_maxX());
            bbox.setMaxY(b->get_maxY() + r);
            return bbox;
        });

    TaskGroup phaseThreeTasks;
    phaseThreeTasks.sendTasks = {
        Task(Operation::Send, Neighbor::Right, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_maxX() - 2 * r);
            bbox.setMinY(b->get_minY() - 2 * r);
            bbox.setMaxX(b->get_maxX() + 2 * r);
            bbox.setMaxY(b->get_maxY() - 2 * r);
            return bbox;
        })
    };
    phaseThreeTasks.receiveTasks = {
        Task(Operation::Receive, Neighbor::Left, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - 2 * r);
            bbox.setMinY(b->get_minY() - 2 * r);
            bbox.setMaxX(b->get_minX() + 2 * r);
            bbox.setMaxY(b->get_maxY() - 2 * r);
            return bbox;
        })
    };
    phaseThreeTasks.refineTask =
        Task(Operation::Refine, [](Bbox2* b, double r) { 
            Bbox2 bbox = Bbox2();
            bbox.setMinX((b->get_minX() + b->get_maxX()) / 2 - r);
            bbox.setMinY(b->get_minY());
            bbox.setMaxX(b->get_maxX() + r);
            bbox.setMaxY((b->get_minY() + b->get_maxY()) / 2);
            return bbox;
        });

    TaskGroup phaseFourTasks;
    phaseFourTasks.sendTasks = {
        Task(Operation::Send, Neighbor::Left, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - 2 * r);
            bbox.setMinY(b->get_minY());
            bbox.setMaxX(b->get_minX());
            bbox.setMaxY(b->get_maxY() - 2 * r);
            return bbox;
        }),
        Task(Operation::Send, Neighbor::BL, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX() - 2 * r);
            bbox.setMinY(b->get_minY() - 2 * r);
            bbox.setMaxX(b->get_minX());
            bbox.setMaxY(b->get_minY());
            return bbox;
        }),
        Task(Operation::Send, Neighbor::Bottom, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX());
            bbox.setMinY(b->get_minY() - 2 * r);
            bbox.setMaxX(b->get_maxX() - 2 * r);
            bbox.setMaxY(b->get_minY());
            return bbox;
        })
    };
    phaseFourTasks.receiveTasks = {
        Task(Operation::Receive, Neighbor::Right, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_maxX() - 2 * r);
            bbox.setMinY(b->get_minY());
            bbox.setMaxX(b->get_maxX());
            bbox.setMaxY(b->get_maxY() - 2 * r);
            return bbox;
        }),
        Task(Operation::Receive, Neighbor::TR, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_maxX() - 2 * r);
            bbox.setMinY(b->get_maxY() - 2 * r);
            bbox.setMaxX(b->get_maxX());
            bbox.setMaxY(b->get_maxY());
            return bbox;
        }),
        Task(Operation::Receive, Neighbor::Top, [](Bbox2* b, double r) {
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX());
            bbox.setMinY(b->get_maxY() - 2 * r);
            bbox.setMaxX(b->get_maxX() - 2 * r);
            bbox.setMaxY(b->get_maxY());
            return bbox;
        })
    };
    phaseFourTasks.refineTask =
        Task(Operation::Refine, [](Bbox2* b, double r) { 
            Bbox2 bbox = Bbox2();
            bbox.setMinX(b->get_minX());
            bbox.setMinY(b->get_minY());
            bbox.setMaxX((b->get_minX() + b->get_maxX()) / 2);
            bbox.setMaxY((b->get_minY() + b->get_maxY()) / 2);
            return bbox;
        });

    std::vector<TaskGroup> taskGroups = {phaseZeroTasks, phaseOneTasks, phaseTwoTasks, phaseThreeTasks, phaseFourTasks};
    return taskGroups;
}
