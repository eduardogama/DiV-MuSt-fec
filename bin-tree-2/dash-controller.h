#ifndef DASH_CONTROLLER_HH_
#define DASH_CONTROLLER_HH_

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/dashplayer-tracer.h"
#include "ns3/node-throughput-tracer.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"

#include "ns3/uinteger.h"
#include "ns3/netanim-module.h"

#include <map>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>

/*
	imports in graph-utils.h file 
	#include <string>
	#include <vector>
	#include <unordered_map>
	#include <list>
*/
#include "graph-utils.h"

namespace ns3 {

class EndUser {
	public:
		EndUser(){}
	
		void setId(unsigned int _id) {this->stream_id = _id;}
		unsigned int getId() {return this->stream_id;}
		
		void setIp(std::string _ip) {this->user_ip = _ip;}
		std::string getIp() {return this->user_ip;}

		void setContent(int c){this->content = c;}
		int getContent(){return this->content;}
		
	private:

		unsigned int stream_id;
		std::string user_ip;
		
		int content;
};

class GroupUser {
	public:
		GroupUser(){}
		
		void setId(std::string _id) {this->group_id = _id;}
		std::string getId() {return this->group_id;}
		
		void addUser(EndUser *user){this->users.push_back(user);}
		EndUser *getUser(int i){return this->users.at(i);}
				
		void addAddress(Ipv4Address addr){address = addr;}
		Ipv4Address getAddress(){return this->address;}
		
		void setContent(int c){this->content = c;}
		int getContent(){return this->content;}
		
		void setIpv4(Ptr<Ipv4> &ipv4){this->ipv4 = ipv4;}
		Ptr<Ipv4> getIpv4(){return this->ipv4;}
		
		void setSrc(int src){this->src = src;}
		int getSrc(){return this->src;}
		
		void setDst(int dest){this->dst = dest;}
		int getDst(){return this->dst;}
		
		void setRoute(vector<int> &route) {this->route = route;}
		vector<int> &getRoute() {return this->route;}
		int getRoute(int i){return this->route[i];}
		
		void setServerIp(std::string server_ip) {this->server_ip = server_ip;}
		std::string getServerIp() {return this->server_ip;}
		
		std::vector<EndUser *> getUsers(){return this->users;}
		
	private:
			
		std::string group_id;
		std::string server_ip;
	
		vector<EndUser *> users;
		Ipv4Address address;
		
		Ptr<Ipv4> ipv4;
		
		int content;
		
		int src;
		int dst;
	
	public:
		vector<int> route;
		int current = 0;
};

class InterfaceNode {
	public:
		InterfaceNode() {}
		
		void Reset() {this->target_capacity = 0;}
		
		double getTargetCapacity() {return this->target_capacity;}
		void setTargetCapacity(double tc) {this->target_capacity = tc;}

		double geCapacity() {return this->capacity;}
		void setCapacity(double tc) {this->capacity = tc;}

		void setSrcId(int srcid) {this->srcid = srcid;}
		int getSrcId() {return this->srcid;}

		void setDstId(int dstid) {this->dstid = dstid;}
		int getDstId() {return this->dstid;}

		double target_capacity;
		double capacity;
		int srcid, dstid;
		
		Ptr<Ipv4> ipv4src;
		int ipv4addr;
		
//		vector<string> str_groups;
		vector<GroupUser *> groups;
};


class DashController : public Application
{
	public:
		DashController();
		virtual ~DashController();
		
		/**
		* Register this type.
		* \return The TypeId.
		*/
		static TypeId GetTypeId (void);
		void Setup (Address address, uint16_t port);
	
		bool ConnectionRequested (Ptr<Socket> socket, const Address& address);
		void ConnectionAccepted (Ptr<Socket> socket, const Address& address);
		
		void HandleIncomingData(Ptr<Socket> socket);
		void HandleReadyToTransmit(Ptr<Socket> socket, string &video);
				
		void ConnectionClosedNormal (Ptr<Socket> socket);
		void ConnectionClosedError (Ptr<Socket> socket);
		
		void DoSendRedirect();
		
		void SetupRequestRoutingEntry (unsigned int userId, int cont, int src, int dest);
		
		
		bool SearchInterfeceNode(string &ip);
		bool InsertInterfaceNodeInsertInterfaceNode(string &ipsrc, double &target_capacity, double &capacity, 
														int &srcid, int &dstid, int &l, Ptr<Ipv4> ipv4src);
		
		
    	void setGraph(Graph *graph) {this->graph = graph;}
    	Graph *getGraph() {return this->graph;}
    	
    	void setNodes(NodeContainer *nodes) {this->nodes = nodes;}
    	NodeContainer *getNodes() {return this->nodes;}
    	
    	
    	bool m_sendRedirect;
    	string m_srv_ip;
    	
	private:
		virtual void StartApplication (void);
		virtual void StopApplication (void);
		
		Ptr<Socket>     m_socket;
		Address         m_listeningAddress;
		uint32_t        m_packetSize;
		uint32_t        m_nPackets;
		DataRate        m_dataRate;
		EventId         m_sendEvent;
		bool            m_running;
		uint32_t        m_packetsSent;
        uint16_t 	    m_port; //!< Port on which we listen for incoming packets.
        
        map< string, Ptr<Socket> > m_clientSocket;
        
        vector<GroupUser *> groups;
        
        map<string, InterfaceNode> m_interfaceNode;
        
        Graph *graph;
        NodeContainer *nodes;
};


NS_LOG_COMPONENT_DEFINE ("DashControllerApplication");

NS_OBJECT_ENSURE_REGISTERED (DashController);

DashController::DashController ()
						: m_socket (0),
							m_listeningAddress (),
							m_packetSize (0),
							m_nPackets (0),
							m_dataRate (0),
							m_sendEvent ()
{
}

DashController::~DashController ()
{
	m_socket = 0;
}

/* static */
TypeId DashController::GetTypeId (void)
{
	static TypeId tid = TypeId ("DashController")
		.SetParent<Application> ()
		.SetGroupName ("Tutorial")
		.AddConstructor<DashController> ()
		.AddAttribute ("ListeningAddress",
                   "The listening Address for the inbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&DashController::m_listeningAddress),
                   MakeAddressChecker ())
		.AddAttribute ("Port", "Port on which we listen for incoming packets (default: 1317).",
                   UintegerValue (1317),
                   MakeUintegerAccessor (&DashController::m_port),
                   MakeUintegerChecker<uint16_t> ())
                   ;
	return tid;
}

void DashController::Setup (Address address, uint16_t port)
{
	this->m_listeningAddress = address;
	this->m_port = port;
}

void DashController::StartApplication (void) 
{
	this->m_sendRedirect = false;
	this->m_running = true;
	this->m_packetsSent = 0;
	
	if (m_socket == 0) {
		TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");

        m_socket = Socket::CreateSocket(GetNode(), tid);
        
        // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
        if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
        m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
        {
            fprintf(stderr,"Using BulkSend with an incompatible socket type. "
                          "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                          "In other words, use TCP instead of UDP.\n");
        }
        
        if (Ipv4Address::IsMatchingType(m_listeningAddress) == true)
        {
            InetSocketAddress local = InetSocketAddress (Ipv4Address::ConvertFrom(m_listeningAddress), m_port);
            cout << "Listening on Ipv4 " << Ipv4Address::ConvertFrom(m_listeningAddress) << ":" << m_port << endl;
            m_socket->Bind (local);
        } else if (Ipv6Address::IsMatchingType(m_listeningAddress) == true)
        {
            Inet6SocketAddress local6 = Inet6SocketAddress (Ipv6Address::ConvertFrom(m_listeningAddress), m_port);
            cout << "Listening on Ipv6 " << Ipv6Address::ConvertFrom(m_listeningAddress) << endl;
            m_socket->Bind (local6);
        } else {
            cout << "Not sure what type the m_listeningaddress is... " << m_listeningAddress << endl;
        }
	}
	
	m_socket->Listen ();
	
	// And make sure to handle requests and accepted connections
    m_socket->SetAcceptCallback(MakeCallback(&DashController::ConnectionRequested, this),
        						MakeCallback(&DashController::ConnectionAccepted, this));
}

void DashController::StopApplication (void)
{
	m_running = false;

	if (m_sendEvent.IsRunning ()) {
		Simulator::Cancel (m_sendEvent);
	}

	if (m_socket) {
		m_socket->Close ();
	}
}


void DashController::SetupRequestRoutingEntry (unsigned int userId, int cont, int src, int dest)
{
//	vector<GroupUser *> &groups
}

bool DashController::ConnectionRequested (Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION (this << socket << address);
    NS_LOG_DEBUG (Simulator::Now () << " Socket = " << socket << " " << " Server: ConnectionRequested");
 
    return true;
}

void DashController::ConnectionAccepted (Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION (this << socket << address);
   	
	InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (address);

	cout << "DashController(" << socket << ") " << Simulator::Now ()
			  << " Successful socket creation Connection Accepted From " << iaddr.GetIpv4 () 
			  << " port: " << iaddr.GetPort ()<< endl;
	
	stringstream gstream;
    gstream << iaddr.GetIpv4 ();    

	m_clientSocket[gstream.str()] = socket;
	
    socket->SetRecvCallback (MakeCallback (&DashController::HandleIncomingData, this));


    socket->SetCloseCallbacks(MakeCallback (&DashController::ConnectionClosedNormal, this),
                              MakeCallback (&DashController::ConnectionClosedError,  this));
}

void DashController::HandleIncomingData (Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;

    packet = socket->RecvFrom (from);

    if (!(packet)) {
        return;
    }

	uint8_t *buffer = new uint8_t[packet->GetSize ()];
	packet->CopyData(buffer, packet->GetSize ());
	string str_s = string(buffer, buffer+packet->GetSize());
	
	cout << "================================================================================\n";
    fprintf(stderr, "DashController : HandleIncomingData - connection with DashReqServer received %s\n", str_s.c_str());
    
    DashController::HandleReadyToTransmit(socket, str_s);
}

void DashController::HandleReadyToTransmit(Ptr<Socket> socket, std::string &requestString)
{
	cout << "DashController " << Simulator::Now () << " Socket HandleReadyToTransmit (DashReqServer Class) From " << socket << endl;
	cout << "Server Ip = " << requestString << endl;
//	string requestString = this->m_videostreams->getIps(video);
//	string requestString = "";
	
    uint8_t* buffer = (uint8_t*)requestString.c_str();
    	
	Ptr<Packet> pkt = Create<Packet> (buffer, requestString.length());


    // call to the trace sinks before the packet is actually sent,
    // so that tags added to the packet can be sent as well
//    m_txTrace (pkt);
    socket->Send (pkt);
}

void DashController::DoSendRedirect()
{
	for (auto& group : groups) {
		for (auto& user : group->getUsers()) {
			
			cout << "User Ip = "  << user->getIp() << " Server = " << group->getServerIp() << " user size = " << group->getUsers().size() << endl;
			string serverIp = group->getServerIp();
			
			if (m_clientSocket[user->getIp()] == 0) {
				continue;
			}
			
			DashController::HandleReadyToTransmit(m_clientSocket[user->getIp()], serverIp);
		}
	}
}

void DashController::ConnectionClosedNormal (Ptr<Socket> socket)
{}

void DashController::ConnectionClosedError (Ptr<Socket> socket)
{}

bool DashController::SearchInterfeceNode(string &ip)
{
	return (m_interfaceNode.find(ip) == m_interfaceNode.end());
}

bool DashController::InsertInterfaceNode(string &ipsrc, double &target_capacity, double &capacity, int &srcid, int &dstid, int &l, Ptr<Ipv4> ipv4src)
{
	this->insert(make_pair(ipsrc, InterfaceNode()));

	this->m_interfaceNode[ipsrc].target_capacity = target_capacity;
	this->m_interfaceNode[ipsrc].capacity = capacity;
	this->m_interfaceNode[ipsrc].srcid = srcid;
	this->m_interfaceNode[ipsrc].dstid = dstid;
	this->m_interfaceNode[ipsrc].l = l;
	this->m_interfaceNode[ipsrc].ipv4src = ipv4src;

	return true;
}


}
#endif // DASH_CONTROLLER_HH_
