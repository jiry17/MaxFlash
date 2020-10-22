#ifndef L2S_MATRIX_OPERATOR_H
#define L2S_MATRIX_OPERATOR_H

#include "semantics.h"

// All operators used in the matrix domain.
class ReshapeSemantics: public Semantics {
public:
    ReshapeSemantics(): Semantics({TMATRIX, TMATRIX}, TMATRIX, "Reshape") {}
    virtual Data run(const DataList &inp, GlobalInfo *global_info);
    virtual WitnessList witnessFunction(const DataList &oup, GlobalInfo *global_info);
};

class PermuteSemantics: public Semantics {
    void permuteIndex(int pos, const std::vector<int>& perm, const Matrix& matrix, int current_index, std::vector<int>& result);
public:
    PermuteSemantics(): Semantics({TMATRIX, TMATRIX}, TMATRIX, "Permute") {}
    virtual Data run(const DataList &inp, GlobalInfo *global_info);
    virtual WitnessList witnessFunction(const DataList &oup, GlobalInfo *global_info);
};

class MatrixIDSemantics: public Semantics {
public:
    MatrixIDSemantics(): Semantics({TMATRIX}, TMATRIX, "Var") {}
    virtual Data run(const DataList &inp, GlobalInfo *global_info);
    virtual WitnessList witnessFunction(const DataList &oup, GlobalInfo *global_info);
};

class FliplrSemantics: public Semantics {
    void flipLR(int pos, const Matrix& matrix, std::vector<int>& init_dim, std::vector<int>& result);
public:
    FliplrSemantics(): Semantics({TMATRIX}, {TMATRIX}, "Fliplr") {}
    virtual Data run(const DataList &inp, GlobalInfo *global_info);
    virtual WitnessList witnessFunction(const DataList &oup, GlobalInfo *global_info);
};

class FlipudSemantics: public Semantics {
    void flipUD(int pos, const Matrix& matrix, std::vector<int>& init_dim, std::vector<int>& result);
public:
    FlipudSemantics(): Semantics({TMATRIX}, {TMATRIX}, "Flipud") {}
    virtual Data run(const DataList &inp, GlobalInfo *global_info);
    virtual WitnessList witnessFunction(const DataList &oup, GlobalInfo *global_info);
};

class VectorInitSemantics: public Semantics {
public:
    VectorInitSemantics(): Semantics({TINT, TINT}, TMATRIX, "B") {}
    virtual Data run(const DataList &inp, GlobalInfo *global_info) {
#ifdef DEBUG
        check(inp);
#endif
        return Data(new MatrixValue({inp[0].getInt(), inp[1].getInt()}, {2}));
    }
    virtual WitnessList witnessFunction(const DataList &oup, GlobalInfo *global_info) {
#ifdef DEBUG
        assert(oup.size() == 1);
#endif
        auto matrix = oup[0].getMatrix();
#ifdef DEBUG
        assert(matrix.shape.size() == 1 && matrix.contents.size() >= 2);
#endif
        if (matrix.contents.size() > 2) return {};
        return {{{Data(new IntValue(matrix.contents[0]))}, {Data(new IntValue(matrix.contents[1]))}}};
    }
};

class VectorConcatSemantics: public Semantics {
public:
    VectorConcatSemantics(): Semantics({TINT, TMATRIX}, TMATRIX, "L") {}
    virtual Data run(const DataList &inp, GlobalInfo *global_info) {
#ifdef DEBUG
        check(inp);
#endif
        auto matrix = inp[1].getMatrix();
#ifdef DEBUG
        assert(matrix.shape.size() == 1 && matrix.contents.size() >= 2);
#endif
        std::vector<int> new_contents = {inp[0].getInt()};
        for (int value: matrix.contents) {
            new_contents.push_back(value);
        }
        return Data(new MatrixValue(new_contents, {matrix.shape[0] + 1}));
    }
    virtual WitnessList witnessFunction(const DataList &oup, GlobalInfo *global_info) {
#ifdef DEBUG
        assert(oup.size() == 1);
#endif
        auto matrix = oup[0].getMatrix();
#ifdef DEBUG
        assert(matrix.shape.size() == 1 && matrix.contents.size() >= 2);
#endif
        if (matrix.contents.size() == 2) return {};
        std::vector<int> new_contents;
        for (int i = 1; i < matrix.contents.size(); ++i) {
            new_contents.push_back(matrix.contents[i]);
        }
        return {{{Data(new IntValue(matrix.contents[0]))},
                 {Data(new MatrixValue(new_contents, {matrix.shape[0] - 1}))}}};
    }

};



#endif //L2S_MATRIX_OPERATOR_H
