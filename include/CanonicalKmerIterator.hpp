// This code is derived and modified from Pall Melsted's
// KmerIterator class (part of bfgraph) : https://github.com/pmelsted/bfgraph/blob/master/src/KmerIterator.cpp

#ifndef MER_ITERATOR_HPP
#define MER_ITERATOR_HPP

#include <iterator>
#include "string_view.hpp"
#include "CanonicalKmer.hpp"

namespace pufferfish {
  //class CanonicalKmerIterator : public std::iterator<std::input_iterator_tag, std::pair<CanonicalKmer, int>, int> {
  class CanonicalKmerIterator : public std::iterator<std::input_iterator_tag, CanonicalKmer, int> {
    stx::string_view s_;
    //std::pair<CanonicalKmer, int> p_;
    CanonicalKmer km_;
    int pos_;
    bool invalid_;
    int lastinvalid_;
    int k_;
  public:
		typedef CanonicalKmer value_type;
		typedef value_type& reference;
		typedef value_type* pointer;
		typedef std::input_iterator_tag iterator_category;
		typedef int64_t difference_type;
    CanonicalKmerIterator() : s_(), /*p_(),*/ km_(), pos_(), invalid_(true), lastinvalid_(-1), k_(CanonicalKmer::k()) {}
    CanonicalKmerIterator(const std::string& s) : s_(s), /*p_(),*/ km_(), pos_(), invalid_(false), lastinvalid_(-1), k_(CanonicalKmer::k()) { find_next(-1,-1);/*,false)*/;}
    CanonicalKmerIterator(const CanonicalKmerIterator& o) : s_(o.s_), /*p_(o.p_),*/ km_(o.km_), pos_(o.pos_), invalid_(o.invalid_), lastinvalid_(o.lastinvalid_), k_(o.k_) {}

    //void find_next(int i, int j);//, bool last_valid);
// use:  ++iter;
// pre:
// post: *iter is now exhausted
//       OR *iter is the next valid pair of kmer and location
inline CanonicalKmerIterator& operator++() {
  auto lpos = pos_ + k_;
  invalid_ = invalid_ || lpos >= s_.length();
  if (!invalid_) {
    int c = my_mer::code(s_[lpos]);
    if (c!=-1) { km_.shiftFw(c); } else { lastinvalid_ = pos_ + k_; }
    ++pos_;
  }
  return *this;
}


// use:  iter++;
// pre:
// post: iter has been incremented by one
inline CanonicalKmerIterator operator++(int) {
  CanonicalKmerIterator tmp(*this);
  operator++();
  return tmp;
}


// use:  val = (a == b);
// pre:
// post: (val == true) if a and b are both exhausted
//       OR a and b are in the same location of the same string.
//       (val == false) otherwise.
inline bool operator==(const CanonicalKmerIterator& o) {
  return (invalid_  || o.invalid_) ? invalid_ && o.invalid_ : ((km_ == o.km_) && (pos_ == o.pos_));
}

inline bool operator!=(const CanonicalKmerIterator& o) { return !this->operator==(o);}

inline bool kmerIsValid() {
  return (pos_+k_ - lastinvalid_ > k_);
}

// use:  p = *iter;
// pre:
// post: p is NULL or a pair of Kmer and int
inline reference operator*() {
  return km_;
}


// use:  example 1: km = iter->first;
//       example 2:  i = iter->second;
// pre:  *iter is not NULL
// post: km will be (*iter).first, i will be (*iter).second
inline pointer operator->() {
  return &(operator*());
}
  private:
// use:  find_next(i,j, last_valid);
// pre:
// post: *iter is either invalid or is a pair of:
//       1) the next valid kmer in the string that does not have any 'N'
//       2) the location of that kmer in the string
inline void find_next(int i, int j){//}, bool last_valid) {
  ++i;
  ++j;
  bool valid{false};
  // j is the last nucleotide in the k-mer we're building
  while (j < s_.length()) {
    // get the code for the last nucleotide, save it as c
    int c = my_mer::code(s_[j]);
    // c is a valid code if != -1
    if (c != -1) {
      km_.shiftFw(c);
      valid = (j - lastinvalid_ >= k_);
    } else {
      // if c is not a valid code, then j is the last invalid position
      lastinvalid_ = j;
      // the start position is the next (potentially) valid position
      i = j+1;
      // this k-mer is clearly not valid
      valid = false;
    }
    if (valid) {
      //p_.second = i;
      pos_ = i;
      return;
    }
    ++j;
  }
  invalid_ = true;
}



  private:
  };
}

#endif // MER_ITERATOR_HPP

