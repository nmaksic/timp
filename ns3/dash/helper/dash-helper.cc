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
#include "ns3/dash.h"
#include "ns3/dash-global.h"
#include "ns3/dash-helper.h"

namespace ns3 {

DashHelper::DashHelper () {
  m_factory.SetTypeId ("ns3::Dash");
  m_dashGlobalFactory.SetTypeId ("ns3::DashGlobal");
}

DashHelper::DashHelper (const DashHelper &o)
  : m_factory (o.m_factory) {
}

DashHelper::~DashHelper () {
}

DashHelper*
DashHelper::Copy (void) const {
  return new DashHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
DashHelper::Create (Ptr<Node> node) const {
  Ptr<Dash> dash = m_factory.Create<Dash> ();


  node->AggregateObject (dash);
  return dash;
}

void
DashHelper::Set (std::string name, const AttributeValue &value) {
  m_factory.Set (name, value);
}


Ptr<DashGlobal>
DashHelper::GetDashGlobal () {
  if (!m_dashGlobal)
    {
      m_dashGlobal = m_dashGlobalFactory.Create<DashGlobal> ();
    }
  return m_dashGlobal;
}

}

