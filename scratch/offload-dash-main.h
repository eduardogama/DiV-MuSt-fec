#ifndef OFFLOAD_DASH_MAIN_H_
#define OFFLOAD_DASH_MAIN_H_

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"

using namespace ns3;
using namespace std;


namespace ns3 {

class DashOffloading : public Application
{
public:
  DashOffloading ();
  virtual ~DashOffloading ();

  static TypeId GetTypeId (void);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

}

TypeId DashOffloading::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DashOffloading")
    .SetParent<Application> ()
    .SetGroupName ("Applications")
    .AddConstructor<DashOffloading> ()
  ;

  return tid;
}

void DashOffloading::StartApplication (void)
{
}

void DashOffloading::StopApplication (void)
{
}

}

#endif // OFFLOAD_DASH_MAIN_H_
