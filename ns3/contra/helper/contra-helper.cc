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
#include "ns3/contra.h"
#include "ns3/contra-global.h"
#include "ns3/contra-helper.h"

namespace ns3 {

ContraHelper::ContraHelper () {
  m_factory.SetTypeId ("ns3::Contra");
  m_contraGlobalFactory.SetTypeId ("ns3::ContraGlobal");
}

ContraHelper::ContraHelper (const ContraHelper &o)
  : m_factory (o.m_factory) {
}

ContraHelper::~ContraHelper () {
}

ContraHelper*
ContraHelper::Copy (void) const {
  return new ContraHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
ContraHelper::Create (Ptr<Node> node) const {
  Ptr<Contra> contra = m_factory.Create<Contra> ();


  node->AggregateObject (contra);
  return contra;
}

void
ContraHelper::Set (std::string name, const AttributeValue &value) {
  m_factory.Set (name, value);
}


Ptr<ContraGlobal>
ContraHelper::GetContraGlobal () {
  if (!m_contraGlobal)
    {
      m_contraGlobal = m_contraGlobalFactory.Create<ContraGlobal> ();
    }
  return m_contraGlobal;
}

}
