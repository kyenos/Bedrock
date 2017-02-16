#pragma once
#include <sqlitecluster/SQLiteNode.h>

class BedrockServer;
class BedrockPlugin;
struct BedrockTester; // Defined in BedrockTester.h, but can't include else circular

class BedrockNode : public SQLiteNode {
  public:
    // Construct the base class
    BedrockNode(const SData& args, int threadId, int threadCount, BedrockServer* server_);
    virtual ~BedrockNode();

    BedrockServer* server;

    bool isWorker();

    // Returns _dbReady, which is only useful on a sync node.
    bool dbReady();

    // STCPManager API: Socket management
    void postSelect(fd_map& fdm, uint64_t& nextActivity);

    // Handle an exception thrown by a plugin while peek/processing a command.
    void handleCommandException(SQLite& db, Command* command, const string& errorStr, bool wasProcessing);

  protected:
    // SQLiteNode API: Command management
    virtual void _abortCommand(SQLite& db, Command* command);
    virtual void _cleanCommand(Command* command);
    virtual bool _passToExternalQueue(Command* command);
    virtual bool _peekCommand(SQLite& db, Command* command);
    virtual bool _processCommand(SQLite& db, Command* command);
    virtual void _setState(SQLCState state);

  private:
    // If we're the sync node, we keep track of whether our database is ready to use.
    bool _dbReady = false;
};
