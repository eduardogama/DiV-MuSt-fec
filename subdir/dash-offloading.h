#ifndef OFFLOADING_H_
#define OFFLOADING_H_


#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include "ns3/mac48-address.h"
#include <map>
#include "ns3/nstime.h"

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"

#include <vector>
#include <iostream>
#include <string>
#include <list>
#include <limits> // for numeric_limits
#include <set>
#include <utility> // for pair
#include <algorithm>
#include <iterator>

//#include "utils.h"

using namespace ns3;
using namespace std;


class Socket;
class Packet;

enum StrategyType
{
  STRATEGYTYPE_DUMMYBID = 1, //!< STRATEGYTYPE_DUMMYBID
  STRATEGYTYPE_DUMMYPATH = 2, //!< STRATEGYTYPE_DUMMYPATH
  STRATEGYTYPE_TIGHTNESS = 3, //!< STRATEGYTYPE_TIGHTNESS
};

enum PreferenceFunctionType
{
  PREFERENCEFUNCTION_PLANE = 1, //!< PREFERENCEFUNCTION_PLANE
  PREFERENCEFUNCTION_GAUSS = 2, //!< PREFERENCEFUNCTION_GAUSS
  PREFERENCEFUNCTION_GAUSS_1 = 3, //!< PREFERENCEFUNCTION_GAUSS_1 (with cn=1)
};

enum NodeType
{
  NODETYPE_AP = 1, //!< NODETYPE_AP
  NODETYPE_NETWORK = 2, //!< NODETYPE_NETWORK
};

typedef int vertex_t;
typedef double weight_t;

struct neighbor {
  vertex_t target;
  weight_t weight;
  neighbor(vertex_t arg_target, weight_t arg_weight)
    : target(arg_target),
      weight(arg_weight)
  {}
};

typedef std::vector<std::vector<neighbor> > adjacency_list_t;

class DashOffloading
{
public:
  DashOffloading ();
  virtual ~DashOffloading ();

  void SetRemote (Address ip, uint16_t port);
  void SetRemote (Ipv4Address ip, uint16_t port);
  void SetBackbone (vector<Ipv4Address > backbone);
  void SetTightnessParameters (double k1, double k2, double budget_percentage, double fine_percentage);
  void SetMapNodes (map<Ipv4Address, int> nodes_id);
  void SetTopologyName (string topologyName);
  void SetSeedIndex (int seedIndex);
  void SetParamName (string paramName);
  void SetExpIndex (string expIndex);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void SendData (Ipv4Address source, Ipv4Address destination, int dataEventNumber, bool toBackbone);
  void SendBid(Ptr<Socket> socket, uint32_t Bu, uint32_t Fu, uint8_t H0, int packetID);
  void BufferBid(Address from, uint32_t bid, Ipv4Address source, Ipv4Address destination);
  void SendRFB(Ipv4Address source, Ipv4Address destination, int packetID);

  void ScheduleSendFirstRFB (Time dt);
  void ScheduleSendData (Time dt);
  bool IsLastHop (Ipv4Address dest);
  //void SendLastHopData(void);
  bool isBackbone(Ipv4Address ipv4address);
  bool haveNeighborBackbone(Ipv4Address dest, Ipv4Address src);
  bool isOldPacket(Ipv4Address source, int packetID);

  //int FindBidNumber(Ipv4Address source, Ipv4Address destination);
  void HandleRead (Ptr<Socket> socket);
  void ImprimeTopologySet (void);
  void WritePacket(int sourceID, int pktID, int nextID, int status, double reserved);

  void HopCountComputation (Ipv4Address dest, int nodetype);
  void BidComputation (uint32_t Bu, uint32_t Fu, uint8_t H0);

  void PopulateArpCache (void);

  void MapNode (void);
  void BuildGraphDijkstra (adjacency_list_t &grafo_dijkstra);
  void ImprimeGrafo (bool sort, adjacency_list_t &grafo_dijkstra);
  bool FindGraphEdge (int a, int b, adjacency_list_t &grafo_dijkstra);
  Ipv4Address FindMapAddress (int a);
  void DijkstraComputePaths(vertex_t source,
                            const adjacency_list_t &adjacency_list,
                            std::vector<weight_t> &min_distance,
                            std::vector<vertex_t> &previous);
  list<vertex_t> DijkstraGetShortestPathTo(vertex_t vertex, const vector<vertex_t> &previous);

  double m_offerBid;
  double m_Bn;
  double m_Fn;
  double m_payFINE, m_winner_bid;
  uint8_t m_deadline;

  //Tightness strategy parameters:
  double m_k1, m_k2, m_budget_percentage, m_fine_percentage;

  //Hop count and delta:
  vector<int> m_hc;
  vector<int> m_delta, m_able_nodes;
  uint8_t m_pu;

  int m_nPackets;
  Ipv4Address m_dest, m_source;
  //std::vector<std::pair<Ipv4Address, Ipv4Address> > m_backbonePair;//Vetor de leilÃţes que estÃčo ocorrendo simultaneamente neste nÃş
  Ipv4Address m_nodeAddr;
  Mac48Address m_nodeMacAddr;
  Address m_upload;
  Mac48Address m_uploadMAC;
  Ipv4Address m_dropDest;

  double m_tempo_envio;
  double m_start;
  vector<pair<Ipv4Address, int> > m_loopControl;//Fila de leilÃţes que jÃą terminaram neste nÃş
  int m_pkt_count;
  double m_timeout;
  double m_timeBetweenSourcePacket;

  Ptr<Node> m_thisNode;
  vector<uint32_t > m_cn_buffer;
  vector<uint32_t > m_bid_buffer;
  vector<Address > m_address_buffer;
  vector<Ipv4Address > m_source_buffer;
  vector<Ipv4Address > m_destination_buffer;
  vector<Ipv4Address > m_backbone;
  vector<Ipv4Address > m_hc_address;

  map<Ipv4Address, int> m_map_address_graphnode, m_nodes_id;

  Ptr<Socket> m_socket;
  Address m_peerAddress;
  uint16_t m_peerPort;
  EventId m_send_AP_rfbEvent;
  EventId m_send_dataEvent;
  EventId m_send_bidEvent;

  //Callbacks for tracing the packet Tx and Rx events:
  TracedCallback<Ptr<const Packet> > m_txTrace;
  TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;

  //Lado "Servidor":
  uint16_t m_port;
  Address m_local;

  uint16_t node_type, strategy_type, preference_function_type;
  int number_nodes;

  //Create files to report auctions and budgets:
  FILE *report_user;
  string dir_myreport_user, filename_report_user, content, m_topologyName, m_paramName;
  int m_seedIndex;
  string m_expIndex;

  int m_wrong_order;
};


#endif // OFFLOADING_H_
