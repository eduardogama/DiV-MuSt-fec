#include "dash-offloading.h"

#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/socket.h"


using namespace ns3;
using namespace std;


DashOffloading::DashOffloading ()
{
  m_socket = 0;
  m_send_AP_rfbEvent = EventId ();
  m_send_dataEvent = EventId ();
  m_send_bidEvent = EventId ();
  m_tempo_envio = 0.0;
  m_timeout = 0.050;
  m_timeBetweenSourcePacket = 3.0;
  m_pkt_count = 0;
  m_pu = 0;
  m_offerBid = 0.0;
  m_winner_bid = 0.0;
  m_peerAddress = Ipv4Address("255.255.255.255");
  filename_report_user = "report_user";
  m_wrong_order = 0;
}

DashOffloading::~DashOffloading ()
{
}

void DashOffloading::SetRemote (Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port
}

void DashOffloading::SetRemote (Ipv4Address ip, uint16_t port)
{
  m_peerAddress = Address (ip);
  m_peerPort = port;
}

void DashOffloading::SetBackbone (vector<Ipv4Address> backbone)
{
  m_backbone = backbone;
}

void DashOffloading::SetTightnessParameters (double k1, double k2, double budget_percentage, double fine_percentage)
{
  m_k1 = k1;
  m_k2 = k2;
  m_budget_percentage = budget_percentage;
  m_fine_percentage = fine_percentage;
}

void DashOffloading::SetMapNodes (std::map<Ipv4Address, int> nodes_id)
{
  m_nodes_id = nodes_id;
}

void DashOffloading::SetTopologyName (string topologyName)
{
  m_topologyName = topologyName;
}

void DashOffloading::SetSeedIndex (int seedIndex)
{
  m_seedIndex = seedIndex;
}

void DashOffloading::SetParamName (string paramName)
{
  m_paramName = paramName;
}
void DashOffloading::SetExpIndex (std::string expIndex)
{
  m_expIndex = expIndex;
}

void DashOffloading::DoDispose (void)
{
  // Application::DoDispose ();
}

void DashOffloading::StartApplication (void)
{
  // Saving this node address:
  m_thisNode = this->GetNode();
  Ptr<WifiNetDevice> netDevice = DynamicCast<WifiNetDevice> (m_thisNode->GetDevice(0));
  m_nodeMacAddr = Mac48Address::ConvertFrom (netDevice->GetAddress());
  Ptr<Ipv4> ipv4 = m_thisNode->GetObject<Ipv4>();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1,0);
  m_nodeAddr = iaddr.GetLocal();
  cout << "ADDRESS = " << m_nodeAddr << endl;

  //Accountability report file name and directory:
  if(m_thisNode->GetId() < 10)
    filename_report_user = filename_report_user + "00" + boost::lexical_cast<std::string>(m_thisNode->GetId());
  else if(m_thisNode->GetId() < 100)
    filename_report_user = filename_report_user + "0" + boost::lexical_cast<std::string>(m_thisNode->GetId());
  else
    filename_report_user = filename_report_user + boost::lexical_cast<std::string>(m_thisNode->GetId());

  string home_dir(getenv("HOME"));
  if ((strategy_type == STRATEGYTYPE_TIGHTNESS) && (preference_function_type == PREFERENCEFUNCTION_PLANE)) {
    dir_myreport_user = home_dir + "/Dropbox/unb/mestrado/tese/simulations/topology_config/exp" + m_expIndex + "/" + m_paramName + "/seed"
        + boost::lexical_cast<std::string>(m_seedIndex) + "/tp" + m_topologyName + "/report_user";
  } else {
    dir_myreport_user = home_dir + "/Dropbox/unb/mestrado/tese/simulations/topology_config/exp" + m_expIndex + "/seed"
          + boost::lexical_cast<std::string>(m_seedIndex) + "/tp" + m_topologyName + "/report_user";
  }

  mkdir (dir_myreport_user.c_str(),0777);
  dir_myreport_user = dir_myreport_user + "/" + filename_report_user + ".txt";

  //Report table header:
  content = content + "Source" + "\t";
  content = content + "pktID" + "\t";
  content = content + "Next" + "\t";
  content = content + "eBID" + "\t";
  content = content + "pFINE" + "\t";
  content = content + "pBID" + "\t";
  content = content + "eFINE" + "\t";
  content = content + "sBUDGET" + "\t";
  content = content + "fBUDGET" + "\t";
  content = content + "status" + "\t";
  content = content + "BALANCE" + "\t";
  content = content + "accumulative" + "\t";
  content = content + "\n";

  cout << "->" << m_nodeAddr << endl;
  report_user = fopen(dir_myreport_user.c_str(), "w+");
  fputs(content.c_str(), report_user);
  fclose(report_user);

  //Identify if this node is an AP or not:
  node_type = NODETYPE_NETWORK;
  if (isBackbone(m_nodeAddr))
    node_type = NODETYPE_AP;

  //Configure socket:
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  if (m_socket == 0) {
    m_socket = Socket::CreateSocket (GetNode (), tid);
    if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
    {
      int status;
      InetSocketAddress src = InetSocketAddress (Ipv4Address::GetAny (), m_peerPort);
      status = m_socket->Bind (src);

      InetSocketAddress dst = InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort);
      status = m_socket->Connect (dst);

      m_socket->SetAllowBroadcast(true);
    }
  }

  m_socket->SetRecvCallback (MakeCallback (&DashOffloading::HandleRead, this));
  //Graph Mapping: node number --> address
  MapNode();
  if ((node_type == NODETYPE_AP) && (m_nPackets > 0)) {
    m_pu = 0;
    m_send_AP_rfbEvent = Simulator::Schedule (Seconds (m_start), &DashOffloading::SendRFB, this, m_nodeAddr, m_dest, 0);
  }
}

DashOffloading::StopApplication ()
{
  if (m_socket != 0)
  {
    m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    m_socket = 0;
  }
  Simulator::Cancel (m_send_AP_rfbEvent);
  Simulator::Cancel (m_send_dataEvent);
  Simulator::Cancel (m_send_bidEvent);
}

void DashOffloading::HandleRead (Ptr<Socket> socket)
{
  cout << "--> PACKET RECEIVED BY: " << m_nodeAddr << endl;

  RFBHeader rfbHeader;
  BidHeader bidHeader;
  DataHeader dataHeader;

  Ptr<Packet> packet;
  Address from;

  double getOfferbid;
  int packetID;
  bool toBackbone;
  //int bid_index = 0;

  while ((packet = socket->RecvFrom (from)))
  {
    TypeHeader tHeader (OFFLOADINGTYPE_RFB);
    packet->RemoveHeader (tHeader);

    if (!tHeader.IsValid ()) {
      cout << "Offloading message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop" << endl;
      return; // drop
    }

    if (InetSocketAddress::IsMatchingType (from)) {
      switch (tHeader.Get()) {
        case OFFLOADINGTYPE_RFB:
          //If this node is an AP, so don't process the RFB (backbone cannot participate on the biddings):
          if (node_type == NODETYPE_NETWORK) {
            //Extract packet:
            packet->RemoveHeader (rfbHeader);
            m_dest      = rfbHeader.GetDst();
            m_source    = rfbHeader.GetSrc();
            packetID    = rfbHeader.GetPacketID();
            m_pu        = rfbHeader.GetHopcount();
            m_deadline  = rfbHeader.GetDeadline();
            m_upload    = from;
            m_uploadMAC = rfbHeader.GetSrcRFB();
            m_payFINE   = rfbHeader.GetFine()/pow(10,2); //In case of packet delivery failure, paid this Fine;

            //If this packet was already auctioned, do nothing. Otherwise, proceed with the bidding process:
            if (!isOldPacket(m_source, packetID)) {
              //Save the bids that this node is participating at this moment:
              //m_backbonePair.push_back(std::make_pair(rfbHeader.GetSrc(),rfbHeader.GetDst()));
              //bid_index = FindBidNumber(rfbHeader.GetSrc(), rfbHeader.GetDst());
              NS_LOG_INFO ("["<< m_source <<"->"<< m_dest <<"]["<<packetID<<"]At " << Simulator::Now ().GetSeconds () << "s "<< Ipv4Address::ConvertFrom (m_nodeAddr) <<
              " received a RFB from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () );
              m_send_bidEvent = Simulator::Schedule (Seconds (0.), &DashOffloading::SendBid, this,
              socket, rfbHeader.GetB0(), rfbHeader.GetFine(), rfbHeader.GetDeadline(), packetID);
              //SendBid(socket, rfbHeader.GetB0(), rfbHeader.GetFine(), rfbHeader.GetDeadline());
            }
          }
          break;

        case OFFLOADINGTYPE_BID:
          packet->RemoveHeader (bidHeader);

          getOfferbid = bidHeader.GetOfferedBid()/pow(10,2);
          packetID = bidHeader.GetPacketID();

          //bid_index = FindBidNumber(bidHeader.GetSrc(), bidHeader.GetDst());
          cout << "["<< bidHeader.GetSrc() <<"->"<< bidHeader.GetDst() <<"]["<<packetID<<"]At " << Simulator::Now ().GetSeconds () << "s "
               << Ipv4Address::ConvertFrom (m_nodeAddr)
               << " received a BID from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ()
               << " Offered Bid = " << getOfferbid << endl;

          BufferBid(from, bidHeader.GetOfferedBid(), bidHeader.GetSrc(), bidHeader.GetDst());
          break;

        case OFFLOADINGTYPE_DATA:
          //DATA received.
          packet->RemoveHeader (dataHeader);
          packetID = dataHeader.GetPacketID();
          m_pu = dataHeader.GetHopcount() + 1; //Update 'pu' (hop count until here)
          //bid_index = FindBidNumber(dataHeader.GetSrc(), dataHeader.GetDst());

          cout << "["<< dataHeader.GetSrc() <<"->"<< dataHeader.GetDst() <<"]["<<packetID<<"]At "
                << Simulator::Now ().GetSeconds () << "s "<< Ipv4Address::ConvertFrom (m_nodeAddr)
                << " received a DATA from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << endl;

          if (isBackbone(m_nodeAddr)) { //If this node is an AP:
            if (m_nodeAddr == dataHeader.GetDst()) {
              if (m_pu <= m_deadline) {
                //If the packet arrived to the correct destination within deadline:
                cout << "-----------------------------------------------------------------------------------------" << endl;
                cout << "SUCCESSFULL PACKET [" << packetID << "] RECEIVED AT AP[" << dataHeader.GetDst() << "] FROM AP[" << dataHeader.GetSrc()
                     << "] - Hop Count \'pu\' = " << static_cast<int>(m_pu) << "." << endl;
                cout << "-----------------------------------------------------------------------------------------" << endl;

                //WRITE SUCCESSFULL PACKET:
                int pktStatus = 1;
                int sourceID = m_nodes_id[Ipv4Address::ConvertFrom(dataHeader.GetSrc())];
                int nextID = -1;
                WritePacket(sourceID, packetID, nextID, pktStatus, 0.0);
              } else {
                //If the packet arrived to the correct destination, but after the deadline:
                cout << "-----------------------------------------------------------------------------------------" << endl;
                cout << "(ALMOST) SUCCESSFULL PACKET [" << packetID << "] RECEIVED AT AP[" << dataHeader.GetDst() << "] FROM AP[" << dataHeader.GetSrc()
                     << "] - Hop Count \'pu\' = " << static_cast<int>(m_pu) << "." << endl;
                cout << "-----------------------------------------------------------------------------------------" << endl;

                //WRITE ALMOST SUCCESSFULL PACKET:
                int pktStatus = 3;
                int sourceID = m_nodes_id[Ipv4Address::ConvertFrom(dataHeader.GetSrc())];
                int nextID = -1;
                WritePacket(sourceID, packetID, nextID, pktStatus, 0.0);
              }
            } else {
              if (m_pu <= m_deadline) {
                cout << "-----------------------------------------------------------------------------------------" << endl;
                cout << "SUPER FINE [" << packetID << "] RECEIVED AT AP[" << dataHeader.GetDst() << "] FROM AP[" << dataHeader.GetSrc()
                     << "] - Hop Count \'pu\' = " << static_cast<int>(m_pu) << "." << endl;
                cout << "-----------------------------------------------------------------------------------------" << endl;
                //WRITE BACKBONE SUPER FINE PACKET:
                int pktStatus = 2;
                int sourceID = m_nodes_id[Ipv4Address::ConvertFrom(dataHeader.GetSrc())];
                int nextID = -1;
                WritePacket(sourceID, packetID, nextID, pktStatus, 0.0);
              } else {
                //If the packet arrived to the correct destination, but after the deadline:
                cout << "-----------------------------------------------------------------------------------------" << endl;
                cout << "(ALMOST) SUCCESSFULL PACKET [" << packetID << "] RECEIVED AT AP[" << dataHeader.GetDst() << "] FROM AP[" << dataHeader.GetSrc()
                     << "] - Hop Count \'pu\' = " << static_cast<int>(m_pu) << "." << endl;
                cout << "-----------------------------------------------------------------------------------------" << endl;

                //WRITE ALMOST SUCCESSFULL PACKET:
                int pktStatus = 3;
                int sourceID = m_nodes_id[Ipv4Address::ConvertFrom(dataHeader.GetSrc())];
                int nextID = -1;
                WritePacket(sourceID, packetID, nextID, pktStatus, 0.0);
              }
            }
          } else {
            if (m_pu < m_deadline) { //Caso esteja dentro do prazo, enviar RFB (ou DATA, caso seja Ãžltimo salto):
              if (IsLastHop(dataHeader.GetDst())) {
                toBackbone = true;
                SendData(dataHeader.GetSrc(),dataHeader.GetDst(), packetID, toBackbone);
              } else {
                cout << "BIDDER = " << m_nodeAddr << endl;
                //TODO Analyze a backbone delivery (the Budget X Throughput tradeoff). Not used in this strategy, but can be used at other strategies.
                SendRFB(dataHeader.GetSrc(),dataHeader.GetDst(), packetID);
              }
            } else {
              cout << ("Prazo (Deadline) estourado. Entregando pacote para o Backbone..." << endl;
              if (haveNeighborBackbone(dataHeader.GetDst(), dataHeader.GetSrc())) {
                //If the backbone is near,send to the AP neighbor (ALMOST SUCCESSFULL case):
                toBackbone = true;
                //TODO If m_dropDest is the source, report this as a loop...
                SendData(dataHeader.GetSrc(),m_dropDest, packetID, toBackbone);
              } else { //Caso backbone esteja distante, tratar pacote como perdido:
                cout << "----------------------------------------------------------------------------------" << endl;
                cout << "DROP PACKET [" << packetID << "] AT NODE[" << m_nodeAddr << "] FROM AP[" << dataHeader.GetSrc()
                     << "] - Hop Count \'pu\' = " << static_cast<int>(m_pu) << "." << endl;
                cout <<"----------------------------------------------------------------------------------" << endl;
                //WRITE DROP PACKET:
                int pktStatus = 4;
                int sourceID = m_nodes_id[Ipv4Address::ConvertFrom(dataHeader.GetSrc())];
                int nextID = -1;
                WritePacket(sourceID, packetID, nextID, pktStatus, 0.0);
              }
            }
          }
          break;
      }
    }
    m_rxTrace (packet, from);
  }
}

// Se receber Dados, enviar RFB
void DashOffloading::SendRFB(Ipv4Address source, Ipv4Address destination, int packetID)
{
  //int bid_index = 0;
  RFBHeader rfbHeader;
  rfbHeader.SetDeadline(m_deadline);
  rfbHeader.SetHopcount(m_pu);
  rfbHeader.SetPacketID(packetID);
  rfbHeader.SetSrcRFB(m_nodeMacAddr);
  rfbHeader.SetSrc(source);
  rfbHeader.SetDst(destination);
  if (node_type == NODETYPE_AP) {
    //m_backbonePair.push_back(std::make_pair(source,destination));
    //bid_index = FindBidNumber(source, destination);
    //ImprimeTopologySet ();
    //std::cout << std::endl;
    //std::cout << " NEW PACKET -> ID["<< packetID <<"] SOURCE["<< source <<"]" << std::endl;
    //std::cout <<"\n";

  } else {
    //bid_index = FindBidNumber(source, destination);
    //rfbHeader.SetSrc(source);
    //rfbHeader.SetDst(destination);
    //if(strategy_type == STRATEGYTYPE_TIGHTNESS){
    m_Bn = m_budget_percentage * m_offerBid;
    m_Fn = m_fine_percentage * m_Bn;
    cout << "Budget percentage = " << m_budget_percentage << ", Fine percentage = " << m_fine_percentage << endl;
    //}
  }

  cout << "Bn = " << m_Bn << endl;
  rfbHeader.SetB0(m_Bn*pow(10,2));
  rfbHeader.SetFine(m_Fn*pow(10,2));

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (rfbHeader);
  TypeHeader tHeader (OFFLOADINGTYPE_RFB);
  packet->AddHeader (tHeader);

  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (packet);

  SetRemote(Ipv4Address("255.255.255.255"),m_peerPort);
  InetSocketAddress dst = InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort);
  m_socket->Connect(dst);
  m_socket->Send (packet);
  m_tempo_envio = Simulator::Now ().GetSeconds ();
  double getBn = rfbHeader.GetB0()/pow(10,2);
  double getFn = rfbHeader.GetFine()/pow(10,2);
  if (Ipv4Address::IsMatchingType (m_peerAddress))
  {
    cout << "[" << source << "->" << destination << "][" << packetID << "] At "
         << Simulator::Now ().GetSeconds () << "s "
         << Ipv4Address::ConvertFrom (m_nodeAddr)
         << " sent a RFB. Budget B0 = " << getBn
         << " Fine F0 = " << getFn
         << " Deadline H0 = " << static_cast<int>(rfbHeader.GetDeadline())
         << " Hop Count 'pu' = " << static_cast<int>(rfbHeader.GetHopcount()) << endl;
  } else {
    cout << "[" << source << "->" << destination << "][" << packetID << "] At " << Simulator::Now ().GetSeconds () << "s "
         << Ipv4Address::ConvertFrom (m_nodeAddr)
         << " sent a RFB."
         << " Budget B0 = " << getBn
         << " Fine F0 = " << getFn
         << " Deadline H0 = " << static_cast<int>(rfbHeader.GetDeadline())
         << " Hop Count 'pu' = " << static_cast<int>(rfbHeader.GetHopcount()) << endl;
    m_send_dataEvent = Simulator::Schedule (Seconds (m_timeout), &DashOffloading::SendData, this, source, destination, packetID, false);
    if ((packetID < (m_nPackets - 1))&&(node_type==NODETYPE_AP)) {
      m_pu = 0;
      packetID++;
      m_send_AP_rfbEvent = Simulator::Schedule (Seconds (m_timeBetweenSourcePacket), &DashOffloading::SendRFB, this, m_nodeAddr, m_dest, packetID);
    }
  }
}

void DashOffloading::SendData (Ipv4Address source, Ipv4Address destination, int packetID, bool toBackbone)
{
}
