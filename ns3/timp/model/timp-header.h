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

#ifndef TIMP_HEADER_H
#define TIMP_HEADER_H

#include <list>
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"


namespace ns3 {

class TimpUpdateHeader : public Header
{
public:
  TimpUpdateHeader (void);

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


  void SetCommand (Command_e command);

  Command_e GetCommand (void) const;

  void SetDstId (uint32_t dstId);

  uint32_t GetDstId (void) const;

  void SetDstPrice (uint32_t dstPrice);

  uint32_t GetDstPrice (void) const;

private:
  uint8_t m_command; 
  uint32_t m_dstId;
  uint32_t m_dstPrice;
};

class TimpTag : public Tag
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

}


#endif /* Timp_HEADER_H */
