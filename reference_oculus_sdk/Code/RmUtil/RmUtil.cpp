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

#include "RmUtil.h"

double RmUtil::markers[6] = {0.1, 0.2, 0.15, 0.25, 0.50, 0.75};

const double RmUtil::threshold = 0.000001;

const double RmUtil::r2d     = 57.295779513082320876798154814105;
const double RmUtil::d2r     =  0.017453292519943295769236907684886;
const double RmUtil::pi      =  3.1415926535897932384626433832795;
const double RmUtil::pi_2    =  1.57079632679489661923132169163975;
const double RmUtil::pi3_2   =  4.71238898038468985769396507491925;
const double RmUtil::pi_4    =  0.785398163397448309615660845819876;
const double RmUtil::pi2     =  6.28318530717958647692528676655901;
const double RmUtil::root2   =  1.4142135623730950488016887242097;
const double RmUtil::root2_2 =  0.707106781186547524400844362104849;
const double RmUtil::e       =  2.71828182845904523536028747135266;

// ----------------------------------------------------------------------------
// Get the nearest marker to the number defined (always round up
double RmUtil::NearestMkr(double marker)
{
  double ratio = 0.1;

  while (marker >= markers[5] * ratio)
    ratio *= 10.0;

  double last = markers[5] * ratio / 10.0;

  for (int pos = 0; pos < 6; pos++)
  {
    double next = markers[pos] * ratio;

    if (marker < next)
    {
      if (fabs(marker - last) < fabs(marker - next))
        return last;
      else
        return next;
    }

    last = next;
  }

  return marker;
}

// ----------------------------------------------------------------------------
// Get the next highest spacing based upon the given spacing
double RmUtil::NextMkr(double marker)
{
  bool   found = false;
  int    pos   = 0;
  double ratio = 0.1;

  while (marker >= markers[5] * ratio)
    ratio *= 10.0;

  pos = 0;

  while (!found && pos < 6)
  {
    if (marker < markers[pos] * ratio)
    {
      marker = markers[pos] * ratio;
      found  = true;
    }

    pos++;
  }

  return marker;
}

// ----------------------------------------------------------------------------
// Get the next smallest spacing based upon the given spacing
double RmUtil::PrevMkr(double marker)
{
  bool   found = false;
  int    pos   = 0;
  double ratio = 0.1;

  while (marker >= markers[5] * ratio)
    ratio *= 10.0;

  pos = 5;

  while (!found && pos > 0)
  {
    if (   marker <= markers[pos]     * ratio
        && marker >  markers[pos - 1] * ratio)
    {
      marker = markers[pos - 1] * ratio;
      found  = true;
    }

    pos--;
  }

  return marker;
}

// ----------------------------------------------------------------------------
// Return the range between two lat longs based on simple sheroid "haversine formula"
double RmUtil::LL2LLRng(double lat1, double long1, double lat2, double long2)
{
  double R = 6371000; // metres (average radius of the spheroid)

  double _lat1 = qDegreesToRadians(lat1);
  double _lat2 = qDegreesToRadians(lat2);
  double dLat  = qDegreesToRadians(lat2 - lat1);
  double dLong = qDegreesToRadians(long2 - long1);

  double a = sin(dLat / 2.0) * sin(dLat / 2) +
             cos(_lat1) * cos(_lat2) * sin(dLong / 2.0) * sin(dLong / 2.0);
  double c = 2.0 * atan2(sqrt(a), sqrt(1 - a));

  return R * c;
}

// ----------------------------------------------------------------------------
// Return the bearing between two lat longs based on simple sheroid
double RmUtil::LL2LLBrg(double lat1, double long1, double lat2, double long2)
{
  double _lat1 = qDegreesToRadians(lat1);
  double _lat2 = qDegreesToRadians(lat2);
  double dLong = qDegreesToRadians(long2 - long1);

  double y = sin(dLong) * cos(_lat2);
  double x = cos(_lat1) * sin(_lat2) - sin(_lat1) * cos(_lat2) * cos(dLong);

  double degrees = qRadiansToDegrees(atan2(y, x));

  degrees = fmod(360.0 + degrees, 360.0);

  return degrees;
}

// ----------------------------------------------------------------------------
// Return the distance cross track of a position relative to two ll positions
double RmUtil::CrossTrack(double lat1, double long1, double lat2, double long2, double atLat, double atLong)
{
  double R = 6371000; // metres (average radius of the spheroid)
  double r1a = LL2LLRng(lat1, long1, atLat, atLong);
  double t1a = LL2LLBrg(lat1, long1, atLat, atLong);
  double t12 = LL2LLBrg(lat1, long1, lat2, long2);

  return asin(sin(r1a) * sin(t1a - t12)) * R;
}

// ----------------------------------------------------------------------------
// Return the distance along track from ll1 of a position relative to two ll positions
double RmUtil::AlongTrack(double lat1, double long1, double lat2, double long2, double atLat, double atLong)
{
  double R = 6371000; // metres (average radius of the spheroid)
  double r1a = LL2LLRng(lat1, long1, atLat, atLong);
  double rxt = CrossTrack(lat1, long1, lat2, long2, atLat, atLong);

  return acos(cos(r1a) / cos(rxt)) * R;
}

// ----------------------------------------------------------------------------
// Return the bearing offset between two values based on vector difference
double RmUtil::BrgOffsetDegrees(double centre, double reference)
{
  return fmod(((fmod((reference - centre), 360.0)) + 540.0), 360.0) - 180.0;
}


// ================================================================================================
// Matrix class

// ------------------------------------------------------------------------------------------------
RmMatrix::RmMatrix()
{
  SetToIdentity();
}

// ------------------------------------------------------------------------------------------------
// Set this matrix to the identity matrix
void RmMatrix::SetToIdentity()
{
  m[0][0] = 1.0f;
  m[0][1] = 0.0f;
  m[0][2] = 0.0f;
  m[0][3] = 0.0f;
  m[1][0] = 0.0f;
  m[1][1] = 1.0f;
  m[1][2] = 0.0f;
  m[1][3] = 0.0f;
  m[2][0] = 0.0f;
  m[2][1] = 0.0f;
  m[2][2] = 1.0f;
  m[2][3] = 0.0f;
  m[3][0] = 0.0f;
  m[3][1] = 0.0f;
  m[3][2] = 0.0f;
  m[3][3] = 1.0f;

  flagBits = eIdentity;
}

// ------------------------------------------------------------------------------------------------
// multiply this matrix by the given
RmMatrix& RmMatrix::operator *= (const RmMatrix& other)
{
  if (flagBits == eIdentity)
  {
    *this = other;
    return *this;
  }
  else
  if (other.flagBits == eIdentity)
  {
    return *this;
  }
  else
  {
    *this = *this * other;
    return *this;
  }
}

// ------------------------------------------------------------------------------------------------
bool RmMatrix::operator == (const RmMatrix& other) const
{
  return m[0][0] == other.m[0][0] &&
         m[0][1] == other.m[0][1] &&
         m[0][2] == other.m[0][2] &&
         m[0][3] == other.m[0][3] &&
         m[1][0] == other.m[1][0] &&
         m[1][1] == other.m[1][1] &&
         m[1][2] == other.m[1][2] &&
         m[1][3] == other.m[1][3] &&
         m[2][0] == other.m[2][0] &&
         m[2][1] == other.m[2][1] &&
         m[2][2] == other.m[2][2] &&
         m[2][3] == other.m[2][3] &&
         m[3][0] == other.m[3][0] &&
         m[3][1] == other.m[3][1] &&
         m[3][2] == other.m[3][2] &&
         m[3][3] == other.m[3][3];
}





// ------------------------------------------------------------------------------------------------
void RmMatrix::Scale(const RmVector& vector)
{
  float vx = vector.x;
  float vy = vector.y;
  float vz = vector.z;

  if (flagBits == eIdentity)
  {
    m[0][0] = vx;
    m[1][1] = vy;
    m[2][2] = vz;
    flagBits = eScale;
  }
  else
  if (flagBits == eScale || flagBits == (eScale | eTranslation))
  {
    m[0][0] *= vx;
    m[1][1] *= vy;
    m[2][2] *= vz;
  }
  else
  if (flagBits == eTranslation)
  {
    m[0][0] = vx;
    m[1][1] = vy;
    m[2][2] = vz;
    flagBits |= eScale;
  }
  else
  {
    m[0][0] *= vx;
    m[0][1] *= vx;
    m[0][2] *= vx;
    m[0][3] *= vx;
    m[1][0] *= vy;
    m[1][1] *= vy;
    m[1][2] *= vy;
    m[1][3] *= vy;
    m[2][0] *= vz;
    m[2][1] *= vz;
    m[2][2] *= vz;
    m[2][3] *= vz;
    flagBits = eGeneral;
  }
}


// ------------------------------------------------------------------------------------------------
void RmMatrix::Translate(const RmVector& vector)
{
  float vx = vector.x;
  float vy = vector.y;
  float vz = vector.z;

  if (flagBits == eIdentity)
  {
    m[3][0] = vx;
    m[3][1] = vy;
    m[3][2] = vz;
    flagBits = eTranslation;
  }
  else
  if (flagBits == eTranslation)
  {
    m[3][0] += vx;
    m[3][1] += vy;
    m[3][2] += vz;
  }
  else
  if (flagBits == eScale)
  {
    m[3][0] = m[0][0] * vx;
    m[3][1] = m[1][1] * vy;
    m[3][2] = m[2][2] * vz;
    flagBits |= eTranslation;
  }
  else
  if (flagBits == (eScale | eTranslation))
  {
    m[3][0] += m[0][0] * vx;
    m[3][1] += m[1][1] * vy;
    m[3][2] += m[2][2] * vz;
  }
  else
  {
    m[3][0] += m[0][0] * vx + m[1][0] * vy + m[2][0] * vz;
    m[3][1] += m[0][1] * vx + m[1][1] * vy + m[2][1] * vz;
    m[3][2] += m[0][2] * vx + m[1][2] * vy + m[2][2] * vz;
    m[3][3] += m[0][3] * vx + m[1][3] * vy + m[2][3] * vz;

    if (flagBits == eRotation)
      flagBits |= eTranslation;
    else
    if (flagBits != (eRotation | eTranslation))
      flagBits = eGeneral;
  }
}

// ------------------------------------------------------------------------------------------------
void RmMatrix::Rotate(float angle, const RmVector& vector)
{
  Rotate(angle, vector.x, vector.y, vector.z);
}

// ------------------------------------------------------------------------------------------------
void RmMatrix::Rotate(float angle, float x, float y, float z)
{
  if (angle == 0.0f)
    return;

  RmMatrix m;

  float c, s, ic;

  if (angle == 90.0f || angle == -270.0f)
  {
    s = 1.0f;
    c = 0.0f;
  }
  else
  if (angle == -90.0f || angle == 270.0f)
  {
    s = -1.0f;
    c = 0.0f;
  }
  else
  if (angle == 180.0f || angle == -180.0f)
  {
      s = 0.0f;
      c = -1.0f;
  }
  else
  {
    float a = angle * M_PI / 180.0f;
    c = cos(a);
    s = sin(a);
  }

  bool quick = false;

  if (x == 0.0f)
  {
    if (y == 0.0f)
    {
      if (z != 0.0f)
      {
        // Rotate around the Z axis.
        m.SetToIdentity();
        m.m[0][0] = c;
        m.m[1][1] = c;

        if (z < 0.0f)
        {
          m.m[1][0] = s;
          m.m[0][1] = -s;
        }
        else
        {
          m.m[1][0] = -s;
          m.m[0][1] = s;
        }

        m.flagBits = eGeneral;
        quick = true;
      }
    }
    else
    if (z == 0.0f)
    {
      // Rotate around the Y axis.
      m.SetToIdentity();
      m.m[0][0] = c;
      m.m[2][2] = c;

      if (y < 0.0f)
      {
          m.m[2][0] = -s;
          m.m[0][2] = s;
      }
      else
      {
          m.m[2][0] = s;
          m.m[0][2] = -s;
      }

      m.flagBits = eGeneral;
      quick = true;
    }
  }
  else
  if (y == 0.0f && z == 0.0f)
  {
    // Rotate around the X axis.
    m.SetToIdentity();
    m.m[1][1] = c;
    m.m[2][2] = c;

    if (x < 0.0f)
    {
      m.m[2][1] = s;
      m.m[1][2] = -s;
    }
    else
    {
      m.m[2][1] = -s;
      m.m[1][2] = s;
    }

    m.flagBits = eGeneral;
    quick = true;
  }

  if (!quick)
  {
    float len = x * x + y * y + z * z;

    if (!RmUtil::EqualT(len, 1.0f) && !RmUtil::EqualT(len, 0.0f))
    {
      len = sqrt(len);
      x /= len;
      y /= len;
      z /= len;
    }

    ic = 1.0f - c;
    m.m[0][0] = x * x * ic + c;
    m.m[1][0] = x * y * ic - z * s;
    m.m[2][0] = x * z * ic + y * s;
    m.m[3][0] = 0.0f;
    m.m[0][1] = y * x * ic + z * s;
    m.m[1][1] = y * y * ic + c;
    m.m[2][1] = y * z * ic - x * s;
    m.m[3][1] = 0.0f;
    m.m[0][2] = x * z * ic - y * s;
    m.m[1][2] = y * z * ic + x * s;
    m.m[2][2] = z * z * ic + c;
    m.m[3][2] = 0.0f;
    m.m[0][3] = 0.0f;
    m.m[1][3] = 0.0f;
    m.m[2][3] = 0.0f;
    m.m[3][3] = 1.0f;
  }

  int flags = flagBits;
  *this *= m;

  if (flags != eIdentity)
    flagBits = flags | eRotation;
  else
    flagBits = eRotation;
}

// ------------------------------------------------------------------------------------------------
void RmMatrix::Ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
  // Bail out if the projection volume is zero-sized.
  if (left == right || bottom == top || nearPlane == farPlane)
      return;

    // Construct the projection.
  float width = right - left;
  float invheight = top - bottom;
  float clip = farPlane - nearPlane;

  if (clip == 2.0f && (nearPlane + farPlane) == 0.0f)
  {
    // We can express this projection as a translate and scale
    // which will be more efficient to modify with further
    // transformations than producing a "General" matrix.
    Translate(RmVector
        (-(left + right) / width,
         -(top + bottom) / invheight,
         0.0f));

    Scale(RmVector
        (2.0f / width,
         2.0f / invheight,
         -1.0f));
    return;
  }

  RmMatrix m;
  m.m[0][0] = 2.0f / width;
  m.m[1][0] = 0.0f;
  m.m[2][0] = 0.0f;
  m.m[3][0] = -(left + right) / width;
  m.m[0][1] = 0.0f;
  m.m[1][1] = 2.0f / invheight;
  m.m[2][1] = 0.0f;
  m.m[3][1] = -(top + bottom) / invheight;
  m.m[0][2] = 0.0f;
  m.m[1][2] = 0.0f;
  m.m[2][2] = -2.0f / clip;
  m.m[3][2] = -(nearPlane + farPlane) / clip;
  m.m[0][3] = 0.0f;
  m.m[1][3] = 0.0f;
  m.m[2][3] = 0.0f;
  m.m[3][3] = 1.0f;

  // Apply the projection.
  *this *= m;
  return;
}

// ------------------------------------------------------------------------------------------------
// Calculate the determinant of a 3x3 sub-matrix.
//     | A B C |
// M = | D E F |   det(M) = A * (EI - HF) - B * (DI - GF) + C * (DH - GE)
//     | G H I |
float RmMatrix::Det3(int col0, int col1, int col2, int row0, int row1, int row2)
{
  return m[col0][row0] *
         (m[col1][row1] * m[col2][row2] -
          m[col1][row2] * m[col2][row1]) -
         m[col1][row0] *
         (m[col0][row1] * m[col2][row2] -
          m[col0][row2] * m[col2][row1]) +
         m[col2][row0] *
         (m[col0][row1] * m[col1][row2] -
          m[col0][row2] * m[col1][row1]);
}

// Calculate the determinant of a 4x4 matrix.
float RmMatrix::Det4()
{
  float det;
  det  = m[0][0] * Det3(1, 2, 3, 1, 2, 3);
  det -= m[1][0] * Det3(0, 2, 3, 1, 2, 3);
  det += m[2][0] * Det3(0, 1, 3, 1, 2, 3);
  det -= m[3][0] * Det3(0, 1, 2, 1, 2, 3);
  return det;
}

// ------------------------------------------------------------------------------------------------
// Return the ortho normal inverse of the matrix
RmMatrix RmMatrix::OrthoInverse()
{
  RmMatrix result;

  result.m[0][0] = m[0][0];
  result.m[1][0] = m[0][1];
  result.m[2][0] = m[0][2];

  result.m[0][1] = m[1][0];
  result.m[1][1] = m[1][1];
  result.m[2][1] = m[1][2];

  result.m[0][2] = m[2][0];
  result.m[1][2] = m[2][1];
  result.m[2][2] = m[2][2];

  result.m[0][3] = 0.0f;
  result.m[1][3] = 0.0f;
  result.m[2][3] = 0.0f;

  result.m[3][0] = -(result.m[0][0] * m[3][0] + result.m[1][0] * m[3][1] + result.m[2][0] * m[3][2]);
  result.m[3][1] = -(result.m[0][1] * m[3][0] + result.m[1][1] * m[3][1] + result.m[2][1] * m[3][2]);
  result.m[3][2] = -(result.m[0][2] * m[3][0] + result.m[1][2] * m[3][1] + result.m[2][2] * m[3][2]);
  result.m[3][3] = 1.0f;

  result.flagBits = eRotation | eTranslation;

  return result;
}

// ------------------------------------------------------------------------------------------------
// If possible return the inverse of this matrix (ok flags success)
RmMatrix RmMatrix::Inverse(bool& ok)
{
  // Handle some of the easy cases first.
  if (flagBits == eIdentity)
  {
    ok = true;
    return RmMatrix();
  }
  else
  if (flagBits == eTranslation)
  {
    RmMatrix inv;
    inv.m[3][0] = -m[3][0];
    inv.m[3][1] = -m[3][1];
    inv.m[3][2] = -m[3][2];
    inv.flagBits = eTranslation;
    ok = true;
    return inv;
  }
  else
  // If the matrix is orthonormal then just transpose
  if (flagBits == eRotation || flagBits == (eRotation | eTranslation))
  {
    ok = true;
    return OrthoInverse();
  }

  RmMatrix inv;

  float det = Det4();

  if (det == 0.0f)
  {
    ok = false;
    return RmMatrix();
  }

  det = 1.0f / det;

  inv.m[0][0] =  Det3(1, 2, 3, 1, 2, 3) * det;
  inv.m[0][1] = -Det3(0, 2, 3, 1, 2, 3) * det;
  inv.m[0][2] =  Det3(0, 1, 3, 1, 2, 3) * det;
  inv.m[0][3] = -Det3(0, 1, 2, 1, 2, 3) * det;
  inv.m[1][0] = -Det3(1, 2, 3, 0, 2, 3) * det;
  inv.m[1][1] =  Det3(0, 2, 3, 0, 2, 3) * det;
  inv.m[1][2] = -Det3(0, 1, 3, 0, 2, 3) * det;
  inv.m[1][3] =  Det3(0, 1, 2, 0, 2, 3) * det;
  inv.m[2][0] =  Det3(1, 2, 3, 0, 1, 3) * det;
  inv.m[2][1] = -Det3(0, 2, 3, 0, 1, 3) * det;
  inv.m[2][2] =  Det3(0, 1, 3, 0, 1, 3) * det;
  inv.m[2][3] = -Det3(0, 1, 2, 0, 1, 3) * det;
  inv.m[3][0] = -Det3(1, 2, 3, 0, 1, 2) * det;
  inv.m[3][1] =  Det3(0, 2, 3, 0, 1, 2) * det;
  inv.m[3][2] = -Det3(0, 1, 3, 0, 1, 2) * det;
  inv.m[3][3] =  Det3(0, 1, 2, 0, 1, 2) * det;

  inv.flagBits = eGeneral;

  ok = true;
  return inv;
}
// ================================================================================================
// Vector class
RmVector::RmVector()
{
  pV = &x;

  x = 0.0f;
  y = 0.0f;
  z = 0.0f;
}

RmVector::RmVector(float ix, float iy, float iz)
{
  pV = &x;

  x = ix;
  y = iy;
  z = iz;
}


//==============================================================================
//Define static variables
unsigned short CCrc16::_lut[256];

/*!-----------------------------------------------------------------------------
Function that initialises the CRC16 lookup table to save on arithmetic when
further CRC functions are performed
@param[in] poly	Generator polynomial used for the lookup tables
*/
void CCrc16::Init(unsigned poly)
{
  unsigned i, j, temp;
  unsigned short* lut = CCrc16::_lut;

  for(i = 0; i < 256; i++) {
    temp = i;
    for(j = 0; j < 8; j++) {
      if(temp & 0x0001u)
        temp = (temp >> 1) ^ poly;
      else
        temp >>= 1;
    }
    *lut = temp;
    lut++;
  }
}

// ================================================================================================
// Function that computes the CRC16 value for a buffer of data
// @param[in]	buf		Pointer to the buffer containing the data to analyse.
// @param[in]	offset	The offset from the begining of the buffer to start checksumming data at.
// @param[in]	length	The number of bytes to run through the checksum.
// @param[in]	init	The inital value to start computing the CRC from (this may be a previous CRC operation)
// @result				The computed CRC value.

unsigned short CCrc16::Calc(unsigned char* buf, unsigned offset, unsigned length, unsigned short init)
{
  unsigned ctr = 0;
  unsigned csum = init;
  unsigned tmp;

  for(ctr = offset; ctr < (length + offset); ctr++) {
    //tmp = *(buf + ctr) ^ ((uint8)csum);
    //csum >>= 8;
    //csum ^= CCrc16::_lut[tmp];
    tmp = *(buf + ctr) ^ (csum & 0xFF);
    csum >>= 8;
    csum ^= CCrc16::_lut[(unsigned char)tmp];
  }
  return csum & 0xFFFFu;
}

