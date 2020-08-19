//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Natasa Maksic <maksicn@etf.rs>
//

#include "timp-global.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include <fstream>
#include <sstream>
#include "ns3/ipv4-list-routing.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TimpGlobal")
  ;

NS_OBJECT_ENSURE_REGISTERED (TimpGlobal)
  ;


TypeId 
TimpGlobal::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TimpGlobal")
    .SetParent<Object> ()
    .AddConstructor<TimpGlobal> ();
  return tid;
}

TypeId 
TimpGlobal::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

TimpGlobal::TimpGlobal ()
  : m_enabled (false)
{
}

void
TimpGlobal::DoDispose (void)
{
  for (uint32_t i = 0; i < m_sabLayers.size (); i++)
    {
      m_sabLayers[i]->Dispose ();
      m_sabLayers[i] = 0;
    }
  Object::DoDispose (); 
}


Ptr<Timp>
TimpGlobal::GetTimpRouting (Ptr<Ipv4> ipv4) const
{
   NS_LOG_FUNCTION (this);
   Ptr<Ipv4RoutingProtocol> ipv4rp = ipv4->GetRoutingProtocol ();
   if (DynamicCast<Timp> (ipv4rp))
      return DynamicCast<Timp> (ipv4rp); 
   if (DynamicCast<Ipv4ListRouting> (ipv4rp)){
      Ptr<Ipv4ListRouting> lrp = DynamicCast<Ipv4ListRouting> (ipv4rp);
      int16_t priority;
      for (uint32_t i = 0; i < lrp->GetNRoutingProtocols ();  i++) {
         Ptr<Ipv4RoutingProtocol> temp = lrp->GetRoutingProtocol (i, priority);
         if (DynamicCast<Timp> (temp)) 
            return DynamicCast<Timp> (temp);
      }
   }
   return 0;
}


void
TimpGlobal::Initialize (NodeContainer devices, std::list< int > &torlist)
{
      char routesfile[100];
      sprintf(routesfile, "routes1.txt");

		std::ifstream routes(routesfile);
		std::string nodeidstr;
		std::string destnetwork;
		std::string destnetworkmask;
		std::string interfacestr;
      std::string neighborstr;
      std::string neighborifcstr;
      std::string neighborifcadr;
      std::string dststr;

		if (routes.is_open()) {
		   while ( routes.good() ) {
			   getline (routes, nodeidstr);
			   getline (routes, destnetwork);
			   getline (routes, destnetworkmask);
			   getline (routes, interfacestr);
            getline (routes, neighborstr);
            getline (routes, neighborifcstr);
            getline (routes, neighborifcadr);
            getline (routes, dststr);

			
			   int nodeid = atoi(nodeidstr.c_str());
			   int interface = atoi(interfacestr.c_str());

            int nbr = atoi(neighborstr.c_str());
            int nbrifc = atoi(neighborifcstr.c_str());
            int dst = atoi(dststr.c_str());


            Ptr<Ipv4> ipv4 = devices.Get(nodeid)->GetObject<Ipv4>();
            Ptr<Timp> timp = GetTimpRouting(ipv4);
            // hosts do not have routing
            if (timp != 0) {  
               Ipv4Address network(destnetwork.c_str());
			      Ipv4Mask networkmask(destnetworkmask.c_str());			
               timp->AddNetworkRouteTo(network, networkmask, interface);


               bool found = (std::find(torlist.begin(), torlist.end(), dst) != torlist.end());
               if (found) {
                  // add information for tor switches network
                  Ipv4Mask networkmask1("255.255.255.255");
                  timp->AddTorRouteTo(network, networkmask1, dst, interface);
               }
            }


            Ptr<Ipv4> ipv41 = devices.Get(nbr)->GetObject<Ipv4>();
            Ptr<Timp> timp1 = GetTimpRouting(ipv41);
            if (timp1 != 0) {  
                  timp1->AddPreviousPort(dst, nbrifc, neighborifcadr);
            }


            if (timp != 0)
            if (std::find(rtrpointers.begin(), rtrpointers.end(), timp) == rtrpointers.end()) {
               rtrpointers.push_back(timp);
            }


		     }
		    routes.close();
		}

      for(std::vector<Ptr<Timp>>::iterator it = rtrpointers.begin(); it != rtrpointers.end(); ++it) 
         (*it)->PrintPrices(true, torlist.size());
      

}

void
TimpGlobal::PrintStatistics ()
{
   for(std::vector<Ptr<Timp>>::iterator it = rtrpointers.begin(); it != rtrpointers.end(); ++it) 
      (*it)->PrintStatistics();
}

} // namespace ns3

