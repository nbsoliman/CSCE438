#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
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
using csce438::SNSCoord;
using csce438::CoordRequest;
using csce438::CoordReply;
using csce438::CoordMessage;
using csce438::ClientInfo;
using csce438::FollowerSyncInfo;
using csce438::FollowerSyncRTInfo;
using csce438::SyncIdInfo;
using csce438::ClusterInfo;
using csce438::MasterRequest;
using csce438::MasterReply;

// i = server_id
// MasterRT[i][0] Port
// MasterRT[i][1] Status
// MasterRT[i][2] IP

std::unordered_map<std::string, int> routingTableInfo;
std::string MasterRT[3][3];
std::string SlaveRT[3][3];
std::string FollowerRT[3][3];
std::vector<std::string> clusterOneClients;
std::vector<std::string> clusterTwoClients;
std::vector<std::string> clusterThreeClients;
    
class SNSCoordImpl final : public SNSCoord::Service {
    Status Access(ServerContext* context, const CoordRequest* request, CoordReply* reply) override {
        printRoutingTable();
        int client_id = std::stoi(request->client_id());
        
        if (((client_id % 3) - 1) == 0) {
        	clusterOneClients.push_back(request->client_id());
        } else if (((client_id % 3) - 1) == 1) {
        	clusterTwoClients.push_back(request->client_id());
        } else if (((client_id % 3) - 1) == -1) {  // Original: else
        	clusterThreeClients.push_back(request->client_id());
        }
        std::cout << "Received message from client with id: " << client_id << std::endl;
        std::vector<std::string> RT = getServer(client_id);
        
        std::cout << "Ip Address of Server: " << RT[0] << std::endl;
        std::cout << "Port of Server: " << RT[1] << std::endl;
        reply->set_server_ip_address(RT[0]);
        reply->set_server_port(RT[1]);
        
        return Status::OK;
    }
    
    Status Heartbeat(ServerContext* context, ServerReaderWriter<CoordMessage, CoordMessage>* stream) override {
        std::cout << "Heartbeat Called" << std::endl;
        CoordMessage message;
        
        /*
        After timer hits 20 seconds, check:
        stream->Read(&message);
        if (!(stream->Read(&message))) {
            Change the routing table status
        }
        */
        std::string server_port;
        bool recievedFirstHeartbeat = false;
        while (true) {
            if (stream->Read(&message) && recievedFirstHeartbeat == false) {
                recievedFirstHeartbeat = true;
                std::cout << "Heartbeat While Loop" << std::endl;
                std::string server_ip = message.server_ip();
                server_port = message.server_port2();
                std::string server_type = message.server_type();
                std::string routingTableLookup = server_ip + ":" + server_port;
                if (routingTableInfo.find(routingTableLookup) == routingTableInfo.end()) {
                    routingTableInfo[routingTableLookup] = 1;
                    fillRoutingTable(server_ip, server_port, server_type); 
                }
                std::cout << server_ip << ":" << server_port << std::endl;                
            } // if 
            
            if (!(stream->Read(&message))) {
                changeRoutingTable(server_port);
            } // if
            
            sleep(20);
        } // while(true)
        
        /*
        while (stream->Read(&message)) {
            std::cout << "Heartbeat While Loop" << std::endl;
            std::string server_ip = message.server_ip();
            std::string server_port = message.server_port2();
            std::string server_type = message.server_type();
            std::string routingTableLookup = server_ip + ":" + server_port;
            if (routingTableInfo.find(routingTableLookup) == routingTableInfo.end()) {
                routingTableInfo[routingTableLookup] = 1;
                fillRoutingTable(server_ip, server_port, server_type); 
            }
            std::cout << server_ip << ":" << server_port << std::endl;
        }
        */
        
        return Status::OK;
    }
    
    Status GetSlaveInfo (ServerContext* context, const MasterRequest* request, MasterReply* reply) {
    	reply->set_slave_ip(SlaveRT[std::stoi(request->server_id()) - 1][2]);
    	reply->set_slave_port(SlaveRT[std::stoi(request->server_id()) - 1][0]);
    	return Status::OK;
    }
    
    Status FillFollowerSyncRT (ServerContext* context, const FollowerSyncRTInfo* request, SyncIdInfo* reply) {
    	//int index = (std::stoi(request->sync_id()) % 3) - 1;
    	int index = std::stoi(request->sync_id()) - 1;
    	FollowerRT[index][0] = request->sync_port();
    	FollowerRT[index][1] = "Active";
    	FollowerRT[index][2] = request->sync_ip_address();
    	reply->set_sync_id("Success");
    	std::cout << "In FillFollowerSyncInfo: sync_port: " << request->sync_port() << " sync_ip_address: "  << request->sync_ip_address() << std::endl;
    	printFollowerSyncRoutingTable();
    	
    	return Status::OK;
    }
    
    Status GetFollowerSyncInfo (ServerContext* context, const ClientInfo* clientInfo, FollowerSyncInfo* syncInfo) {
    	int clientId = std::stoi(clientInfo->client_id2());
    	int followerSyncId = (clientId % 3) - 1;
    	//change
    	if (followerSyncId == -1) {
    		followerSyncId = 2;
    	}
    	
    	syncInfo->set_sync_ip_address(FollowerRT[followerSyncId][2]);
    	syncInfo->set_sync_port(FollowerRT[followerSyncId][0]);
    	syncInfo->set_sync_id2(std::to_string(followerSyncId + 1));
    	
    	return Status::OK;
    }
    
    Status GetClusterInfo (ServerContext* context, const SyncIdInfo* syncinfo, ClusterInfo* clusterinfo) {
    	int clusterNum = ((std::stoi(syncinfo->sync_id())) % 3) - 1;
    	if (clusterNum == 0) {
    		for (std::string client : clusterOneClients) {
    			clusterinfo->add_cluster_clients(client);
    		}
    	} else if (clusterNum == 1) {
    		for (std::string client : clusterTwoClients) {
    			clusterinfo->add_cluster_clients(client);
    		}
    	} else if (clusterNum == -1) {    // original : else if (clusterNum == 2)
    		for (std::string client : clusterThreeClients) {
    			clusterinfo->add_cluster_clients(client);
    		}
    	} // else if
    	
    	return Status::OK;
    	
    }
    
    void changeRoutingTable(std::string port) {
        for (int i = 0; i < 3; i++) {
            if (MasterRT[i][0] == port) {
                MasterRT[i][1] = "Inactive";
                break;
            }
        } // for
    } // changeRoutingTable
    
    void fillRoutingTable(std::string ip, std::string port, std::string type) {
        std::cout << "fillRoutingTable: " << ip << ":" << port << ":" << type; 
        if (type == "master") {
            int index = -1;
            for (int i = 0; i < 3; i++) {
                if (MasterRT[i][0] == "") {
                    index = i;
                    break;
                } // if
            } // for
            
            if (index != -1) { 
                MasterRT[index][0] = port;
                MasterRT[index][1] = "Active";
                MasterRT[index][2] = ip;
            } // if
        } else {
            int index = -1;
            for (int i = 0; i < 3; i++) {
                if (SlaveRT[i][0] == "") {
                    index = i;
                    break;
                } // if
            } // for
            
            if (index != -1) { 
                SlaveRT[index][0] = port;
                SlaveRT[index][1] = "Active";
                SlaveRT[index][2] = ip;
            } // if
        } // else
    } // fillRoutingTable
    
    void printRoutingTable() {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                std::cout << "i: " << i << " j: " << j << ", Info: " << MasterRT[i][j] << std::endl; 
            }
        }
    }
    
    void printSlaveRoutingTable() {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                std::cout << "i: " << i << " j: " << j << ", Slave Info: " << SlaveRT[i][j] << std::endl; 
            }
        }
    }
    
    void printFollowerSyncRoutingTable() {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                std::cout << "i: " << i << " j: " << j << ", Info: " << FollowerRT[i][j] << std::endl; 
            }
        }
    }
    
    std::vector<std::string> getServer(int client_id){
        std::vector<std::string> serverData(2);
        int serverId = (client_id % 3) - 1;   // int serverId = (client_id % 3) + 1;
        // change
        if (serverId == -1) {
        	serverId = 2;
        }
        std::cout << "getServer serverId: " << serverId << " status info: "  << MasterRT[serverId][2] << std::endl;
        if (MasterRT[serverId][1] == "Active") {
            serverData[0]=MasterRT[serverId][2];
            serverData[1]=MasterRT[serverId][0];
            return serverData;
        } else{
            serverData[0]=SlaveRT[serverId][2];
            serverData[1]=SlaveRT[serverId][0];
            return serverData;
        }
}
};

void RunServer(std::string port) {
  std::string server_address = "0.0.0.0:"+port;
  SNSCoordImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char** argv) {
    std::string port = "3010"; //default port
    
    //Gets arguments from console (port)
    int opt = 0;
    while ((opt = getopt(argc, argv, "p:")) != -1){
        switch(opt) {
            case 'p':
                port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }
    
    RunServer(port);
    return 0;
}


// getFollowerSyncer(client_id){
//     serverId = -1;
//     if (serverId == ((client_id % 3) + 1)) {
//         serverId = (client_id % 3) + 1;
//     }
// }
