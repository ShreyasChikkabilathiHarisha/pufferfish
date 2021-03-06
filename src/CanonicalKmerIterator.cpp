// This code is derived and modified from Pall Melsted's
// KmerIterator class (part of bfgraph) :
// https://github.com/pmelsted/bfgraph/blob/master/src/KmerIterator.cpp

#include "CanonicalKmerIterator.hpp"
#include "CanonicalKmer.hpp"
#include <iterator>
#include <utility>

namespace pufferfish {
  namespace kmers = combinelib::kmers;

/* Note: That an iter is exhausted means that (iter._invalid == true) */

// use:  find_next(i,j, last_valid);
// pre:
// post: *iter is either invalid or is a pair of:
//       1) the next valid kmer in the string that does not have any 'N'
//       2) the location of that kmer in the string
void CanonicalKmerIterator::find_next(int j) { //}, bool last_valid) {
  ++i;
  ++j;
  int dist{(j - lastinvalid_)};
  // j is the last nucleotide in the k-mer we're building
  for (; j < s_.length(); ++j) {
    // while (j < s_.length()) {
    // get the code for the last nucleotide, save it as c
    int c = kmers::codeForChar(s_[j]);
    // c is a valid code if != -1
    if (c != -1) {
      km_.shiftFw(c);
      ++dist;
      valid = (dist >= k_);
    } else {
      // if c is not a valid code, then j is the last invalid position
      lastinvalid_ = j;
      // the start position is the next (potentially) valid position
      i = j + 1;
      // this k-mer is clearly not valid
      valid = false;
    }
    if (valid) {
      // p_.second = i;
      pos_ = i;
      return;
    }
    ++j;
  }
  invalid_ = true;
}

// use:  ++iter;
// pre:
// post: *iter is now exhausted
//       OR *iter is the next valid pair of kmer and location
inline CanonicalKmerIterator& CanonicalKmerIterator::operator++() {
  auto lpos = pos_ + k_;
  invalid_ = invalid_ || lpos >= s_.length();
  if (!invalid_) {
    find_next(pos_, lpos - 1);
    /*
    int c = kmers::codeForChar(s_[lpos]);
    if (c!=-1) { km_.shiftFw(c); } else { lastinvalid_ = pos_ + k_; }
    ++pos_;
    */
  }
  return *this;
  // int pos_ = p_.second;
  /*
  if (!invalid_) {
    if (pos_+k_ >= s_.length()) {
      invalid_ = true;
      return *this;
    } else {
      find_next(pos_,pos_+k_-1);//,true);
      return *this;
    }
  }
  return *this;
  */
}

// use:  iter++;
// pre:
// post: iter has been incremented by one
CanonicalKmerIterator CanonicalKmerIterator::operator++(int) {
  CanonicalKmerIterator tmp(*this);
  operator++();
  return tmp;
}

// use:  val = (a == b);
// pre:
// post: (val == true) if a and b are both exhausted
//       OR a and b are in the same location of the same string.
//       (val == false) otherwise.
bool CanonicalKmerIterator::operator==(const CanonicalKmerIterator& o) {
  return (invalid_ || o.invalid_) ? invalid_ && o.invalid_
                                  : ((km_ == o.km_) && (pos_ == o.pos_));
  /*
  if (invalid_  || o.invalid_) {
    return invalid_ && o.invalid_;
  } else {
    //return (s_ == o.s_) && (p_.second == o.p_.second);
    return (s_ == o.s_) && (pos_ == o.pos_);
  }
  */
}

bool CanonicalKmerIterator::kmerIsValid() {
  return (pos_ + k_ - lastinvalid_ > k_);
}

// use:  p = *iter;
// pre:
// post: p is NULL or a pair of Kmer and int
CanonicalKmerIterator::reference CanonicalKmerIterator::operator*() {
  return km_;
}

// use:  example 1: km = iter->first;
//       example 2:  i = iter->second;
// pre:  *iter is not NULL
// post: km will be (*iter).first, i will be (*iter).second
CanonicalKmerIterator::pointer CanonicalKmerIterator::operator->() {
  return &(operator*());
}

// use:  iter.raise(km, rep);
// post: iter has been incremented by one
//       if iter is not invalid, km is iter->first and rep is km.rep()
/*
void CanonicalKmerIterator::raise(Kmer& km, Kmer& rep) {
  operator++();
  if (!invalid_) {
    km = p_.first;
    rep = km.rep();
  }
}
*/

// use:  find_next(i,j, last_valid);
// pre:
// post: *iter is either invalid or is a pair of:
//       1) the next valid kmer in the string that does not have any 'N'
//       2) the location of that kmer in the string
/*
void CanonicalKmerIterator::find_next(size_t i, size_t j, bool last_valid) {

++i;
++j;

while (s_[j] != 0) {
  //auto c =
  //char c = s_[j] & 0xDF; // mask lowercase bit
  int c = kmers::codeForChar(s_[j]);
  if (c != -1) {//} == 'A' || c == 'C' || c == 'G' || c == 'T') {
    if (last_valid) {
      p_.first.shiftFw(c);
      break; // default case,
    } else {
      if (i + k_ - 1 == j) {
        p_.first.fromStr(s_+i);
        last_valid = true;
        break; // create k-mer from scratch
      } else {
        ++j;
      }
    }
  } else {
    ++j;
    i = j;
    last_valid = false;
  }
}
if (i+k_-1 == j && s_[j] != 0) {
  p_.second = i;
} else {
  invalid_ = true;
}
}
*/
}
