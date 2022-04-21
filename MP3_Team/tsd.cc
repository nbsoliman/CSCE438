/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <map>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>

#include "sns.grpc.pb.h"
#include "coord.grpc.pb.h"

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
using csce438::ListReply;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;
using csce438::SNSCoord;
using csce438::CoordRequest;
using csce438::CoordReply;
using csce438::CoordMessage;
using csce438::MasterRequest;
using csce438::MasterReply;
using csce438::SyncIdInfo;
using csce438::ClusterInfo;
using grpc::ClientContext;           // Used since server is a client of Coordinator's RPC methods
using grpc::ClientReader; 
using grpc::ClientReaderWriter;
using grpc::ClientWriter;

std::string t = "master";
std::string identifier;

struct Client {
  std::string username;
  bool connected = true;
  int following_file_size = 0;
  time_t last_mod_time;
  int timeline_file_size = 0;
  std::vector<Client*> client_followers;
  std::vector<Client*> client_following;
  ServerReaderWriter<Message, Message>* stream = 0;
  bool operator==(const Client& c1) const{
    return (username == c1.username);
  }
};

//Vector that stores every client that has been created
std::vector<Client> client_db;

//Helper function used to _ a Client object given its username
int find_user(std::string username){
  int index = 0;
  for(Client c : client_db){
    if(c.username == username)
      return index;
    index++;
  }
  return -1;
}

class SNSServiceImpl final : public SNSService::Service {
public:
  std::unique_ptr<SNSService::Stub> slave_stub_;
  
  /*if (t == "master") {
  std::cout << "t is: " << t << std::endl;
  std::string login_info = "localhost:1265";
  std::unique_ptr<SNSService::Stub> slave_stub_ = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
    grpc::CreateChannel(
      login_info, grpc::InsecureChannelCredentials())));
  }*/
private:   
  Status List(ServerContext* context, const Request* request, ListReply* list_reply) override {
    std::cout << "List t is: " << t << std::endl;
    if (t == "master") {
    ClientContext mastercontext;
    Request masterrequest;
    masterrequest.set_username(request->username());
    ListReply masterreply;
    slave_stub_->List(&mastercontext, masterrequest, &masterreply);
    }
    
    /*Client user = client_db[find_user(request->username())];
    int index = 0;
    for(Client c : client_db){
      list_reply->add_all_users(c.username);
    }*/
    
    std::string line1;
    std::ifstream users(t + identifier + "users.txt");
    while (getline(users, line1)) {
    	list_reply->add_all_users(line1);
    }
    
    
    /*
    std::vector<Client*>::const_iterator it;
    for(it = user.client_followers.begin(); it!=user.client_followers.end(); it++){
      list_reply->add_followers((*it)->username);
    }
    */
    
    std::ifstream follower_file(t + request->username() + "followerlist.txt");
    std::string line;
    while (getline(follower_file, line)) {
      list_reply->add_followers(line);
    } // while
    
    return Status::OK;
  }

  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
    std::cout << "Follow t is: " << t << std::endl;
    if (t == "master") {
    ClientContext mastercontext;
    Reply masterreply;
    slave_stub_->Follow(&mastercontext, *request, &masterreply);
    } // if
    
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);
    
    if(username1 == username2) {
      reply->set_msg("Join Failed -- Invalid Username");
    } else{
      std::ifstream in(t + username1 + "followinglist.txt");
      std::string line;
      while (getline(in, line)) {
      	if (line == username2) {
      		reply->set_msg("Join Failed -- Already Following User");
      		return Status::OK;
      	} // if (line == username2)
      } // while
      
      std::ofstream following_file(t + username1 + "followinglist.txt", std::ios::app|std::ios::out|std::ios::in);
      //std::cout << "Attempted to create: " << t << username1 << "followinglist.txt" << std::endl;
      std::string fileinput = username2 + "\n";
      following_file << fileinput;
      following_file.close();

      reply->set_msg("Join Successful");
    } // else
    
    return Status::OK; 
    
    /*
    std::cout << "Follow t is: " << t << std::endl;
    if (t == "master") {
    ClientContext mastercontext;
    Reply masterreply;
    slave_stub_->Follow(&mastercontext, *request, &masterreply);
    }
    
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);
    int join_index = find_user(username2);
    if(join_index < 0 || username1 == username2)
      reply->set_msg("Join Failed -- Invalid Username");
    else{
      Client *user1 = &client_db[find_user(username1)];
      Client *user2 = &client_db[join_index];
      if(std::find(user1->client_following.begin(), user1->client_following.end(), user2) != user1->client_following.end()){
	reply->set_msg("Join Failed -- Already Following User");
        return Status::OK;
      }
      user1->client_following.push_back(user2);
      user2->client_followers.push_back(user1);
      
      std::ofstream following_file(t + username1 + "followinglist.txt", std::ios::app|std::ios::out|std::ios::in);
      //std::cout << "Attempted to create: " << t << username1 << "followinglist.txt" << std::endl;
      std::string fileinput = username2 + "\n";
      following_file << fileinput;
      following_file.close();
      std::ofstream follower_file(t + username2 + "followerlist.txt", std::ios::app|std::ios::out|std::ios::in);
      //std::cout << "Attempted to create: " << t << username2 << "followerlist.txt" << std::endl;
      fileinput = username1 + "\n";
      follower_file << fileinput;
      follower_file.close();
      
      reply->set_msg("Join Successful");
    }
    return Status::OK;  
    */
  }

  Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
    if (t == "master") {
    ClientContext mastercontext;
    Reply masterreply;
    slave_stub_->UnFollow(&mastercontext, *request, &masterreply);
    }
    
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);
    int leave_index = find_user(username2);
    if(leave_index < 0 || username1 == username2)
      reply->set_msg("Leave Failed -- Invalid Username");
    else{
      Client *user1 = &client_db[find_user(username1)];
      Client *user2 = &client_db[leave_index];
      if(std::find(user1->client_following.begin(), user1->client_following.end(), user2) == user1->client_following.end()){
	reply->set_msg("Leave Failed -- Not Following User");
        return Status::OK;
      }
      user1->client_following.erase(find(user1->client_following.begin(), user1->client_following.end(), user2)); 
      user2->client_followers.erase(find(user2->client_followers.begin(), user2->client_followers.end(), user1));
      reply->set_msg("Leave Successful");
    }
    return Status::OK;
  }
  
  Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    if (t == "master") {
    ClientContext mastercontext;
    Reply masterreply;
    slave_stub_->Login(&mastercontext, *request, &masterreply);
    }
    
    Client c;
    std::string username = request->username();
    int user_index = find_user(username);
    if(user_index < 0){
      c.username = username;
      client_db.push_back(c);
      reply->set_msg("Login Successful!");
      std::ofstream cluster_file(t + identifier + "clusterusers.txt",std::ios::app|std::ios::out|std::ios::in);
      cluster_file << username << std::endl;
    }
    else{ 
      Client *user = &client_db[user_index];
      if(user->connected)
        reply->set_msg("Invalid Username");
      else{
        std::string msg = "Welcome Back " + user->username;
	reply->set_msg(msg);
        user->connected = true;
        std::ofstream cluster_file(t + identifier + "clusterusers.txt",std::ios::app|std::ios::out|std::ios::in);
        cluster_file << username << std::endl;
      }
    }
    return Status::OK;
  }

  Status SlaveTimeline(ServerContext* context, 
		ServerReaderWriter<Message, Message>* stream) override {
    std::cout << "Slave Timeline t is: " << t << std::endl;
    Message message;
    Client *c;
    while(stream->Read(&message)) {
      std::cout << "Start of " << t << " timeline while loop" << std::endl;
      std::string username = message.username();
      int user_index = find_user(username);
      c = &client_db[user_index];
 
      //Write the current message to "username.txt"
      std::string filename = t + username+".txt";
      std::ofstream user_file(filename,std::ios::app|std::ios::out|std::ios::in);
      std::ofstream actualtimeline_file(t + username+"actualtimeline.txt",std::ios::app|std::ios::out|std::ios::in);
      google::protobuf::Timestamp temptime = message.timestamp();
      std::string time = google::protobuf::util::TimeUtil::ToString(temptime);
      std::string fileinput = time+" :: "+message.username()+":"+message.msg()+"\n";
      //"Set Stream" is the default message from the client to initialize the stream
      if(message.msg() != "Set Stream") {
        user_file << fileinput;
        actualtimeline_file << fileinput;
        std::string pathname = "./" + t + username + ".txt";
        /*
        struct stat sb;
        if (stat(pathname.c_str(), &sb) != -1) {
        	c->last_mod_time = sb.st_mtime;
        	c->timeline_file_size = c->timeline_file_size + 2;
        }
        */
        std::cout << "At " << pathname << " timeline, the server wrote in: " << fileinput << std::endl; 
      //If message = "Set Stream", print the first 20 chats from the people you follow
      } else{
        if(c->stream==0)
      	  c->stream = stream;
      	std::string pathname = "./" + t + username + ".txt";
      	/*
        struct stat sb;
        if (stat(pathname.c_str(), &sb) != -1) {
        	c->last_mod_time = sb.st_mtime;
        	//c->timeline_file_size = c->timeline_file_size + 2;
        }
        */
        std::string line;
        std::vector<std::string> newest_twenty;
        std::ifstream in(t + username+"actualtimeline.txt");
        int count = 0;
        //Read the last up-to-20 lines (newest 20 messages) from userfollowing.txt
        /*
        while(getline(in, line)){
          if(c->timeline_file_size > 20){
	    if(count < c->timeline_file_size-20){
              count++;
	      continue;
            }
          }
          newest_twenty.push_back(line);
        }
        c->timeline_file_size = count;
        */
        
        while(getline(in, line)){
          count++;
        } // while
        in.close();
        
        std::ifstream in2(t + username+"actualtimeline.txt");
        int count2 = 0;
        while(getline(in2, line)) {
          if(count > 20){
	    if(count2 < count-20){
              count2++;
	      continue;
            }
          }
          newest_twenty.push_back(line);
        } // while
        
        Message new_msg; 
 	//Send the newest messages to the client to be displayed
	for(int i = 0; i<newest_twenty.size(); i++){
	  new_msg.set_msg(newest_twenty[i]);
          stream->Write(new_msg);
          //slave_stream->Write(new_msg);
        }    
        continue;
      }
      
      //if (t == "master") {
      //Send the message to each follower's stream
      //std::vector<Client*>::const_iterator it;
      /*
      for(it = c->client_followers.begin(); it!=c->client_followers.end(); it++){
        Client *temp_client = *it;
      	if(temp_client->stream!=0 && temp_client->connected)
	  temp_client->stream->Write(message);
   
        //For each of the current user's followers, put the message in their following.txt file
        
        std::string temp_username = temp_client->username;
        std::string temp_file = t + temp_username + "following.txt";
	std::ofstream following_file(temp_file,std::ios::app|std::ios::out|std::ios::in);
	following_file << fileinput;
        temp_client->following_file_size++;
	std::ofstream user_file(t + temp_username + ".txt",std::ios::app|std::ios::out|std::ios::in);
        user_file << fileinput;
       
      //}
      }
      */
    } // while
    //If the client disconnected from Chat Mode, set connected to false
    c->connected = false;
    
    std::ofstream cluster_file;
    cluster_file.open(t + identifier + "clusterusers.txt",std::ofstream::out|std::ofstream::trunc);
    cluster_file.close();
    std::ofstream cluster_file2(t + identifier + "clusterusers.txt",std::ios::app|std::ios::out|std::ios::in);
    for (Client c : client_db) {
    	if (c.connected == true) {
    		cluster_file2 << c.username << std::endl; 
    	}
    }
    
    return Status::OK;
  } // SlaveTimeline
  
  Status Timeline(ServerContext* context, 
		ServerReaderWriter<Message, Message>* stream) override {
    std::cout << "Timeline t is: " << t << std::endl;
    ClientContext mastercontext;
    
    std::shared_ptr<ClientReaderWriter<Message, Message>> slave_stream; 
    if (t == "master") {
    	slave_stream = std::shared_ptr<ClientReaderWriter<Message, Message>> (slave_stub_->SlaveTimeline(&mastercontext));
    }
    
    /*std::shared_ptr<ClientReaderWriter<Message, Message>> slave_stream(         // Change to SlaveTimeline
    		slave_stub_->SlaveTimeline(&mastercontext));*/
    //std::cout << t << " process is at 223" << std::endl;	
    Message message;
    Client *c;
    //std::cout << t << " process is at 231" << std::endl;
    while(stream->Read(&message)) {
      std::cout << "Start of " << t << " timeline while loop" << std::endl;
      if (t == "master") {
        Message messageClone;                                            // Without this code, the slave does not get a seg fault, but the master still core dumps
        messageClone.set_username(message.username());
        messageClone.set_msg(message.msg());
        //set timestamp
        google::protobuf::Timestamp* timestamp = new google::protobuf::Timestamp();
        timestamp->set_seconds(message.timestamp().seconds());
        timestamp->set_nanos(0);
        messageClone.set_allocated_timestamp(timestamp);
        //std::cout << t << " process is at 243" << std::endl;
      	slave_stream->Write(messageClone);
      }
      std::string username = message.username();
      int user_index = find_user(username);
      c = &client_db[user_index];
 
      //Write the current message to "username.txt"
      std::string filename = t + username+".txt";
      std::ofstream user_file(filename,std::ios::app|std::ios::out|std::ios::in);
      std::ofstream actualtimeline_file(t + username+"actualtimeline.txt",std::ios::app|std::ios::out|std::ios::in);
      google::protobuf::Timestamp temptime = message.timestamp();
      std::string time = google::protobuf::util::TimeUtil::ToString(temptime);
      std::string fileinput = time+" :: "+message.username()+":"+message.msg()+"\n";
      //"Set Stream" is the default message from the client to initialize the stream
      if(message.msg() != "Set Stream") {
        user_file << fileinput;
        actualtimeline_file << fileinput;
        std::string pathname = "./" + t + username + ".txt";
        /*
        struct stat sb;
        if (stat(pathname.c_str(), &sb) != -1) {
        	c->last_mod_time = sb.st_mtime;
        	c->timeline_file_size = c->timeline_file_size + 2;
        }
        */
        std::cout << "At " << pathname << " timeline, the server wrote in: " << fileinput << std::endl; 
      //If message = "Set Stream", print the first 20 chats from the people you follow
      } else{
        if(c->stream==0)
      	  c->stream = stream;
      	/*
      	std::string pathname = "./" + t + username + ".txt";
        struct stat sb;
        if (stat(pathname.c_str(), &sb) != -1) {
        	c->last_mod_time = sb.st_mtime;
        	//c->timeline_file_size = c->timeline_file_size + 2;
        }
        */
        std::string line;
        std::vector<std::string> newest_twenty;
        std::ifstream in(t + username+"actualtimeline.txt");
        int count = 0;
        //Read the last up-to-20 lines (newest 20 messages) from userfollowing.txt
        /*
        while(getline(in, line)){
          if(c->timeline_file_size > 20){
	    if(count < c->timeline_file_size-20){
              count++;
	      continue;
            }
          }
          newest_twenty.push_back(line);
        } // while
        c->timeline_file_size = count;
        */
        while(getline(in, line)){
          count++;
        } // while
        in.close();
        
        std::ifstream in2(t + username+"actualtimeline.txt");
        int count2 = 0;
        while(getline(in2, line)){
          if(count > 20){
	    if(count2 < count-20){
              count2++;
	      continue;
            }
          }
          newest_twenty.push_back(line);
        } // while
        
        Message new_msg; 
 	//Send the newest messages to the client to be displayed
	for(int i = 0; i<newest_twenty.size(); i++){
	  new_msg.set_msg(newest_twenty[i]);
          stream->Write(new_msg);
          //slave_stream->Write(new_msg);
        }    
        continue;
      }
      
      //if (t == "master") {
      //Send the message to each follower's stream
      //std::vector<Client*>::const_iterator it;
      /*
      for(it = c->client_followers.begin(); it!=c->client_followers.end(); it++){
        Client *temp_client = *it;
      	if(temp_client->stream!=0 && temp_client->connected)
	  temp_client->stream->Write(message);
     
        //For each of the current user's followers, put the message in their following.txt file
        
        std::string temp_username = temp_client->username;
        std::string temp_file = t + temp_username + "following.txt";
	std::ofstream following_file(temp_file,std::ios::app|std::ios::out|std::ios::in);
	following_file << fileinput;
        temp_client->following_file_size++;
	std::ofstream user_file(t + temp_username + ".txt",std::ios::app|std::ios::out|std::ios::in);
        user_file << fileinput;
        
      //}
      }
      */
    } // while
    //If the client disconnected from Chat Mode, set connected to false
    c->connected = false;
    std::ofstream cluster_file;
    cluster_file.open(t + identifier + "clusterusers.txt",std::ofstream::out|std::ofstream::trunc);
    cluster_file.close();
    std::ofstream cluster_file2(t + identifier + "clusterusers.txt",std::ios::app|std::ios::out|std::ios::in);
    for (Client c : client_db) {
    	if (c.connected == true) {
    		cluster_file2 << c.username << std::endl; 
    	}
    }
    
    return Status::OK;
  } // Timeline


};

void RunServer(std::string hostname, std::string cport, std::string port_no, std::string server_id) {
  std::string server_address = "localhost:"+port_no;
  SNSServiceImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  
  if (t == "master") {
  std::cout << "t is: " << t << std::endl;
  std::string login_info = hostname + ":" + cport;
  std::unique_ptr<SNSCoord::Stub> coord_stub_ = std::unique_ptr<SNSCoord::Stub>(SNSCoord::NewStub(
    grpc::CreateChannel(
      login_info, grpc::InsecureChannelCredentials())));
  ClientContext context;
  MasterRequest request;
  request.set_server_id(server_id);
  MasterReply reply;
  coord_stub_->GetSlaveInfo(&context, request, &reply);
  while (reply.slave_ip() == "") {
  	ClientContext context2;
  	coord_stub_->GetSlaveInfo(&context2, request, &reply);
  } // while
  
  login_info = reply.slave_ip() + ":" + reply.slave_port();
  std::cout << "At line 423: " << login_info << std::endl;
  service.slave_stub_ = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
    grpc::CreateChannel(
      login_info, grpc::InsecureChannelCredentials())));
  } // if (t == "master")
  
  std::cout << "At line 428" << std::endl;
  
  server->Wait();
}

void RunClient(std::string ip, std::string port){
  std::string login_info = ip + ":" + port;
  /*stub_ = std::unique_ptr<SNSService::Stub>(SNSService::NewStub(
    grpc::CreateChannel(
      login_info, grpc::InsecureChannelCredentials()))); */
}

void StreamHeartbeat(std::string hostname, std::string cport, std::string type, std::string port){
   std::cout << "Heartbeat: Initial" << std::endl;
  std::string login_info = hostname + ":" + cport;
  std::unique_ptr<SNSCoord::Stub> coord_stub_ = std::unique_ptr<SNSCoord::Stub>(SNSCoord::NewStub(
    grpc::CreateChannel(
      login_info, grpc::InsecureChannelCredentials())));
  
  ClientContext context;
  std::shared_ptr<ClientReaderWriter<CoordMessage, CoordMessage>> stream(
        coord_stub_->Heartbeat(&context));
  std::cout << "Heartbeat: Before While" << std::endl;
  while (true) {
    std::cout << hostname << std::endl;
    CoordMessage m;
    m.set_server_ip("localhost");
    m.set_server_port2(port);
    m.set_status("Active");
    m.set_server_type(type);
    stream->Write(m);
    std::cout << "Heartbeat" << std::endl;
    sleep(10);
  }
} // StreamHeartbeat

void PeriodicCheck(std::string hostname, std::string cport, std::string server_id, std::string type) {
  std::string login_info = hostname + ":" + cport;
  std::unique_ptr<SNSCoord::Stub> coord_stub_ = std::unique_ptr<SNSCoord::Stub>(SNSCoord::NewStub(
    grpc::CreateChannel(
      login_info, grpc::InsecureChannelCredentials())));
  while (true) {
  sleep(30);
  SyncIdInfo syncinfo;
  ClientContext context1;
  ClusterInfo clusterinfo;
  syncinfo.set_sync_id(server_id);
  coord_stub_->GetClusterInfo(&context1, syncinfo, &clusterinfo);
  
  for (int i = 0; i < clusterinfo.cluster_clients().size(); i++) {
  	std::string username = clusterinfo.cluster_clients(i);
  	int user_index = find_user(username);
  	if (user_index < 0) {
  		std::cout << "Error in Periodic Check: Can't find client object" << std::endl;
  	} else {
  		Client *user = &client_db[user_index];
  		std::string pathname = "./" + type + username + "following.txt";
  		struct stat sb;
  		if (stat(pathname.c_str(), &sb) != -1) {
  			if (sb.st_mtime != user->last_mod_time) {
  				user->last_mod_time = sb.st_mtime;
  				std::ifstream in(type + username + "following.txt");
  				int count = 0;
  				std::string line;
  				std::cout << "At Periodic Check second if, Cluster Client: " << username << " following file lines: " << user->following_file_size << std::endl;
  				while (getline(in, line)) {
  					count++;
  					if (count > user->following_file_size) {
  						// Call sync_stubs and put into their streams
  						//newTimelinePosts.push_back(line);
  						if (user->stream != 0) {
  						Message message;
  						message.set_msg(line);
  						std::cout << "here at Periodic Check" << std::endl;
  						user->stream->Write(message);
  						}
  						//std::ofstream user_file(type + username + "actualtimeline.txt",std::ios::app|std::ios::out|std::ios::in);
  						//user_file << line << "\n";
  						std::cout << "At Periodic Check second if, it read in " << line << std::endl;
  					} // if
  				} // getline
  				user->following_file_size = count;
  				std::cout << "At Periodic Check second if, Changed Cluster Client: " << username << " following file lines: " << user->following_file_size << std::endl;
  			} // if (sb.st_mtime != user->last_mod_time)
  		} // if (stat(pathname.c_str(), &sb) != -1) 
  	} // else
  } // for
  } // while
}

int main(int argc, char** argv) {
  
  std::string hostname = "localhost";
  std::string username = "default";
  std::string port = "3010";
  std::string server_id = "0";
  std::string cport = "5050";

  int opt = 0;
  while ((opt = getopt(argc, argv, "h:c:p:i:t:")) != -1){
      switch(opt) {
          case 'h': //Coordinator IP
              hostname = optarg;break;
          case 'c': //Coordinator Port
              cport = optarg;break;
          case 'p': //Port
              port = optarg;break;
          case 'i': //server id
              server_id = optarg;break;
          case 't': //server type (master or slave)
              t = optarg;break;
          default:
              std::cerr << "Invalid Command Line Argument\n";
      }
  }
  
  identifier = server_id;
  
  //thread 1
  std::thread thread_one(StreamHeartbeat, hostname, cport, t, port);
  
  //thread 2
  //if (t == "master"){
    std::thread thread_two(RunServer, hostname, cport, port, server_id);
    
  //thread 3
  std::thread thread_three(PeriodicCheck, hostname, cport, server_id, t);
  //}else if (t == "slave"){
  //  std::thread thread_two(RunClient, port);
  //} else {
  //  std::cerr << "Invalid Type\n";
  //}
  thread_one.join();
  thread_two.join();
  thread_three.join();

  return 0;
}
