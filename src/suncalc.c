/*
 * based on
 * - http://williams.best.vwh.net/sunrise_sunset_algorithm.htm
 */
#include "suncalc.h"
#include "my_math.h"

float calcSun(int year, int month, int day, float latitude, float longitude, int sunset, float zenith)
{
  int N1 = my_floor(275 * month / 9);
  int N2 = my_floor((month + 9) / 12);
  int N3 = (1 + my_floor((year - 4 * my_floor(year / 4) + 2) / 3));
  int N = N1 - (N2 * N3) + day - 30;

  float lngHour = longitude / 15;
  
  float t;
  if (!sunset)
  {
    //if rising time is desired:
    t = N + ((6 - lngHour) / 24);
  }
  else
  {
    //if setting time is desired:
    t = N + ((18 - lngHour) / 24);
  }

  float M = (0.9856 * t) - 3.289;

  //calculate the Sun's true longitude
  //L = M + (1.916 * sin(M)) + (0.020 * sin(2 * M)) + 282.634
  float L = M + (1.916 * my_sin(M_PI * M / 180.0f)) + (0.020 * my_sin(M_PI * M / 90.0f)) + 282.634;
  if (L<0) L+=360.0f;
  if (L>360) L-=360.0f;

  //5a. calculate the Sun's right ascension
  //RA = atan(0.91764 * tan(L))
  float RA = (180.0f/M_PI) * my_atan(0.91764 * my_tan(M_PI * L / 180.0f));
  if (RA<0) RA+=360;
  if (RA>360) RA-=360;

  //5b. right ascension value needs to be in the same quadrant as L
  float Lquadrant  = my_floor(L / 90.0f) * 90.0f;
  float RAquadrant = my_floor(RA / 90.0f) * 90.0f;
  RA = RA + (Lquadrant - RAquadrant);

  //5c. right ascension value needs to be converted into hours
  RA = RA / 15;

  //6. calculate the Sun's declination
  float sinDec = 0.39782 * my_sin(M_PI * L / 180.0f);
  float cosDec = my_cos(my_asin(sinDec));

  //7a. calculate the Sun's local hour angle
  //cosH = (cos(zenith) - (sinDec * sin(latitude))) / (cosDec * cos(latitude))
  float cosH = (my_cos(M_PI * zenith / 180.0f) - (sinDec * my_sin(M_PI * latitude / 180.0f))) / (cosDec * my_cos(M_PI * latitude / 180.0f));
  
  if (cosH >  1) {
    return 0;
  }
  else if (cosH < -1)
  {
    return 0;
  }
    
  //7b. finish calculating H and convert into hours
  
  float H;
  if (!sunset)
  {
    //if rising time is desired:
    H = 360 - (180.0f/M_PI) * my_acos(cosH);
  }
  else
  {
    //if setting time is desired:
    H = (180.0f/M_PI) * my_acos(cosH);
  }
  
  H = H / 15;

  //8. calculate local mean time of rising/setting
  float T = H + RA - (0.06571 * t) - 6.622;

  //9. adjust back to UTC
  float UT = T - lngHour;
  if (UT<0) {UT+=24;}
  if (UT>24) {UT-=24;}

  return UT;
}
