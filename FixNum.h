#ifndef FIXNUM_H_
#define FIXNUM_H_

#include "fmt_util.h"
#include "FixNumUtil.h"

// Public API for FixNum library

template<typename T, byte prec> class FixNum {
private:
  T _mantissa;
public:
  static FixNum<T, prec> invalid();

  enum Fmt {
    NONE        = 0,
    SIGN        = FMT_SIGN,            // print sign at the first position 
    SPACE       = FMT_SPACE,           // fill with spaces (with zeroes by default)
    LEFT_SPACE  = FMT_LEFT | FMT_SPACE // left-align the result and fill with spaces
  };

  FixNum();
  FixNum(T mantissa);

  T mantissa();
  operator T ();
  boolean valid();
  byte format(char* pos, byte size, Fmt fmt = NONE);   
  size_t printTo(Print& p);
  template<typename T2, byte prec2> operator FixNum<T2, prec2>();
  // Comparisons
  template<typename T2, byte prec2> boolean operator ==(FixNum<T2, prec2> other);
  template<typename T2, byte prec2> boolean operator !=(FixNum<T2, prec2> other);
  template<typename T2, byte prec2> boolean operator < (FixNum<T2, prec2> other);
  template<typename T2, byte prec2> boolean operator <=(FixNum<T2, prec2> other);
  template<typename T2, byte prec2> boolean operator > (FixNum<T2, prec2> other);
  template<typename T2, byte prec2> boolean operator >=(FixNum<T2, prec2> other);
};

// overload print method for FixNums
template<typename T, byte prec> inline void print(FixNum<T, prec> val) {
  val.printTo(Serial);
}

template<typename T> class FixNumParser {
  private:
    enum State {
      START,
      INT_PART,
      FRAC_PART
    };
    
    boolean _neg;
    boolean _ok;
    T _mantissa;
    byte _prec;
    State _state;
  public:
    enum Result {
      BAD,
      NUM,
      DONE
    };
    static const byte MAX_PREC = 6;

    void reset();    
    Result parse(char ch);
    template<typename T2, byte prec2> operator FixNum<T2, prec2>();
};

// ----------- class FixNum implementation -----------

template<typename T, byte prec> inline FixNum<T, prec> FixNum<T, prec>::invalid() {
  return FixNum<T, prec>(FixNumUtil::Limits<T>::maxValue);
}

template<typename T, byte prec> inline FixNum<T, prec>::FixNum() : _mantissa(FixNumUtil::Limits<T>::maxValue) {}

template<typename T, byte prec> inline FixNum<T, prec>::FixNum(T mantissa) : _mantissa(mantissa) {}

template<typename T, byte prec> inline T FixNum<T, prec>::mantissa() { 
  return _mantissa;
}

template<typename T, byte prec> inline FixNum<T, prec>::operator T() { 
  return _mantissa;
}

template<typename T, byte prec> boolean FixNum<T, prec>::valid() { 
  return _mantissa < FixNumUtil::Limits<T>::maxValue && _mantissa > FixNumUtil::Limits<T>::minValue;
}

template<typename T, byte prec> inline byte FixNum<T, prec>::format(char* pos, byte size, Fmt fmt) {
  return formatDecimal(_mantissa, pos, size, fmt | prec);
}

template<typename T, byte prec> size_t FixNum<T, prec>::printTo(Print& p) {
  char buf[FixNumUtil::Limits<T>::bufSize];
  byte size = format(buf, sizeof(buf) - 1, LEFT_SPACE);
  buf[size] = 0;
  return p.print(buf);
}

template<typename T, byte prec> template<typename T2, byte prec2> inline FixNum<T, prec>::operator FixNum<T2, prec2>() {
  return FixNum<T2, prec2>(FixNumUtil::convert<T, T2>(_mantissa, prec, prec2));
}

template<typename T, byte prec> template<typename T2, byte prec2> boolean FixNum<T, prec>::operator ==(FixNum<T2, prec2> other) {
  if (!valid() || !other.valid())
    return false;
  typedef typename FixNumUtil::Common<T, T2>::type T0;
  byte p0 = max(prec, prec2);
  T0 x1 = FixNumUtil::scale((T0)_mantissa, prec, p0);
  T0 x2 = FixNumUtil::scale((T0)other.mantissa(), prec2, p0);
  return x1 == x2;  
}

template<typename T, byte prec> template<typename T2, byte prec2> boolean FixNum<T, prec>::operator !=(FixNum<T2, prec2> other) {
  if (!valid() || !other.valid())
    return false;
  typedef typename FixNumUtil::Common<T, T2>::type T0;
  byte p0 = max(prec, prec2);
  T0 x1 = FixNumUtil::scale((T0)_mantissa, prec, p0);
  T0 x2 = FixNumUtil::scale((T0)other.mantissa(), prec2, p0);
  return x1 != x2;  
}
  
template<typename T, byte prec> template<typename T2, byte prec2> boolean FixNum<T, prec>::operator < (FixNum<T2, prec2> other) {
  if (!valid() || !other.valid())
    return false;
  typedef typename FixNumUtil::Common<T, T2>::type T0;
  byte p0 = max(prec, prec2);
  T0 x1 = FixNumUtil::scale((T0)_mantissa, prec, p0);
  T0 x2 = FixNumUtil::scale((T0)other.mantissa(), prec2, p0);
  return x1 < x2;  
}
  
template<typename T, byte prec> template<typename T2, byte prec2> boolean FixNum<T, prec>::operator <=(FixNum<T2, prec2> other) {
  if (!valid() || !other.valid())
    return false;
  typedef typename FixNumUtil::Common<T, T2>::type T0;
  byte p0 = max(prec, prec2);
  T0 x1 = FixNumUtil::scale((T0)_mantissa, prec, p0);
  T0 x2 = FixNumUtil::scale((T0)other.mantissa(), prec2, p0);
  return x1 <= x2;  
}
  
template<typename T, byte prec> template<typename T2, byte prec2> boolean FixNum<T, prec>::operator > (FixNum<T2, prec2> other) {
  if (!valid() || !other.valid())
    return false;
  typedef typename FixNumUtil::Common<T, T2>::type T0;
  byte p0 = max(prec, prec2);
  T0 x1 = FixNumUtil::scale((T0)_mantissa, prec, p0);
  T0 x2 = FixNumUtil::scale((T0)other.mantissa(), prec2, p0);
  return x1 > x2;  
}
  
template<typename T, byte prec> template<typename T2, byte prec2> boolean FixNum<T, prec>::operator >=(FixNum<T2, prec2> other) {
  if (!valid() || !other.valid())
    return false;
  typedef typename FixNumUtil::Common<T, T2>::type T0;
  byte p0 = max(prec, prec2);
  T0 x1 = FixNumUtil::scale((T0)_mantissa, prec, p0);
  T0 x2 = FixNumUtil::scale((T0)other.mantissa(), prec2, p0);
  return x1 >= x2;  
}

// ----------- class FixNumParser implementation -----------

template<typename T> inline void FixNumParser<T>::reset() {
  _state = START;
}

template<typename T> typename FixNumParser<T>::Result FixNumParser<T>::parse(char ch) {
  switch (_state) {
    case START:
      if (ch == '+' || ch == '-' || ch == '.' || (ch >= '0' && ch <= '9')) {
        _neg = ch == '-';
        _ok = (ch >= '0' && ch <= '9');
        _mantissa = (ch >= '0' && ch <= '9') ? ch - '0' : 0;
        _prec = 0;
        _state = (ch == '.') ? FRAC_PART : INT_PART;
        return NUM;
      } else
        return BAD;
    case INT_PART:
      if (ch == '.' ) {
          _state = FRAC_PART;
          return NUM;
      }
      // falls through!!!
    case FRAC_PART: 
      if (ch >= '0' && ch <= '9') {
        if (_state == FRAC_PART) {
          if (_prec >= MAX_PREC)
            break; // skip the rest
          _prec++; 
        }
        _ok = true;
        byte digit = ch - '0';
        if (_mantissa > (FixNumUtil::Limits<T>::maxValue - 9) / 10)
          _mantissa = FixNumUtil::Limits<T>::maxValue;
        else
          _mantissa = _mantissa * 10 + digit;
        return NUM;
      }
      // fall through to the end of method
  }
  // number is over
  _state = START;
  return _ok ? DONE : BAD;
}

template<typename T> template<typename T2, byte prec2> inline FixNumParser<T>::operator FixNum<T2, prec2>() {
  T x = _neg ? -_mantissa : _mantissa;
  return FixNum<T2, prec2>(FixNumUtil::convert<T, T2>(x, _prec, prec2));  
}

#endif
