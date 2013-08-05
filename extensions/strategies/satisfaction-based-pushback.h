/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2013 University of California, Los Angeles
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */


#ifndef SATISFACTION_BASED_PUSHBACK_H
#define SATISFACTION_BASED_PUSHBACK_H

#include <ns3/event-id.h>

#include "stats.h"

namespace ns3 {
namespace ndn {
namespace fw {

/**
 * \ingroup ndn
 * \brief Strategy implementing per-FIB entry limits
 */
template<class Parent>
class SatisfactionBasedPushback :
    public Stats<Parent>
{
private:
  typedef Stats<Parent> super;

public:
  static TypeId
  GetTypeId ();

  /**
   * @brief Default constructor
   */
  SatisfactionBasedPushback ()
    : m_announceEventScheduled (false)
  {
  }

  static std::string
  GetLogName ();
  
  virtual void
  OnInterest (Ptr<Face> face,
              Ptr<Interest> interest);

  virtual void
  AddFace (Ptr<Face> face);
  
private:
  void
  AnnounceLimits ();

  void
  ApplyAnnouncedLimit (Ptr<Face> inFace,
                       Ptr<const Interest> interest);

protected:
  // from Object
  virtual void
  NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object

  virtual void
  DoDispose ();
    
private:
  static LogComponent g_log;
  
  bool m_announceEventScheduled;

  double m_graceThreshold;

  TracedCallback< Ptr<const Face>, double /* ratio */, double/* adjusted weight */, double /* limit */ > m_onLimitsAnnounce;
};


} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // SATISFACTION_BASED_PUSHBACK_H
