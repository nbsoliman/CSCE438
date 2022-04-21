#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>

#include "sns.grpc.pb.h"
#include "synchronizer.grpc.pb.h"
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
using grpc::ClientContext; 
using grpc::ClientReader; 
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using csce438::SNSSync;
using csce438::FollowRequest;
using csce438::FollowReply;
using csce438::SyncMessage;
using csce438::SNSService;
using csce438::SNSCoord;
using csce438::ClientInfo;
using csce438::FollowerSyncInfo;
using csce438::FollowerSyncRTInfo;
using csce438::SyncIdInfo;
using csce438::ClusterInfo;

std::unique_ptr<SNSSync::Stub> syncOne_stub_;
std::unique_ptr<SNSSync::Stub> syncTwo_stub_;

//Struct that stores relevant client information about their timeline files
struct Client {
  std::string username;
  int following_file_size;
  time_t last_mod_time;
  std::vector<std::string> newPosts;
  std::vector<std::string> followers;
  bool operator==(const Client& c1) const{
    return (username == c1.username);
  }
};

//Struct that stores relevant client information about their following files
struct ClientV2 {
  std::string username;
  int following_file_lines;
  time_t last_mod_time;
  std::vector<std::string> newFollowing;
};

//Vector that stores every client that has been created for timeline checks
std::vector<Client> client_db;

//Vector that stores every client that has been created for following checks
std::vector<ClientV2> client_db_v2;

//Helper function used to find a Client object given its username
int find_user(std::string username){
  int index = 0;
  for(Client c : client_db){
    if(c.username == username)
      return index;
    index++;
  }
  return -1;
}

//Helper function used to find a ClientV2 object given its username
int find_user_v2(std::string username){
  int index = 0;
  for(ClientV2 c : client_db_v2){
    if(c.username == username)
      return index;
    index++;
  }
  return -1;
}

class SNSSyncImpl final : public SNSSync::Service {
/*
public:
	std::unique_ptr<SNSSync::Stub> syncOne_stub_;
	std::unique_ptr<SNSSync::Stub> syncTwo_stub_;
	
private:
*/
	Status InformFollow(ServerContext* context, const FollowRequest* request, FollowReply* reply) override {
		// Find which .txt file to access. The files will be named type + id + follower.txt. These files will differentiated by master/slave tag in their name. We need to find out which of the two to access
			// ANSWER: We can just write to both slave and master id + follower.txt files. Code will probably be easy
		// Output relavent info
		
		std::string user = request->username();
		std::string follower = request->follower();
		std::ofstream masterfile("master" + user + "followerlist.txt", std::ios::app|std::ios::out|std::ios::in);
		std::string fileinput = follower + "\n";
		masterfile << fileinput;
		masterfile.close();
		std::ofstream slavefile("slave" + user + "followerlist.txt", std::ios::app|std::ios::out|std::ios::in);
		slavefile << fileinput;
		slavefile.close();
		
		return Status::OK;
	}
	
	Status InformTimeline(ServerContext* context, ServerReaderWriter<SyncMessage, SyncMessage>* stream) override {
		/*
		for each follower:
			write the message to their associated timeline, which is identified by type + id.txt. Write to both master and slave .txt files.
		*/
			// To implement the for loop, we need a follower vector in SyncMessage payload. So when you do a InformTimeline RPC call, you must find the followers of client, along with the client's new timeline posts. Each of the posts (SyncMessage) will have that vector in repeated string followers
			
		SyncMessage message; 
		while (stream->Read(&message)) {
			for (int i = 0; i < message.followers().size(); i++) {
				/*
				std::ofstream outputfile1("master" + message.followers(i) + ".txt", std::ios::app|std::ios::out|std::ios::in);
				outputfile1 << message.msg() << "\n";
				outputfile1.close();
				
				std::ofstream outputfile2("slave" + message.followers(i) + ".txt", std::ios::app|std::ios::out|std::ios::in);
				outputfile2 << message.msg() << "\n";
				outputfile2.close();
				*/
				
				std::ofstream outputfile3("master" + message.followers(i) + "following.txt", std::ios::app|std::ios::out|std::ios::in);
				outputfile3 << message.msg() << "\n";
				outputfile3.close();
				
				std::ofstream outputfile4("slave" + message.followers(i) + "following.txt", std::ios::app|std::ios::out|std::ios::in);
				outputfile4 << message.msg() << "\n";
				outputfile4.close();
				
				std::ofstream outputfile5("master" + message.followers(i) + "actualtimeline.txt", std::ios::app|std::ios::out|std::ios::in);
				outputfile5 << message.msg() << "\n";
				outputfile5.close();
				
				std::ofstream outputfile6("slave" + message.followers(i) + "actualtimeline.txt", std::ios::app|std::ios::out|std::ios::in);
				outputfile6 << message.msg() << "\n";
				outputfile6.close();
				
				std::cout << message.username() << "sent " << message.msg() << " to " << message.followers(i) << std::endl;
				
				int user_index = find_user(message.followers(i));
				if (user_index < 0) {
					std::cout << "Error: At Inform Timeline, Client object not stored in cluster" << std::endl;
				} else {
					//Client *follower_client = &client_db[user_index];
					//follower_client->following_file_size++;
					/*
					std::string pathname = "./master" + message.followers(i) + ".txt";
					struct stat sb;
					if (stat(pathname.c_str(), &sb) != -1) {
						follower_client->last_mod_time = sb.st_mtime;
					} else {
						std::cout << "Error: At Inform Timeline, File not found" << std::endl;
					}
					*/
				} // else
			} // for
		} // while
		
		// Change the two fields of followingfilesize and mod_time in the client-db
		// Review AddClientToDB method
		// Clear newPosts and followers in PeriodicCheck
		return Status::OK;
	}
};

void RunServer(std::string hostname, std::string cport, std::string port, std::string server_id) {
  std::string server_address = "0.0.0.0:"+port;
  SNSSyncImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  
  std::cout<< "On line 97" << std::endl;
  std::string login_info = hostname + ":" + cport;
  std::unique_ptr<SNSCoord::Stub> coord_stub_ = std::unique_ptr<SNSCoord::Stub>(SNSCoord::NewStub(
    grpc::CreateChannel(
      login_info, grpc::InsecureChannelCredentials())));
  
  std::cout<< "On line 103" << std::endl;
  ClientContext context1;  
  FollowerSyncRTInfo rtInfo;
  SyncIdInfo idInfo;
  rtInfo.set_sync_ip_address("localhost");
  rtInfo.set_sync_port(port);
  rtInfo.set_sync_id(server_id);
  coord_stub_->FillFollowerSyncRT(&context1, rtInfo, &idInfo);
  
  std::cout<< "On line 112" << std::endl;
  ClientInfo message;
  FollowerSyncInfo syncInfo;
  std::string sync_one;
  std::string sync_two;
  
  if (server_id == "1") {
  	sync_one = "2";
  	sync_two = "3";
  } else if (server_id == "2") {
  	sync_one = "1";
  	sync_two = "3";
  } else if (server_id == "3") {
  	sync_one = "1";
  	sync_two = "2";
  }
  
  std::cout<< "On line 129" << std::endl;
  	ClientContext context2; 
  	message.set_client_id2(sync_one);
  	coord_stub_->GetFollowerSyncInfo(&context2, message, &syncInfo);
  	std::cout<< "On line 129 + 3" << std::endl;
	while ((syncInfo.sync_ip_address() == "") && (syncInfo.sync_port() == "")) {
		ClientContext context3;
		coord_stub_->GetFollowerSyncInfo(&context3, message, &syncInfo);
	}
	std::cout<< "On line 129 + 3 + 3" << std::endl;
	login_info = syncInfo.sync_ip_address() + ":" + syncInfo.sync_port();
	std::cout << "In Follower Synchronizer's Run Server: " << login_info << std::endl;
  	syncOne_stub_ = std::unique_ptr<SNSSync::Stub>(SNSSync::NewStub(
    grpc::CreateChannel(
      login_info, grpc::InsecureChannelCredentials())));
	
	ClientContext context4;
  	message.set_client_id2(sync_two);
  	coord_stub_->GetFollowerSyncInfo(&context4, message, &syncInfo);
	while ((syncInfo.sync_ip_address() == "") && (syncInfo.sync_port() == "")) {
		ClientContext context5;
		coord_stub_->GetFollowerSyncInfo(&context5, message, &syncInfo);
	}
	login_info = syncInfo.sync_ip_address() + ":" + syncInfo.sync_port();
	std::cout << "In Follower Synchronizer's Run Server: " << login_info << std::endl;
  	syncTwo_stub_ = std::unique_ptr<SNSSync::Stub>(SNSSync::NewStub(
    grpc::CreateChannel(
      login_info, grpc::InsecureChannelCredentials())));
      
  server->Wait();
}

void AddToClientDB(std::string username) {
	Client c;
	c.username = username;
  	std::string pathname = "./master" + username + ".txt";
  	struct stat sb;
 	if (stat(pathname.c_str(), &sb) != -1) {
  		c.last_mod_time = sb.st_mtime;
  		std::ifstream in("master" + username + ".txt");
  		int count = 0;
  		std::string line;
  		while (getline(in, line)) {
  			count++;
 		} // getline
  		c.following_file_size = count;
  	} else {
  		c.following_file_size = 0;
  	}
  	std::cout << "At AddToClientDB, Cluster Client: " << username << " following file size: " << c.following_file_size << std::endl;
  	client_db.push_back(c);
} // AddToClientDB

void PeriodicFollowingCheck(std::string hostname, std::string cport, std::string server_id) {
  sleep(30);
  /*
  ClientContext context1;
  std::shared_ptr<ClientReaderWriter<SyncMessage, SyncMessage>> syncOne_stream(
        syncOne_stub_->InformTimeline(&context1));
  ClientContext context2;
  std::shared_ptr<ClientReaderWriter<SyncMessage, SyncMessage>> syncTwo_stream(
        syncTwo_stub_->InformTimeline(&context2));
  */
        
  std::string login_info = hostname + ":" + cport;
  std::unique_ptr<SNSCoord::Stub> coord_stub_ = std::unique_ptr<SNSCoord::Stub>(SNSCoord::NewStub(
    grpc::CreateChannel(
      login_info, grpc::InsecureChannelCredentials())));
  
  int iterationCount = 0;
  while (true) {
  iterationCount++;
  std::cout << "Iteration Count of PeriodicFollowingCheck: " << iterationCount << std::endl;
  SyncIdInfo syncinfo;
  ClientContext context1;
  ClusterInfo clusterinfo;
  syncinfo.set_sync_id(server_id);
  coord_stub_->GetClusterInfo(&context1, syncinfo, &clusterinfo);
  
  for (int i = 0; i < clusterinfo.cluster_clients().size(); i++) {
  	std::string username = clusterinfo.cluster_clients(i);
  	std::cout << "At PeriodicFollowingCheck, First For Loop Cluster Client: " << username << std::endl;
  	
  	int user_index = find_user_v2(username);
  	if (user_index < 0) {
  		ClientV2 c;
  		c.username = username;
  		std::string pathname = "./slave" + username + "followinglist.txt";
  		struct stat sb;
  		if (stat(pathname.c_str(), &sb) != -1) {
  			c.last_mod_time = sb.st_mtime;
  			std::ifstream in("slave" + username + "followinglist.txt");
  			int count = 0;
  			std::string line;
  			while (getline(in, line)) {
  				c.newFollowing.push_back(line);
  				std::cout << "At PeriodicFollowingCheck first if, it read in " << username << "is following" << line << std::endl;
  				count++;
  			} // getline
  			c.following_file_lines = count;
  			std::cout << "At PeriodicFollowingCheck first if, Cluster Client: " << username << " following file lines: " << c.following_file_lines << std::endl;
  		} else {
  			c.following_file_lines = 0;
  		}
  		client_db_v2.push_back(c);
  	} else {
  		ClientV2 *user = &client_db_v2[user_index];
  		std::string pathname = "./slave" + user->username + "followinglist.txt";
  		struct stat sb;
  		std::cout << "At line 272" << std::endl;
  		if (stat(pathname.c_str(), &sb) != -1) {
  			if (sb.st_mtime != user->last_mod_time) {
  				user->last_mod_time = sb.st_mtime;
  				std::ifstream in("slave" + user->username + "followinglist.txt");
  				int count = 0;
  				std::string line;
  				std::cout << "At PeriodicFollowingCheck second if, Cluster Client: " << username << " following file lines: " << user->following_file_lines << std::endl;
  				while (getline(in, line)) {
  					count++;
  					if (count > user->following_file_lines) {
  						// Call sync_stubs and put into their streams
  						//newTimelinePosts.push_back(line);
  						user->newFollowing.push_back(line);
  						std::cout << "At PeriodicFollowingCheck second if, it read in " << username << "is following" << line << std::endl;
  					} // if
  				} // getline
  				user->following_file_lines = count;
  			} // if (sb.st_mtime != user->last_mod_time)
  		} // if (stat(pathname.c_str(), &sb) != -1) 
  	} // else
  } // for (int i = 0; i < clusterinfo.cluster_clients().size(); i++) 
  
  for (int i = 0; i < clusterinfo.cluster_clients().size(); i++)  {
  	std::string username = clusterinfo.cluster_clients(i);
  	std::cout << "At PeriodicFollowingCheck, Second For Loop Cluster Client: " << username << std::endl;
  	int user_index = find_user_v2(username);
  	ClientV2 *user = &client_db_v2[user_index];
  	
  	for (std::string following : user->newFollowing) {
  		ClientContext context2;
	  	ClientInfo message;
	  	message.set_client_id2(following);
	  	FollowerSyncInfo fsyncInfo;
		coord_stub_->GetFollowerSyncInfo(&context2, message, &fsyncInfo);
		
	  	if (server_id == fsyncInfo.sync_id2()) {
	  		std::ofstream outputfile1("master" + following + "followerlist.txt", std::ios::app|std::ios::out|std::ios::in);
			outputfile1 << user->username << "\n";
			outputfile1.close();
			/* if (find_user_v2(following) < 0) {
				std::cout << "Error in PeriodicFollowingCheck: Can't find the user that is being followed" << std::endl;
			} else {
				ClientV2 *follower_client = &client_db_v2[find_user_v2(following)];
				//follower_client->last_mod_time = sb.st_mtime;
				follower_client->following_file_lines++;
				std::cout << "At PeriodicFollowingCheck, Changing Cluster Client: " << following << " following file size: " << follower_client->following_file_lines << std::endl;
			} // else */
			std::ofstream outputfile2("slave" + following + "followerlist.txt", std::ios::app|std::ios::out|std::ios::in);
			outputfile2 << user->username << "\n";
			outputfile2.close();
			
			/*ClientV2 *follower_client = &client_db_v2[find_user_v2(following)];
			struct stat follower_stat;
			std::string pathname = "./master" + following + "followerlist.txt";
			if (stat(pathname.c_str(), &follower_stat) != -1) {
				follower_client->last_mod_time = follower_stat.st_mtime;
			} // if*/
			continue;
	  	} // if
	  	ClientContext context3;
	  	FollowRequest req;
	  	req.set_username(following);
	  	req.set_follower(user->username);
	  	FollowReply rep;
	  	if (server_id == "1") {
			if (fsyncInfo.sync_id2() == "2") {
				syncOne_stub_->InformFollow(&context3, req, &rep);
			}
			if (fsyncInfo.sync_id2() == "3") {
				syncTwo_stub_->InformFollow(&context3, req, &rep);
			}
		} else if (server_id == "2") {
			if (fsyncInfo.sync_id2() == "1") {
				syncOne_stub_->InformFollow(&context3, req, &rep);
			}
			if (fsyncInfo.sync_id2() == "3") {
				syncTwo_stub_->InformFollow(&context3, req, &rep);
			}
		} else if (server_id == "3") {
			if (fsyncInfo.sync_id2() == "1") {
				syncOne_stub_->InformFollow(&context3, req, &rep);
			}
			if (fsyncInfo.sync_id2() == "2") {
				syncTwo_stub_->InformFollow(&context3, req, &rep);
			}
		} // else if
  	} // for (std::string following : user->newFollowing)
  	user->newFollowing.clear();
  } // for (int i = 0; i < clusterinfo.cluster_clients().size(); i++) 
  std::cout << std::endl;
  sleep(30);
  } // while (true)
}// PeriodicFollowingCheck

void PeriodicTimelineCheck(std::string hostname, std::string cport, std::string server_id) {
  sleep(50);
  ClientContext context1;
  std::shared_ptr<ClientReaderWriter<SyncMessage, SyncMessage>> syncOne_stream(
        syncOne_stub_->InformTimeline(&context1));
  ClientContext context2;
  std::shared_ptr<ClientReaderWriter<SyncMessage, SyncMessage>> syncTwo_stream(
        syncTwo_stub_->InformTimeline(&context2));
        
  std::string login_info = hostname + ":" + cport;
  std::unique_ptr<SNSCoord::Stub> coord_stub_ = std::unique_ptr<SNSCoord::Stub>(SNSCoord::NewStub(
    grpc::CreateChannel(
      login_info, grpc::InsecureChannelCredentials())));
  //Might call infinite while loop after these stub initializations
  
  int iterationCount = 0;
  while (true) {
  iterationCount++;
  std::cout << "Iteration Count of PeriodicTimelineCheck: " << iterationCount << std::endl;
  SyncIdInfo syncinfo;
  ClientContext context3;
  ClusterInfo clusterinfo;
  syncinfo.set_sync_id(server_id);
  coord_stub_->GetClusterInfo(&context3, syncinfo, &clusterinfo);	
  
  for (int i = 0; i < clusterinfo.cluster_clients().size(); i++) {
  	std::string username = clusterinfo.cluster_clients(i);
  	std::cout << "At PeriodicTimelineCheck, First For Loop Cluster Client: " << username << std::endl;
  	
  	int user_index = find_user(username);
  	if (user_index < 0) {
  		Client c;
  		c.username = username;
  		std::string pathname = "./slave" + username + ".txt";
  		struct stat sb;
  		std::cout << "At line 253" << std::endl;
  		if (stat(pathname.c_str(), &sb) != -1) {
  			std::cout << "At line 255" << std::endl;
  			c.last_mod_time = sb.st_mtime;
  			std::ifstream in("slave" + username + ".txt");
  			int count = 0;
  			std::string line;
  			while (getline(in, line)) {
  				count++;
  			} // getline
  			c.following_file_size = count;
  		} else {
  			c.following_file_size = 0;
  		} // else
  		std::cout << "At PeriodicTimelineCheck first if, Cluster Client: " << username << " timeline file size: " << c.following_file_size << std::endl;
  		std::cout << "At line 266" << std::endl;
  		client_db.push_back(c);
  	} else {
  		Client *user = &client_db[user_index];
  		std::string pathname = "./slave" + user->username + ".txt";
  		struct stat sb;
  		std::cout << "At line 272" << std::endl;
  		if (stat(pathname.c_str(), &sb) != -1) {
  			std::cout << "At line 274" << std::endl;
  			if (sb.st_mtime != user->last_mod_time) {
  				std::cout << "At line 276" << std::endl;
  				user->last_mod_time = sb.st_mtime;
  				/*
  				std::vector<std::string> newTimelinePosts;
  				std::vector<std::string> clientFollowers;
  				*/
  				
  				std::ifstream in("slave" + user->username + ".txt");
  				int count = 0;
  				std::string line;
  				std::cout << "At PeriodicTimelineCheck second if, Cluster Client: " << username << " timeline file size: " << user->following_file_size << std::endl;
  				while (getline(in, line)) {
  					count++;
  					if (count > user->following_file_size) {
  						// Call sync_stubs and put into their streams
  						//newTimelinePosts.push_back(line);
  						user->newPosts.push_back(line);
  						std::cout << "At PeriodicTimelineCheck second if, Cluster Client: " << username << " read in new timeline post from synchronizer: " << line << std::endl;
  					} // if
  				} // getline
  				user->following_file_size = count;
  				std::cout << "At PeriodicTimelineCheck second if, Cluster Client: " << username << " changed timeline file size to: " << user->following_file_size << std::endl;
  				
  				std::ifstream in2("slave" + user->username + "followerlist.txt");
  				while (getline(in2, line)) {
  					//clientFollowers.push_back(line);
  					user->followers.push_back(line);
  				} // getline
  				
  				/*
  				for (std::string follower : clientFollowers) {
  					std::cout << "At line 298" << std::endl;
  					ClientContext context4;
  					ClientInfo message;
  					message.set_client_id2(follower);
  					FollowerSyncInfo syncInfo;
  					coord_stub_->GetFollowerSyncInfo(&context4, message, &syncInfo);
  					for (std::string post : newTimelinePosts) {
  						std::cout << "At line 305" << std::endl;
  						SyncMessage m;
  						m.set_username(user->username);
  						m.set_msg(post);
  						m.add_followers(follower);
  						// Might need to update following_file_size of clients
  						if (server_id == syncInfo.sync_id2()) {
  							std::cout << "At line 312" << std::endl;
  							std::ofstream outputfile1("master" + follower + ".txt", std::ios::app|std::ios::out|std::ios::in);
  							outputfile1 << post << "\n";
							outputfile1.close();
							if (find_user(follower) < 0) {
								AddToClientDB(follower);
							} else {
								Client *follower_client = &client_db[find_user(follower)];
								follower_client->last_mod_time = sb.st_mtime;
								follower_client->following_file_size++;
								std::cout << "Changing Cluster Client: " << follower << " following file size: " << follower_client->following_file_size << std::endl;
							}
							//std::ofstream outputfile2("slave" + follower + ".txt", std::ios::app|std::ios::out|std::ios::in);
  							//outputfile2 << post << "\n";
							//outputfile2.close();
							std::ofstream outputfile3("master" + follower + "following.txt", std::ios::app|std::ios::out|std::ios::in);
							outputfile3 << post << "\n";
							outputfile3.close();
							
							//std::ofstream outputfile4("slave" + follower + "following.txt", std::ios::app|std::ios::out|std::ios::in);
							//outputfile4 << post << "\n";
							//outputfile4.close();
							continue;
  						} // if
						if (server_id == "1") {
							if (syncInfo.sync_id2() == "2") {
								syncOne_stream->Write(m);
							}
							if (syncInfo.sync_id2() == "3") {
								syncTwo_stream->Write(m);
							}
						} else if (server_id == "2") {
							if (syncInfo.sync_id2() == "1") {
								syncOne_stream->Write(m);
							}
							if (syncInfo.sync_id2() == "3") {
								syncTwo_stream->Write(m);
							}
						} else if (server_id == "3") {
							if (syncInfo.sync_id2() == "1") {
								syncOne_stream->Write(m);
							}
							if (syncInfo.sync_id2() == "2") {
								syncTwo_stream->Write(m);
							}
						} // else if
  					} // for
  				} // for
  				*/
  			} // if (sb.st_mtime != user->last_mod_time)
  		} // if (stat(pathname, &sb)) != -1)
  		std::cout << "At line 357" << std::endl;
  	} // else
  } //for
  
  for (int i = 0; i < clusterinfo.cluster_clients().size(); i++) {
  	std::string username = clusterinfo.cluster_clients(i);
  	std::cout << "At PeriodicTimelineCheck, Second For Loop Cluster Client: " << username << std::endl;
  	int user_index = find_user(username);
  	Client *user = &client_db[user_index];
	
	for (std::string follower : user->followers) {
		std::cout << "At line 298" << std::endl;
		ClientContext context4;
		ClientInfo message;
		message.set_client_id2(follower);
		FollowerSyncInfo syncInfo;
		coord_stub_->GetFollowerSyncInfo(&context4, message, &syncInfo);
		for (std::string post : user->newPosts) {
			std::cout << "At line 305" << std::endl;
			SyncMessage m;
			m.set_username(user->username);
			m.set_msg(post);
			m.add_followers(follower);
			// Might need to update following_file_size of clients
			if (server_id == syncInfo.sync_id2()) {
				std::cout << "At PeriodicTimelineCheck in line 597, sending in same cluster" << std::endl;
				/*
				std::ofstream outputfile1("master" + follower + ".txt", std::ios::app|std::ios::out|std::ios::in);
				outputfile1 << post << "\n";
				outputfile1.close();
				*/
				if (find_user(follower) < 0) {
					AddToClientDB(follower);
				} else {
					//Client *follower_client = &client_db[find_user(follower)];
					//follower_client->last_mod_time = sb.st_mtime;
					//follower_client->following_file_size++;
					//std::cout << "At PeriodicTimelineCheck, Changing Cluster Client: " << follower << " timeline file size: " << follower_client->following_file_size << std::endl;
				}
				/*
				std::ofstream outputfile2("slave" + follower + ".txt", std::ios::app|std::ios::out|std::ios::in);
				outputfile2 << post << "\n";
				outputfile2.close();
				*/
				std::ofstream outputfile3("master" + follower + "following.txt", std::ios::app|std::ios::out|std::ios::in);
				outputfile3 << post << "\n";
				outputfile3.close();
				
				std::ofstream outputfile4("slave" + follower + "following.txt", std::ios::app|std::ios::out|std::ios::in);
				outputfile4 << post << "\n";
				outputfile4.close();
				
				std::ofstream outputfile5("master" + follower + "actualtimeline.txt", std::ios::app|std::ios::out|std::ios::in);
				outputfile5 << post << "\n";
				outputfile5.close();
				
				std::ofstream outputfile6("slave" + follower + "actualtimeline.txt", std::ios::app|std::ios::out|std::ios::in);
				outputfile6 << post << "\n";
				outputfile6.close();
				
				std::cout << user->username << "sent " << post << " to " << follower << std::endl;
				continue;
			} // if
			std::cout << "At PeriodicTimelineCheck in line 622, sending in different cluster" << std::endl;
			if (server_id == "1") {
				if (syncInfo.sync_id2() == "2") {
					syncOne_stream->Write(m);
				}
				if (syncInfo.sync_id2() == "3") {
					syncTwo_stream->Write(m);
				}
			} else if (server_id == "2") {
				if (syncInfo.sync_id2() == "1") {
					syncOne_stream->Write(m);
				}
				if (syncInfo.sync_id2() == "3") {
					syncTwo_stream->Write(m);
				}
			} else if (server_id == "3") {
				if (syncInfo.sync_id2() == "1") {
					syncOne_stream->Write(m);
				}
				if (syncInfo.sync_id2() == "2") {
					syncTwo_stream->Write(m);
				}
			} // else if
			std::cout << user->username << "sent " << post << " to " << follower << std::endl;
		} // for (std::string post : users->newPosts)
		/*
		if (server_id == syncInfo.sync_id2()) {
			Client *follower_client = &client_db[find_user(follower)];
			struct stat follower_stat;
			std::string pathname = "./master" + follower + ".txt";
			if (stat(pathname.c_str(), &follower_stat) != -1) {
				follower_client->last_mod_time = follower_stat.st_mtime;
			} // if
		} // if (server_id == syncInfo.sync_id2())
		*/
	} // for (std::string follower : user->followers)
	user->newPosts.clear();
	user->followers.clear();
  } // for
  std::cout << std::endl;
  sleep(30);
  } // while (true)
  
  // To implement further, try to have a client database (just like the master) that has the number of lines in corresponding client file
  // As you iterate through client file, once the iteration is more than the number of lines stored, then send the new lines to the gRPC streams
} // PeriodicTimelineCheck


int main(int argc, char** argv) {
    std::string hostname = "localhost";
    std::string port = "3010";
    std::string server_id = "0";
    std::string cport = "5050";

    int opt = 0;
    while ((opt = getopt(argc, argv, "h:c:p:i:")) != -1){
        switch(opt) {
            case 'h': //Coordinator IP
                hostname = optarg;break;
            case 'c': //Coordinator Port
                cport = optarg;break;
            case 'p': //Port
                port = optarg;break;
            case 'i':
                server_id = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }
    
    std::thread thread_one(RunServer, hostname, cport, port, server_id);
    std::thread thread_two(PeriodicTimelineCheck, hostname, cport, server_id);
    std::thread thread_three(PeriodicFollowingCheck, hostname, cport, server_id);
    
    thread_one.join();
    thread_two.join();
    thread_three.join();
    
    
    //Periodic checks
} // main
