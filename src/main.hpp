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