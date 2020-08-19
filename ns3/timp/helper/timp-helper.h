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

#ifndef TIMP_HELPER_H
#define TIMP_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/node-container.h"
#include "ns3/node.h"

namespace ns3 {

class TimpHelper : public Ipv4RoutingHelper
{
public:

  TimpHelper ();


  TimpHelper (const TimpHelper &o);

  virtual ~TimpHelper ();

  TimpHelper* Copy (void) const;


  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;


  void Set (std::string name, const AttributeValue &value);

  Ptr<TimpGlobal> GetTimpGlobal ();

private:

  TimpHelper &operator = (const TimpHelper &o);

  ObjectFactory m_factory; 
  ObjectFactory m_timpGlobalFactory; 

  Ptr<TimpGlobal> m_timpGlobal;      

};

} 


#endif /* TIMP_HELPER_H */

