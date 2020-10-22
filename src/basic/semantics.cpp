#include "semantics.h"
#include "util.h"

WitnessList ParamSemantics::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    auto* info = dynamic_cast<ParamInfo*>(global_info);
#ifdef DEBUG
    assert(info != nullptr);
#endif
    if (util::checkInOupList((*info)[id], oup)) {
        return {{}};
    } else return {};
}

WitnessList ConstSemantics::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (util::checkInOupList(value, oup)) {
        return {{}};
    } else return {};
}