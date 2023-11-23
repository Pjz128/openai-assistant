//
// Created by Pjz128 on 23-11-16.
//

#include "include/openai_conversation.h"

int main() {


//  nlohmann::json assistant_res = vwm::sds::OpenaiHelper::CreateAssistant();
//  assert(!assistant_res.is_null());
//  std::string assistant_id = assistant_res["id"];

  std::string assistant_id = "asst_rMvDEFuZ6qOpYn7e8deytNub";
  OpenAIConversation conv(assistant_id);
  assert(conv.Create());
  conv.RegisterMessageCallback(
      [](const std::string &role, const std::string &content) {
        std::cout << "role: " << role << std::endl;
        std::cout << "content: " << content << std::endl;
      });

  conv.RegisterActionCallback(
      [](const std::string &name, std::string &arg) {
        std::cout<<"name: "<<name<<std::endl;
        std::cout<<"arg: "<<arg<<std::endl;
        if(name == "get_weather"){
//            arg = R"({"location":"北京","data_time":1700646642,"is_success":true,"temperature":-10})";
        }
      });

  while (true) {
    std::cout << "UESR:" << std::endl;
    std::string input;
    std::cin >> input;
    if(conv.Say("user", input)){
        std::cout<<"role: "<<"user"<<std::endl;
        std::cout<<"content: "<<input<<std::endl;
    } else
        std::cout<<"message create failed!"<<std::endl;
  }
}
