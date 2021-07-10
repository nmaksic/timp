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

#ifndef CONTRA_HEADER_H
#define CONTRA_HEADER_H

#include <list>
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"


namespace ns3 {

class ContraUpdateHeader : public Header
{
public:
  ContraUpdateHeader (void);

  static TypeId GetTypeId (void);

  virtual TypeId GetInstanceTypeId (void) const;

  virtual void Print (std::ostream& os) const;

  virtual uint32_t GetSerializedSize (void) const;

  virtual void Serialize (Buffer::Iterator start) const;

  virtual uint32_t Deserialize (Buffer::Iterator start);


  enum Command_e
  {
    UPDATE = 0x1
  };

  void SetOrigin (uint32_t origin);

  uint32_t GetOrigin (void) const;

  void SetPid (uint32_t pid);

  uint32_t GetPid (void) const;

  void SetTag (uint32_t tag);

  uint32_t GetTag (void) const;

  void SetMv (uint64_t mv);

  uint64_t GetMv (void) const;
  
  void SetVersion (uint64_t mv);

  uint64_t GetVersion (void) const;

private:

  uint32_t m_origin;
  uint32_t m_pid;
  uint32_t m_tag;
  uint64_t m_mv; // metrics vector - utilization
  uint64_t m_version; // probe version for loop prevention, name version is used in the Contra paper
};


class ContraTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  // these are our accessors to our tag structure
  /**
   * Set the tag value
   * \param value The tag value.
   */
  void SetSimpleValue (uint8_t value);
  /**
   * Get the tag value
   * \return the tag value.
   */
  uint8_t GetSimpleValue (void) const;
private:
  uint8_t m_simpleValue;  //!< tag value
};

class ContraPacketTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  // these are our accessors to our tag structure
  /**
   * Set the pid value
   * \param value The pid value.
   */
  void SetPid (uint32_t value);
  /**
   * Get the pid value
   * \return the pid value.
   */
  uint32_t GetPid (void) const;
    /**
   * Set the tag value
   * \param value The tag value.
   */
  void SetTag (uint32_t value);
  /**
   * Get the tag value
   * \return the tag value.
   */
  uint32_t GetTag (void) const;
private:
  uint32_t m_pid;  //!< tag value
  uint32_t m_tag;  //!< tag value
};

}

#endif /* Contra_HEADER_H */

