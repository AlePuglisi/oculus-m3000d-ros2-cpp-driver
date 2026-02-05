/******************************************************************************
 * (c) Copyright 2017 Blueprint Subsea.
 * This file is part of Oculus Viewer
 *
 * Oculus Viewer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Oculus Viewer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#pragma once

#include <QtMath>
#include <QString>

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)


// ----------------------------------------------------------------------------
// Some useful static utility functions

class RmUtil
{
public:

  // KW
  // Added a routine which formats a uint32 IP address as a string
  static QString FormatIpAddress(uint32_t ipAddr)
  {
	  uchar ip1 = (uchar)(ipAddr & 0xff);
	  uchar ip2 = (uchar)((ipAddr & 0xff00) >> 8);
	  uchar ip3 = (uchar)((ipAddr & 0xff0000) >> 16);
	  uchar ip4 = (uchar)((ipAddr & 0xff000000) >> 24);

      return QString::number(ip1) + "." + QString::number(ip2) + "." + QString::number(ip3) + "." + QString::number(ip4);
  }
  // END KW

  static quint16 SwapWord(quint16 w)
  {
    return ((w & 0xff) << 8) + ((w & 0xff00) >> 8);
  }

  static quint32 SwapDWord(quint32 w)
  {
    quint32 out;
    quint8* bytes = (quint8*)&out;
    quint8* dword = (quint8*)&w;

    bytes[0] = dword[3];
    bytes[1] = dword[2];
    bytes[2] = dword[1];
    bytes[3] = dword[0];

    return out;
  }

  static float SwapFloat(float w)
  {
    float out;
    quint8* bytes = (quint8*)&out;
    quint8* dword = (quint8*)&w;

    bytes[0] = dword[3];
    bytes[1] = dword[2];
    bytes[2] = dword[1];
    bytes[3] = dword[0];

    return out;
  }

  // ----------------------------------------------------------------------------
  // Return the interpolate value
  static double Interpolate (double x0, double x1, double x, double y0, double y1)
  {
    double d = x1 - x0;

    if (d)
      return y0 + ( (x - x0) / d ) * (y1 - y0);
    else
      return y0;
  }

  // ----------------------------------------------------------------------------
  // Return the next 2power that is equivalent or larger than the given value
  static unsigned Pow2Up(unsigned v)
  {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
  }

  static double NearestMkr(double marker);
  static double NextMkr(double marker);
  static double PrevMkr(double marker);

  // Lat long calculations
  static double LL2LLRng(double lat1, double long1, double lat2, double long2);
  static double LL2LLBrg(double lat1, double long1, double lat2, double long2);
  static double CrossTrack(double lat1, double long1, double lat2, double long2, double atLat, double atLong);
  static double AlongTrack(double lat1, double long1, double lat2, double long2, double atLat, double atLong);

  static double BrgOffsetDegrees(double centre, double reference);

  // Data
  static double   markers[6];

  static double R2D (double r) { return r * r2d; }
  static double D2R (double d) { return d * d2r; }
  static double Rnd (double x) { return floor (x + 0.5); }

  static double ClipUp (double x, double y) { return ceil(x / y) * y; }
  static double ClipDn (double x, double y) { return floor(x / y) * y; }
  static double ClipRnd (double x, double y) { return Rnd(x / y) * y; }
  static double ClipRng (double x, double a, double b) { return (Max ((a), Min ((b), (x)))); }

  static double Sqr (double x) { return x * x; }
  static double Cube (double x) { return x * x * x; }
  static double Hypot3 (double x, double y, double z) { return sqrt(Sqr(x) + Sqr(y) + Sqr(z)); }

  static bool   Even (double x) { return ( ((double)x / 2.0) - floor((double)x / 2.0) == 0.0 ? true  : false ); }
  static bool   Odd (double x)  { return ( ((double)x / 2.0) - floor((double)x / 2.0) == 0.0 ? false : true );  }
  static bool   ExactDivisible (double x, double y) { return ( ((double)x / (double)y) - floor((double)x / (double)y) == 0.0 ? true : false ); }

  static bool   LessThanT (double x, double y) { return (((y - x) > threshold)  ? 1 : 0); }
  static bool   MoreThanT (double x, double y) { return (((x - y) > threshold)  ? 1 : 0); }
  static bool   EqualT (double x, double y)    { return ((fabs(y - x) < threshold)  ? 1 : 0); }
  static bool   EqualT (double x, double y, double t)    { return ((fabs(y - x) < t)  ? 1 : 0); }
  static double FModT (double x, double y)     { double z = fmod(x, y); return (EqualT(z, y) ? 0.0 : z); }
  static double HypotT(double x, double y)     { return (EqualT(x, 0.0) && EqualT(y, 0.0) ? 0 : sqrt(x * x + y * y)); }

  static double HeadAngleD (double x) { return ( fmod(720.0 - (x) + 90.0, 360.0) ); }
  static double HeadAngleR (double x) { return ( fmod((4 * pi) - (x) + pi_2, pi2) ); }
  static double AngleHeadD (double x) { return HeadAngleD(x); }
  static double AngleHeadR (double x) { return HeadAngleR(x); }

  static double Max (double x, double y) { if (x > y) return x; else return y; }
  static double Min (double x, double y) { if (x > y) return y; else return x; }
  static double Mid (double x, double y) { return (x + y) / 2.0; }
  static double ACosh(double x) { return log(x + sqrt((x * x - 1))); }
  static double ATan2(double y, double x) { return (x == 0.0) ? ((y > 0.0) ? RmUtil::pi_2 : -RmUtil::pi_2) : atan2(y, x); }
  static double Hypot(double x, double y) { return sqrt(x * x + y * y); }

  static const double threshold;
  static const double r2d;
  static const double d2r;
  static const double pi;
  static const double pi_2;
  static const double pi3_2;
  static const double pi_4;
  static const double pi2;
  static const double root2;
  static const double root2_2;
  static const double e;
};



class RmVector;

// ------------------------------------------------------------------------------------------------
// Matrix class
class RmMatrix
{
public:
  RmMatrix();

  // Operators
  RmMatrix& operator *= (const RmMatrix& other);
  bool operator==(const RmMatrix& other) const;



  // Methods
  void SetToIdentity();
  void Ortho(float left, float right, float top, float bottom, float near, float far);
  void Scale(const RmVector& v);
  void Translate(const RmVector& vector);
  void Rotate(float angle, const RmVector& vector);
  void Rotate(float angle, float x, float y, float z);

  float Det3(int col0, int col1, int col2, int row0, int row1, int row2);
  float Det4();
  RmMatrix OrthoInverse();
  RmMatrix Inverse(bool& ok);

  // Matrix state types
  enum
  {
    eIdentity        = 0x0001,   // Identity matrix
    eGeneral         = 0x0002,   // General matrix, unknown contents
    eTranslation     = 0x0004,   // Contains a simple translation
    eScale           = 0x0008,   // Contains a simple scale
    eRotation        = 0x0010    // Contains a simple rotation
  };

  // Data
  int flagBits;           // Flag bits from the enum below.
  float m[4][4];
};


// ------------------------------------------------------------------------------------------------
// Vecrtor class
class RmVector
{
public:
  RmVector();
  RmVector(float x, float y, float z = 0.0f);

  // Data
  float x;
  float y;
  float z;

  float* pV;
};


// ------------------------------------------------------------------------------------------------
inline RmMatrix operator * (const RmMatrix& m1, const RmMatrix& m2)
{
  if (m1.flagBits == RmMatrix::eIdentity)
    return m2;
  else if (m2.flagBits == RmMatrix::eIdentity)
    return m1;

  RmMatrix m;

  m.m[0][0] = m1.m[0][0] * m2.m[0][0] +
              m1.m[1][0] * m2.m[0][1] +
              m1.m[2][0] * m2.m[0][2] +
              m1.m[3][0] * m2.m[0][3];
  m.m[0][1] = m1.m[0][1] * m2.m[0][0] +
              m1.m[1][1] * m2.m[0][1] +
              m1.m[2][1] * m2.m[0][2] +
              m1.m[3][1] * m2.m[0][3];
  m.m[0][2] = m1.m[0][2] * m2.m[0][0] +
              m1.m[1][2] * m2.m[0][1] +
              m1.m[2][2] * m2.m[0][2] +
              m1.m[3][2] * m2.m[0][3];
  m.m[0][3] = m1.m[0][3] * m2.m[0][0] +
              m1.m[1][3] * m2.m[0][1] +
              m1.m[2][3] * m2.m[0][2] +
              m1.m[3][3] * m2.m[0][3];
  m.m[1][0] = m1.m[0][0] * m2.m[1][0] +
              m1.m[1][0] * m2.m[1][1] +
              m1.m[2][0] * m2.m[1][2] +
              m1.m[3][0] * m2.m[1][3];
  m.m[1][1] = m1.m[0][1] * m2.m[1][0] +
              m1.m[1][1] * m2.m[1][1] +
              m1.m[2][1] * m2.m[1][2] +
              m1.m[3][1] * m2.m[1][3];
  m.m[1][2] = m1.m[0][2] * m2.m[1][0] +
              m1.m[1][2] * m2.m[1][1] +
              m1.m[2][2] * m2.m[1][2] +
              m1.m[3][2] * m2.m[1][3];
  m.m[1][3] = m1.m[0][3] * m2.m[1][0] +
              m1.m[1][3] * m2.m[1][1] +
              m1.m[2][3] * m2.m[1][2] +
              m1.m[3][3] * m2.m[1][3];
  m.m[2][0] = m1.m[0][0] * m2.m[2][0] +
              m1.m[1][0] * m2.m[2][1] +
              m1.m[2][0] * m2.m[2][2] +
              m1.m[3][0] * m2.m[2][3];
  m.m[2][1] = m1.m[0][1] * m2.m[2][0] +
              m1.m[1][1] * m2.m[2][1] +
              m1.m[2][1] * m2.m[2][2] +
              m1.m[3][1] * m2.m[2][3];
  m.m[2][2] = m1.m[0][2] * m2.m[2][0] +
              m1.m[1][2] * m2.m[2][1] +
              m1.m[2][2] * m2.m[2][2] +
              m1.m[3][2] * m2.m[2][3];
  m.m[2][3] = m1.m[0][3] * m2.m[2][0] +
              m1.m[1][3] * m2.m[2][1] +
			  m1.m[2][3] * m2.m[2][2] +
              m1.m[3][3] * m2.m[2][3];
  m.m[3][0] = m1.m[0][0] * m2.m[3][0] +
              m1.m[1][0] * m2.m[3][1] +
              m1.m[2][0] * m2.m[3][2] +
              m1.m[3][0] * m2.m[3][3];
  m.m[3][1] = m1.m[0][1] * m2.m[3][0] +
              m1.m[1][1] * m2.m[3][1] +
              m1.m[2][1] * m2.m[3][2] +
              m1.m[3][1] * m2.m[3][3];
  m.m[3][2] = m1.m[0][2] * m2.m[3][0] +
              m1.m[1][2] * m2.m[3][1] +
              m1.m[2][2] * m2.m[3][2] +
              m1.m[3][2] * m2.m[3][3];
  m.m[3][3] = m1.m[0][3] * m2.m[3][0] +
              m1.m[1][3] * m2.m[3][1] +
              m1.m[2][3] * m2.m[3][2] +
              m1.m[3][3] * m2.m[3][3];
  return m;
}

// ------------------------------------------------------------------------------------------------
inline RmVector operator*(const RmVector& vector, const RmMatrix& matrix)
{
  float x, y, z, w;
  x = vector.x * matrix.m[0][0] +
      vector.y * matrix.m[0][1] +
      vector.z * matrix.m[0][2] +
      matrix.m[0][3];

  y = vector.x * matrix.m[1][0] +
      vector.y * matrix.m[1][1] +
      vector.z * matrix.m[1][2] +
      matrix.m[1][3];
  z = vector.x * matrix.m[2][0] +
      vector.y * matrix.m[2][1] +
      vector.z * matrix.m[2][2] +
      matrix.m[2][3];
  w = vector.x * matrix.m[3][0] +
      vector.y * matrix.m[3][1] +
      vector.z * matrix.m[3][2] +
      matrix.m[3][3];

  if (w == 1.0f)
    return RmVector(x, y, z);
  else
    return RmVector(x / w, y / w, z / w);
}

// ------------------------------------------------------------------------------------------------
inline RmVector operator*(const RmMatrix& matrix, const RmVector& vector)
{
  float x, y, z, w;

  if (matrix.flagBits == RmMatrix::eIdentity)
  {
    return vector;
  }
  else
  if (matrix.flagBits == RmMatrix::eTranslation)
  {
    return RmVector(vector.x + matrix.m[3][0],
                    vector.y + matrix.m[3][1],
                    vector.z + matrix.m[3][2]);
  }
  else
  if (matrix.flagBits == (RmMatrix::eTranslation | RmMatrix::eScale))
  {
    return RmVector(vector.x * matrix.m[0][0] + matrix.m[3][0],
                    vector.y * matrix.m[1][1] + matrix.m[3][1],
                    vector.z * matrix.m[2][2] + matrix.m[3][2]);
  }
  else
  if (matrix.flagBits == RmMatrix::eScale)
  {
    return RmVector(vector.x * matrix.m[0][0],
                    vector.y * matrix.m[1][1],
                    vector.z * matrix.m[2][2]);
  }
  else
  {
    x = vector.x * matrix.m[0][0] +
        vector.y * matrix.m[1][0] +
        vector.z * matrix.m[2][0] +
        matrix.m[3][0];
    y = vector.x * matrix.m[0][1] +
        vector.y * matrix.m[1][1] +
        vector.z * matrix.m[2][1] +
        matrix.m[3][1];
    z = vector.x * matrix.m[0][2] +
        vector.y * matrix.m[1][2] +
        vector.z * matrix.m[2][2] +
        matrix.m[3][2];
    w = vector.x * matrix.m[0][3] +
        vector.y * matrix.m[1][3] +
        vector.z * matrix.m[2][3] +
        matrix.m[3][3];

    if (w == 1.0f)
      return RmVector(x, y, z);
    else
      return RmVector(x / w, y / w, z / w);
  }
}


// ----------------------------------------------------------------------------
// Base class for RmTool based classes
class RmObject
{

};


// ----------------------------------------------------------------------------
// COperation - An operation, basically a function that can be called

// COperation generic callback
typedef void (RmObject::*TObjExe) ();

// COperation index callback
typedef void (RmObject::*TObjInd) (int index);

// COperation value callback
typedef void (RmObject::*TObjDbl) (double val);

// COperation string callback
//typedef void (RmObject::*TObjStr) (const std::string& str);

// COperation pointer callback
typedef void (RmObject::*TObjPtr) (void* pHint);

class RmListPos;

// ----------------------------------------------------------------------------
// The position type
typedef RmListPos* RMPOS;

// ----------------------------------------------------------------------------
// The entry type
class RmListPos
{
public:
  RmListPos()
  {
    m_pLast  = nullptr;
    m_pNext  = nullptr;
    m_pEntry = nullptr;
  }

  ~RmListPos()
  {
  }

  RMPOS m_pLast;
  RMPOS m_pNext;
  void* m_pEntry;
};

// ----------------------------------------------------------------------------
// The pointer list type - this contains references to data held elsewhere
template <class C> class RmLstPtr
{
public:

  RmLstPtr()
  {
    m_pHead = nullptr;
    m_pTail = nullptr;
  }

  virtual ~RmLstPtr()
  {
    RemoveAll();
  }

  RmLstPtr<C>& operator = (const RmLstPtr<C>& src);

  RMPOS m_pHead;
  RMPOS m_pTail;

  C GetHead() const;
  C GetTail() const;

  C RemoveHead();
  C RemoveTail();

  RMPOS AddHead(C pEntry);
  RMPOS AddTail(C pEntry);

  void RemoveAll();

  RMPOS GetHeadPos() const {return m_pHead;}
  RMPOS GetTailPos() const {return m_pTail;}

  C    GetNext(RMPOS& rPosition) const;
  C    GetPrev(RMPOS& rPosition) const;
  C    GetAt(RMPOS rPosition) const;
  void SetAt(RMPOS& rPosition, C pEntry);
  void RemoveAt(RMPOS rPosition);
  void RemoveBefore(RMPOS rPosition);
  void RemoveAfter(RMPOS rPosition);
  void Remove(C value);

  RMPOS InsertBefore(RMPOS rPosition, C newElement);
  RMPOS InsertAfter(RMPOS rPosition, C newElement);
  RMPOS MoveUp(RMPOS rPosition);
  RMPOS MoveDown(RMPOS rPosition);

  RMPOS Find(C searchValue, RMPOS startAfter = nullptr) const;

  RMPOS FindIndex(int nIndex) const;
  int GetIndex(C searchValue) const;
  int GetIndex(RMPOS find) const;

  int GetCount() const;

  bool IsEmpty() const { return (m_pHead == nullptr); }
};

// ----------------------------------------------------------------------------
// copy the references from one list to the next
template <class C> RmLstPtr<C>& RmLstPtr<C>::operator = (const RmLstPtr<C>& src)
{
  RemoveAll();

  RMPOS in = src.GetHeadPos();

  while (in)
    AddTail(src.GetNext(in));

  return *this;
}

// ----------------------------------------------------------------------------
// Get the head pointer
template <class C> C RmLstPtr<C>::GetHead() const
{
  if (m_pHead)
    return (C)m_pHead->m_pEntry;
  else
    return nullptr;
}

// ----------------------------------------------------------------------------
// Get the tail pointer
template <class C> C RmLstPtr<C>::GetTail() const
{
  if (m_pTail)
    return (C)m_pTail->m_pEntry;
  else
    return nullptr;
}

// ----------------------------------------------------------------------------
// Remove the head pointer
template <class C> C RmLstPtr<C>::RemoveHead()
{
  if (m_pHead)
  {
    C pEntry = (C)m_pHead->m_pEntry;
    RMPOS pNext = m_pHead->m_pNext;

    delete m_pHead;

    if (pNext)
    {
      m_pHead = pNext;
      m_pHead->m_pLast = nullptr;
    }
    else
    {
      m_pHead = nullptr;
      m_pTail = nullptr;
    }

    return pEntry;
  }
  else
    return nullptr;
}

// ----------------------------------------------------------------------------
// Remove the tail pointer
template <class C> C RmLstPtr<C>::RemoveTail()
{
  if (m_pTail)
  {
    C pEntry = (C)m_pTail->m_pEntry;
    RMPOS pLast = m_pTail->m_pLast;

    delete m_pTail;

    if (pLast)
    {
      m_pTail = pLast;
      m_pTail->m_pNext = nullptr;
    }
    else
    {
      m_pHead = nullptr;
      m_pTail = nullptr;
    }

    return pEntry;
  }
  else
    return nullptr;
}

// ----------------------------------------------------------------------------
// Add element to the head of the list
template <class C> RMPOS RmLstPtr<C>::AddHead(C pEntry)
{
  RMPOS head = new RmListPos;
  RMPOS oldHead = m_pHead;

  m_pHead = head;
  m_pHead->m_pEntry = (void*)pEntry;

  if (oldHead)
  {
    m_pHead->m_pNext = oldHead;
    oldHead->m_pLast = m_pHead;
  }
  else
    m_pTail = m_pHead;

  return m_pHead;
}

// ----------------------------------------------------------------------------
// Add element to the tail of the list
template <class C> RMPOS RmLstPtr<C>::AddTail(C pEntry)
{
  RMPOS tail = new RmListPos;
  RMPOS oldTail = m_pTail;

  m_pTail = tail;
  m_pTail->m_pEntry = (void*)pEntry;

  if (oldTail)
  {
    m_pTail->m_pLast = oldTail;
    oldTail->m_pNext = m_pTail;
  }
  else
    m_pHead = m_pTail;

  return m_pTail;
}

// ----------------------------------------------------------------------------
// Remove all the elements in the list
template <class C> void RmLstPtr<C>::RemoveAll()
{
  RMPOS delme;
  RMPOS pos;

  for (pos = m_pHead; pos;)
  {
    delme = pos;
    pos   = delme->m_pNext;
    delete delme;
  }

  m_pHead = nullptr;
  m_pTail = nullptr;
}

// ----------------------------------------------------------------------------
// Get entry and adjust pointer fwd
template <class C> C RmLstPtr<C>::GetNext(RMPOS& rPosition) const
{
  if (!rPosition)
    return nullptr;

  C pEntry = (C)rPosition->m_pEntry;

  rPosition = rPosition->m_pNext;

  return pEntry;
}

// ----------------------------------------------------------------------------
// Get entry and adjust pointer back
template <class C> C RmLstPtr<C>::GetPrev(RMPOS& rPosition) const
{
  if (!rPosition)
    return nullptr;

  C pEntry = (C)rPosition->m_pEntry;

  rPosition = rPosition->m_pLast;

  return pEntry;
}

// ----------------------------------------------------------------------------
// Get entry
template <class C> C RmLstPtr<C>::GetAt(RMPOS rPosition) const
{
  if (!rPosition)
    return nullptr;

  return (C)rPosition->m_pEntry;
}

// ----------------------------------------------------------------------------
// Set entry
template <class C> void RmLstPtr<C>::SetAt(RMPOS& rPosition, C pEntry)
{
  if (!rPosition)
    return;

  rPosition->m_pEntry = (void*)pEntry;
}

// ----------------------------------------------------------------------------
// Remove entry given pointer
template <class C> void RmLstPtr<C>::Remove(C value)
{
  RMPOS pos = m_pHead;

  while (pos)
  {
    if (pos->m_pEntry == value)
    {
      RemoveAt(pos);
      return;
    }

    pos = pos->m_pNext;
  }
}

// ----------------------------------------------------------------------------
// remove entry
template <class C> void RmLstPtr<C>::RemoveAt(RMPOS rPosition)
{
  if (!rPosition)
    return;

  if (rPosition == m_pHead && rPosition == m_pTail)
  {
    m_pHead = nullptr;
    m_pTail = nullptr;
  }
  else
    if (rPosition == m_pHead)
    {
      m_pHead = m_pHead->m_pNext;
      m_pHead->m_pLast = nullptr;
    }
    else
      if (rPosition == m_pTail)
      {
        m_pTail = m_pTail->m_pLast;
        m_pTail->m_pNext = nullptr;
      }
      else
      {
        rPosition->m_pLast->m_pNext = rPosition->m_pNext;
        rPosition->m_pNext->m_pLast = rPosition->m_pLast;
      }

  delete rPosition;
}

// ----------------------------------------------------------------------------
// remove entry before the specified position
template <class C> void RmLstPtr<C>::RemoveBefore(RMPOS rPosition)
{
  if (!rPosition || !rPosition->m_pLast)
    RemoveTail();
  else
    RemoveAt(rPosition->m_pLast);
}

// ----------------------------------------------------------------------------
// remove entry before the specified position
template <class C> void RmLstPtr<C>::RemoveAfter(RMPOS rPosition)
{
  if (!rPosition || !rPosition->m_pNext)
    RemoveHead();
  else
    RemoveAt(rPosition->m_pNext);
}

// ----------------------------------------------------------------------------
// Insert before a specified entry
template <class C> RMPOS RmLstPtr<C>::InsertBefore( RMPOS position, C newElement )
{
  if (position == nullptr || position == m_pHead)
    return AddHead(newElement);
  else
  {
    RMPOS pos = new RmListPos;

    pos->m_pNext  = position;
    pos->m_pLast  = position->m_pLast;
    pos->m_pEntry = (void*) newElement;

    position->m_pLast->m_pNext = pos;
    position->m_pLast = pos;

    return pos;
  }
}

// ----------------------------------------------------------------------------
// Insert after a specified entry
template <class C> RMPOS RmLstPtr<C>::InsertAfter( RMPOS position, C newElement )
{
  if (position == nullptr || position == m_pTail)
    return AddTail(newElement);
  else
  {
    RMPOS pos = new RmListPos;

    pos->m_pNext  = position->m_pNext;
    pos->m_pLast  = position;
    pos->m_pEntry = (void*) newElement;

    position->m_pNext->m_pLast = pos;
    position->m_pNext = pos;

    return pos;
  }
}

// ----------------------------------------------------------------------------
// Moves an entry up the linked list
template <class C> RMPOS RmLstPtr<C>::MoveUp( RMPOS position )
{
  if (position == nullptr || position == m_pTail)
    return (position);
  else
  {
    RMPOS pos;
    pos = position->m_pNext;
    void* nextEntry = pos->m_pEntry;
    pos->m_pEntry = position->m_pEntry;
    position->m_pEntry = nextEntry;

    return pos;
  }
}

// ----------------------------------------------------------------------------
// Moves an entry up the linked list
template <class C> RMPOS RmLstPtr<C>::MoveDown( RMPOS position )
{
  if (position == nullptr || position == m_pHead)
    return (position);
  else
  {
    RMPOS pos;
    pos = position->m_pLast;
    void* prevEntry = pos->m_pEntry;
    pos->m_pEntry = position->m_pEntry;
    position->m_pEntry = prevEntry;

    return pos;
  }
}

// ----------------------------------------------------------------------------
// Find an entry in the list
template <class C> RMPOS RmLstPtr<C>::Find(C searchValue, RMPOS startAfter) const
{
  if (!startAfter)
    startAfter = m_pHead;

  RMPOS pos = startAfter;

  while (pos)
  {
    if (pos->m_pEntry == (void*)searchValue)
      return pos;

    pos = pos->m_pNext;
  }

  return pos;
}

// ----------------------------------------------------------------------------
// Find an entry in the list
template <class C> int RmLstPtr<C>::GetIndex(C searchValue) const
{
  int nIndex = 0;

  RMPOS pos = m_pHead;

  while (pos)
  {
    if (pos->m_pEntry == (void*)searchValue)
      return nIndex;

    pos = pos->m_pNext;
    nIndex++;
  }

  return -1;
}

// ----------------------------------------------------------------------------
// Find an entry in the list
template <class C> int RmLstPtr<C>::GetIndex(RMPOS find) const
{
  int nIndex = 0;

  for (RMPOS pos = m_pHead; pos;)
  {
    if (pos == find)
      return nIndex;

    pos = pos->m_pNext;
    nIndex++;
  }

  return -1;
}

// ----------------------------------------------------------------------------
// find the position of an index
template <class C> RMPOS RmLstPtr<C>::FindIndex(int nIndex) const
{
  if (nIndex < 0)
    return nullptr;

  int n = 0;

  RMPOS pos = m_pHead;

  while (pos)
  {
    if (n == nIndex)
      return pos;

    pos = pos->m_pNext;
    n++;
  }

  return pos;
}

// ----------------------------------------------------------------------------
// Count the number of elements in the list
template <class C> int RmLstPtr<C>::GetCount() const
{
  int n = 0;

  RMPOS pos = m_pHead;

  while (pos)
  {
    pos = pos->m_pNext;
    n++;
  }

  return n;
}



//=============================================================================
// Store pointers and generate classes for the pointers in an array
// This array manages the creation and deletion of classes

template <class C> class RmPtrAry
{
public:

  RmPtrAry();
  RmPtrAry(const RmPtrAry<C>& dsa);

  virtual ~RmPtrAry();

public:
  void     SetSize(unsigned nEl);
  void     Append(C* pEl);
  void     Grow(unsigned nEl = 1);

  void     RemoveAt (unsigned i);

  int      GetIndex(C* pEl) const;

  C* operator [] (unsigned nEl) const { return m_pEl[nEl]; }

  C*       GetAt (unsigned nEl) const { return m_pEl[nEl]; }
  C*       GetHead() const { return m_pEl[0]; }
  C*       GetTail() const { return m_pEl[m_nEl - 1]; }

  unsigned GetSize()  const { return m_nEl; }
  bool     ValidIndex(int index)  const { return (0 <= index && index < (int)m_nEl); }

  RmPtrAry<C>& operator= (const RmPtrAry<C>& dsa);

  void Swap(unsigned i, unsigned j );
  void Sort(int (*comparator)(const void*, const void*));

protected:
  unsigned m_nEl;
  C**      m_pEl;
};

// ----------------------------------------------------------------------------
template <class C> RmPtrAry<C>::RmPtrAry()
{
  m_nEl = 0;
  m_pEl = nullptr;
}

// ----------------------------------------------------------------------------
template <class C> RmPtrAry<C>::RmPtrAry(const RmPtrAry<C>& dsa)
{
  m_nEl = 0;
  m_pEl = nullptr;

  SetSize(dsa.GetSize());

  for (unsigned u = 0; u < m_nEl; u++)
    (*m_pEl[u]) = (*(dsa[u]));
}

// ----------------------------------------------------------------------------
template <class C> RmPtrAry<C>::~RmPtrAry()
{
  SetSize(0);
}

// ----------------------------------------------------------------------------
template <class C> RmPtrAry<C>& RmPtrAry<C>::operator= (const RmPtrAry<C>& dsa)
{
  SetSize(dsa.GetSize());

  for (unsigned u = 0; u < m_nEl; u++)
    (*m_pEl[u]) = (*(dsa[u]));

  return *this;
}

// ----------------------------------------------------------------------------
template <class C> void RmPtrAry<C>::SetSize(unsigned nEl)
{
  if (nEl == m_nEl)
    return;
  else
    if (nEl == 0)
    {
      for (unsigned u = 0; u < m_nEl; u++)
        delete m_pEl[u];

      delete m_pEl;
      m_pEl = nullptr;
    }
    else
      if (nEl < m_nEl)
      {
        for (unsigned u = nEl; u < m_nEl; u++)
          delete m_pEl[u];

        m_pEl = (C**) realloc (m_pEl, sizeof (C*) * nEl);
      }
      else
        if (nEl > m_nEl)
        {
          m_pEl = (C**) realloc (m_pEl, sizeof (C*) * nEl);

          for (unsigned u = m_nEl; u < nEl; u++)
            m_pEl[u] = new C;
        }

  m_nEl = nEl;
}

// ----------------------------------------------------------------------------
template <class C> void RmPtrAry<C>::Grow(unsigned nEl)
{
  SetSize(m_nEl + nEl);
}

// ----------------------------------------------------------------------------
template <class C> void RmPtrAry<C>::Append(C* pEl)
{
  m_pEl = (C**) realloc (m_pEl, sizeof (C*) * (m_nEl + 1));
  m_pEl[m_nEl] = pEl;
  m_nEl++;
}

// ----------------------------------------------------------------------------
template <class C> void RmPtrAry<C>::RemoveAt (unsigned i)
{
  // Bounds Check
  if (i > m_nEl - 1)
    return;

  // Remove the specified element
  delete m_pEl[i];

  // Shift remaining elements down by 1
  for (unsigned u = i + 1; u < GetSize(); u++)
  {
    m_pEl[u-1] = m_pEl[u];
    m_pEl[u] = nullptr;
  }

  // Resize the array
  m_nEl--;
  m_pEl = (C**) realloc (m_pEl, m_nEl * sizeof(C*));
}

// ----------------------------------------------------------------------------
template <class C> int RmPtrAry<C>::GetIndex(C* pEl) const
{
  for (unsigned u = 0; u < m_nEl; u++)
  {
    if (m_pEl[u] == pEl)
      return (int)u;
  }

  return -1;
}

// ----------------------------------------------------------------------------
template <class C> void RmPtrAry<C>::Swap(unsigned i, unsigned j )
{
  C* temp;

  temp = m_pEl[i];
  m_pEl[i] = m_pEl[j];
  m_pEl[j] = temp;
}

// ----------------------------------------------------------------------------
// compatator function will be called as:
//  comparator( (C**)A, (C**)B )
// should return -ve if A < B, 0 if A==B, and +1 if A > B
template <class C> void RmPtrAry<C>::Sort(int (*comparator)(const void*, const void*))
{
  qsort(m_pEl, m_nEl, sizeof(C**), comparator);
}

// ----------------------------------------------------------------------------
// This is a managed list class - when items are removed from the list they are deleted
// when the list is destroyed the items sre deleted and dynamic creation can be used using SetSize and Grow
template <class C> class RmLstMgd
{
public:

  RmLstMgd()
  {
    m_pHead = nullptr;
    m_pTail = nullptr;
  }

  virtual ~RmLstMgd()
  {
    RemoveAll();
  }

  virtual RmLstMgd<C>& operator = (const RmLstMgd<C>& src);
  virtual RmLstMgd<C>& operator = (const RmLstPtr<C*>& src);

  void FillPtrList(RmLstPtr<C*>& out);

  RMPOS m_pHead;
  RMPOS m_pTail;

  RMPOS AddHead(const C* pEntry);
  RMPOS AddTail(const C* pEntry);

  RMPOS GetHeadPos() const {return m_pHead;}
  RMPOS GetTailPos() const {return m_pTail;}

  C*   GetHead() const;
  C*   GetTail() const;
  C*   GetNext(RMPOS& rPosition) const;
  C*   GetPrev(RMPOS& rPosition) const;
  C*   GetAt(RMPOS rPosition) const;

  void SetAt(RMPOS rPosition, const C* pEntry);

  void RemoveAll();
  void RemoveHead();
  void RemoveTail();
  void RemoveAt(RMPOS rPosition);
  void RemoveBefore(RMPOS rPosition);
  void RemoveAfter(RMPOS rPosition);
  void Remove(C* pVal);

  RMPOS InsertBefore(RMPOS rPosition, const C* newElement);
  RMPOS InsertAfter(RMPOS rPosition, const C* newElement);

  RMPOS Find(const C* searchValue, RMPOS startAfter = nullptr) const;

  RMPOS FindIndex(int nIndex) const;
  int GetIndex(const C* searchValue) const;
  int GetIndex(RMPOS find) const;

  int GetCount() const;

  bool IsEmpty() const { return (m_pHead == nullptr); }

  // These functions only work for certain runtime classes
  void SetSize(unsigned size);
  void Grow(int grow = 1);

  void ShiftUp(RMPOS pos);
  void ShiftDn(RMPOS pos);
  void ShiftToHead(RMPOS pos);
  void ShiftToTail(RMPOS pos);
  void Swap(RMPOS pos1, RMPOS pos2);
};


// ----------------------------------------------------------------------------
// copy from one list to the next
template <class C> RmLstMgd<C>& RmLstMgd<C>::operator = (const RmLstMgd<C>& src)
{
  RMPOS in   = src.GetHeadPos();
  RMPOS out  = GetHeadPos();

  while (in)
  {
    C* pIn  = src.GetNext(in);
    C* pOut = nullptr;

    if (out)
      pOut = GetNext(out);
    else
    {
      pOut = new C;
      AddTail(pOut);
    }

    *pOut = *pIn;
  }

  // now remove any additional data
  RMPOS last;
  while ( (last = out) != nullptr )
  {
    GetNext(out);
    RemoveAt(last);
  }

  return *this;
}

// ----------------------------------------------------------------------------
// Copy the contents of the references into stores within the managed list
// Note a little optimisation is done on the internal list - mem is resused if possible
template <class C> RmLstMgd<C>& RmLstMgd<C>::operator = (const RmLstPtr<C*>& src)
{
  RMPOS in   = src.GetHeadPos();
  RMPOS out  = GetHeadPos();

  while (in)
  {
    C* pIn  = src.GetNext(in);
    C* pOut = nullptr;

    if (out)
      pOut = GetNext(out);
    else
    {
      pOut = new C;
      AddTail(pOut);
    }

    *pOut = *pIn;
  }

  // now remove any additional data
  RMPOS last;
  while ( (last = out) != nullptr )
  {
    GetNext(out);
    RemoveAt(last);
  }

  return *this;
}

// ----------------------------------------------------------------------------
// copy the references from one list to the next
template <class C> void RmLstMgd<C>::FillPtrList(RmLstPtr<C*>& out)
{
  out.RemoveAll();

  RMPOS in = GetHeadPos();

  while (in)
    out.AddTail(GetNext(in));
}

// ----------------------------------------------------------------------------
// Get the head pointer
template <class C> C* RmLstMgd<C>::GetHead() const
{
  if (m_pHead)
    return (C*)m_pHead->m_pEntry;
  else
    return nullptr;
}

// ----------------------------------------------------------------------------
// Get the tail pointer
template <class C> C* RmLstMgd<C>::GetTail() const
{
  if (m_pTail)
    return (C*)m_pTail->m_pEntry;
  else
    return nullptr;
}

// ----------------------------------------------------------------------------
// Remove the head pointer
template <class C> void RmLstMgd<C>::RemoveHead()
{
  if (m_pHead)
  {
    RmListPos* pNext = m_pHead->m_pNext;

    delete (C*)m_pHead->m_pEntry;
    delete m_pHead;

    if (pNext)
    {
      m_pHead = pNext;
      m_pHead->m_pLast = nullptr;
    }
    else
    {
      m_pHead = nullptr;
      m_pTail = nullptr;
    }
  }
}

// ----------------------------------------------------------------------------
// Remove the tail pointer
template <class C> void RmLstMgd<C>::RemoveTail()
{
  if (m_pTail)
  {
    RMPOS pLast = m_pTail->m_pLast;

    delete (C*)m_pTail->m_pEntry;
    delete m_pTail;

    if (pLast)
    {
      m_pTail = pLast;
      m_pTail->m_pNext = nullptr;
    }
    else
    {
      m_pHead = nullptr;
      m_pTail = nullptr;
    }
  }
}

// ----------------------------------------------------------------------------
// Add element to the head of the list
template <class C> RMPOS RmLstMgd<C>::AddHead(const C* pEntry)
{
  RMPOS head = new RmListPos;
  RMPOS oldHead = m_pHead;

  m_pHead = head;
  m_pHead->m_pEntry = (void*)pEntry;

  if (oldHead)
  {
    m_pHead->m_pNext = oldHead;
    oldHead->m_pLast = m_pHead;
  }
  else
    m_pTail = m_pHead;

  return m_pHead;
}

// ----------------------------------------------------------------------------
// Add element to the tail of the list
template <class C> RMPOS RmLstMgd<C>::AddTail(const C* pEntry)
{
  RMPOS tail = new RmListPos;
  RMPOS oldTail = m_pTail;

  m_pTail = tail;
  m_pTail->m_pEntry = (void*)pEntry;

  if (oldTail)
  {
    m_pTail->m_pLast = oldTail;
    oldTail->m_pNext = m_pTail;
  }
  else
    m_pHead = m_pTail;

  return m_pTail;
}

// ----------------------------------------------------------------------------
// Remove all the elements in the list
template <class C> void RmLstMgd<C>::RemoveAll()
{
  RMPOS delme;
  RMPOS pos;

  for (pos = m_pHead; pos;)
  {
    delme = pos;
    pos   = delme->m_pNext;

    delete (C*)delme->m_pEntry;
    delete delme;
  }

  m_pHead = nullptr;
  m_pTail = nullptr;
}

// ----------------------------------------------------------------------------
// Get entry and adjust pointer fwd
template <class C> C* RmLstMgd<C>::GetNext(RMPOS& rPosition) const
{
  if (!rPosition)
    return nullptr;

  C* pEntry = (C*)rPosition->m_pEntry;

  rPosition = rPosition->m_pNext;

  return pEntry;
}

// ----------------------------------------------------------------------------
// Get entry and adjust pointer back
template <class C> C* RmLstMgd<C>::GetPrev(RMPOS& rPosition) const
{
  if (!rPosition)
    return nullptr;

  C* pEntry = (C*)rPosition->m_pEntry;

  rPosition = rPosition->m_pLast;

  return pEntry;
}

// ----------------------------------------------------------------------------
// Get entry
template <class C> C* RmLstMgd<C>::GetAt(RMPOS rPosition) const
{
  if (!rPosition)
    return nullptr;

  return (C*)rPosition->m_pEntry;
}

// ----------------------------------------------------------------------------
// Set entry
template <class C> void RmLstMgd<C>::SetAt(RMPOS rPosition, const C* pEntry)
{
  if (!rPosition)
    return;

  rPosition->m_pEntry = (void*)pEntry;
}


// ----------------------------------------------------------------------------
// Remove entry given pointer
template <class C> void RmLstMgd<C>::Remove(C* pVal)
{
  RMPOS pos = m_pHead;

  while (pos)
  {
    if (pos->m_pEntry == pVal)
    {
      RemoveAt(pos);
      return;
    }

    pos = pos->m_pNext;
  }
}

// ----------------------------------------------------------------------------
// remove entry
template <class C> void RmLstMgd<C>::RemoveAt(RMPOS rPosition)
{
  if (!rPosition)
    return;

  if (rPosition == m_pHead && rPosition == m_pTail)
  {
    m_pHead = nullptr;
    m_pTail = nullptr;
  }
  else
    if (rPosition == m_pHead)
    {
      m_pHead = m_pHead->m_pNext;
      m_pHead->m_pLast = nullptr;
    }
    else
      if (rPosition == m_pTail)
      {
        m_pTail = m_pTail->m_pLast;
        m_pTail->m_pNext = nullptr;
      }
      else
      {
        rPosition->m_pLast->m_pNext = rPosition->m_pNext;
        rPosition->m_pNext->m_pLast = rPosition->m_pLast;
      }

  delete (C*)rPosition->m_pEntry;
  delete rPosition;
}

// ----------------------------------------------------------------------------
// remove entry before the specified position
template <class C> void RmLstMgd<C>::RemoveBefore(RMPOS rPosition)
{
  if (!rPosition || !rPosition->m_pLast)
    RemoveTail();
  else
    RemoveAt(rPosition->m_pLast);
}

// ----------------------------------------------------------------------------
// remove entry before the specified position
template <class C> void RmLstMgd<C>::RemoveAfter(RMPOS rPosition)
{
  if (!rPosition || !rPosition->m_pNext)
    RemoveHead();
  else
    RemoveAt(rPosition->m_pNext);
}

// ----------------------------------------------------------------------------
// Insert before a specified entry
template <class C> RMPOS RmLstMgd<C>::InsertBefore( RMPOS position, const C* newElement )
{
  if (position == nullptr || position == m_pHead)
    return AddHead(newElement);
  else
  {
    RMPOS pos = new RmListPos;

    pos->m_pNext  = position;
    pos->m_pLast  = position->m_pLast;
    pos->m_pEntry = (void*) newElement;

    position->m_pLast->m_pNext = pos;
    position->m_pLast = pos;

    return pos;
  }
}

// ----------------------------------------------------------------------------
// Insert after a specified entry
template <class C> RMPOS RmLstMgd<C>::InsertAfter( RMPOS position, const C* newElement )
{
  if (position == nullptr || position == m_pTail)
    return AddTail(newElement);
  else
  {
    RMPOS pos = new RmListPos;

    pos->m_pNext  = position->m_pNext;
    pos->m_pLast  = position;
    pos->m_pEntry = (void*) newElement;

    position->m_pNext->m_pLast = pos;
    position->m_pNext = pos;

    return pos;
  }
}

// ----------------------------------------------------------------------------
// Find an entry in the list
template <class C> RMPOS RmLstMgd<C>::Find(const C* searchValue, RMPOS startAfter) const
{
  if (!startAfter)
    startAfter = m_pHead;

  RMPOS pos = startAfter;

  while (pos)
  {
    if (pos->m_pEntry == (void*)searchValue)
      return pos;

    pos = pos->m_pNext;
  }

  return pos;
}

// ----------------------------------------------------------------------------
// Find an entry in the list
template <class C> int RmLstMgd<C>::GetIndex(const C* searchValue) const
{
  int nIndex = 0;

  for (RMPOS pos = m_pHead; pos;)
  {
    if (pos->m_pEntry == (void*)searchValue)
      return nIndex;

    pos = pos->m_pNext;
    nIndex++;
  }

  return -1;
}

// ----------------------------------------------------------------------------
// Find an entry in the list
template <class C> int RmLstMgd<C>::GetIndex(RMPOS find) const
{
  int nIndex = 0;

  for (RMPOS pos = m_pHead; pos;)
  {
    if (pos == find)
      return nIndex;

    pos = pos->m_pNext;
    nIndex++;
  }

  return -1;
}

// ----------------------------------------------------------------------------
// find the position of an index
template <class C> RMPOS RmLstMgd<C>::FindIndex(int nIndex) const
{
  if (nIndex < 0)
    return nullptr;

  int n = 0;

  RMPOS pos = m_pHead;

  while (pos)
  {
    if (n == nIndex)
      return pos;

    pos = pos->m_pNext;
    n++;
  }

  return pos;
}

// ----------------------------------------------------------------------------
// Count the number of elements in the list
template <class C> int RmLstMgd<C>::GetCount() const
{
  int n = 0;

  for (RMPOS pos = m_pHead; pos;)
  {
    pos = pos->m_pNext;
    n++;
  }

  return n;
}

// ----------------------------------------------------------------------------
// Increase the size of the list by creating a new entry and adding it to the
// head of the list
template <class C> void RmLstMgd<C>::Grow(int grow)
{
  int i;

  if (grow > 0)
  {
    for (i = 0; i < grow; i++)
    {
      C* pC = new C;
      AddTail(pC);
    }
  }
  else
    if (grow < 0)
    {
      grow = -grow;

      for (i = 0; i < grow; i++)
        RemoveTail();
    }
}

// ----------------------------------------------------------------------------
// Set the list size to the specified amount - if required a managed list will
// create or delete blank classes as required
template <class C> void RmLstMgd<C>::SetSize(unsigned size)
{
  unsigned lSize = GetCount();
  unsigned entry;

  if (lSize == size)
    return;
  else
    if (lSize > size)
    {
      for (entry = size; entry < lSize; entry++)
        RemoveTail();
    }
    else
    {
      for (entry = lSize; entry < size; entry++)
      {
        C* pC = new C;
        AddTail(pC);
      }
    }
}

// ----------------------------------------------------------------------------
// Move an entry one place up
template <class C> void RmLstMgd<C>::ShiftUp(RMPOS pos)
{
  void* pEntry = pos->m_pEntry;

  if (pos->m_pNext)
  {
    pos->m_pEntry          = pos->m_pNext->m_pEntry;
    pos->m_pNext->m_pEntry = pEntry;
  }
}

// ----------------------------------------------------------------------------
// Move an entry one place down
template <class C> void RmLstMgd<C>::ShiftDn(RMPOS pos)
{
  void* pEntry = pos->m_pEntry;

  if (pos->m_pLast)
  {
    pos->m_pEntry          = pos->m_pLast->m_pEntry;
    pos->m_pLast->m_pEntry = pEntry;
  }
}

// ----------------------------------------------------------------------------
// Move an entry to the head of the list
template <class C> void RmLstMgd<C>::ShiftToHead(RMPOS pos)
{
  void* pEntry = pos->m_pEntry;

  if (m_pHead)
  {
    pos->m_pEntry     = m_pHead->m_pEntry;
    m_pHead->m_pEntry = pEntry;
  }
}

// ----------------------------------------------------------------------------
// Move an entry to the tail of the list
template <class C> void RmLstMgd<C>::ShiftToTail(RMPOS pos)
{
  if (!pos)
    return;

  if (pos == m_pHead && pos == m_pTail)
    return;
  if (pos == m_pTail)
    return;
  else
    if (pos == m_pHead)
    {
      m_pHead = m_pHead->m_pNext;
      m_pHead->m_pLast = nullptr;
    }
    else
    {
      pos->m_pLast->m_pNext = pos->m_pNext;
      pos->m_pNext->m_pLast = pos->m_pLast;
    }

  m_pTail->m_pNext = pos;
  pos->m_pLast = m_pTail;
  pos->m_pNext = nullptr;
  m_pTail = pos;
}

// ----------------------------------------------------------------------------
// Swap two entries in the list
template <class C> void RmLstMgd<C>::Swap(RMPOS pos1, RMPOS pos2)
{
  void* pEntry = pos1->m_pEntry;

  pos1->m_pEntry = pos2->m_pEntry;
  pos2->m_pEntry = pEntry;
}


//==============================================================================
#define CRC16_GEN_POLY		0xA001u			/* CRC16-IBM Generator polynomial(x^16 + x^15 + x^2 + 1) - LSB first code */

#define CRC16_LEN			sizeof(uint16)

class CCrc16
{
private:
  //Declare static variables
  static unsigned short _lut[256];

public:
  //Static Methods
  static void   Init(unsigned poly = CRC16_GEN_POLY);
  static unsigned short Calc(unsigned char* buf, unsigned offset, unsigned length, unsigned short init = 0);
};

