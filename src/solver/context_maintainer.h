#ifndef L2S_CONTEXT_MAINTAINER_H
#define L2S_CONTEXT_MAINTAINER_H

#include "context.h"
#include "program.h"

typedef std::vector<int> PathInfo;

// An abstracted topdown context model, maintains the transitions between contexts.
class ContextMaintainer {
protected:
    Program* locateSubProgram(const PathInfo& path);
public:
    Program* partial_program;
    ContextMaintainer(): partial_program(nullptr) {}
    ContextMaintainer(Program* _partial_program): partial_program(_partial_program) {}
    virtual Context* getAbstractedContext(const PathInfo& path) = 0;
    virtual Context* getMinimalContext(const PathInfo& path) = 0;
    void update(const PathInfo& path, Semantics* _semantics);
    void clear() {
        if (partial_program != nullptr) {
            delete partial_program;
            partial_program = nullptr;
        }
    }
};

// A n-gram model.
class TopDownContextMaintainer: public ContextMaintainer {
protected:
    std::vector<Program*> getTrace(const PathInfo& path);
    std::string semantics2String(Semantics* semantics);
    std::string encodeConstant(const Data& data);
public:
    virtual TopDownContext* getAbstractedContext(const PathInfo& path);
    virtual TopDownContext* getMinimalContext(const PathInfo& path) {
        return getAbstractedContext(path);
    }
};

#endif //L2S_CONTEXT_MAINTAINER_H
