#ifndef FIXNUMUTIL_H_
#define FIXNUMUTIL_H_

#include <limits.h>

// Internal utility methods and classes for FixNum library

namespace FixNumUtil {
  // ----------- Define limits for byte, int, and long types -----------
  
  template<typename T> class Limits {};
  
  template<> class Limits<byte> {
  public:
    static const byte minValue = 0;
    static const byte maxValue = 255;
    static const byte bufSize = 6;
  };
  
  template<> class Limits<int> {
  public:
    static const int minValue = -INT_MAX;
    static const int maxValue = INT_MAX;
    static const byte bufSize = 8;
  };
  
  template<> class Limits<long> {
  public:
    static const long minValue = -LONG_MAX;
    static const long maxValue = LONG_MAX;
    static const byte bufSize = 13;
  };
  
  // ----------- Define largest type between a pair -----------

  template<typename T1, typename T2> class Common {};
  
  template<> class Common<byte, byte> {
  public:
    typedef byte type;
  };
  
  template<> class Common<byte, int> {
  public:
    typedef int type;
  };
  
  template<> class Common<int, byte> {
  public:
    typedef int type;
  };
  
  template<> class Common<int, int> {
  public:
    typedef int type;
  };
  
  template<> class Common<byte, long> {
  public:
    typedef long type;
  };
  
  template<> class Common<long, byte> {
  public:
    typedef long type;
  };
  
  template<> class Common<int, long> {
  public:
    typedef long type;
  };
  
  template<> class Common<long, int> {
  public:
    typedef long type;
  };
  
  template<> class Common<long, long> {
  public:
    typedef long type;
  };
  
  // ----------- Narrow one type into the other -----------
  
  template<typename T1, typename T2> inline T2 narrow(T1 x) {
    T2 x2 = (T2)x;
    if (x2 != x) // does not fit
      return x < 0 ? Limits<T2>::minValue : Limits<T2>::maxValue;
    return x2; // narrow Ok  
  }

  // ----------- Change decimal precision -----------
  
  template<typename T> T scale(T x, byte prec1, byte prec2) {
    if (prec2 >= prec1) {
      for (byte i = prec1; i < prec2; i++) {
        if (x > Limits<T>::maxValue / 10)
          return Limits<T>::maxValue;
        if (x < Limits<T>::minValue / 10)
          return Limits<T>::minValue;
        x *= 10; 
      }
      return x;
    } else {
      if (x >= Limits<T>::maxValue || x <= Limits<T>::minValue)
        return x;
      // prec2 < prec1
      for (byte i = prec2; i < prec1; i++) {
        T mod = x % 10;
        x = x / 10;
        if (mod >= 5)
          x++;
        else if (mod <= -5)
          x--;  
      }
      return x;  
    }  
  }

  // ----------- Change decimal precision and type -----------
  
  template<typename T1, typename T2> T2 convert(T1 x, byte prec1, byte prec2) {
    typedef typename Common<T1,T2>::type T0;
    T0 x0 = scale((T0)x, prec1, prec2);
    return narrow<T0,T2>(x0);
  }
}

#endif
