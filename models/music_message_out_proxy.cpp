/*
 *  music_message_out_proxy.cpp
 *
 *  This file is part of NEST.
 *
 *  Copyright (C) 2004 The NEST Initiative
 *
 *  NEST is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NEST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NEST.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#ifdef HAVE_MUSIC

#include "music_message_out_proxy.h"
#include "network.h"
#include "integerdatum.h"
#include "doubledatum.h"
#include "arraydatum.h"
#include "music.hh"


/* ---------------------------------------------------------------- 
 * Default constructors defining default parameters and state
 * ---------------------------------------------------------------- */

nest::music_message_out_proxy::Parameters_::Parameters_()
  : port_name_("message_out"),
    max_buffered_(-1)
{}

nest::music_message_out_proxy::Parameters_::Parameters_(const Parameters_& op)
  : port_name_(op.port_name_),
    max_buffered_(op.max_buffered_)
{}

nest::music_message_out_proxy::State_::State_()
   : published_(false)
{}


/* ---------------------------------------------------------------- 
 * Parameter extraction and manipulation functions
 * ---------------------------------------------------------------- */

void nest::music_message_out_proxy::Parameters_::get(DictionaryDatum &d) const
{
  (*d)[names::port_name] = port_name_; 
  (*d)["max_buffered"] = max_buffered_;
}
 
void nest::music_message_out_proxy::Parameters_::set(const DictionaryDatum& d, State_& s)
{
  if (!s.published_)
  {
    updateValue<string>(d, names::port_name, port_name_);
    updateValue<long>(d, "max_buffered", max_buffered_);
  }
}

void nest::music_message_out_proxy::State_::get(DictionaryDatum &d) const
{
  (*d)["published"] = published_; 
} 

void nest::music_message_out_proxy::State_::set(const DictionaryDatum&, const Parameters_&)
{}


/* ---------------------------------------------------------------- 
 * Default and copy constructor for node
 * ---------------------------------------------------------------- */

nest::music_message_out_proxy::music_message_out_proxy()
  : Node(),
    P_(),
    S_()
{}

nest::music_message_out_proxy::music_message_out_proxy(const music_message_out_proxy& n)
  : Node(n), 
    P_(n.P_),
    S_(n.S_)
{}


/* ---------------------------------------------------------------- 
 * Node initialization functions
 * ---------------------------------------------------------------- */

void nest::music_message_out_proxy::init_state_(const Node& proto)
{ 
  const music_message_out_proxy& pr = downcast<music_message_out_proxy>(proto);

  S_ = pr.S_;
}

void nest::music_message_out_proxy::init_buffers_()
{}

void nest::music_message_out_proxy::calibrate()
{
  // only publish the port once,
  if (!S_.published_)
  {
    MUSIC::Setup* s = nest::Communicator::get_music_setup();
    if (s == 0)
      throw MUSICSimulationHasRun(get_name());

    V_.MP_ = s->publishMessageOutput(P_.port_name_);

    if (!V_.MP_->isConnected())
      throw MUSICPortUnconnected(get_name(), P_.port_name_);
  
    int max_buffered = P_.max_buffered_;

    if (max_buffered > 0)
      V_.MP_->map(max_buffered);
    else
      V_.MP_->map();
    S_.published_ = true;

    std::string msg = String::compose("Mapping MUSIC output port '%1' with max buf=%2.",
                                      P_.port_name_, P_.max_buffered_);
    net_->message(SLIInterpreter::M_INFO, "music_message_out_proxy::calibrate()", msg.c_str());
  }
}

#endif
