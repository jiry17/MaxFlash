#include "context_maintainer.h"
#include "config.h"

Program* ContextMaintainer::locateSubProgram(const PathInfo &path) {
    auto* current_node = partial_program;
    for (auto pos: path) {
#ifdef DEBUG
        assert(current_node->sub_list.size() > pos);
#endif
        current_node = current_node->sub_list[pos];
    }
    return current_node;
}

void ContextMaintainer::update(const PathInfo &path, Semantics *semantics) {
    auto* current_node = locateSubProgram(path);
#ifdef DEBUG
    assert(current_node->semantics == nullptr);
#endif
    current_node->semantics = semantics;
    int param_num = semantics->inp_type_list.size();
    for (int i = 0; i < param_num; ++i) {
        current_node->sub_list.push_back(new Program());
    }
}

std::vector<Program*> TopDownContextMaintainer::getTrace(const PathInfo &path) {
    std::vector<Program*> result;
    auto* current_node = partial_program;
    for (auto pos: path) {
        result.push_back(current_node);
#ifdef DEBUG
        assert(current_node->sub_list.size() > pos);
#endif
        current_node = current_node->sub_list[pos];
    }
    return result;
}

std::string TopDownContextMaintainer::encodeConstant(const Data& data) {
    if (global::spec_type != S_PBE || data.getType() != TSTRING)
        return "Constant@" + util::type2String(data.getType());
    return util::getStringConstType(data.getString());
}

std::string TopDownContextMaintainer::semantics2String(Semantics *semantics) {
    auto* param_semantics = dynamic_cast<ParamSemantics*>(semantics);
    if (param_semantics != nullptr) {
        return "Param@" + util::type2String(param_semantics->oup_type);
    }
    auto* constant_semantics = dynamic_cast<ConstSemantics*>(semantics);
    if (constant_semantics != nullptr) {
        return encodeConstant(constant_semantics->value);
    }
    return semantics->name;
}

TopDownContext* TopDownContextMaintainer::getAbstractedContext(const PathInfo &path) {
    auto trace = getTrace(path);
    std::vector<std::string> result;
    int start_pos = 0;
    if (trace.size() < global::KContextDepth) {
        for (int i = trace.size(); i < global::KContextDepth; ++i) result.emplace_back("None");
    } else {
        start_pos = trace.size() - global::KContextDepth;
    }
    for (int i = start_pos; i < trace.size(); ++i) {
        Program* current_node = trace[i];
        result.push_back(TopDownContextMaintainer::semantics2String(current_node->semantics) + "@" + std::to_string(path[i] + 1));
    }
    return new TopDownContext(result);
}

