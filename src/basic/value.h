#ifndef L2S_VALUE_H
#define L2S_VALUE_H

enum Type {
    TINT, TBOOL, TSTRING, TMATRIX
};

class Value {
    Type type;
public:
    Type getType() const {return type;}
    Value(Type _type): type(_type) {}
    virtual Value* copy() const = 0;
    virtual ~Value() = default;
};

class IntValue: public Value {
    int value;
public:
    IntValue(int _value): Value(TINT), value(_value) {}
    virtual Value* copy() const {return new IntValue(value);}
    int getValue() const {return value;}
    virtual ~IntValue() = default;
};

class BoolValue: public Value {
    bool value;
public:
    BoolValue(bool _value): Value(TBOOL), value(_value) {}
    virtual Value* copy() const {return new BoolValue(value);}
    bool getValue() const {return value;}
    virtual ~BoolValue() = default;
};

class StringValue: public Value {
    std::string value;
public:
    StringValue(std::string _value): Value(TSTRING), value(_value) {}
    virtual Value* copy() const {return new StringValue(value);}
    std::string getValue() const {return value;}
    virtual ~StringValue() = default;
};

struct Matrix {
    std::vector<int> contents;
    std::vector<int> shape;
    Matrix(const std::vector<int>& _contents, const std::vector<int>& _shape): contents(_contents), shape(_shape) {}
    bool operator == (const Matrix& matrix) const {
        return shape == matrix.shape && contents == matrix.contents;
    }
};

class MatrixValue: public Value {
public:
    std::vector<int> contents;
    std::vector<int> shape;
    MatrixValue(const std::vector<int>& _contents, const std::vector<int>& _shape): Value(TMATRIX), contents(_contents), shape(_shape) {
#ifdef DEBUG
        int size = 1;
        for (int dim_size: shape) {
            size *= dim_size;
        }
        assert(size == contents.size());
#endif
    }
    virtual Value* copy() const {return new MatrixValue(contents, shape);}
    virtual ~MatrixValue() = default;
    std::string toString() const {
        std::string result = "{";
        for (int i = 0; i < contents.size(); ++i) {
            result += std::to_string(contents[i]) + ",";
        }
        result += "}@{";
        for (int i: shape) {
            result += std::to_string(i) + ",";
        }
        result += "}";
        return result;
    }
    Matrix getValue() const {return Matrix(contents, shape);}
};


#endif //L2S_VALUE_H
