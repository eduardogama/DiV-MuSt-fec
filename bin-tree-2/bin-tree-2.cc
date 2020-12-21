#include "dash-utils.h"
#include "dash-controller.h"

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



using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("BinTreeTwo");


string dir;

bool CreateDir (string name) {
    dir = name;
    return system(string(string("mkdir -p ") + name).c_str()) != -1;
}

string getDirectory (void) {return dir;}
void   setDirectory (string _dir) {dir = _dir;}

string GetCurrentWorkingDir(void)
{
	char buff[250];
	getcwd( buff, 250 );
	string current_working_dir(buff);
	return current_working_dir;
}


double averageArrival = 5;
double lamda = 1 / averageArrival;
std::mt19937 rng (0);
std::vector<double> sum_probs;

std::exponential_distribution<double> poi(lamda);
std::uniform_real_distribution<double> unif(0, 1);

double sumArrivalTimes = 0;
double newArrivalTime;

double poisson()
{
	newArrivalTime = poi.operator() (rng);// generates the next random number in the distribution 
	sumArrivalTimes = sumArrivalTimes + newArrivalTime;
	std::cout << "newArrivalTime: " << newArrivalTime  << ", sumArrivalTimes: " << sumArrivalTimes << std::endl;
	if (sumArrivalTimes < 3.6) {
		sumArrivalTimes = 3.6;
	}
	return sumArrivalTimes;
}

int zipf(double alpha, int n)
{
	static int first = true;      // Static first time flag
	static double c = 0;          // Normalization constant                 
	double z;                     // Uniform random number (0 < z < 1)
	int    zipf_value;            // Computed exponential value to be returned
	int    i;                     // Loop counter
	int    low, high, mid;        // Binary-search bounds

	// Compute normalization constant on first call only
	if (first == true)
	{
		sum_probs.reserve(n+1); // Pre-calculated sum of probabilities
		sum_probs.resize(n+1);
		for (i = 1; i <= n; i++)
			c = c + (1.0 / pow((double) i, alpha));
		c = 1.0 / c;

		//sum_probs = malloc((n+1)*sizeof(*sum_probs));
		sum_probs[0] = 0;
		for (i = 1; i <= n; i++) {
			sum_probs[i] = sum_probs[i-1] + c / pow((double) i, alpha);
		}
		first = false;
	}

	// Pull a uniform random number (0 < z < 1)
	do {
		z = unif.operator()(rng);
	} while ((z == 0) || (z == 1));

	//for (i=0; i<=n; i++)
	//{
	//  std::cout << "sum_probs:  " << sum_probs[i] << std::endl;
	//}
	//std::cout << "z:  " << z << std::endl;
	// Map z to the value
	low = 1; high = n;
	do {
		mid = floor((low+high)/2);
		if (sum_probs[mid] >= z && sum_probs[mid-1] < z) {
			zipf_value = mid;
			break;
		} else if (sum_probs[mid] >= z) {
			high = mid-1;
		} else {
			low = mid+1;
		}
	} while (low <= high);
	std::cout << "ZIPF:  " << zipf_value << std::endl;
	// Assert that zipf_value is between 1 and N
	assert((zipf_value >=1) && (zipf_value <= n));
	
	return (zipf_value);
}

void ExecutionFlow(Ptr<DashController> &appController, unsigned int src,int userId)
{
	int cont = zipf(0.7, 100);
	int dest = 7;

	appController->SetupRequestRoutingEntry(userId, cont, src, dest);
	
	if (appController->m_redirectRequest) {
		appController->DoSendRedirect();
		
		appController->m_redirectRequest = false;
	}
}

void InsertInterfaceNode(_Link *link, Ptr<DashController> controller, NodeContainer &nodes) 
{
	Ptr<Node> nsrc = nodes.Get(linksData.at(i)->getSrcId());
	Ptr<Ipv4> ipv4src = nsrc->GetObject<Ipv4>();
	
	Ptr<Node> ndest = nodes.Get(linksData.at(i)->getDstId());
	Ptr<Ipv4> ipv4dest = ndest->GetObject<Ipv4>();
	
	int32_t int_prev = -1;
	for(uint32_t l = 1; l < ipv4src->GetNInterfaces(); l++) {
		for(uint32_t l1 = 1; l1 < ipv4dest->GetNInterfaces(); l1++) {	
			int_prev = ipv4dest->GetInterfaceForPrefix(ipv4src->GetAddress(l, 0).GetLocal(), ipv4src->GetAddress(l, 0).GetMask());
			if (int_prev != -1) break;
		}
		
		string ipsrc = Ipv4AddressToString(ipv4src->GetAddress(l, 0).GetLocal());			

		if (int_prev != -1 && appController->SearchInterfeceNode(ipsrc)) {
			appController->InsertInterfaceNode(ipsrc, 
											   linksData.at(i)->getRate(),
											   linksData.at(i)->getRate(),
											   linksData.at(i)->getSrcId(),
											   linksData.at(i)->getDstId(),
											   ipv4src,
											   l);
			break;
		}
	}
}

int main (int argc, char *argv[])
{
	string scenarioFiles = GetCurrentWorkingDir() + "/../content/scenario";
	string DashTraceFile = "report.csv";
	string ServerThroughputTraceFile = "server_throughput.csv";
	string RepresentationType = "netflix";	
	
	string AdaptationLogicToUse = "dash::player::RateAndBufferBasedAdaptationLogic";	
	int stopTime = 10;
	double rdm = 0.0;
	int UserId;
	int seed = 0;
	
	CommandLine cmd;
	
	//default parameters
	cmd.AddValue("DashTraceFile", "Filename of the DASH traces", DashTraceFile);
	cmd.AddValue("ServerThroughputTraceFile", "Filename of the server throughput traces", ServerThroughputTraceFile);
	cmd.AddValue("RepresentationType", "Input representation type name", RepresentationType);
    cmd.AddValue("stopTime", "The time when the clients will stop requesting segments", stopTime);
    cmd.AddValue("AdaptationLogicToUse", "Adaptation Logic to Use.", AdaptationLogicToUse);    
    cmd.AddValue("seed", "Seed experiment.", seed);	
	cmd.Parse (argc, argv);
	
	vector<_Link *> linksData;
	vector<_Node *> nodesData;
	
	vector<Edge> edges;
	
	io_read_topology(scenarioFiles + "/tree_l3_link_2", scenarioFiles + "/tree_l3_nodes", linksData, nodesData);
	
	
	NodeContainer nodes;
	nodes.Create( nodesData.size() );

	for (unsigned int i = 0; i < nodesData.size(); i += 1) {
		std::ostringstream ss;
		ss << nodesData.at(i)->getId();
		Names::Add(nodesData.at(i)->getType() + ss.str(), nodes.Get( nodesData.at(i)->getId() ));
	}
	
	// ===============================================================================
	// Creacting ApplicationContainer
	Ptr<DashController> appController = CreateObject<DashController> ();
	appController->Setup(Ipv4Address::GetAny(), 1317);
	// ===============================================================================
		
	// Later we add IP Addresses
	NS_LOG_INFO("Assign IP Addresses.");
	InternetStackHelper internet;
	
	fprintf(stderr, "Installing Internet Stack\n");
	// Now add ip/tcp stack to all nodes.
	internet.Install(nodes);
	
		// create p2p links
	vector<NetDeviceContainer> netDevices;
	Ipv4AddressHelper address;
	address.SetBase ("10.0.0.0", "255.255.255.0");
	PointToPointHelper p2p;
	
	for (unsigned int i = 0; i < linksData.size(); i += 1) {
		p2p.SetDeviceAttribute("DataRate", DataRateValue( linksData.at(i)->getRate() )); // Mbit/s
		
		// And then install devices and channels connecting our topology
		NetDeviceContainer deviceContainer;
		deviceContainer = p2p.Install(nodes.Get(linksData.at(i)->getSrcId()), nodes.Get(linksData.at(i)->getDstId()));
		
		address.Assign(deviceContainer);
		address.NewNetwork();
		netDevices.push_back(deviceContainer);
		
		edges.push_back({linksData.at(i)->getSrcId(), linksData.at(i)->getDstId(), linksData.at(i)->getRate()});


		InsertInterfaceNode(linksData.at(i), appController, nodes);
		
//		Ptr<Node> nsrc = nodes.Get(linksData.at(i)->getSrcId());
//		Ptr<Ipv4> ipv4src = nsrc->GetObject<Ipv4>();
//		
//		Ptr<Node> ndest = nodes.Get(linksData.at(i)->getDstId());
//		Ptr<Ipv4> ipv4dest = ndest->GetObject<Ipv4>();

//		int32_t int_prev = -1;
//		for(uint32_t l = 1; l < ipv4src->GetNInterfaces(); l++) {	
//			for(uint32_t l1 = 1; l1 < ipv4dest->GetNInterfaces(); l1++) {	
//				int_prev = ipv4dest->GetInterfaceForPrefix(ipv4src->GetAddress(l, 0).GetLocal(), ipv4src->GetAddress(l, 0).GetMask());
//				if (int_prev != -1) break;
//			}
//			
//			string ipsrc = Ipv4AddressToString(ipv4src->GetAddress(l, 0).GetLocal());			

//			if (int_prev != -1 && appController->SearchInterfeceNode(ipsrc)) {
//				appController->InsertInterfaceNode(ipsrc, 
//												   linksData.at(i)->getRate(),
//												   linksData.at(i)->getRate(),
//												   linksData.at(i)->getSrcId(),
//												   linksData.at(i)->getDstId(),
//												   ipv4src,
//												   l);
//				break;
//			}
//		}
	}
	
	Graph graph(edges, nodes.GetN());
	graph.printGraph();
	appController->setGraph(&graph);
	appController->setNodes(&nodes);
	
	//Store IP adresses
	std::string addr_file = "addresses";
	ofstream out_addr_file(addr_file.c_str());
	for(unsigned int i = 0; i < nodes.GetN(); i++) {
		Ptr<Node> n = nodes.Get(i);
		Ptr<Ipv4> ipv4 = n->GetObject<Ipv4>();
		for(uint32_t l = 1; l < ipv4->GetNInterfaces(); l++) {
			out_addr_file << i <<  " " << ipv4->GetAddress(l, 0).GetLocal() << endl;
		}
	}
	out_addr_file.flush();
	out_addr_file.close();
	
	
	// %%%%%%%%%%%% Set up the DASH server
	
	stringstream gstream;

    Ptr<Node> n = nodes.Get(7);
    Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();    
//    gstream << ipv4->GetAddress(1,0).GetLocal();
    
    std::cout << "CDN Server = " << Ipv4AddressToString(ipv4->GetAddress(1,0).GetLocal()) << std::endl;
	//=======================================================================================
	
    NS_LOG_INFO("Create Applications.");
    appControlle->srv_ip = Ipv4AddressToString(ipv4->GetAddress(1,0).GetLocal());
    gstream.str("");
	
    string representationStrings = GetCurrentWorkingDir() + "/../../dataset/netflix_vid1.csv";
	fprintf(stderr, "representations = %s\n", representationStrings.c_str ());

	DASHServerHelper server (Ipv4Address::GetAny (), 80,  appController->srv_ip, 
		                   "/content/mpds/", representationStrings, "/content/segments/");
	
	ApplicationContainer serverApps = server.Install(nodes.Get(7));
	serverApps.Start (Seconds(0.0));
	serverApps.Stop (Seconds(stopTime));

	
	nodes.Get(7)->AddApplication(app);
	appController->SetStartTime(Seconds(0.0));
	appController->SetStopTime(Seconds(stopTime));
	

	// %%%%%%%%%%%% Set up the Mobility Position
	MobilityHelper mobility;
	// setup the grid itself: objects are laid out
	// started from (-100,-100) with 20 objects per row, 
	// the x interval between each object is 5 meters
	// and the y interval between each object is 20 meters
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
		                          "MinX", DoubleValue (-10.0),
		                          "MinY", DoubleValue (-10.0),
		                          "DeltaX", DoubleValue (20.0),
		                          "DeltaY", DoubleValue (20.0),
		                          "GridWidth", UintegerValue (20),
		                          "LayoutType", StringValue ("RowFirst"));
	// each object will be attached a static position.
	// i.e., once set by the "position allocator", the
	// position will never change.
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	

	
	YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
	
	unsigned n_ap = 0, n_clients = 5;
	NodeContainer clients;
	
	Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
	
	for (unsigned int i = 0; i < nodesData.size(); i += 1) {
		std::size_t found = Names::FindName( nodes.Get(nodesData.at(i)->getId()) ).find("ap");
		if (found != std::string::npos) {
			mobility.Install(nodes.Get(nodesData.at(i)->getId()));
			
			Ptr<MobilityModel> mob = (nodes.Get(nodesData.at(i)->getId()))->GetObject<MobilityModel>();
			
		    int aux = 360/n_clients;
			double x = mob->GetPosition().x, y = mob->GetPosition().y;
			for (size_t i = 0; i < 360; i += aux) {
				double cosseno = cos(i);
				double seno    = sin(i);
				positionAlloc->Add(Vector(5*cosseno + x , 5*seno + y, 0));
			}
			
			NodeContainer c;
			c.Create(n_clients);
			clients.Add(c);	
		
			internet.Install(c);
			
			WifiHelper wifi;
			WifiMacHelper mac;
		    phy.SetChannel(channel.Create());
			
			std::ostringstream ss;
			ss << "ns-3-ssid-" << ++n_ap;
		    Ssid ssid = Ssid(ss.str());
			
		
			wifi.SetRemoteStationManager("ns3::AarfWifiManager");
			wifi.SetStandard (WIFI_PHY_STANDARD_80211g);

			NodeContainer ap = nodes.Get(nodesData.at(i)->getId());			
			mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
			NetDeviceContainer ap_dev =  wifi.Install(phy, mac, ap);
			
			mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
			NetDeviceContainer sta_dev = wifi.Install(phy, mac, c);
			
			address.Assign(ap_dev);
			address.Assign(sta_dev);
			address.NewNetwork();
		}
	}
	
    positionAlloc->Add(Vector(0, 0, 0));   // Ap 1
    positionAlloc->Add(Vector(30, 0, 0));   // Ap 2
    positionAlloc->Add(Vector(15, 15, 0)); // node2
    positionAlloc->Add(Vector(15, 30, 0)); // node3
	
	mobility.SetPositionAllocator(positionAlloc);    
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(clients);

	mobility.Install(nodes.Get(1));
	mobility.Install(nodes.Get(2));
	mobility.Install(nodes.Get(0));
	mobility.Install(nodes.Get(7));
	
	NetworkSingleton::getInstance()->setClients(&clients);

	// %%%%%%%%%%%% End Set up the Mobility Position
		
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();
	
	// %%%%%%%%%%%% Set up the DASH Clients

	unsigned aux1 = 0;
	SeedManager::SetRun(time(0));
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
	for (unsigned int ap_i = 0; ap_i < nodesData.size(); ap_i += 1) 
	{	
		std::size_t found = Names::FindName( nodes.Get(nodesData.at(ap_i)->getId()) ).find("ap");
		if (found != std::string::npos) 
		{
			for (unsigned int user = 0; user < n_clients; user++) 
			{
				rdm += uv->GetValue();
				UserId = user + aux1 * n_clients;				
				
				int UserId = NetworkSingleton::getInstance()->UserId;
				int screenWidth = 1920;
				int screenHeight = 1080;

				std::stringstream mpd_baseurl;
				mpd_baseurl << "http://" << NetworkSingleton::getInstance()->srv_ip << "/content/mpds/";

				std::stringstream ssMPDURL;
				ssMPDURL << mpd_baseurl.str() << "vid" << 1 << ".mpd.gz";

				DASHHttpClientHelper player (ssMPDURL.str ());
				player.SetAttribute("AdaptationLogic", StringValue(AdaptationLogicToUse));
				player.SetAttribute("StartUpDelay", StringValue("4"));
				player.SetAttribute("ScreenWidth", UintegerValue(screenWidth));
				player.SetAttribute("ScreenHeight", UintegerValue(screenHeight));
				player.SetAttribute("UserId", UintegerValue(UserId));
				player.SetAttribute("AllowDownscale", BooleanValue(true));
				player.SetAttribute("AllowUpscale", BooleanValue(true));
				player.SetAttribute("MaxBufferedSeconds", StringValue("60"));

				ApplicationContainer clientApps;
				clientApps = player.Install(clients->Get(UserId));

				clientApps.Start(Seconds(0.5 + rdm));
				clientApps.Stop(Seconds (stopTime));
			
				Simulator::Schedule(Seconds(rdm + 0.25), ExecutionFlow, appController, ap_i, user + aux1*n_clients);
			}
			aux1++;
		}
	}
	
	// %%%%%%%%%%%% sort out the simulation
	Experiment exp;

    gstream << "../bin-tree";
    if (!exp.CreateDir(gstream.str())) {
        printf("Error creating directory!\n");
        exit(1);
    }
    gstream.str("");


    gstream << exp.getDirectory() << "/output-dash-" << seed << ".csv";
    DASHPlayerTracer::InstallAll(gstream.str());
    gstream.str("");

    gstream << exp.getDirectory() << "/throughput-" << seed << ".csv";
    NodeThroughputTracer::InstallAll(gstream.str());
    gstream.str("");
	
	AnimationInterface anim(exp.getDirectory() + std::string("/topology.netanim"));
	
	Simulator::Stop(Seconds(stopTime));
	Simulator::Run();
	Simulator::Destroy();
	
	DASHPlayerTracer::Destroy();
	
	
	return 0;
}
