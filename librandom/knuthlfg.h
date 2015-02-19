/*
 *  knuthlfg.h
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

#ifndef KNUTHLFG_H
#define KNUTHLFG_H

/*
 *  Built-in implementation of Knuth's LFG generator.
 *  This code is a C++ adaptation of the code published by Knuth on his
 *  website, http://www-cs-faculty.stanford.edu/~knuth/programs/rng.c,
 *  retrieved 8 Jan 2008. See also Knuth's header comment below.
 */
 
/* Header comment by  D E Knuth ------------------------------------------ */

/*    This program by D E Knuth is in the public domain and freely copyable.
 *    It is explained in Seminumerical Algorithms, 3rd edition, Section 3.6
 *    (or in the errata to the 2nd edition --- see
 *        http://www-cs-faculty.stanford.edu/~knuth/taocp.html
 *    in the changes to Volume 2 on pages 171 and following).              */

/*    N.B. The MODIFICATIONS introduced in the 9th printing (2002) are
      included here; there's no backwards compatibility with the original. */

/*    This version also adopts Brendan McKay's suggestion to
      accommodate naive users who forget to call ran_start(seed).          */

/*    If you find any bugs, please report them immediately to
 *                 taocp@cs.stanford.edu
 *    (and you will be rewarded if the bug is genuine). Thanks!            */

/************ see the book for explanations and caveats! *******************/
/************ in particular, you need two's complement arithmetic **********/

/* End of Header comment by  D E Knuth ----------------------------------- */

#include "randomgen.h"
#include <vector>

namespace librandom {
  
  /**
   * Built-in implementation of Knuth's Lagged Fibonacci generator.
   * This implementation is directly derived from Knuth's C code and
   * generates the same random number sequence as the GSL implementation.
   */
  class KnuthLFG : public RandomGen {
  public:
    
    //! Create generator with given seed
    explicit KnuthLFG(unsigned long);
    
    ~KnuthLFG() {};

    RngPtr clone(unsigned long s)
      {
	return RngPtr(new KnuthLFG(s));
      }

  private:
    //! implements seeding for RandomGen
    void   seed_(unsigned long);
    
    //! implements drawing a single [0,1) number for RandomGen
    double drand_();
    
  private:  
    static const long    KK_;        //!< the long lag
    static const long    LL_;        //!< the short lag
    static const long    MM_;        //!< the modulus
    static const long    TT_;        //!< guaranteed separation between streams
    static const long    QUALITY_;   //!< number of RNGs to fill for each cycle
    static const double  I2DFactor_; //!< int to double factor 

    static long mod_diff_(long, long);  //!< subtraction module MM 
    static bool is_odd_(long);
    
    std::vector<long> ran_x_;       //!< the generator state
    std::vector<long> ran_buffer_;  //!< generated numbers, 0..KK-1 are shipped
    const std::vector<long>::const_iterator end_;  //!< marker past last to deliver
    std::vector<long>::const_iterator next_; //!< next number to deliver
    
    /** 
     * Generates numbers, refilling buffer.
     * @note Buffer must be passed as argument, since ran_start_() and
     *       self_test_() must pass other buffers than ran_buffer_.
     */
    void ran_array_(std::vector<long>& rbuff);
    void ran_start_(long seed); //!< initializes buffer
    long ran_draw_();           //!< deliver integer random number from ran_buffer_
    
    /**
     * Perform minimal self-test given by Knuth.
     * The test will break an assertion if it fails. This is acceptable,
     * since failure indicates either lack of two's complement arithmetic
     * or problems with the size of data types.
     */
    void self_test_(); 
  };  

  inline
  void KnuthLFG::seed_(unsigned long seed)
  {  
    ran_start_(seed);
  }
  

  inline
  double KnuthLFG::drand_()
  {  
    return I2DFactor_ * ran_draw_();
  }
  
  
  inline
  long KnuthLFG::mod_diff_(long x, long y)
  {
    // modulo computation assumes two's complement
    return ( x - y ) & ( MM_ - 1 );
  }
  
  inline
  bool KnuthLFG::is_odd_(long x)
  {
    return x & 1;
  }
  
  inline long KnuthLFG::ran_draw_()
  {
    if ( next_ == end_ )
    {
      ran_array_(ran_buffer_);  // refill
      next_ = ran_buffer_.begin();
    }
    
    return *next_++;   // return next and increment
  }

}  // namespace librandom

#endif
