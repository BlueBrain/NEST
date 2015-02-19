/*
 *  cont_delay_connection.h
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

#ifndef CONT_DELAY_CONNECTION_H
#define CONT_DELAY_CONNECTION_H

/* BeginDocumentation
  Name: cont_delay_synapse - Synapse type for continuous delays

  Description:
  cont_delay_synapse relaxes the condition that NEST only implements delays
  which are an integer multiple of the time step h. A continuous delay is 
  decomposed into an integer part (delay_) and a double (delay_offset_) so
  that the actual delay is given by  delay_*h - delay_offset_. This can be
  combined with off-grid spike times.

  Transmits: SpikeEvent, RateEvent, CurrentEvent, ConductanceEvent, DoubleDataEvent

  References: none
  FirstVersion: June 2007
  Author: Abigail Morrison
  SeeAlso: synapsedict, static_synapse, iaf_psc_alpha_canon
*/


#include "connection.h"
#include <cmath>

namespace nest
{

  template<typename targetidentifierT>
  class ContDelayConnection : public Connection<targetidentifierT> {

  public:

    typedef CommonSynapseProperties CommonPropertiesType;
    typedef Connection<targetidentifierT> ConnectionBase;

  /**
   * Default Constructor.
   * Sets default values for all parameters. Needed by GenericConnectorModel.
   */
  ContDelayConnection();
  
  /**
   * Copy constructor.
   * Needs to be defined properly in order for GenericConnector to work.
   */
  ContDelayConnection(const ContDelayConnection &);

  /**
   * Default Destructor.
   */
  ~ContDelayConnection() {}

  // Explicitly declare all methods inherited from the dependent base ConnectionBase.
  // This avoids explicit name prefixes in all places these functions are used.
  // Since ConnectionBase depends on the template parameter, they are not automatically
  // found in the base class.
  using ConnectionBase::get_delay_steps;
  using ConnectionBase::set_delay_steps;
  using ConnectionBase::get_rport;
  using ConnectionBase::get_target;

  //! Used by ConnectorModel::add_connection() for fast initialization
  void set_weight(double_t w) { weight_ = w; }

  /**
   * Get all properties of this connection and put them into a dictionary.
   */
  void get_status(DictionaryDatum & d) const;
  
  /**
   * Set properties of this connection from the values given in dictionary.
   */
  void set_status(const DictionaryDatum & d, ConnectorModel &cm);

  /**
   * Send an event to the receiver of this connection.
   * \param e The event to send
   * \param t_lastspike Point in time of last spike sent.
   * \param cp common properties of all synapses (empty).
   */
  void send(Event& e, thread t, double_t t_lastspike, const CommonSynapseProperties &cp);
  
  class ConnTestDummyNode: public ConnTestDummyNodeBase
  {
  public:
	// Ensure proper overriding of overloaded virtual functions.
	// Return values from functions are ignored.
	using ConnTestDummyNodeBase::handles_test_event;
    port handles_test_event(SpikeEvent&, rport) { return invalid_port_; }
    port handles_test_event(RateEvent&, rport) { return invalid_port_; }
    port handles_test_event(CurrentEvent&, rport) { return invalid_port_; }
    port handles_test_event(ConductanceEvent&, rport) { return invalid_port_; }
    port handles_test_event(DoubleDataEvent&, rport) { return invalid_port_; }
  };

  void check_connection(Node & s, Node & t, rport receptor_type, double_t, const CommonPropertiesType &)
  {
    ConnTestDummyNode dummy_target;
    ConnectionBase::check_connection_(dummy_target, s, t, receptor_type);
  }

 private:
  double_t weight_;         //!< synaptic weight
  double_t delay_offset_;   //!< fractional delay < h, total delay = delay_ - delay_offset_
  };

/**
 * Send an event to the receiver of this connection.
 * \param e The event to send
 * \param p The port under which this connection is stored in the Connector.
 * \param t_lastspike Time point of last spike emitted
 */
template<typename targetidentifierT>
inline
void ContDelayConnection<targetidentifierT>::send(Event& e, thread t, double_t, const CommonSynapseProperties &)
{
  e.set_receiver(*get_target(t));
  e.set_weight(weight_);
  e.set_rport(get_rport());
  double orig_event_offset = e.get_offset();
  double total_offset = orig_event_offset + delay_offset_;
  // As far as i have seen, offsets are outside of tics regime provided
  // by the Time-class to allow more precise spike-times, hence comparing
  // on the tics level here is not reasonable. Still, the double comparison
  // seems save.
  if (total_offset < Time::get_resolution().get_ms())
  {
    e.set_delay(get_delay_steps());
    e.set_offset(total_offset);
  }
  else
  {
    e.set_delay(get_delay_steps() - 1);
    e.set_offset(total_offset - Time::get_resolution().get_ms());
  }
  e();
  //reset offset to original value
  e.set_offset(orig_event_offset);
}

} // of namespace nest

#endif // of #ifndef CONT_DELAY_CONNECTION_H
