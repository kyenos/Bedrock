#include <libstuff/libstuff.h>
#include "BedrockPlugin.h"
#include "BedrockServer.h"

map<string, function<BedrockPlugin*(BedrockServer&)>> BedrockPlugin::g_registeredPluginList;

BedrockPlugin::BedrockPlugin(BedrockServer& s) : server(s) {
}

BedrockPlugin::~BedrockPlugin() {
    for (auto httpsManager : httpsManagers) {
        delete httpsManager;
    }
}

void BedrockPlugin::verifyAttributeInt64(const SData& request, const string& name, size_t minSize) {
    if (request[name].size() < minSize) {
        STHROW("402 Missing " + name);
    }
    if (!request[name].empty() && request[name] != SToStr(SToInt64(request[name]))) {
        STHROW("402 Malformed " + name);
    }
}

void BedrockPlugin::verifyAttributeSize(const SData& request, const string& name, size_t minSize, size_t maxSize) {
    if (request[name].size() < minSize) {
        STHROW("402 Missing " + name);
    }
    if (request[name].size() > maxSize) {
        STHROW("402 Malformed " + name);
    }
}

void BedrockPlugin::verifyAttributeBool(const SData& request, const string& name, bool require)
{
    if (require && !request[name].size()) {
        STHROW("402 Missing " + name);
    }
    if (!request[name].empty() && !SIEquals(request[name], "true") && !SIEquals(request[name], "false")) {
        STHROW("402 Malformed " + name);
    }
}

bool BedrockPlugin::shouldEnableQueryRewriting(const SQLite& db, const BedrockCommand& command, bool (**rewriteHandler)(int, const char*, string&)) {
    return false;
}

STable BedrockPlugin::getInfo() {
    return STable();
}

string BedrockPlugin::getName() {
    SERROR("No name defined by this plugin, aborting.");
}

bool BedrockPlugin::peekCommand(SQLite& db, BedrockCommand& command) {
    return false;
}

bool BedrockPlugin::processCommand(SQLite& db, BedrockCommand& command) {
    return false;
}

bool BedrockPlugin::shouldSuppressTimeoutWarnings() {
    return false;
}

bool BedrockPlugin::preventAttach() {
    return false;
}

void BedrockPlugin::timerFired(SStopwatch* timer) {}

void BedrockPlugin::upgradeDatabase(SQLite& db) {}

void BedrockPlugin::handleFailedReply(const BedrockCommand& command) {
    // Default implementation does nothing.
}
