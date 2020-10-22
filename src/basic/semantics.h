#ifndef L2S_SEMANTICS_H
#define L2S_SEMANTICS_H

#include <vector>
#include <algorithm>
#include "util.h"
#include "data.h"

typedef std::vector<DataList> WitnessTerm;
typedef std::vector<WitnessTerm> WitnessList;

// An abstracted class representing the global info which is possibly used by wintess functions.
class GlobalInfo {
public:
    virtual std::string getName() {return "GlobalInfo";}
};

// For a given example, an object of "ParamInfo" contains the values of all variables.
class ParamInfo: public GlobalInfo {
protected:
    DataList param_value;
public:
    virtual std::string getName() {return "ParamInfo";}
    ParamInfo(const DataList& _param_value): param_value(_param_value) {}
    ParamInfo() = default;
    Data& operator [] (int k) {return param_value[k];}
    int size() {return param_value.size();}
};

// An abstracted class representing the semantics of an operator.
// If a new kind of operator is used, it must be implemented as a subclass of "Semantics", including its
// witness function and its semantics. Besides, this operator should also be register in "semantics_factory.h".
class Semantics {
protected:
    void check(const DataList& inp_list) {
        assert(inp_list.size() == inp_type_list.size());
        for (int i = 0; i < inp_list.size(); ++i) {
            assert(inp_list[i].getType() == inp_type_list[i]);
        }
    }
public:
    std::vector<Type> inp_type_list;
    Type oup_type;
    std::string name;
    Semantics(const std::vector<Type>& _inp_type_list, Type _oup_type, std::string _name):
        inp_type_list(_inp_type_list), oup_type(_oup_type), name(_name) {}
    virtual WitnessList witnessFunction(const DataList &oup, GlobalInfo* global_info) = 0;
    virtual Data run(const DataList &inp, GlobalInfo* global_info) = 0;
};

// A special semantics for all constant values.
class ConstSemantics: public Semantics{
public:
    Data value;
    ConstSemantics(Data _value):
        value(_value), Semantics({}, _value.getType(), _value.toString()) {}
    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);

    virtual Data run(const DataList &inp, GlobalInfo *global_info) {
        return value;
    }
};

// A special semantics for all parameters.
class ParamSemantics: public Semantics {
    int id;
    Type type;
public:
    ParamSemantics(int _id, Type _type): id(_id), type(_type),
        Semantics({}, _type, "Param" + std::to_string(_id)) {}
    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
    virtual Data run(const DataList &inp, GlobalInfo *global_info) {
        ParamInfo* param_info = dynamic_cast<ParamInfo*>(global_info);
#ifdef DEBUG
        assert(param_info != nullptr);
#endif
        return (*param_info)[id];
    }
};


#endif //L2S_SEMANTICS_H
