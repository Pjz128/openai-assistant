//
// Created by Pjz128 on 23-11-17.
//

#ifndef ASSISTANT_OPENAI_CONVERSATION_H
#define ASSISTANT_OPENAI_CONVERSATION_H

#include <condition_variable>
#include <exception>
#include <shared_mutex>

#include "message_queue.h"
#include "openai_helper.h"

class OpenAIConversation {
public:
  explicit OpenAIConversation(std::string assistant_id)
  :assistant_id_(std::move(assistant_id)){}

  bool Create() {
    nlohmann::json res = OpenaiHelper::CreateThread();
    if (!res.is_null()) {
      conversation_id_ = res["id"];
      std::thread(&OpenAIConversation::ProcessRuns, this).detach();
      return true;
    }
    return false;
  }
#if 0 //One message for a run.
  void ProcessRuns() {
    auto callback_newest_chat = [this]() {
      std::string role, content;
      if (NewestLog(role, content))
        if (message_cb_)
          message_cb_(role, content);
        else
          std::cout << "role: " << role << std::endl
                    << "content: " << content << std::endl;
      else
        std::cerr << "chat log get failed" << std::endl;
    };
    try {
      while (true) {
        if (!run_deque_.Empty()) {

          nlohmann::json run_res =
              OpenaiHelper::RetrieveRun(conversation_id_, run_deque_.Front());
          std::string status = run_res["status"];
          if (!status.empty()) {
            std::cout << "current status: " << status << std::endl;

            // TODO: The status include :
            //  queued, in_progress, requires_action, cancelling, cancelled,
            //  failed, completed,or expired, we only check completed and
            //  requires_action status now.

            if (status == "completed") {
              callback_newest_chat();
              run_deque_.FrontPop();
            } else if (status == "requires_action") {
              // some function to continue this run.
              std::cout << "required_action: " << run_res["required_action"]
                        << std::endl;
              auto tool_calls = run_res["required_action"]
                                       ["submit_tool_outputs"]["tool_calls"];

              std::unordered_map<std::string, std::string> tool_outputs_mp;
              for (auto tool_call : tool_calls) {
                std::string id = tool_call["id"];
                std::string type = tool_call["type"];
                std::string arguments = tool_call[type]["arguments"];
                std::string name = tool_call[type]["name"];
                if (action_cb_) {
                  action_cb_(name, arguments);
                }
                tool_outputs_mp[id] = arguments;
              }
              nlohmann::json submit_res = OpenaiHelper::SubmitToolOutputsToRun(
                  conversation_id_, run_deque_.Front(), tool_outputs_mp);
            }
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void Say(const std::string &role, const std::string &content,const std::string &assistant_id) {
    try {
      nlohmann::json msg_res =
          OpenaiHelper::CreateMessage(conversation_id_, role, content);
      if (msg_res.is_null()) {
        std::cerr << "create message failed" << std::endl;
        return;
      }
//        const std::string &assistant_id
      nlohmann::json run_res =
          OpenaiHelper::CreateRun(conversation_id_, assistant_id);
      if (run_res.is_null()) {
        std::cerr << "create run failed" << std::endl;
        return;
      }
      run_deque_.BackPush(run_res["id"]);
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
  }
#endif

  void ProcessRuns() {
    auto callback_newest_chat = [this]() {
      std::string role, content;
      if (NewestLog(role, content))
        if (message_cb_)
          message_cb_(role, content);
        else
          std::cout << "role: " << role << std::endl
                    << "content: " << content << std::endl;
      else
        std::cerr << "chat log get failed" << std::endl;
    };
    try {
      while (true) {
        if (!msg_deque_.Empty()) {
          std::lock_guard<std::recursive_mutex> lock(run_mutex_);
          nlohmann::json run_res = OpenaiHelper::CreateRun(
              conversation_id_, assistant_id_);
          std::string run_id = run_res["id"];
          if (run_res.is_null() || run_id.empty()) {
            std::cerr << "create run failed!" << std::endl;
            continue;
          }
          while (true) {
            run_res = OpenaiHelper::RetrieveRun(conversation_id_, run_id);
            std::string status = run_res["status"];
            if (status.empty()) {
              // TODO:
              break;
            }
            std::cout << "current status: " << status << std::endl;
            // TODO: The status include :
            //  queued, in_progress, requires_action, cancelling, cancelled,
            //  failed, completed,or expired, we only check completed and
            //  requires_action status now.

            if (status == "requires_action") {
              // some function to continue this run.
              std::cout << "required_action: " << run_res["required_action"]
                        << std::endl;
              auto tool_calls = run_res["required_action"]
                                       ["submit_tool_outputs"]["tool_calls"];

              std::unordered_map<std::string, std::string> tool_outputs_mp;
              for (auto tool_call : tool_calls) {
                std::string call_id = tool_call["id"];
                std::string type = tool_call["type"];
                std::string arguments = tool_call[type]["arguments"];
                std::string name = tool_call[type]["name"];
                if (action_cb_) {
                  action_cb_(name, arguments);
                }
                tool_outputs_mp[call_id] = arguments;
              }
              nlohmann::json submit_res = OpenaiHelper::SubmitToolOutputsToRun(
                  conversation_id_, run_id, tool_outputs_mp);
            } else if (status == "queued") {
            } else if (status == "in_progress") {
            } else if (status == "cancelling") {
            } else if (status == "cancelled") {
            } else if (status == "failed") {
              std::cerr << "run status: failed" << std::endl;
              break;
            } else if (status == "expired") {
            } else if (status == "completed") {
              callback_newest_chat();
              msg_deque_.Clear();
              break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
  }

  bool Say(const std::string &role, const std::string &content) {
    try {
      std::lock_guard<std::recursive_mutex> lock(run_mutex_);
      nlohmann::json msg_res =
          OpenaiHelper::CreateMessage(conversation_id_, role, content);
      if (!msg_res.is_null()) {
        msg_deque_.BackPush(std::make_tuple(role, content));
        return true;
      }
      return false;
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
  }

  bool NewestLog(std::string &role, std::string &content) {
    nlohmann::json msgs_res = OpenaiHelper::ListMessage(conversation_id_);
    if (!msgs_res["data"].is_null() && !msgs_res["first_id"].is_null()) {
      role = msgs_res["data"][0]["role"];
      content = msgs_res["data"][0]["content"][0]["text"]["value"];
      return true;
    }
    return false;
  }

  void RegisterMessageCallback(
      std::function<void(const std::string &, const std::string &)> func) {
    message_cb_ = std::move(func);
  }

  void RegisterActionCallback(
      std::function<void(const std::string &, std::string &)> func) {
    action_cb_ = std::move(func);
  }

private:
  std::string conversation_id_;
  std::string assistant_id_;
  std::function<void(const std::string &, const std::string &)>
      message_cb_; // string role,string content
  std::function<void(const std::string &, std::string &)>
      action_cb_; // string name,string output
  Queue<std::tuple<std::string, std::string>>
      msg_deque_; // assistant_id,role,content
  std::recursive_mutex run_mutex_;
};

#endif // ASSISTANT_OPENAI_CONVERSATION_H
