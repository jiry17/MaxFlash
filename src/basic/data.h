#ifndef L2S_DATA_H
#define L2S_DATA_H

#include "value.h"

#include <cassert>
#include <vector>

#include "json/json.h"

// A common class representing all possible values.
// If a new type of values is used, this type must be registered in this class.
class Data {
public:
    Value* value;
    Type getType() const {return value->getType();}
    Data(Value* _value): value(_value) {}

    Data(): value(nullptr) {}
    Data(int _value): value(new IntValue(_value)) {}
    Data(bool _value): value(new BoolValue(_value)) {}
    Data(const Data& _data): value(_data.value->copy()) {}
    ~Data() {if (value) delete value; value = nullptr;}

    std::string toString() const {
        switch (value->getType()) {
            case TINT: return std::to_string(getInt());
            case TBOOL: return getBool() ? "True" : "False";
            case TSTRING: return "\"" + getString() + "\"";
            case TMATRIX: return dynamic_cast<MatrixValue*>(value)->toString();
        }
    }

    int getInt() const {
#ifdef DEBUG
        assert(value->getType() == TINT);
#endif
        return dynamic_cast<IntValue*>(value)->getValue();
    }

    bool getBool() const {
#ifdef DEBUG
        assert(value->getType() == TBOOL);
#endif
        return dynamic_cast<BoolValue*>(value)->getValue();
    }

    std::string getString() const {
#ifdef DEBUG
        assert(value->getType() == TSTRING);
#endif
        return dynamic_cast<StringValue*>(value)->getValue();
    };

    Matrix getMatrix() const {
#ifdef DEBUG
        assert(value->getType() == TMATRIX);
#endif
        return dynamic_cast<MatrixValue*>(value)->getValue();
    }

    bool operator == (const Data& data) const {
        if (getType() != data.getType()) return false;
        switch (getType()) {
            case TINT: return getInt() == data.getInt();
            case TBOOL: return getBool() == data.getBool();
            case TSTRING: return getString() == data.getString();
            case TMATRIX: return getMatrix() == data.getMatrix();
        }
    }

    bool operator != (const Data& data) const {
        return !((*this) == data);
    }
};

typedef std::vector<Data> DataList;

#endif //L2S_DATA_H
