//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%                   QoE-aware Routing Computation                  %%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//                                                                    %
// Code for solving the QoE-aware routing problem as the              %
// network evolves. The code can be called by ns3 to admit            %
// a new demand, reconfigure the network according to its status and  %
// the QoE experiend by e2e connections [1].                          %
//                                                                    %
// Created by                                                         %
// - Paris Reseach Center, Huawei Technologies Co. Ltd.               %
// - Laboratoire d'Informatique, Signaux et Systèmes de               %
//   Sophia Antipolis (I3S) Universite Cote d'Azur and CNRS           %
//                                                                    %
// Contributors:                                                      %
// - Giacomo CALVIGIONI (I3S)                                         %
// - Ramon APARICIO-PARDO (I3S)                                       %
// - Lucile SASSATELLI (I3S)                                          %
// - Jeremie LEGUAY (Huawei)                                          %
// - Stefano PARIS (Huawei)                                           %
// - Paolo MEDAGLIANI (Huawei)                                        %
//                                                                    %
// References:                                                        %
// [1] Giacomo Calvigioni, Ramon Aparicio-Pardo, Lucile Sassatelli,   %
//     Jeremie Leguay, Stefano Paris, Paolo Medagliani,               %
//     "Quality of Experience-based Routing of Video Traffic for      %
//      Overlay and ISP Networks". In the Proceedings of the IEEE     %
//     International Conference on Computer Communications            %
//      (INFOCOM), 15-19 April 2018.                                  %
//                                                                    %
// Contacts:                                                          %
// - For any question please use the address: qoerouting@gmail.com    %
//                                                                    %
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


#include <iostream>
#include <fstream>
#include <string>


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
#include "ns3/uinteger.h"
#include "ns3/flow-monitor-helper.h"

#include "dash-define.h"
#include "dash-utils.h"

#include <fstream>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
using namespace ns3;

#include <unistd.h>
#include<iostream>

string GetCurrentWorkingDir(void)
{
	char buff[250];
	getcwd( buff, 250 );
	std::string current_working_dir(buff);
	return current_working_dir;
}

static string Ipv4AddressToString(Ipv4Address ad) 
{
	std::ostringstream oss;
	ad.Print (oss);
	return oss.str ();
}

NS_LOG_COMPONENT_DEFINE ("HttpClientServerExample");


int main (int argc, char *argv[])
{
	//LogComponentEnable ("TcpCubic", LOG_LEVEL_INFO);
	CommandLine cmd;
	std::string scenarioFiles = GetCurrentWorkingDir() + "/../content/scenario";
	std::string requestsFile = "requests";
	std::string DashTraceFile = "report.csv";
	std::string ServerThroughputTraceFile = "server_throughput.csv";
	std::string RepresentationType = "netflix";

	//default parameters
	cmd.AddValue("DashTraceFile", "Filename of the DASH traces", DashTraceFile);
	cmd.AddValue("ServerThroughputTraceFile", "Filename of the server throughput traces", ServerThroughputTraceFile);
	cmd.AddValue("RepresentationType", "Input representation type name", RepresentationType);
	cmd.Parse (argc, argv);

	int max_simulation_time = 550; // 24 h + 10 seconds

	// SET TCP segment size (MSS) to be NOT 536 bytes as default..., we set it to MTU - 40
	Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (1400)); // 1460 is too large it seems
	Config::SetDefault("ns3::PointToPointNetDevice::Mtu", UintegerValue (1500)); // ETHERNET MTU
	//Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue (16)); // default: 1
//	Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic")); // default
	Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno")); // default
	Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (0.01));
	Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));

	AsciiTraceHelper ascii;
	Ptr< OutputStreamWrapper > stream=ascii.CreateFileStream ("demo.tr");
	//PointToPointHelper::EnablePcapAll ("demo");

	// read inputs
	vector<_Link *> linksData;
	vector<_Node *> nodesData;
	vector<_Request *> requestsData;
	vector<_Route *> routesData;
	
	cout << scenarioFiles + "/requests" << endl;
	io_read_scenario(scenarioFiles + "/adj_matrix", scenarioFiles + "/nodes", scenarioFiles + "/requests", scenarioFiles + "/routes", linksData, nodesData, requestsData, routesData);

	// create nodes
	NodeContainer nodes;
	nodes.Create(nodesData.size());
	for(unsigned int i=0;i<nodesData.size();i++){
		std::ostringstream ss;
		ss << nodesData.at(i)->getId();
		Names::Add(nodesData.at(i)->getType() + ss.str(), nodes.Get(nodesData.at(i)->getId()));
	}

	// initialize static routing
	Ipv4StaticRoutingHelper staticRouting;
	InternetStackHelper internet;
	//std::string tcpCong = "cubic";
	//std::string nscStack = "liblinux2.6.26.so";
	//internet.SetTcp ("ns3::NscTcpL4Protocol","Library",StringValue (nscStack));
	internet.SetRoutingHelper(staticRouting);
	fprintf(stderr, "Installing Internet Stack\n");
	// Now add ip/tcp stack to all nodes.
	internet.InstallAll ();

	// create p2p links
	vector<NetDeviceContainer> netDevices;
	Ipv4AddressHelper address;
	address.SetBase ("10.0.0.0", "255.255.255.0");
	PointToPointHelper p2p;
	for(unsigned int i=0;i<linksData.size();i++){
		double errRate = linksData.at(i)->getPLoss();
		DoubleValue rate(errRate);
		Ptr<RateErrorModel> em1 = CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0,Max=1.0]"), "ErrorRate", rate);
		Ptr<RateErrorModel> em2 = CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0,Max=1.0]"), "ErrorRate", rate);

		p2p.SetDeviceAttribute ("DataRate", DataRateValue ( linksData.at(i)->getRate() )); // 100 Mbit/s
		p2p.SetChannelAttribute ("Delay", TimeValue ( MilliSeconds (linksData.at(i)->getDelay()) ));

		uint32_t queueSize = 1000000; 
		p2p.SetQueue("ns3::DropTailQueue", "MaxPackets", UintegerValue(queueSize));

		// And then install devices and channels connecting our topology
		NetDeviceContainer deviceContainer;
		deviceContainer = p2p.Install(nodes.Get(linksData.at(i)->getSrcId()), nodes.Get(linksData.at(i)->getDstId()));
		deviceContainer.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue (em1));
		deviceContainer.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue (em2));
		address.Assign(deviceContainer);
		address.NewNetwork();
		netDevices.push_back(deviceContainer);
	}
	//Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	
	//Store IP adresses
	std::string addr_file="addresses";
	ofstream out_addr_file(addr_file.c_str());
	for(unsigned int i=0;i<nodes.GetN();i++){
		Ptr<Node> n = nodes.Get (i);
		Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
		for(uint32_t l=1;l<ipv4->GetNInterfaces();l++){
			out_addr_file << i <<  " " << ipv4->GetAddress(l, 0).GetLocal() << endl;
		}
	}
	out_addr_file.flush();
	out_addr_file.close();

	//Create requests
	std::string token = ",";
	std::string mpd_baseurl = "content/mpds/";
	for(unsigned int i=0;i<requestsData.size();i++){
		
		// retreive IP from server
		Ptr<Node> n = nodes.Get (requestsData.at(i)->getServerId());
		
		std::cout << "->" << n << " \n";
		std::cout << "->" << requestsData.at(i)->getServerId() << "\n";
		Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
		Ipv4InterfaceAddress ipv4_int_addr = ipv4->GetAddress (1, 0);
		std::cout << "2\n";
		Ipv4Address server_ip = ipv4_int_addr.GetLocal ();  
		cout << "server at: " << Ipv4AddressToString(server_ip) << endl;

		// retreive IP from client
		n = nodes.Get (requestsData.at(i)->getSrcId());
		ipv4 = n->GetObject<Ipv4> ();
		Ipv4InterfaceAddress c_ipv4_int_addr = ipv4->GetAddress (1, 0);
		Ipv4Address client_ip = c_ipv4_int_addr.GetLocal ();
		cout << "cient at: " << Ipv4AddressToString(client_ip) << endl;

		//find nodeData
		int node=-1;
		for(unsigned int j=0;j<nodesData.size();j++){
			if(nodesData.at(j)->getId() == requestsData.at(i)->getSrcId())
 				node=j;
		}
		if (node==-1){
			cout << "bad inputs" << endl;
			exit(1);
		}
		
		// installing client
		std::stringstream ssMPDURL;
		ssMPDURL << "http://" << Ipv4AddressToString(server_ip) << "/" << mpd_baseurl << "vid" << requestsData.at(i)->getVideoId()+1 << ".mpd.gz";
		DASHHttpClientHelper client(ssMPDURL.str());
		client.SetAttribute("AdaptationLogic", StringValue("dash::player::" + nodesData.at(node)->getAdaptationLogic()));
		client.SetAttribute("StartUpDelay", StringValue(nodesData.at(node)->getStartUpDelay()));
		client.SetAttribute("ScreenWidth", UintegerValue(requestsData.at(i)->getScreenWidth()));
		client.SetAttribute("ScreenHeight", UintegerValue(requestsData.at(i)->getScreenHeight()));
		client.SetAttribute("UserId", UintegerValue(nodesData.at(node)->getId()));
		client.SetAttribute("AllowDownscale", BooleanValue(nodesData.at(node)->getAllowDownscale()));
		client.SetAttribute("AllowUpscale", BooleanValue(nodesData.at(node)->getAllowUpscale()));
		client.SetAttribute("MaxBufferedSeconds", StringValue(nodesData.at(node)->getMaxBufferedSeconds()));

		ApplicationContainer apps;
		apps = client.Install(nodes.Get(requestsData.at(i)->getSrcId()));
		apps.Start (Seconds (requestsData.at(i)->getStartsAt()));
		apps.Stop (Seconds (requestsData.at(i)->getStopsAt()));
		fprintf(stderr, "Client starts at %f and stops at %f\n", requestsData.at(i)->getStartsAt(), requestsData.at(i)->getStopsAt());
	}

	//add routes from clients to servers
	for(unsigned int j=0;j<routesData.size();j++){
		vector<int> hops = routesData.at(j)->getHops();
		if(hops.size()<=1) 
			exit(0);
		
		Ptr<Ipv4> client_ip = nodes.Get(hops.at(0))->GetObject<Ipv4> ();
		Ptr<Ipv4> dst_ip = nodes.Get(hops.at(hops.size()-1))->GetObject<Ipv4> ();
		cout << "add: " << hops.at(0) << " " << hops.at(hops.size()-1) << endl;
		
		for(unsigned int k=0;k<hops.size()-1;k++){
			Ptr<Ipv4> src_ip = nodes.Get(hops.at(k))->GetObject<Ipv4> ();
			Ptr<Ipv4> next_ip = nodes.Get(hops.at(k+1))->GetObject<Ipv4> ();
			Ptr<Ipv4StaticRouting> staticRoutingSrc = staticRouting.GetStaticRouting (src_ip);
			Ptr<Ipv4StaticRouting> staticRoutingNext = staticRouting.GetStaticRouting (next_ip);

			//seach for the interface to each next hop
			cout << "add at : " << src_ip->GetAddress(1, 0).GetLocal() << " dst: " << dst_ip->GetAddress(1, 0).GetLocal() << endl;   
			int32_t int_next=-1;
			for(uint32_t l=1;l<next_ip->GetNInterfaces();l++){
				int_next=src_ip->GetInterfaceForPrefix(next_ip->GetAddress(l, 0).GetLocal(), next_ip->GetAddress(l, 0).GetMask());
				cout << int_next <<  " " << next_ip->GetAddress(l, 0).GetLocal() << endl;
				if(int_next!=-1) break;
			}
			if(int_next!=-1) {
				cout << "add " << src_ip->GetAddress(1, 0).GetLocal() << " " << dst_ip->GetAddress(1, 0).GetLocal() << endl;
				staticRoutingSrc->AddHostRouteTo(dst_ip->GetAddress(1, 0).GetLocal(), next_ip->GetAddress(1, 0).GetLocal(), int_next, 20);
			}
			Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);
			staticRoutingSrc->PrintRoutingTable(routingStream);

			//search for the interface to each previous hop
			int32_t int_prev=-1;
			for(uint32_t l=1;l<src_ip->GetNInterfaces();l++){
				int_prev=next_ip->GetInterfaceForPrefix(src_ip->GetAddress(l, 0).GetLocal(), src_ip->GetAddress(l, 0).GetMask());
				if(int_prev!=-1) break;
			}
			if(int_prev!=-1){
				cout << "add " << next_ip->GetAddress(1, 0).GetLocal() << " " << client_ip->GetAddress(1, 0).GetLocal() << endl;
				staticRoutingNext->AddHostRouteTo(client_ip->GetAddress(1, 0).GetLocal(), client_ip->GetAddress(1, 0).GetLocal(), int_prev, 20);
			}
		}
	}


	fprintf(stderr, "Installing Servers\n");
	for(unsigned int i=0;i<nodesData.size();i++){
		if(nodesData.at(i)->getType().compare("server")==0){
			std::string representationStrings =     "content/representations/" + RepresentationType + "_vid1.csv,content/representations/" + RepresentationType + "_vid2.csv,content/representations/" + RepresentationType + "_vid3.csv,content/representations/" + RepresentationType + "_vid4.csv,content/representations/" + RepresentationType + "_vid5.csv,content/representations/" + RepresentationType + "_vid6.csv,content/representations/" + RepresentationType + "_vid7.csv" ;
			fprintf(stderr, "representations = %s\n", representationStrings.c_str());

			// Retreive IP from server
			Ptr<Node> n = nodes.Get (nodesData.at(i)->getId());
			Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
			Ipv4InterfaceAddress ipv4_int_addr = ipv4->GetAddress (1, 0);
			Ipv4Address server_ip = ipv4_int_addr.GetLocal ();

			DASHServerHelper server(Ipv4Address::GetAny (), 80, Ipv4AddressToString(server_ip), "/content/mpds/", representationStrings, "/content/segments/");

			ApplicationContainer apps;
			apps = server.Install(nodes.Get(nodesData.at(i)->getId()));
			apps.Start (Seconds (0.0));
			apps.Stop (Seconds (max_simulation_time));
		}
	}

	NodeThroughputTracer::Install(nodes, ServerThroughputTraceFile);
	DASHPlayerTracer::Install(nodes, DashTraceFile);

	//FlowMonitor
	Ptr<FlowMonitor> flowMon;
	FlowMonitorHelper flowMonHelper;
	flowMon = flowMonHelper.InstallAll();

	p2p.EnableAsciiAll (stream);

	fprintf(stderr, "Done, Starting simulation...\n");

	// Finally, set up the simulator to run.  The 1000 second hard limit is a
	// failsafe in case some change above causes the simulation to never end
	Simulator::Stop (Seconds (max_simulation_time + 1));
	Simulator::Run ();

	flowMon->SetAttribute("DelayBinWidth", DoubleValue(0.01));
	flowMon->SetAttribute("JitterBinWidth", DoubleValue(0.01));
	flowMon->SetAttribute("PacketSizeBinWidth", DoubleValue(1));
	flowMon->CheckForLostPackets();
	flowMon->SerializeToXmlFile("FlowMonitor.xml", true, true);

	Simulator::Destroy ();
	ns3::DASHPlayerTracer::Destroy();

	std::cout << "Done!" << std::endl;
}

