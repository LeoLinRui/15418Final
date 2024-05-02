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