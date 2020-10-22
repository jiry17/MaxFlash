#include "context.h"
#include "util.h"
#include "config.h"

#include <cassert>
#include <fstream>
#include <sstream>
#include <json/json.h>

void ContextInfoMap::setContextInfo(Context* ctx, const ContextInfo &info) {
    std::string context_string = ctx->encodeContext();
#ifdef DEBUG
    assert(pos_map.count(context_string) == 0);
#endif
    pos_map[context_string] = info_list.size();
    info_list.push_back(info);
}

ContextInfo* ContextInfoMap::getContextInfo(Context *ctx) {
    std::string context_string = ctx->encodeContext();
    if (pos_map.count(context_string) == 0) {
        pos_map[context_string] = info_list.size();
        info_list.push_back({});
        return &info_list[info_list.size() - 1];
    }
    int pos = pos_map[context_string];
    return &info_list[pos];
}

#include <iostream>

TopDownContext::TopDownContext(std::string context_string) {
#ifdef DEBUG
    assert(context_string[0] == '{' && context_string[context_string.length() - 1] == '}');
#endif
    size_t len = context_string.length();
    std::vector<int> split_pos = {0};
    for (size_t i = context_string.find(","); i != std::string::npos; i = context_string.find(",", i + 1)) {
        split_pos.push_back(i + 1);
    }
    split_pos.push_back(context_string.length());
    for (int i = 1; i < split_pos.size(); ++i) {
        info.push_back(context_string.substr(split_pos[i-1] + 1, split_pos[i] - split_pos[i-1] - 2));
    }
    depth = info.size();
}

std::string TopDownContext::encodeContext() const {
    std::string res = "{";
    for (int i = 0; i < info.size(); ++i) {
        if (i) res += ", ";
        res += info[i];
    }
    return res + "}";
}

ContextInfoMap::ContextInfoMap(std::string json_file_name) {
    Json::Reader reader;
    Json::Value root;

    std::string json_string = util::loadStringFromFile(json_file_name);
    assert(reader.parse(json_string, root));
    for (auto& context_info_node: root) {
        auto& context_node = context_info_node["context"];
        std::vector<std::string> context_list;
        for (auto& context_name: context_node) {
            context_list.push_back(context_name.asString());
        }
        global::KContextDepth = context_list.size();
        auto* context = new TopDownContext(context_list);
        ContextInfo info_list;
        for (auto& rule: context_info_node["rule"]) {
            std::string term = rule["term"].asString();
            double probability = rule["p"].asDouble();
            info_list.push_back(std::make_pair(term, probability));
        }
        std::sort(info_list.begin(), info_list.end(),
                  [](std::pair<std::string, double> x, std::pair<std::string, double> y){
                      return x.second > y.second;
                  });
        setContextInfo(context, info_list);
    }
}

void ContextInfoMap::print() {
    for (auto& pos_pair: pos_map) {
        std::string context_string = pos_pair.first;
        int pos = pos_pair.second;
        ContextInfo& context_info = info_list[pos];
        printf("%s => ", context_string.c_str());
        for (int i = 0; i < context_info.size(); ++i) {
            if (i) printf(", ");
            printf("%s: %.3lf", context_info[i].first.c_str(), context_info[i].second);
        }
        puts("");
    }
}