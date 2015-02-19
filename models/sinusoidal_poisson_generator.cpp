/*
 *  sinusoidal_poisson_generator.cpp
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

#include "exceptions.h"
#include "sinusoidal_poisson_generator.h"
#include "network.h"
#include "dict.h"
#include "integerdatum.h"
#include "doubledatum.h"
#include "arraydatum.h"
#include "dictutils.h"
#include "numerics.h"
#include "universal_data_logger_impl.h"

#include <cmath>
#include <limits>

namespace nest {
  RecordablesMap<sinusoidal_poisson_generator> sinusoidal_poisson_generator::recordablesMap_;

  template <>
  void RecordablesMap<sinusoidal_poisson_generator>::create()
  {
    insert_(Name(names::rate), &sinusoidal_poisson_generator::get_rate_);
  }
}

/* ---------------------------------------------------------------- 
 * Default constructors defining default parameter
 * ---------------------------------------------------------------- */
    
nest::sinusoidal_poisson_generator::Parameters_::Parameters_()
  : om_(0.0),    // radian/s
    phi_(0.0),   // radian
    dc_(0.0),    // spikes/s
    ac_(0.0),    // spikes/s
    individual_spike_trains_(true)
{}

nest::sinusoidal_poisson_generator::Parameters_::Parameters_(const Parameters_& p )
  : om_(p.om_),
    phi_(p.phi_),
    dc_(p.dc_),
    ac_(p.ac_),
    individual_spike_trains_(p.individual_spike_trains_)
{}

nest::sinusoidal_poisson_generator::Parameters_& 
nest::sinusoidal_poisson_generator::Parameters_::operator=(const Parameters_& p)
{
  if ( this == &p )
    return *this;

  dc_ = p.dc_;
  om_ = p.om_;
  phi_ = p.phi_;
  ac_ = p.ac_;
  individual_spike_trains_ = p.individual_spike_trains_;

  return *this;
}

nest::sinusoidal_poisson_generator::State_::State_()
  : rate_(0)
{}


nest::sinusoidal_poisson_generator::Buffers_::Buffers_(sinusoidal_poisson_generator& n)
  : logger_(n)
{}

nest::sinusoidal_poisson_generator::Buffers_::Buffers_(const Buffers_&, sinusoidal_poisson_generator& n)
  : logger_(n)
{}


/* ---------------------------------------------------------------- 
 * Parameter extraction and manipulation functions
 * ---------------------------------------------------------------- */

void nest::sinusoidal_poisson_generator::Parameters_::get(DictionaryDatum &d) const
{
  (*d)[names::dc]  = dc_ * 1000.0; 
  (*d)[names::freq]= om_ / ( 2.0 * numerics::pi / 1000.0);
  (*d)[names::phi] = phi_;
  (*d)[names::ac]  = ac_ * 1000.0;
  (*d)[names::individual_spike_trains] = individual_spike_trains_;
} 

void nest::sinusoidal_poisson_generator::State_::get(DictionaryDatum &) const
{
}  

void nest::sinusoidal_poisson_generator::Parameters_::set(const DictionaryDatum& d,
							  const sinusoidal_poisson_generator& n)
{
  if ( not n.is_model_prototype() && d->known(names::individual_spike_trains) )
    throw BadProperty("The individual_spike_trains property can only be set as"
		      " a model default using SetDefaults or upon CopyModel.");
  
  updateValue<bool>(d, names::individual_spike_trains, individual_spike_trains_);

  if ( updateValue<double_t>(d, names::dc, dc_) )
    dc_ /= 1000.0;           // scale to ms^-1
  
  if ( updateValue<double_t>(d, names::freq, om_) )
    om_ *= 2.0 * numerics::pi / 1000.0;

  updateValue<double_t>(d, names::phi, phi_);

  if ( updateValue<double_t>(d, names::ac, ac_) )
      ac_ /= 1000.0;
}

/* ---------------------------------------------------------------- 
 * Default and copy constructor for node
 * ---------------------------------------------------------------- */

nest::sinusoidal_poisson_generator::sinusoidal_poisson_generator()
  : Node(),
    device_(),
    P_(),
    S_(),
    B_(*this)
{
    recordablesMap_.create();
}

nest::sinusoidal_poisson_generator::sinusoidal_poisson_generator(const sinusoidal_poisson_generator&n)
  : Node(n),
    device_(n.device_),
    P_(n.P_),
    S_(n.S_),
    B_(n.B_, *this)
{
}

/* ---------------------------------------------------------------- 
 * Node initialization functions
 * ---------------------------------------------------------------- */

void nest::sinusoidal_poisson_generator::init_state_(const Node& proto)
{ 
  const sinusoidal_poisson_generator& pr = downcast<sinusoidal_poisson_generator>(proto);
  
  device_.init_state(pr.device_);
  S_ = pr.S_;
}

void nest::sinusoidal_poisson_generator::init_buffers_()
{ 
  device_.init_buffers();
  B_.logger_.reset();
}

void nest::sinusoidal_poisson_generator::calibrate()
{
  B_.logger_.init();  // ensures initialization in case mm connected after Simulate

  device_.calibrate();

  // time resolution
  V_.h_ = Time::get_resolution().get_ms(); 
  const double_t t = network()->get_time().get_ms();

  // initial state
  S_.y_0_ = P_.ac_ * std::cos(P_.om_ * t + P_.phi_);
  S_.y_1_ = P_.ac_ * std::sin(P_.om_ * t + P_.phi_);

  V_.sin_ = std::sin(V_.h_ * P_.om_);         // block elements
  V_.cos_ = std::cos(V_.h_ * P_.om_);

  return;
}

void nest::sinusoidal_poisson_generator::update(Time const& origin, 
			 const long_t from, const long_t to)
{
  assert(to >= 0 && (delay) from < Scheduler::get_min_delay());
  assert(from < to);

  const long_t start = origin.get_steps();

  // random number generator
  librandom::RngPtr rng = net_->get_rng(get_thread());

  // We iterate the dynamics even when the device is turned off,
  // but do not issue spikes while it is off. In this way, the 
  // oscillators always have the right phase.  This is quite 
  // time-consuming, so it should be done only if the device is
  // on most of the time.

  for ( long_t lag = from ; lag < to ; ++lag )
  {
    // update oscillator blocks, accumulate rate as sum of DC and N_osc_ AC elements
    // rate is instantaneous sum of state
    S_.rate_ = P_.dc_;

    const double_t new_y_0 = V_.cos_ * S_.y_0_ - V_.sin_ * S_.y_1_;
    
    S_.y_1_ = V_.sin_ * S_.y_0_ + V_.cos_ * S_.y_1_;
    S_.y_0_ = new_y_0;
    S_.rate_ += S_.y_1_;
    
    if ( S_.rate_ < 0 )
      S_.rate_ = 0;
  
    // store rate in Hz
    B_.logger_.record_data(origin.get_steps()+lag);

    // create spikes
    if ( S_.rate_ > 0 && device_.is_active(Time::step(start+lag)) )
    {
      if ( P_.individual_spike_trains_ )
      {
	DSSpikeEvent se;
	network()->send(*this, se, lag);
      }
      else
      {
	V_.poisson_dev_.set_lambda(S_.rate_ * V_.h_);
	long_t n_spikes = V_.poisson_dev_.ldev(rng);
      	SpikeEvent se;
	se.set_multiplicity(n_spikes);
        network()->send(*this, se, lag);
      }
    }
  }
}                       

void nest::sinusoidal_poisson_generator::event_hook(DSSpikeEvent& e)
{
  librandom::RngPtr rng = net_->get_rng(get_thread());
  V_.poisson_dev_.set_lambda(S_.rate_ * V_.h_);
  long_t n_spikes = V_.poisson_dev_.ldev(rng);

  if ( n_spikes > 0 ) // we must not send events with multiplicity 0
  {
    e.set_multiplicity(n_spikes);
    e.get_receiver().handle(e);
  }
}

void nest::sinusoidal_poisson_generator::handle(DataLoggingRequest& e)
{
  B_.logger_.handle(e);
}




