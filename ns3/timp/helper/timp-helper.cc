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

#include "ns3/node.h"
#include "ns3/node-list.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/timp.h"
#include "ns3/timp-global.h"
#include "ns3/timp-helper.h"

namespace ns3 {

TimpHelper::TimpHelper () {
  m_factory.SetTypeId ("ns3::Timp");
  m_timpGlobalFactory.SetTypeId ("ns3::TimpGlobal");
}

TimpHelper::TimpHelper (const TimpHelper &o)
  : m_factory (o.m_factory) {
}

TimpHelper::~TimpHelper () {
}

TimpHelper*
TimpHelper::Copy (void) const {
  return new TimpHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
TimpHelper::Create (Ptr<Node> node) const {
  Ptr<Timp> timp = m_factory.Create<Timp> ();


  node->AggregateObject (timp);
  return timp;
}

void
TimpHelper::Set (std::string name, const AttributeValue &value) {
  m_factory.Set (name, value);
}


Ptr<TimpGlobal>
TimpHelper::GetTimpGlobal () {
  if (!m_timpGlobal)
    {
      m_timpGlobal = m_timpGlobalFactory.Create<TimpGlobal> ();
    }
  return m_timpGlobal;
}

}

