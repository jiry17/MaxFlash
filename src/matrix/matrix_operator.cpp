#include "matrix_operator.h"
#include "config.h"

#include <cstring>

namespace {
    void searchForFactor(int rem, std::vector<int>& current, std::vector<std::vector<int>>& result) {
        if (rem == 1 && current.size() > 1) {
            result.push_back(current);
        }
        if (current.size() == global::KMaxDim) return;
        for (int dim_size: global::string_info->int_const) {
            if (dim_size <= 1) continue;
            if (rem % dim_size == 0) {
                current.push_back(dim_size);
                searchForFactor(rem / dim_size, current, result);
                current.pop_back();
            }
        }
    }

    std::vector<int> subsize_list;
    int id;

    void initSubsizeList(std::vector<int> shape) {
        subsize_list.resize(shape.size());
        int dim_num = shape.size();
        subsize_list[dim_num - 1] = 1;
        for (int i = dim_num - 2; i >= 0; --i) {
            subsize_list[i] = subsize_list[i + 1] * shape[i + 1];
        }
    }
}

Data ReshapeSemantics::run(const DataList &inp, GlobalInfo *global_info) {
#ifdef DEBUG
    check(inp);
#endif
    auto matrix = inp[0].getMatrix();
    auto shape = inp[1].getMatrix().contents;
#ifdef DEBUG
    int size = matrix.contents.size();
    int new_size = 1;
    for (auto dim_size: shape) {
        new_size *= dim_size;
    }
    assert(size == new_size);
#endif
    return Data(new MatrixValue(inp[0].getMatrix().contents, shape));
}

WitnessList ReshapeSemantics::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
#ifdef DEBUG
    assert(oup.size() == 1);
#endif
    int current_size = 1;
    Matrix matrix = oup[0].getMatrix();
    for (int dim_size: matrix.shape) {
        current_size = current_size * dim_size;
    }
    std::vector<std::vector<int>> factor_schemes = {{1, current_size}};
    std::vector<int> current_scheme;
    searchForFactor(current_size, current_scheme, factor_schemes);
    WitnessList result;
    for (auto& shape: factor_schemes) {
        if (shape == matrix.shape) continue;
        result.push_back({{Data(new MatrixValue(matrix.contents, shape))},
                          {Data(new MatrixValue(matrix.shape, {int(matrix.shape.size())}))}});
    }
    return result;
}

void PermuteSemantics::permuteIndex(int pos, const std::vector<int> &perm, const Matrix &matrix,
                                    int current_index, std::vector<int> &result) {
    if (pos == perm.size()) {
        result[id++] = matrix.contents[current_index];
        return;
    }
    int current_dim = perm[pos];
    int current_size = matrix.shape[current_dim];
    for (int i = 0; i < current_size; ++i) {
        permuteIndex(pos + 1, perm, matrix, current_index, result);
        current_index += subsize_list[perm[pos]];
    }
}

WitnessList PermuteSemantics::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
#ifdef DEBUG
    assert(oup.size() == 1);
#endif
    WitnessList result;
    std::vector<int> perm;
    auto matrix = oup[0].getMatrix();
    for (int i = 0; i < matrix.shape.size(); ++i) {
        perm.push_back(i);
    }
    std::vector<int> new_content(matrix.contents.size());
    std::vector<int> reversed_perm(perm.size());
    std::vector<int> new_shape(perm.size());
    initSubsizeList(matrix.shape);
    while (std::next_permutation(perm.begin(), perm.end())) {
        id = 0;
        permuteIndex(0, perm, matrix, 0, new_content);
        for (int i = 0; i < perm.size(); ++i) {
            reversed_perm[perm[i]] = i;
            new_shape[i] = matrix.shape[perm[i]];
        }
        result.push_back({{Data(new MatrixValue(new_content, new_shape))},
                          {Data(new MatrixValue(reversed_perm, {int(new_shape.size())}))}});
    }
    return result;
}

Data PermuteSemantics::run(const DataList &inp, GlobalInfo *global_info) {
#ifdef DEBUG
    check(inp);
#endif
    auto matrix = inp[0].getMatrix();
    auto perm = inp[1].getMatrix().contents;
#ifdef DEBUG
    assert(perm.size() == matrix.shape.size());
    static int pd[10];
    memset(pd, 0x00, sizeof pd);
    for (int dim: perm) {
        assert(pd[dim] == 0); pd[dim] = 1;
    }
#endif
    std::vector<int> new_content(matrix.contents.size());
    std::vector<int> index(perm.size());
    std::vector<int> new_shape(perm.size());
    for (int i = 0; i < perm.size(); ++i) {
        new_shape[i] = matrix.shape[perm[i]];
    }
    id = 0;
    initSubsizeList(matrix.shape);
    permuteIndex(0, perm, matrix, 0, new_content);
    return Data(new MatrixValue(new_content, new_shape));
}

Data MatrixIDSemantics::run(const DataList &inp, GlobalInfo *global_info) {
#ifdef DEBUG
    check(inp);
#endif
    return inp[0];
}

WitnessList MatrixIDSemantics::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
#ifdef DEBUG
    assert(oup.size() == 1);
#endif
    return {{oup}};
}

void FliplrSemantics::flipLR(int pos, const Matrix &matrix, std::vector<int> &init_dim, std::vector<int> &result) {
    if (pos == matrix.shape.size()) {
        int init_pos = 0;
        for (int i = 0; i < matrix.shape.size(); ++i) {
            init_pos = init_pos * matrix.shape[i] + init_dim[i];
        }
        result.push_back(matrix.contents[init_pos]);
        return;
    }
    if (pos == 1) {
        for (int i = matrix.shape[pos] - 1; i >= 0; --i) {
            init_dim[pos] = i;
            flipLR(pos + 1, matrix, init_dim, result);
        }
    } else {
        for (int i = 0; i < matrix.shape[pos]; ++i) {
            init_dim[pos] = i;
            flipLR(pos + 1, matrix, init_dim, result);
        }
    }
}

Data FliplrSemantics::run(const DataList &inp, GlobalInfo * global_info) {
#ifdef DEBUG
    check(inp);
#endif
    Matrix matrix = inp[0].getMatrix();
    assert(matrix.shape.size() >= 2);
    std::vector<int> new_content(matrix.contents.size());
    std::vector<int> index(matrix.shape.size());
    int sub_size = 1;
    for (int i = 2; i < matrix.shape.size(); ++i) {
        sub_size *= matrix.shape[i];
    }
    int reversed_id = matrix.shape[1] - 1;
    int bias = 0;
    for (int i = 0; i < matrix.contents.size(); ++i) {
        if (i % sub_size == 0) {
            reversed_id += 1;
            if (reversed_id == matrix.shape[1]) {
                reversed_id = 0;
                bias = (matrix.shape[1] - 1) * sub_size;
            } else {
                bias -= (sub_size << 1);
            }
        }
        new_content[bias + i] = matrix.contents[i];
    }
    return Data(new MatrixValue(new_content, matrix.shape));
}

WitnessList FliplrSemantics::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
#ifdef DEBUG
    assert(oup.size() == 1);
#endif
    return {{{run(oup, global_info)}}};
}

void FlipudSemantics::flipUD(int pos, const Matrix &matrix, std::vector<int> &init_dim, std::vector<int> &result) {
    if (pos == matrix.shape.size()) {
        int init_pos = 0;
        for (int i = 0; i < matrix.shape.size(); ++i) {
            init_pos = init_pos * matrix.shape[i] + init_dim[i];
        }
        result.push_back(matrix.contents[init_pos]);
        return;
    }
    if (pos == 0) {
        for (int i = matrix.shape[pos] - 1; i >= 0; --i) {
            init_dim[pos] = i;
            flipUD(pos + 1, matrix, init_dim, result);
        }
    } else {
        for (int i = 0; i < matrix.shape[pos]; ++i) {
            init_dim[pos] = i;
            flipUD(pos + 1, matrix, init_dim, result);
        }
    }
}

Data FlipudSemantics::run(const DataList &inp, GlobalInfo *global_info) {
#ifdef DEBUG
    check(inp);
#endif
    Matrix matrix = inp[0].getMatrix();
    std::vector<int> new_content(matrix.contents.size());
    std::vector<int> index(matrix.shape.size());
    int sub_size = 1;
    for (int i = 1; i < matrix.shape.size(); ++i) {
        sub_size *= matrix.shape[i];
    }
    int bias = (matrix.shape[0] - 1) * sub_size;
    for (int i = 0; i < matrix.contents.size(); ++i) {
        if (i % sub_size == 0 && i) {
            bias -= (sub_size << 1);
        }
        new_content[bias + i] = matrix.contents[i];
    }
    // flipUD(0, matrix, index, new_content);
    return Data(new MatrixValue(new_content, matrix.shape));
}

WitnessList FlipudSemantics::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
#ifdef DEBUG
    assert(oup.size() == 1);
#endif
    return {{{run(oup, global_info)}}};
}