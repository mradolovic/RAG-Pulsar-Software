# -*- coding: utf-8 -*-
"""
Created on Wed Nov 20 10:50:56 2019

@author: Peter
"""

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon Jan  8 12:49:29 2018

@author: wolfgang (modified by pwe 21 November 2019)
"""

from astropy.time import Time
from astropy.coordinates import SkyCoord, EarthLocation
from astropy import units as u
#from astropy.units import cds
#import sys


#intm= 2024-02-15T00:00:00.000
inlt= 53.25
inln= -0.3
print ("Latt =", inlt,"deg" )
print ("Long =", inln,"deg" )
#print ("Date =", intm,"" )
""" UTC time Format """
#Date = (intm)
Date= Time(Date, format='isot',scale='utc')
print ("Date/UTC =", Date )
   
mj= Time(Date, format='mjd')
"""Type in and replace Local Lat and Long"""
Loc = EarthLocation.from_geodetic(lat=inlt, lon=inln, height=0)
print ("MJD =", mj )
"""B0329+54 RA and Dec Coordinates"""
sc = SkyCoord(ra=3.54972*u.hr, dec=54.5786*u.deg)
barycorrn = sc.radial_velocity_correction(obstime=Time(mj, format='mjd'), location=Loc)
c=299792458

"""B0329 ATNF Parameters"""
P0=0.714519699726
P1=2.048265*10**-15
PEP=46473.00
"""Current Barycentric Period"""
PC=((mj.value-PEP)*P1*24*3600) + P0
"""Topocentric correction"""
correctionn = barycorrn.value / c * -10**6
TC=PC*(1+correctionn/10**6)

print ("Doppler Vel =",barycorrn.value, "m/s")
print ("ppm =",correctionn)
print ("B0329 Barycentric period =", PC )
print ("B0329 Topocentric period =", TC )
#timet = Time(mj, format='isot',scale='utc')

