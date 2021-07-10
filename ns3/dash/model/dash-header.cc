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

#include "dash-header.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (DashUpdateHeader);


DashUpdateHeader::DashUpdateHeader ()
  : m_origin (0), m_pid (0), m_tag (0), m_mv(0) {
}

TypeId DashUpdateHeader::GetTypeId (void) {
  static TypeId tid = TypeId ("ns3::DashUpdateHeader")
    .SetParent<Header> ()
    .SetGroupName ("Internet")
    .AddConstructor<DashUpdateHeader> ();
  return tid;
}

TypeId DashUpdateHeader::GetInstanceTypeId (void) const {
  return GetTypeId ();
}

void DashUpdateHeader::Print (std::ostream & os) const {

  os << " origin " << m_origin;
  os << " pid " << m_pid;
  os << " tag " << m_tag;
  os << " mv " << m_mv;
  os << " version " << m_version;

}

uint32_t DashUpdateHeader::GetSerializedSize () const {
  return 28; 
}

void DashUpdateHeader::Serialize (Buffer::Iterator start) const {
  Buffer::Iterator i = start;

  i.WriteU32 (m_origin);
  i.WriteU32 (m_pid);
  i.WriteU32 (m_tag);
  i.WriteU64 (m_mv);
  i.WriteU64 (m_version);

}

uint32_t DashUpdateHeader::Deserialize (Buffer::Iterator start) {
  Buffer::Iterator i = start;

  m_origin = i.ReadU32 ();
  m_pid = i.ReadU32 ();
  m_tag = i.ReadU32 ();
  m_mv = i.ReadU64 ();
  m_version = i.ReadU64 ();

  return GetSerializedSize ();
}

void DashUpdateHeader::SetOrigin (uint32_t origin) {
  m_origin = origin;
}

uint32_t DashUpdateHeader::GetOrigin (void) const {
  return m_origin;
}

void DashUpdateHeader::SetPid (uint32_t pid) {
  m_pid = pid;
}

uint32_t DashUpdateHeader::GetPid (void) const {
  return m_pid;
}

void DashUpdateHeader::SetTag (uint32_t tag) {
  m_tag = tag;
}

uint32_t DashUpdateHeader::GetTag (void) const {
  return m_tag;
}

void DashUpdateHeader::SetMv (uint64_t mv) {
  m_mv = mv;
}

uint64_t DashUpdateHeader::GetMv (void) const {
  return m_mv;
}

void DashUpdateHeader::SetVersion (uint64_t version) {
  m_version = version;
}

uint64_t DashUpdateHeader::GetVersion (void) const {
  return m_version;
}

TypeId 
DashTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DashTag")
    .SetParent<Tag> ()
    .AddConstructor<DashTag> ()
    .AddAttribute ("SimpleValue",
                   "A simple value",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&DashTag::GetSimpleValue),
                   MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}
TypeId 
DashTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
DashTag::GetSerializedSize (void) const
{
  return 1;
}
void 
DashTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_simpleValue);
}
void 
DashTag::Deserialize (TagBuffer i)
{
  m_simpleValue = i.ReadU8 ();
}
void 
DashTag::Print (std::ostream &os) const
{
  os << "v=" << (uint32_t)m_simpleValue;
}
void 
DashTag::SetSimpleValue (uint8_t value)
{
  m_simpleValue = value;
}
uint8_t 
DashTag::GetSimpleValue (void) const
{
  return m_simpleValue;
}


TypeId 
DashPacketTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DashPacketTag")
    .SetParent<Tag> ()
    .AddConstructor<DashTag> ()
    .AddAttribute ("Pid",
                   "Pid",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&DashPacketTag::GetPid),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Tag",
                   "Tag",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&DashPacketTag::GetPid),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}
TypeId 
DashPacketTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
DashPacketTag::GetSerializedSize (void) const
{
  return 8;
}
void 
DashPacketTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_pid);
  i.WriteU32 (m_tag);
}
void 
DashPacketTag::Deserialize (TagBuffer i)
{
  m_pid = i.ReadU32 ();
  m_tag = i.ReadU32 ();
}
void 
DashPacketTag::Print (std::ostream &os) const
{
  os << "pid=" << m_pid << " tag=" << m_tag;
}
void 
DashPacketTag::SetPid (uint32_t value)
{
  m_pid = value;
}
uint32_t 
DashPacketTag::GetPid (void) const
{
  return m_pid;
}
void 
DashPacketTag::SetTag (uint32_t value)
{
  m_tag = value;
}
uint32_t 
DashPacketTag::GetTag (void) const
{
  return m_tag;
}

}


