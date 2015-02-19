/*
 *  music_message_out_proxy.h
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

#ifndef MUSIC_MESSAGE_OUT_PROXY_H
#define MUSIC_MESSAGE_OUT_PROXY_H

#include "config.h"
#ifdef HAVE_MUSIC

#include <vector>
#include <string>
#include "nest.h"
#include "node.h"
#include "communicator.h"
#include "arraydatum.h"
#include "dictutils.h"
#include "stringdatum.h"
#include "compose.hpp"

#include "mpi.h"
#include "music.hh"

/*BeginDocumentation

Name: music_message_out_proxy - A device which receives message strings from MUSIC.

Description:
A music_message_out_proxy can be used to receive message strings from
remote MUSIC applications in NEST.

It uses the MUSIC library to receive message strings from other
applications. The music_message_out_proxy represents an input port to
which MUSIC can connect a message source. The music_message_out_proxy
can queried using GetStatus to retrieve the messages.
      	 
Parameters:
The following properties are available in the status dictionary:

port_name      - The name of the MUSIC input port to listen to (default:
                 message_out)
port_width     - The width of the MUSIC input port
data           - A sub-dictionary that contains the string messages
                 in the form of two arrays:
                 messages      - The strings
                 message_times - The times the messages were sent (ms)
n_messages     - The number of messages.
published      - A bool indicating if the port has been already published
                 with MUSIC

The parameter port_name can be set using SetStatus. The field n_messages
can be set to 0 to clear the data arrays.

Examples:
/music_message_out_proxy Create /mmip Set
10 Simulate
mmip GetStatus /data get /messages get 0 get /command Set
(Executing command ') command join ('.) join =
command cvx exec

Author: Jochen Martin Eppler
FirstVersion: July 2010
Availability: Only when compiled with MUSIC

SeeAlso: music_event_out_proxy, music_event_in_proxy, music_cont_in_proxy
*/

namespace nest
{
  /**
   * Emit spikes at times received from another application via a
   * MUSIC port. The timestamps of the events also contain offsets,
   * which makes it also useful for precise spikes.
   */
  class music_message_out_proxy : public Node
  {
    
  public:      
    music_message_out_proxy();
    music_message_out_proxy(const music_message_out_proxy&);

    bool has_proxies() const {return false;}
    bool one_node_per_process() const {return true;}
    
    void get_status(DictionaryDatum &) const;
    void set_status(const DictionaryDatum &);

  private:
    
    void init_state_(const Node&);
    void init_buffers_();
    void calibrate();
    
    void update(Time const &, const long_t, const long_t) {}

    // ------------------------------------------------------------
    struct State_;
    
    struct Parameters_ {
      std::string port_name_; //!< the name of MUSIC port to connect to

      int max_buffered_;      //!< maximal number ticks to buffer data

      Parameters_();  //!< Sets default parameter values
      Parameters_(const Parameters_&);  //!< Recalibrate all times

      void get(DictionaryDatum&) const;
      
      /**
       * Set values from dictionary.
       */
      void set(const DictionaryDatum&, State_&);  
    };

    // ------------------------------------------------------------
    
    struct State_ {
      bool published_;  //!< indicates whether this node has been published already with MUSIC

      State_();  //!< Sets default state value

      void get(DictionaryDatum&) const;  //!< Store current values in dictionary
      void set(const DictionaryDatum&, const Parameters_&);  //!< Set values from dicitonary
    };
    
    // ------------------------------------------------------------

    struct Buffers_ {
    };
    
    // ------------------------------------------------------------ 

    struct Variables_ {
      MUSIC::MessageOutputPort *MP_; //!< The MUSIC cont port for input of data
    };
    
    // ------------------------------------------------------------

    Parameters_ P_;
    State_      S_;
    Buffers_    B_;
    Variables_  V_;
  };

inline
void music_message_out_proxy::get_status(DictionaryDatum &d) const
{
  P_.get(d);
  S_.get(d);
}

inline
void music_message_out_proxy::set_status(const DictionaryDatum &d)
{
  Parameters_ ptmp = P_;  // temporary copy in case of errors
  ptmp.set(d, S_);        // throws if BadProperty

  State_ stmp = S_;
  stmp.set(d, P_);        // throws if BadProperty

  bool has_messages = d->known ("messages");
  bool has_message_times = d->known ("message_times");
  if (has_messages || has_message_times)
    {
      MUSIC::Runtime* r = nest::Communicator::get_music_runtime();
      if (!r)
	throw MUSICOnlyRuntime (get_name (), "emit messages");

      if (!(has_messages && has_message_times))
	throw BadProperty ("must have both messages and message_times");
      
      ArrayDatum messages = getValue<ArrayDatum>((*d)["messages"]);
      ArrayDatum message_times = getValue<ArrayDatum>((*d)["message_times"]);

      unsigned int n = messages.size ();
      if (n != message_times.size ())
	throw BadProperty ("messages and message_times must be of the same length");

      for (unsigned int i = 0; i < n; ++i)
	{
	  StringDatum* sd = dynamic_cast<StringDatum*> (messages.get (i).datum ());
	  if (sd == 0)
	    {
	      std::string msg = String::compose("not a string in messages[%1]", i);
	      throw BadProperty (msg);
	    }
	  IntegerDatum* id = dynamic_cast<IntegerDatum*> (message_times.get (i).datum ());
	  DoubleDatum* dd = dynamic_cast<DoubleDatum*> (message_times.get (i).datum ());
	  double t;
	  if (id == 0)
	    {
	      if (dd == 0)
		{
		  std::string msg = String::compose("not a number in message_times[%1]", i);
		  throw BadProperty (msg);
		}
	      else
		t = *dd;
	    }
	  else
	    t = *id;
	  
	  V_.MP_->insertMessage (t,
				 const_cast<void*> (static_cast<const void*> (sd->c_str ())),
				 sd->size ());
	}
    }
  
  // if we get here, temporaries contain consistent set of properties
  P_ = ptmp;
  S_ = stmp;
}

} // namespace

#endif

#endif /* #ifndef MUSIC_MESSAGE_OUT_PROXY_H */
