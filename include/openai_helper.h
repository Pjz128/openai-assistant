//
// Created by Pjz128 on 23-11-16.
//

#ifndef ASSISTANT_OPENAI_HELPER_H
#define ASSISTANT_OPENAI_HELPER_H

#include <exception>
#include <utility>

#include "httplib/httplib.h"
#include "nlohmann/json.hpp"
#include "openai_config.h"

using namespace httplib;

static std::string content_type_ = "application/json";
static std::shared_ptr<Client> client_;
static Headers headers_;

static nlohmann::json assistant_data_;
static std::mutex global_message_mutex_;
static std::mutex global_run_mutex_;

class OpenaiHelper {
public:
  OpenaiHelper() {
    // Load config from config files.
    OpenAIConfig config_loader("./configs");

    std::string base_url, api_key, openai_beta;
    assert(config_loader.LoadConfigByKey("client.json", "url", base_url) &&
           config_loader.LoadConfigByKey("client.json", "authorization",
                                         api_key) &&
           config_loader.LoadConfigByKey("client.json", "openai_beta",
                                         openai_beta) &&
           config_loader.LoadConfigAsJson("assistant.json", assistant_data_));
    headers_ = {{"Authorization", api_key}, {"OpenAI-Beta", openai_beta}};
    // Initialize client.
    client_ = std::make_unique<Client>(base_url);
    client_->set_follow_location(true);
  }

//  static std::shared_ptr<Client> GetClient(){
//      std::lock_guard<std::mutex> lock(global_client_mutex_);
//      return client_;
//  }
//  static Headers GetHeaders(){
//      std::lock_guard<std::mutex> lock(global_headers_mutex_);
//      return headers_;
//  }

public:
  class Thread {
  private:
    std::string url_thread = "/v1/threads/";

  public:
    Result CreateThread() {
      return client_->Post(url_thread, headers_, "", 0, content_type_);
    }
    Result RetrievesThread(const std::string &thread_id) {
      return client_->Get(url_thread + thread_id, headers_);
    }
    Result ModifiesThread(const std::string &thread_id,
                          const std::string &body) {
      return client_->Post(url_thread + thread_id, headers_, body.c_str(),
                           body.size(), content_type_);
    }

    Result DeleteThread(const std::string &thread_id) {
      return client_->Delete(url_thread + thread_id, headers_);
    }

    Result ListMessages(const std::string &thread_id) {
      return client_->Get(url_thread + thread_id + "/messages", headers_);
    }
  };

  class Assistant {
  private:
    std::string url_assistant = "/v1/assistants/";

  public:
    httplib::Result CreateAssistant(const std::string &body) {
      return client_->Post(url_assistant, headers_, body.c_str(), body.size(),
                           content_type_);
    }
    Result RetrievesAssistant(const std::string &assistant_id) {
      return client_->Get(url_assistant + assistant_id, headers_);
    }
    Result ModifiesAssistant(const std::string &assistant_id,
                             const std::string &body) {
      return client_->Post(url_assistant + assistant_id, headers_, body.c_str(),
                           body.size(), content_type_);
    }
    Result DeleteAssistant(const std::string &assistant_id) {
      return client_->Delete(url_assistant + assistant_id, headers_);
    }
    Result ListAssistant(int limit = 20, const std::string &order = "desc") {
      return client_->Get(url_assistant + "?order=" + order +
                              "&limit=" + std::to_string(limit),
                          headers_);
    }
  };

  class Message {
  private:
    std::string thread_id_;
    std::string message_path_;

  public:
    explicit Message(std::string thread_id) : thread_id_(std::move(thread_id)) {
      message_path_ = "/v1/threads/" + thread_id_ + "/messages/";
    }

    Result CreateMessage(const std::string &body) {
      std::lock_guard<std::mutex> lock(global_message_mutex_);
      return client_->Post(message_path_, headers_, body.c_str(), body.size(),
                           content_type_);
    }
    Result RetrievesMessage(const std::string &message_id) {
      return client_->Get(message_path_ + message_id, headers_);
    }
    Result ModifiesMessage(const std::string &message_id,
                           const std::string &body) {
      return client_->Post(message_path_ + message_id, headers_, body.c_str(),
                           body.size(), content_type_);
    }
    Result ListMessages() { return client_->Get(message_path_, headers_); }
  };

  class Run {
  private:
    std::string url_thread = "/v1/threads/";
    std::string thread_id_;
    std::string run_path_;

  public:
    explicit Run(std::string thread_id) : thread_id_(std::move(thread_id)) {
      run_path_ = url_thread + thread_id_ + "/runs/";
    }

    httplib::Result CreateRun(const std::string &body) {
      std::lock_guard<std::mutex> lock(global_run_mutex_);
      return client_->Post(run_path_, headers_, body.c_str(), body.size(),
                           content_type_);
    }
    Result CancelRun(const std::string &run_id) {
      return client_->Get(run_path_ + run_id + "cancel", headers_);
    }
    Result ModifiesRun(const std::string &run_id, const std::string &body) {
      return client_->Post(run_path_ + run_id, headers_, body.c_str(),
                           body.size(), content_type_);
    }
    Result ListRuns() { return client_->Get(run_path_, headers_); }
    Result RetrieveRun(const std::string &run_id) {
      return client_->Get(run_path_ + run_id, headers_);
    }
    Result SubmitToolOutputsToRun(const std::string &run_id,
                                  const std::string &body) {
      return client_->Post(run_path_ + run_id + "/submit_tool_outputs",
                           headers_, body.c_str(), body.size(), content_type_);
    }
  };

public:
  static nlohmann::json CreateAssistant() {
    Result res = Assistant().CreateAssistant(assistant_data_.dump());
    if (res->status == 200) {
      return nlohmann::json::parse(res->body);
    }
    return {};
  }

  static nlohmann::json CreateThread() {
    Result res = Thread().CreateThread();
    if (res->status == 200) {
      return nlohmann::json::parse(res->body);
    }
    return {};
  }

  static nlohmann::json CreateMessage(const std::string &thread_id,
                                      const std::string &role,
                                      const std::string &content) {
    nlohmann::json json_data;
    json_data["role"] = role;
    json_data["content"] = content;
    Result res = Message(thread_id).CreateMessage(json_data.dump());
    if (res->status == 200) {
      return nlohmann::json::parse(res->body);
    }
    return {};
  }

  static nlohmann::json CreateRun(const std::string &thread_id,
                                  const std::string &assistant_id) {
    nlohmann::json json_data;
    json_data["assistant_id"] = assistant_id;
    Result res = Run(thread_id).CreateRun(json_data.dump());
    if (res->status == 200) {
      return nlohmann::json::parse(res->body);
    }
    return {};
  }

  static nlohmann::json RetrieveRun(const std::string &thread_id,
                                    const std::string &run_id) {
    Result res = Run(thread_id).RetrieveRun(run_id);
    if (res->status == 200) {
      return nlohmann::json::parse(res->body);
    }
    return {};
  }

  static nlohmann::json ListMessage(const std::string &thread_id) {
    Result res = Message(thread_id).ListMessages();
    if (res->status == 200) {
      return nlohmann::json::parse(res->body);
    }
    return {};
  }

  static nlohmann::json SubmitToolOutputsToRun(const std::string &thread_id,
                                               const std::string &run_id,
                                               const std::unordered_map<std::string,std::string>  &id_output_mp) {

      nlohmann::json tool_outputs = nlohmann::json::array();
      for(const auto& id_output: id_output_mp){
          nlohmann::json json_obj;
          json_obj["tool_call_id"] = id_output.first;
          json_obj["output"] = id_output.second;
          tool_outputs.push_back(json_obj);
      }
      nlohmann::json  json_body;
      json_body["tool_outputs"]= tool_outputs;

      Result res = Run(thread_id).SubmitToolOutputsToRun(run_id,json_body.dump());
      if (res->status == 200) {
          return nlohmann::json::parse(res->body);
      }
      return {};
  }
};

static OpenaiHelper openaiHelper;

#endif // ASSISTANT_OPENAI_HELPER_H
