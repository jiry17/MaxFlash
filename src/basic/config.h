#ifndef L2S_CNFIG_H
#define L2S_CNFIG_H

#include <algorithm>
#include "specification.h"
#include "semantics.h"

#include <unordered_set>
#include <unordered_map>

enum SpecType {
    S_ORACLE,   //Abandoned.
    S_PBE,  // Programming by example tasks. The semantics constratins should be a set of input-output examples.
    S_NONE  // Dummy value.
};

namespace config {
    extern const std::string KSourcePath;   // The path of the source code.
    extern const std::string KParserMainPath;   // The path of the client parser for the string domain.
    extern double KDefaultP;    // Default probability for an operator.
}

class StringInfo : public ParamInfo{
public:
    virtual std::string getName() {return "StringInfo";}
    std::vector<Example*> example_space;    // All input-output examples.
    DataList const_list;    // All possible constants.
    std::unordered_set<std::string> const_set;  // The set of all possible contants.
    std::unordered_map<std::string, std::string> const_cache;   // Cache the abstracted name of constants.
    std::vector<int> int_const; // All possible integer constants.
    Specification* spec;
    void setInp(const DataList& _inp) {
        param_value.clear();
        for (int i = 0; i < _inp.size(); ++i) {
            param_value.push_back(_inp[i]);
        }
    }
    void clear() {
        example_space.clear();
        const_list.clear();
        const_set.clear();
        const_cache.clear();
    }
};

namespace global {
    // The type of the specification.
    extern SpecType spec_type;
    // Global info of a synthesis task in the string domain. It will be used by witness functions.
    extern StringInfo* string_info;
    // The range of possible integer values in the result program.
    extern int KIntMax;
    extern int KIntMin;
    // The depth of the n-gram model.
    extern int KContextDepth;
    // Whether the benchmark is in the matrix domain.
    extern bool isMatrix;
    // The max number of dimensions in the result program.
    extern int KMaxDim;
}

#endif //L2S_CNFIG_H
