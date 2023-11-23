//
// Created by Pjz128 on 23-11-20.
//

#ifndef ASSISTANT_OPPENAI_CONFIG_H
#define ASSISTANT_OPPENAI_CONFIG_H

#include <fstream>
#include <utility>

#include "nlohmann/json.hpp"

class OpenAIConfig {
public:
  explicit OpenAIConfig(std::string base_path)
      : base_path_(std::move(base_path)) {}

  bool LoadConfigAsJson(const std::string &file_name,
                        nlohmann::json &config_data) {
    std::ifstream config_file(joinPaths(base_path_, file_name));
    if (!config_file.is_open()) {
      return false;
    }
    config_file >> config_data;
    return true;
  }

  bool LoadConfigByKey(const std::string &file_name, const std::string &key,
                       std::string &value) {
    nlohmann::json config_data;
    if (!LoadConfigAsJson(file_name, config_data))
      return false;
    if (config_data[key].is_null())
      return false;
    value = config_data[key];
    return true;
  }

private:
  std::string joinPaths(const std::string &path1,
                        const std::string &path2) {
    if (path1.empty())
      return path2;
    if (path2.empty())
      return path1;
    bool hasSlashAtEnd1 = (path1.back() == '/');
    bool hasSlashAtStart2 = (path2.front() == '/');
    if (hasSlashAtEnd1 && hasSlashAtStart2) {
      return path1 + path2.substr(1);
    } else if (!hasSlashAtEnd1 && !hasSlashAtStart2) {
      return path1 + '/' + path2;
    }
    return path1 + path2;
  }

private:
  std::string base_path_;
};

#endif // ASSISTANT_OPPENAI_CONFIG_H
