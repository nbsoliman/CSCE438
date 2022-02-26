#include <iostream>
#include <string>
#include <unistd.h>
#include <grpc++/grpc++.h>
#include "client.h"
#include "sns.grpc.pb.h"
#include <google/protobuf/util/time_util.h>
#include <thread>
// #include <channel_interface.h>
// namespace google::protobuf::util

using csce438::SNSService;
using csce438::Request;
using csce438::Reply;
using csce438::Message;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;

class Client : public IClient
{
    public:
        Client(std::shared_ptr<Channel> channel)
            : _stub(SNSService::NewStub(channel)) {}
        Client(const std::string& hname,
               const std::string& uname,
               const std::string& p)
            :hostname(hname), username(uname), port(p)
            {}
        virtual IReply list(std::string& usernameOG){
            username = usernameOG;
            IReply ire;
            Request request;
            request.set_username(username);
            
            Reply reply;
            ClientContext context;
            
            ire.grpc_status = _stub->List(&context, request, &reply);
            for (int i = 1; i <= std::stoi(reply.all_users()[0]); i++){
                ire.all_users.push_back(reply.all_users()[i]);
            }
            
            for (int i = 1; i <= std::stoi(reply.following_users()[0]); i++){
                ire.following_users.push_back(reply.following_users()[i]);
            }
            
            if (ire.grpc_status.ok()) {
                ire.comm_status = SUCCESS;
            } else {
                ire.comm_status = FAILURE_NOT_EXISTS;
                std::cout << "RPC failed";
            }
            return ire;
        }
        
        virtual int login(std::string& usernameOG){
            username = usernameOG;
            Request request;
            request.set_username(username);
            
            Reply reply;
            ClientContext context;
            
            Status status = _stub->Login(&context, request, &reply);
            std::cout << reply.msg() << std::endl;
            
            if (status.ok()) {
                return 1;
            } else {
                return -1;
            }
        }
        
        virtual IReply follow(std::string& usernameOG, std::string& user){
            IReply ire;
            username = usernameOG;
            Request request;
            request.set_username(username);
            request.add_arguments(user);
            
            Reply reply;
            ClientContext context;
            
            ire.grpc_status = _stub->Follow(&context, request, &reply);
            
            if (reply.msg() == "1"){
                ire.comm_status = FAILURE_INVALID_USERNAME;
            } else if (reply.msg() == "2"){
                ire.comm_status = FAILURE_ALREADY_EXISTS;
            } else if (reply.msg() == "3"){
                ire.comm_status = FAILURE_ALREADY_EXISTS;
            } else {
                ire.comm_status = SUCCESS;
            }
            
            return ire;
        }
        virtual IReply unfollow(std::string& usernameOG, std::string& user){
            IReply ire;
            username = usernameOG;
            Request request;
            request.set_username(username);
            request.add_arguments(user);
            
            Reply reply;
            ClientContext context;
            
            ire.grpc_status = _stub->UnFollow(&context, request, &reply);
            if (reply.msg() != ""){
                ire.comm_status = FAILURE_INVALID_USERNAME;
            } else {
                ire.comm_status = SUCCESS;
            }
            
            return ire;
        }
    protected:
        virtual int connectTo();
        virtual IReply processCommand(std::string& input);
        virtual void processTimeline(std::string& address, std::string& username);
    private:
        std::string hostname;
        std::string username;
        std::string port;
        
        // You can have an instance of the client stub
        // as a member variable.
        std::unique_ptr<SNSService::Stub> _stub;
};

int main(int argc, char** argv) {

    std::string hostname = "localhost";
    std::string username = "default";
    std::string port = "3010";
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:")) != -1){
        switch(opt) {
            case 'h':
                hostname = optarg;break;
            case 'u':
                username = optarg;break;
            case 'p':
                port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }
    
    // std::cout << "Username entered: " << username << std::endl;
    Client myc(hostname, username, port);
    // You MUST invoke "run_client" function to start business logic
    myc.run_client();

    return 0;
}

int Client::connectTo()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to create a stub so that
    // you call service methods in the processCommand/porcessTimeline
    // functions. That is, the stub should be accessible when you want
    // to call any service methods in those functions.
    // I recommend you to have the stub as
    // a member variable in your own Client class.
    // Please refer to gRpc tutorial how to create a stub.
	// ------------------------------------------------------------
	std::string address = hostname + ":" + port;
	Client greeter(
      grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));
    int connection_status = greeter.login(username);
// 	grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    // if(grpc::CreateChannel(address, grpc::InsecureChannelCredentials())){
    //     return 1;
    // } else {
    //     return -1;
    // }
    return connection_status;
}

IReply Client::processCommand(std::string& input)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse the given input
    // command and create your own message so that you call an 
    // appropriate service method. The input command will be one
    // of the followings:
	//
	// FOLLOW <username>
	// UNFOLLOW <username>
	// LIST
    // TIMELINE
	//
	// ------------------------------------------------------------
	
    // ------------------------------------------------------------
	// GUIDE 2:
	// Then, you should create a variable of IReply structure
	// provided by the client.h and initialize it according to
	// the result. Finally you can finish this function by returning
    // the IReply.
	// ------------------------------------------------------------
    
	// ------------------------------------------------------------
    // HINT: How to set the IReply?
    // Suppose you have "Follow" service method for FOLLOW command,
    // IReply can be set as follow:
    // 
    //     // some codes for creating/initializing parameters for
    //     // service method
    //     IReply ire;
    //     grpc::Status status = stub_->Follow(&context, /* some parameters */);
    //     ire.grpc_status = status;
    //     if (status.ok()) {
    //         ire.comm_status = SUCCESS;
    //     } else {
    //         ire.comm_status = FAILURE_NOT_EXISTS;
    //     }
    //      
    //      return ire;
    // 
    // IMPORTANT: 
    // For the command "LIST", you should set both "all_users" and 
    // "following_users" member variable of IReply.
    // ------------------------------------------------------------
    int n = input.length();
    char command[n + 1];
    strcpy(command, input.c_str());
    std::string name;
    
    std::list<std::string> args;
    char delim[] = " ";
        char *token = strtok(command, delim);
        int ctr = 0;
        while(token){
            if (ctr != 0){
                // std::cout <<"num" << ctr << ":" << token << std::endl;
                args.push_back(token);
            }
            if (ctr == 1){
                name = token;
            }
            token = strtok(NULL, delim);
            ctr += 1;
        }
    // std::cout << name << std::endl;
    IReply ire;
    std::string address = hostname + ":" + port;
	Client greeter(
      grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));
    // ire = greeter.list();
    // int n = input.length();
    // char command[n + 1];
    // strcpy(command, input.c_str());
	
    if (input.substr(0,4) == "LIST"){
        ire = greeter.list(username);
    }
    if (input.substr(0,6) == "FOLLOW"){
        ire = greeter.follow(username, name);
    }
    if (input.substr(0,8) == "UNFOLLOW"){
        ire = greeter.unfollow(username, name);
    }
    if (input.substr(0,8) == "TIMELINE"){
        greeter.processTimeline(address, username);
    }
    //     // std::string args = input.erase(0,4);
    //     // std::cout << "Args: " << args << std::endl;
    //     Request request;
    //     Reply reply;
    //     ClientContext context;
        
    //     request.set_username(username);
        
        
    //     // std::unique_ptr<SN<Channel> > channel(
    //     // stub_->List(&context, rect));
        
    //     // Client(std::shared_ptr<Channel> channel)
    //     //     : stub_(SNSService::NewStub(channel)) {}
        
    //     std::cout << "2" << std::endl;
    //     ire.grpc_status = stub_->List(&context, request, &reply);
    //     std::cout << "3" << std::endl;
        
    //     if (ire.grpc_status.ok()) {
    //         std::cout << "4" << std::endl;
    //          std::cout << reply.msg() << std::endl;
    //         //  std::cout << reply.all_users() << std::endl;
    //         //  std::cout << reply.following_users() << std::endl;
    //     } else {
    //         std::cout << "????" <<ire.grpc_status.error_code() << ": " << ire.grpc_status.error_message()<< std::endl;
    //     }
    //     // Client send(
    //     //   grpc::CreateChannel("LIST", grpc::InsecureChannelCredentials()));
    //     //   std::string user(username);
    //     //   std::string reply = greeter.SayHello(user);
    //     //   std::cout << "Greeter received: " << reply << std::endl;
    // }
    // std::cout << "5" << std::endl;
    return ire;
}

void Client::processTimeline(std::string& address, std::string& username)
{
	// ------------------------------------------------------------
    // In this function, you are supposed to get into timeline mode.
    // You may need to call a service method to communicate with
    // the server. Use getPostMessage/displayPostMessage functions
    // for both getting and displaying messages in timeline mode.
    // You should use them as you did in hw1.
	// ------------------------------------------------------------
// 	Client greeter(grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));
    ClientContext context;
    
    std::shared_ptr<ClientReaderWriter<Message, Message> > stream(
        _stub->Timeline(&context));

    std::string user = username;
    std::cout << "Now you are in the timeline" << std::endl;
    std::cout << "Send a message to view timeline" << std::endl;
    std::thread writer([stream, user](){
        Message client_message;
        while(1){
    	    client_message.set_msg(getPostMessage());
    	    client_message.set_username(user);
    	    
    	    stream->Write(client_message);
	    }   
    });
    
    std::string sender;
    std::string msg;
    time_t post_time;
    Message server_message;
    while (stream->Read(&server_message)){
        sender = server_message.username();
        msg = server_message.msg();
        post_time = google::protobuf::util::TimeUtil::TimestampToTimeT(server_message.timestamp());
        displayPostMessage(sender, msg, post_time);
    }
    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    //
    // Once a user enter to timeline mode , there is no way
    // to command mode. You don't have to worry about this situation,
    // and you can terminate the client program by pressing
    // CTRL-C (SIGINT)
	// ------------------------------------------------------------
}
