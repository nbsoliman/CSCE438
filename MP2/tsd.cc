#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <list>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
// #include <jsoncpp/json/json.h>
#include "json.hpp"

#include "sns.grpc.pb.h"

using json = nlohmann::json;
using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using csce438::Message;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;

class SNSServiceImpl final : public SNSService::Service {
  Status List(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // LIST request from the user. Ensure that both the fields
    // all_users & following_users are populated
    // ------------------------------------------------------------
    // std::cout << "List Function" << std::endl;
    // reply->set_msg("hello");
    
    
    // Sends All User data
    std::list<std::string> all = userData["All_Users"];
    int num_users = 0;
    for (auto it = all.begin(); it != all.end(); ++it){
      num_users = num_users + 1;
    }
    reply->add_all_users(std::to_string(num_users));
    for (auto it = all.begin(); it != all.end(); ++it){
      reply->add_all_users(*it);
    }
    
    // Sends Following User Data
    std::list<std::string> foll = userData[request->username()]["Followed_Users"];
    // std::cout << "FOLL:" << foll << std::endl;
    int num_fusers = 0;
    for (auto iter = foll.begin(); iter != foll.end(); ++iter){
      num_fusers = num_fusers + 1;
    }
    // std::cout << "Fusers:" << num_fusers << std::endl;
    reply->add_following_users(std::to_string(num_fusers));
    if (num_fusers > 0){
      for (auto it = foll.begin(); it != foll.end(); ++it){
        reply->add_following_users(*it);
      } 
    }
    
    
    // std::cout << userData[request->username()]["Followed_Users"] << std::endl;
    // reply->add_following_users(userData[request->username()]["Followed_Users"]);
    // reply->add_all_users(userData["All_Users"]);
    return Status::OK;
  }

  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // request from a user to follow one of the existing
    // users
    // ------------------------------------------------------------
    std::ifstream i("userdata.json");
    i >> userData;
    
    int isUserInList = 0;
    for (json::iterator it = userData[request->arguments()[0]]["Followed_Users"].begin(); it != userData[request->arguments()[0]]["Followed_Users"].end(); ++it) {
      if (request->username() == *it){
        isUserInList = 1;
      }
    }
    
    int isUserInAll = 0;
    for (json::iterator it = userData["All_Users"].begin(); it != userData["All_Users"].end(); ++it) {
      if (userData[request->arguments()[0]]["Followed_Users"][0] == *it){
        isUserInAll = 1;
      }
    }
    
    if (isUserInAll == 0){
      reply->set_msg("1"); //no user
    } else if (request->username() == request->arguments()[0]){
        reply->set_msg("2"); //follow yourself
    } else if (isUserInList==1){
        reply->set_msg("3"); //already follow
    } else {
        userData[request->arguments()[0]]["Followed_Users"].push_back(request->username());
    }
    
    //writes to json
    std::ofstream o("userdata.json");
    o << std::setw(4) << userData << std::endl;
    return Status::OK; 
  }

  Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // request from a user to unfollow one of his/her existing
    // followers
    // ------------------------------------------------------------
    std::ifstream i("userdata.json");
    i >> userData;
    
    //finds user in list
    int isUserInList = 0;
    int userIndex =0;
    for (json::iterator it = userData[request->arguments()[0]]["Followed_Users"].begin(); it != userData[request->arguments()[0]]["Followed_Users"].end(); ++it) {
      if (request->username() == *it){
        isUserInList = 1;
        break;
      }
      userIndex = userIndex + 1;
    }
    
    
    if (request->username() == request->arguments()[0]){
        reply->set_msg("You cannot unfollow yourself."); //unfollow yourself
    } else if (isUserInList == 1){
        std::list<std::string> foll = userData[request->arguments()[0]]["Followed_Users"];
        foll.remove(request->username());
        userData[request->arguments()[0]]["Followed_Users"] = foll;
    } else {
        reply->set_msg("That user does not exist."); //not a user
    }
    
    //writes to json
    std::ofstream o("userdata.json");
    o << std::setw(4) << userData << std::endl;
    return Status::OK;
  }
  
  Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // a new user and verify if the username is available
    // or already taken
    // ------------------------------------------------------------
    //reads from json
    std::ifstream i("userdata.json");
    i >> userData;
    
    int isUserInList = 0;
    for (json::iterator it = userData["All_Users"].begin(); it != userData["All_Users"].end(); ++it) {
      if (request->username() == *it){
        isUserInList = 1;
      }
    }
    
    //if it is a new user
    if (isUserInList == 0){
      userData["All_Users"].push_back(request->username());
      userData[request->username()]["Username"] = request->username();
      userData[request->username()]["Followed_Users"] = {request->username()};
      userData[request->username()]["Timeline"] = {};
      reply->set_msg("Username is Available!");
    } else {
      reply->set_msg("Username is Taken...");
    }
    
    //writes to json
    std::ofstream o("userdata.json");
    o << std::setw(4) << userData << std::endl;
    return Status::OK;
  }

  Status Timeline(ServerContext* context, ServerReaderWriter<Message, Message>* stream) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // receiving a message/post from a user, recording it in a file
    // and then making it available on his/her follower's streams
    // ------------------------------------------------------------
    std::ifstream i("userdata.json");
    i >> userData;
    
    Message client_message;
    // std::string full_timeline;
    // std::string iter;
    // for (int i = 0; i < userData[user]["Timeline"].size(); i++){
    //   iter = userData[user]["Timeline"][i];
    //   full_timeline = full_timeline + iter;
    // }
    int ctr = 0;
    while (stream->Read(&client_message)){
      std::string user = client_message.username();
      std::unique_lock<std::mutex> lock(mu_);
      std::string message =client_message.msg();
      // std::cout << "Message: " << message << std::endl;
      
      std::string user_iter;
      for (json::iterator it = userData[user]["Followed_Users"].begin(); it != userData[user]["Followed_Users"].end(); ++it) {
        user_iter = *it;
        userData[user_iter]["Timeline"].push_back(user + ": " + message);
      }
      std::ofstream o("userdata.json");
      o << std::setw(4) << userData << std::endl;
      
      std::string full_timeline;
      std::string iter;
      if (ctr == 0){
        for (int i = 0; i < userData[user]["Timeline"].size(); i++){
          iter = userData[user]["Timeline"][i];
          full_timeline = full_timeline + iter;
          ctr = 0;
        }
      }
      client_message.set_msg(full_timeline);
      
      struct timeval tv;
      gettimeofday(&tv, NULL);
      ::google::protobuf::Timestamp* timestamp = new ::google::protobuf::Timestamp();
      timestamp->set_seconds(tv.tv_sec);
      timestamp->set_nanos(tv.tv_usec * 1000);
      client_message.set_allocated_timestamp(timestamp);
      stream->Write(client_message);
    }
    
    Message server_message;
    
    
    // std::string person_posting = stream->username();
    // std::list<std::string> followed_users = userData[stream->username()]["Followed_Users"];
    // int num_following_users = userData[stream->username()]["Followed_Users"].size();
    // for (int i = 0; i < num_following_users; i++){
    //     std::string person_receiving_message = followed_users[i];
    //     userData[person_receiving_message]["Timeline"].push_back(message);
    // }
    //writes to json
    std::ofstream o("userdata.json");
    o << std::setw(4) << userData << std::endl;
    return Status::OK;
  }
  
  private:
    json userData;
    std::mutex mu_;
};

void RunServer(std::string port_no) {
  std::string host = "0.0.0.0:" + port_no;
  std::string server_address(host);
  SNSServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
  // ------------------------------------------------------------
  // In this function, you are to write code 
  // which would start the server, make it listen on a particular
  // port number.
  // ------------------------------------------------------------
}

int main(int argc, char** argv) {
  
  std::string port = "3010";
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;
          break;
      default:
	         std::cerr << "Invalid Command Line Argument\n";
    }
  }
  RunServer(port);
  return 0;
}
