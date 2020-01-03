#pragma once
#include <libstuff/libstuff.h>
#include <sqlitecluster/SQLite.h>
#include <test/lib/TestHTTPS.h>
#include <test/lib/tpunit++.hpp>

// Track TCP ports to use with the tester.
class PortMap {
  public:

    static const int64_t START_PORT = 10000;
    static const int64_t MAX_PORT = 20000;

    // Get an unused port
    uint16_t getPort()
    {
        lock_guard<mutex> lock(_m);
        uint16_t port = 0;

        // Reuse the first available freed port.
        if (_returned.size()) {
            port = *_returned.begin();
            if (waitForPort(port) == 0) {
                _returned.erase(_returned.begin());
                return port;
            }
        }

        // Otherwise, grab from the next port in our range.
        do {
            port = _from;
            _from++;
            if (waitForPort(port) == 0) {
                return port;
            }
        } while (port<=MAX_PORT);

        // Should never reach this.
        cout << "Ran out of available ports after port " << port << endl;
        SASSERT(port<MAX_PORT);
        return 1;
    }

    // Free up previously used port to be used again
    void returnPort(uint16_t port)
    {
        lock_guard<mutex> lock(_m);
        _returned.insert(port);
    }

    // Waits for a particular port to be free to bind to. This is useful when we've killed a server, because sometimes
    // it takes the OS a few seconds to make the port available again.
    int waitForPort(int port) {
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        int i = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
        sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        int result = 0;
        int count = 0;
        uint64_t start = STimeNow();
        do {
            result = ::bind(sock, (sockaddr*)&addr, sizeof(addr));
            if (result) {
                count++;
                usleep(100'000);
            } else {
                shutdown(sock, 2);
                close(sock);
                return 0;
            }
        // Wait up to 5 seconds.
        } while (result && STimeNow() < start + 5'000'000);

        // Ran out of time, return 1 ("unsuccessful")
        return 1;
    }

    PortMap(uint16_t from = START_PORT) : _from(from)
    {}

  private:
    uint16_t _from;
    set<uint16_t> _returned;
    mutex _m;
};

class BedrockTester {
  public:

    static int mockRequestMode;
    // Generate a temporary filename for a test DB, with an optional prefix.
    static string getTempFileName(string prefix = "");

    // Returns the name of the server binary, by finding the first path that exists in `locations`.
    static string getServerName();

    // Search paths for `getServerName()`. Allowed to be modified before startup by implementer.
    static list<string> locations;

    static string defaultDBFile; // Unused, exists for backwards compatibility.
    static string defaultServerAddr; // Unused, exists for backwards compatibility.

    // This is expected to be set by main, built from argv, to expose command-line options to tests.
    static SData globalArgs;

    // Shuts down all bedrock servers associated with any testers.
    static void stopAll();

    // This is an allocator for TCP ports.
    static PortMap ports;

    // Returns the address of this server.
    string getServerAddr() { return _serverAddr; };

    // Constructor/destructor
    BedrockTester(const map<string, string>& args = {},
                  const list<string>& queries = {},
                  bool startImmediately = true,
                  bool keepFilesWhenFinished = false,
                  uint16_t serverPort = 0,
                  uint16_t nodePort = 0,
                  uint16_t controlPort = 0,
                  bool ownPorts = true);

    // Supply a threadID (now obsolete)
    BedrockTester(int threadID,
                  const map<string, string>& args = {},
                  const list<string>& queries = {},
                  bool startImmediately = true,
                  bool keepFilesWhenFinished = false,
                  uint16_t serverPort = 0,
                  uint16_t nodePort = 0,
                  uint16_t controlPort = 0,
                  bool ownPorts = true);
    ~BedrockTester();

    // Start and stop the bedrock server. If `dontWait` is specified, return as soon as the control port, rather that
    // the cmmand port, is ready.
    string startServer(bool dontWait = false);
    void stopServer(int signal = SIGINT);

    // Takes a list of requests, and returns a corresponding list of responses.
    // Uses `connections` parallel connections to the server to send the requests.
    // If `control` is set, sends the message to the control port.
    vector<SData> executeWaitMultipleData(vector<SData> requests, int connections = 10, bool control = false, bool returnOnDisconnect = false, int* errorCode = nullptr);

    // Sends a single request, returning the response content.
    // If the response method line doesn't begin with the expected result, throws.
    string executeWaitVerifyContent(SData request, const string& expectedResult = "200", bool control = false);

    // Sends a single request, returning the response content as a STable.
    // If the response method line doesn't begin with the expected result, throws.
    STable executeWaitVerifyContentTable(SData request, const string& expectedResult = "200");

    // Read from the DB file. Interface is the same as SQLiteNode's 'read' for backwards compatibility.
    string readDB(const string& query);
    bool readDB(const string& query, SQResult& result);
    SQLite& getSQLiteDB();

    int getServerPID() { return _serverPID; }

    // Expose the ports that the server is listening on.
    int getServerPort();
    int getNodePort();
    int getControlPort();

    // Waits up to timeoutUS for the node to be in state `state`, returning true as soon as that state is reached, or
    // false if the timeout is hit.
    bool waitForState(string state, uint64_t timeoutUS = 60'000'000, bool control = false);

    // Like `waitForState` but wait for any of a set of states.
    bool waitForStates(set<string> states, uint64_t timeoutUS = 60'000'000, bool control = false);

    // get the output of a "Status" command from the command port
    STable getStatus(bool control = false);

    // get the value of a particular term from the output of a "Status" command
    string getStatusTerm(string term, bool control = false);

    // wait for the specified commit, up to "retries" times
    bool waitForCommit(int minCommitCount, int retries = 30, bool control = false);

  protected:
    // Args passed on creation, which will be used to start the server if the `start` flag is set, or if `startServer`
    // is called later on with an empty args list.
    map<string, string> _args;

    // If these are set, they'll be used instead of the global defaults.
    string _serverAddr;
    string _dbName;

    string _controlAddr;

    // The PID of the bedrock server we started.
    int _serverPID = 0;

    // A set of all bedrock testers.
    static set<BedrockTester*> _testers;

    // Flag indicating whether the DB should be kept when the tester is destroyed.
    bool _keepFilesWhenFinished;

    // A version of the DB that can be queries without going through bedrock.
    SQLite* _db = 0;

    // For locking around changes to the _testers list.
    static mutex _testersMutex;

    // The ports the server will listen on.
    uint16_t _serverPort;
    uint16_t _nodePort;
    uint16_t _controlPort;
    const bool _ownPorts;
};
