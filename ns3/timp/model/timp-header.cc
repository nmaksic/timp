/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Natasa Maksic <maksicn@etf.rs>
 *
 */

#include "timp-header.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TimpUpdateHeader);


TimpUpdateHeader::TimpUpdateHeader ()
  : m_command (0), m_dstId (0), m_dstPrice (1001) {
}

TypeId TimpUpdateHeader::GetTypeId (void) {
  static TypeId tid = TypeId ("ns3::TimpUpdateHeader")
    .SetParent<Header> ()
    .SetGroupName ("Internet")
    .AddConstructor<TimpUpdateHeader> ();
  return tid;
}

TypeId TimpUpdateHeader::GetInstanceTypeId (void) const {
  return GetTypeId ();
}

void TimpUpdateHeader::Print (std::ostream & os) const {
  os << " command " << int(m_command);
  os << " dstId " << int(m_dstId);
  os << " dstPrice " << int(m_dstPrice);
}

uint32_t TimpUpdateHeader::GetSerializedSize () const {
  return 9;
}

void TimpUpdateHeader::Serialize (Buffer::Iterator start) const {
  Buffer::Iterator i = start;

  i.WriteU8 (uint8_t (m_command));
  i.WriteU32 (m_dstId);
  i.WriteU32 (m_dstPrice);

}

uint32_t TimpUpdateHeader::Deserialize (Buffer::Iterator start) {
  Buffer::Iterator i = start;

  uint8_t temp;
  temp = i.ReadU8 ();
  if (temp == UPDATE)
      m_command = temp;
  else
      return 0;

  m_dstId = i.ReadU32 ();
  m_dstPrice = i.ReadU32 ();

  return GetSerializedSize ();
}

void TimpUpdateHeader::SetCommand (TimpUpdateHeader::Command_e command) {
  m_command = command;
}

TimpUpdateHeader::Command_e TimpUpdateHeader::GetCommand () const {
  return TimpUpdateHeader::Command_e (m_command);
}

void TimpUpdateHeader::SetDstId (uint32_t dstId) {
  m_dstId = dstId;
}

uint32_t TimpUpdateHeader::GetDstId (void) const {
  return m_dstId;
}

void TimpUpdateHeader::SetDstPrice (uint32_t dstPrice) {
  m_dstPrice = dstPrice;
}

uint32_t TimpUpdateHeader::GetDstPrice (void) const {
  return m_dstPrice;
}

TypeId 
TimpTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TimpTag")
    .SetParent<Tag> ()
    .AddConstructor<TimpTag> ()
    .AddAttribute ("SimpleValue",
                   "A simple value",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&TimpTag::GetSimpleValue),
                   MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}
TypeId 
TimpTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
TimpTag::GetSerializedSize (void) const
{
  return 1;
}
void 
TimpTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_simpleValue);
}
void 
TimpTag::Deserialize (TagBuffer i)
{
  m_simpleValue = i.ReadU8 ();
}
void 
TimpTag::Print (std::ostream &os) const
{
  os << "v=" << (uint32_t)m_simpleValue;
}
void 
TimpTag::SetSimpleValue (uint8_t value)
{
  m_simpleValue = value;
}
uint8_t 
TimpTag::GetSimpleValue (void) const
{
  return m_simpleValue;
}


}
