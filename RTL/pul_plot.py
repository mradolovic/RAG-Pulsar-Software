# -*- coding: utf-8 -*-
# importing the required module 
import numpy as np
import matplotlib.pyplot as plt#
import matplotlib.image as img
#from matplotlib.lines 
import time

import math

from PIL import Image 
timestr=time.strftime("%Y%m%d-%H%M%S")
print (timestr) 


f = open('header.txt', 'r')#header file
f_contents = f.read()
print (f_contents)#print header file
f.close()

lines = []#store for header lines
f = open('header.txt', 'r')#header file
lines = f.readlines()
no_samples = (str(lines[14].split('=',1)[1]))

numlog = (str(lines[17].split('=',1)[1]))
numlog = math.log10(int(numlog))+6
numlog = int(numlog)
dmval = (str(lines[6].split('=',1)[1]))

rolav = (str(lines[23].split('=',1)[1]))
rolavi= int(rolav)

loadprofarray = np.loadtxt('profile.txt')
bins = loadprofarray.shape[0]#number of bands
profdat1 = loadprofarray[0:bins, 1]
profdat2 = loadprofarray[0:bins, 2]
profdat3 = loadprofarray[0:bins, 3]

loaddmprfarray = np.loadtxt('dmprf.txt')
profdm = loaddmprfarray[0:bins]

loadbndsarray = np.loadtxt('bandS.txt')

bands = loadbndsarray.shape[0]#number of fft bands
X = loadbndsarray[0:bands, 0]
Y = loadbndsarray[0:bands, 1]
Z = loadbndsarray[0:bands, 2]/bins#data bin value


loadDMarray = np.loadtxt('dmSearch.txt')#DM text filep#
dmrange = loadDMarray.shape[0]#number of dm points
dmdat0 = loadDMarray[0:dmrange, 0]#dm points
dmdat1 = loadDMarray[0:dmrange, 1]#dm value
dmdat2 = loadDMarray[0:dmrange, 2]/bins#peak dm bin
dmdat3 = loadDMarray[0:dmrange, 3]/15
dmdat4 = loadDMarray[0:dmrange, 4]
dmdatx = range(90)

loadDMnsarray = np.loadtxt('dmSrchns.txt')
dmns1 = loadDMnsarray[0:dmrange, 1]#dm with target removed
dmdatns=dmdat1-dmns1#dm difference

loadmaxarray = np.loadtxt('max.txt')#snr max text file
maxbin = loadmaxarray[0]#target bin number
maxsnr = loadmaxarray[1]#target maximum snr
#print("Max SNR", maxsnr, "Maxbin", int(maxbin))

loadcumbandarray = np.loadtxt('cumbands.txt')#cumulative snr bands
cumbands = loadcumbandarray[0:bands, 1]
band0 = loadcumbandarray[0:bands, 1]*0

loadbandarray = np.loadtxt('bandcum.txt')#target snr
line = loadbandarray[int(maxbin), 0:bands]
                     
#ovo je vjv neki visak u kodu, zbog tog krshi pul plot
#loadbandparray = np.loadtxt('bandcump.txt')#target snr
#linep = loadbandparray[int(maxbin), 0:bands]

loadrolarray = np.loadtxt('secavrol.txt')#rolling average range
sections = loadrolarray.shape[0]
rol = loadrolarray[1:sections, 1]#rolling average
#print(sections)
loadfolarray = np.loadtxt('secavfol.txt')#rolling average range



loadsavarray = np.loadtxt('secavsnr.txt')#section group average
savst0 = loadsavarray[0:sections, 0]# section average centre bin number
savsnr = loadsavarray[0:sections, 1]# section average value
# section average value

loadpuldatarray = np.loadtxt('puldatd.txt')
puld1 = loadpuldatarray[0:sections, int(maxbin)]
puld2 = loadpuldatarray[0:sections, int(maxbin)]
puld0 = loadpuldatarray[0:sections, int(maxbin)]*0

loadfoldarray = np.loadtxt('foldat.txt')
fold = loadfoldarray[0:sections, int(maxbin+0)]
#fold1 = loadfoldarray[0:sections, int(maxbin)]
#fold2 = loadfoldarray[0:sections, int(maxbin):int(2*maxbin)]

loadfoldparray = np.loadtxt('foldatd.txt')
foldp = loadfoldparray[0:sections, int(maxbin)]

loadbandfarray = np.loadtxt('bandat.txt')#band folds
bndflds = loadbandfarray[ 0:bands,int(maxbin)]

loadsecsnrarray = np.loadtxt('secsnr.txt')#section snr
savthry = loadsecsnrarray[0:sections, 3]
secsnr = loadsecsnrarray[0:sections, 1]
sec0 = loadsecsnrarray[0:sections, 0]

loadcumsecarray = np.loadtxt('cumsec.txt')#cumulative peaks
cumsec = loadcumsecarray[0:sections, 1]
cumsec2 = loadcumsecarray[0:sections, 2]/bins

loadPSarray = np.loadtxt('periodS.txt')#period search data
prdrange = loadPSarray.shape[0]
psdat0 = loadPSarray[0:prdrange, 0]
psdat1 = loadPSarray[0:prdrange, 1]
psdat2 = loadPSarray[0:prdrange, 2]/bins
psdat3 = loadPSarray[0:prdrange, 3]
psmin = loadPSarray[0, 0]
psmax = loadPSarray[prdrange-1,0]
#print("psmin ", psmin, "ppm, psmax ", psmax,"ppm")
pxx=str(psmin)#data for printing period search range 
py=str(psmax)
pc =" ppm to "
pm =" ppm"

loadPdarray = np.loadtxt('pdotS.txt')#p-dot search data
pddat0 = loadPdarray[0:prdrange, 0]
pddat1 = loadPdarray[0:prdrange, 1]
pddat2 = loadPdarray[0:prdrange, 2]/bins
pddat3 = loadPdarray[0:prdrange, 3]
pdmin = loadPdarray[0, 0]
pdmax = loadPdarray[prdrange-1,0]
pdmin =round(10*pdmax)/10
xe = "e"#data for printing p-dot search range
xx = str(-numlog)
m = "-"
yd = str(pdmin)
yy = str(numlog)
z = " to "
zz = 10
rangd = str(m+yd+xe+xx+z+yd+xe+xx)
rangp = str(pxx + pc + py + pm)
ran = str(xe+xx)
#print(rangd)
#print(rangp)
loadppdarray = np.loadtxt('ppd2d.txt')#2D period/p-dot plot data

loadallbarray = np.loadtxt('allbands.txt')#allbands data file
alx = loadallbarray.shape[0] # number of elements
allbands = loadallbarray[0:alx]

alx8 = (int)(alx/4)#reduced spectrum range

loadspallbarray = np.loadtxt('spallbands.txt')#compressed data spectrum
spallbands1 = loadspallbarray[0:alx8,0]
spallbands2 = loadspallbarray[0:alx8,1]


pixel_plot = plt.figure() # plotting a 2D plot

pixel_plot.add_subplot()
#fig = plt.figure(figsize =(6, 5)) 

plt.title("2D Period/P-dot")
pixel_plot = plt.imshow(
  loadppdarray, cmap='rainbow', interpolation='nearest', origin='lower')
plt.colorbar(pixel_plot)
plt.ylabel('P-dot Range ' +rangd)
plt.xlabel('Period Range ' +rangp)  

plt.savefig('pixel_plot.png') # save a plot 
#plt.show(pixel_plot)# show plot
plt.show()
pixel2_plot = plt.figure()
pixel2_plot.add_subplot() 
pixel2_plot.set_figwidth(5)
pixel2_plot.set_figheight(4)# customizing plot
plt.title("Cumulative Time Waterfall")
pixel2_plot = plt.imshow(
  loadfoldparray, cmap='rainbow', interpolation='none', origin='lower', extent =[0, bins/10, 0, sections*1])

plt.colorbar(pixel2_plot)
plt.xlabel('Bin Number/10')
plt.ylabel('Section Number')
plt.savefig('pixel2_plot.png')# save a plot
#plt.show(pixel2_plot)# show plot
plt.show()

pixel3_plot = plt.figure()  
pixel3_plot.add_subplot()
pixel3_plot.set_figwidth(5)
pixel3_plot.set_figheight(4)
#fig = plt.figure(figsize =(15, 5))
plt.title("Cumulative Band Waterfall")
pixel3_plot = plt.imshow(
loadbandarray, cmap='rainbow', interpolation='none', origin='lower',extent =[0, 4*bands, 0, bins/10])

plt.colorbar(pixel3_plot)
plt.ylabel('Bin Number/100')
plt.xlabel('Band Number x 4')
plt.savefig('pixel3_plot.png')
#plt.show(pixel3_plot)

plt.show()


fig = plt.figure(figsize =(16, 12))
sub4 = plt.subplot(4, 2, 1)
sub2 = plt.subplot(4, 2, 2)
sub3 = plt.subplot(4, 2, 3)
sub1 = plt.subplot(4, 2, 4)
sub5 = plt.subplot(4, 2, 5)
sub6 = plt.subplot(4, 2, 6)
sub7 = plt.subplot(4, 2, 7)
sub8 = plt.subplot(4, 2, 8)

sub1.plot(X, Y, '*', color='green') 
sub1.plot(X, Z,  '+', color='black') 
#sub1.plot(X,cumbands, '*', color='green')
sub1.plot(X,bndflds,  color='blue' ) 
sub1.plot(line ,color='red')
sub1.plot(X, band0, '-',color='black')
sub1.set_title('Band SNR')
sub1.set_ylabel('SNR')
sub1.set_xlabel('Band Number, (red: cumulative target, grn: band peak, blue: target SNR, blk: target bin')  
#x = np.linspace(0, 1, 100)
sub2.plot(dmdat0, dmdat1, color='red' )
sub2.plot(dmdat0, dmdat2, '+', color='black' )
#sub2.plot(dmdat0, dmns1, color='magenta' )  
#sub2.plot(dmdat0, dmdatns, color='blue' )  
sub2.plot(dmdat0, 3+(maxsnr-3)*dmdat4, '.', color='red' )  
#sub2.plot(dmdatx, dmdat3, color='blue' )  
dmval=float(dmval)
#sub2.plot((dmval)+x/10, (x)*maxsnr);#sub2.add.line(dmdat3) np.sin 
Xa = [dmval, dmval]
Ya = [0, maxsnr]

print(dmval)#Yaa = [0, bndflds]
# Plot vertical line
#Xaa = [X, 0]
#sub2.bar(Xa,Ya,200)
sub2.plot(Xa,Ya)

#sub2.plot.vlines(9, 2, 30, color='red')
sub2.set_title('DM Search')
sub2.set_ylabel('SNR')
sub2.set_xlabel('Dispersion Measure (red: dd search)')  
#, mag: target blanked, blue: target search

sub3.set_title('Period/P-dot Search')
sub3.set_ylabel('SNR')
sub3.set_xlabel('Period, (red: ppm); P-dot, (blue: (x ' + ran +')); + , peak bin numbers')  
sub3.plot(psdat0, psdat3, '.', color='red') 
sub3.plot(pddat0, pddat3, '.', color='blue') 
sub3.plot(psdat0, psdat1, color='red')
sub3.plot(pddat0, pddat1, color='blue')
sub3.plot(psdat0, psdat2, '+', color='red') 
sub3.plot(pddat0, pddat2,'+', color='blue') 


sub4.set_title('Pulse Profiles')
sub4.set_ylabel('SNR')
sub4.set_xlabel('Bin Number, (red: data fold; blue: deDisp fold)')
sub4.plot(profdat3, color='red')  
sub4.plot(profdm, color='blue') 

sub5.set_title('Cumulative SNR')
sub5.plot(cumsec2,'+', color='black')
sub5.plot(cumsec,color='green')  
sub5.plot(foldp,color='red') 
#sub5.plot(rol,'+', color= 'orange')

#sub5.plot(savthry, '.', color='red')
sub5.set_ylabel('SNR')
sub5.set_xlabel('Section Number (red: cum target peak, gr: cum all peak, blk: peak bin)')

sub6.set_title('Rolling Average SNR')
pdx = [0, sections]
pdy =[0, int(maxbin)]
sub6.plot(rol,'+', color= 'orange')
sub6.plot(savst0,savsnr, '-', color='magenta')
sub6.plot(puld0,'-',color='black')
#sub6.plot(secsnr, '.', color='blue' ) 
sub6.plot(puld1,'.', color='red')
sub6.set_ylabel('SNR')
sub6.set_xlabel('Section Number (red: target SNR, or: roll av, mag: roll='+ rolav+ ' )')

sub7.plot(secsnr,  color='red' )  
#sub7.plot(sec0, secsnr, '.', color='blue' ) 
sub7.set_title('Compressed Data')
sub7.set_xlabel('Time (Bins), (red: compr data, blue: target section SNR)')   
sub7.plot(sec0, puld1,'.', color='blue')
sub7.plot(sec0, puld0,'-',color='black')
sub8.plot(spallbands1, spallbands2, color='blue' ) 
sub8.set_title('Compressed Data Spectrum')
sub8.set_xlabel('Frequency (Hz)')   

testImage = img.imread('pixel_plot.png')  

# without writing plt.show() no plot
plt.tight_layout(pad=2.0, w_pad=2.0, h_pad=3.0)
# will be visible
plt.savefig('fig.png')

# Read First Image
im1 = Image.open('pixel_plot.png')
im2 = Image.open('pixel2_plot.png')
im3 = Image.open('pixel3_plot.png')
def get_concat_v(im1, im3):
    dst = Image.new('RGB', (im1.width+im2.width, im1.height))
    dst.paste(im1, (0, 0))
    dst.paste(im2, (im1.width, 0))
    #dst.paste(im3, (0, im1.height))
    return dst
get_concat_v(im1, im2).save('concat_v.png')
im3a = Image.open('concat_v.png')
#im3 = Image.open('pixel3_plot.png')
#get_concat_h(im1, im1).save('data/dst/pillow_concat_h.jpg')
def get_concat_v(im3, im3a):
    dst2 = Image.new('RGB', (im3a.width+im3.width, im3a.height))
    dst2.paste(im3a, (0, 0))
    dst2.paste(im3, (im3a.width, 0))
    #dst.paste(im3, (0, im1.height))
    return dst2

get_concat_v(im3, im3a).save('concat2_v.png')
#testImage = img.imread('concat_v.png')

im1 = Image.open('fig.png')
im2 = Image.open('concat2_v.png')

def get_concat_h(im1, im2):
    dst3 = Image.new('RGB', (im1.width , im1.height+im2.height))
    dst3.paste(im1, (0, 0))
    dst3.paste(im2, (0,im1.height))
    return dst3
get_concat_h(im1, im2).save('concat3_v.png')
testImage = img.imread('concat3_v.png')
plt.show()

testImage = img.imread('concat3_v.png')
plt.figure(figsize=(20.5, 20.5))
#testImage.set_figwidth(5)
#testImage.set_figheight(5)
plt.imshow(testImage,interpolation='nearest')


plt.show()
