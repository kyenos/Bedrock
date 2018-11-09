#include <test/lib/BedrockTester.h>

class BedrockClusterTester {
  public:
    enum ClusterSize {
        ONE_NODE_CLUSTER   = 1,
        THREE_NODE_CLUSTER = 3,
        FIVE_NODE_CLUSTER  = 5,
    };

    // Creates a cluster of the given size and brings up all the nodes. The nodes will have priority in the order of
    // their creation (i.e., node 0 is highest priority and will become master.
    // You can also specify plugins to load if for some reason you need to override the default configuration.
    BedrockClusterTester(ClusterSize size, list<string> queries = {}, int threadID = 0, map<string, string> _args = {}, list<string> uniquePorts = {}, string pluginsToLoad = "db,cache,jobs");
    BedrockClusterTester(int threadID, string pluginsToLoad = "db,cache,jobs");
    ~BedrockClusterTester();

    // Returns the index of the node that's mastering. Returns a negative number on error:
    // -1: no master
    // -2: multiple masters
    int getMasterNodeIndex();

    // Runs the given query on all nodes and verifies the output is the same. Make sure you include "ORDER BY" if you
    // want to verify this across multiple rows, as they're not guaranteed to be returned in the same order for all
    // nodes.
    bool VerifyQuery(string query);

    // Wait until all nodes have the same highest commit count. Optional timeout, set to 0 for no timeout.
    // Returns true on sync, false on timeout.
    bool waitForSync(int timeout_usec = 0);

    // Returns the bedrock tester at the given index in the cluster.
    BedrockTester* getBedrockTester(size_t index);

    // Starts a given node, given the same arguments given by the constructor.
    string startNode(size_t nodeIndex, bool logToConsole = false);

    // Same as above but don't wait for the command port to be ready.
    string startNodeDontWait(size_t nodeIndex);

    // Stops a given node.
    void stopNode(size_t nodeIndex);

  private:

    // The number of nodes in the cluster.
    int _size;

    // A list of all our testers that make up our cluster.
    vector<BedrockTester> _cluster;
};
