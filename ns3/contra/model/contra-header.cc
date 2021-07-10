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

#include "contra-header.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ContraUpdateHeader);


ContraUpdateHeader::ContraUpdateHeader ()
  : m_origin (0), m_pid (0), m_tag (0), m_mv(0) {
}

TypeId ContraUpdateHeader::GetTypeId (void) {
  static TypeId tid = TypeId ("ns3::ContraUpdateHeader")
    .SetParent<Header> ()
    .SetGroupName ("Internet")
    .AddConstructor<ContraUpdateHeader> ();
  return tid;
}

TypeId ContraUpdateHeader::GetInstanceTypeId (void) const {
  return GetTypeId ();
}

void ContraUpdateHeader::Print (std::ostream & os) const {

  os << " origin " << m_origin;
  os << " pid " << m_pid;
  os << " tag " << m_tag;
  os << " mv " << m_mv;
  os << " version " << m_version;

}

uint32_t ContraUpdateHeader::GetSerializedSize () const {
  return 28; //13;
}

void ContraUpdateHeader::Serialize (Buffer::Iterator start) const {
  Buffer::Iterator i = start;

  i.WriteU32 (m_origin);
  i.WriteU32 (m_pid);
  i.WriteU32 (m_tag);
  i.WriteU64 (m_mv);
  i.WriteU64 (m_version);

}

uint32_t ContraUpdateHeader::Deserialize (Buffer::Iterator start) {
  Buffer::Iterator i = start;

  m_origin = i.ReadU32 ();
  m_pid = i.ReadU32 ();
  m_tag = i.ReadU32 ();
  m_mv = i.ReadU64 ();
  m_version = i.ReadU64 ();

  return GetSerializedSize ();
}

void ContraUpdateHeader::SetOrigin (uint32_t origin) {
  m_origin = origin;
}

uint32_t ContraUpdateHeader::GetOrigin (void) const {
  return m_origin;
}

void ContraUpdateHeader::SetPid (uint32_t pid) {
  m_pid = pid;
}

uint32_t ContraUpdateHeader::GetPid (void) const {
  return m_pid;
}

void ContraUpdateHeader::SetTag (uint32_t tag) {
  m_tag = tag;
}

uint32_t ContraUpdateHeader::GetTag (void) const {
  return m_tag;
}

void ContraUpdateHeader::SetMv (uint64_t mv) {
  m_mv = mv;
}

uint64_t ContraUpdateHeader::GetMv (void) const {
  return m_mv;
}

void ContraUpdateHeader::SetVersion (uint64_t version) {
  m_version = version;
}

uint64_t ContraUpdateHeader::GetVersion (void) const {
  return m_version;
}

TypeId 
ContraTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ContraTag")
    .SetParent<Tag> ()
    .AddConstructor<ContraTag> ()
    .AddAttribute ("SimpleValue",
                   "A simple value",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&ContraTag::GetSimpleValue),
                   MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}
TypeId 
ContraTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
ContraTag::GetSerializedSize (void) const
{
  return 1;
}
void 
ContraTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_simpleValue);
}
void 
ContraTag::Deserialize (TagBuffer i)
{
  m_simpleValue = i.ReadU8 ();
}
void 
ContraTag::Print (std::ostream &os) const
{
  os << "v=" << (uint32_t)m_simpleValue;
}
void 
ContraTag::SetSimpleValue (uint8_t value)
{
  m_simpleValue = value;
}
uint8_t 
ContraTag::GetSimpleValue (void) const
{
  return m_simpleValue;
}


TypeId 
ContraPacketTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ContraPacketTag")
    .SetParent<Tag> ()
    .AddConstructor<ContraTag> ()
    .AddAttribute ("Pid",
                   "Pid",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&ContraPacketTag::GetPid),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Tag",
                   "Tag",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&ContraPacketTag::GetPid),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}
TypeId 
ContraPacketTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
ContraPacketTag::GetSerializedSize (void) const
{
  return 8;
}
void 
ContraPacketTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_pid);
  i.WriteU32 (m_tag);
}
void 
ContraPacketTag::Deserialize (TagBuffer i)
{
  m_pid = i.ReadU32 ();
  m_tag = i.ReadU32 ();
}
void 
ContraPacketTag::Print (std::ostream &os) const
{
  os << "pid=" << m_pid << " tag=" << m_tag;
}
void 
ContraPacketTag::SetPid (uint32_t value)
{
  m_pid = value;
}
uint32_t 
ContraPacketTag::GetPid (void) const
{
  return m_pid;
}
void 
ContraPacketTag::SetTag (uint32_t value)
{
  m_tag = value;
}
uint32_t 
ContraPacketTag::GetTag (void) const
{
  return m_tag;
}

}


