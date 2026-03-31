
/*rafft22Lg.c     pwe: 02/06/13
Takes rtlsdr bin files applies FFT in blocks and averages over the input data length, plot data to log base 10.
	       Command format:- rafft22Lg <infile> <outfile><No FFT points>*/

//Example:-
//obscut.bin obsfft.txt 4096


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SWAP(a,b) tempr=(a); (a)=(b); (b)=tempr
#define PI (6.28318530717959/2.0)


long int pts;

long long int file_end,count,p_num;
double dats[262144],datr[131072];
unsigned char ucha;


FILE *fpto;

void sum_dat(void);
void out_dat(void);
void four(double[],long int,int);


main(int argc,char *argv[])
{
long int s;
long long ss;

FILE *fptr;
//FILE *fpto;

/*check command line arguments*/
if(argc !=4)
  	{ 
	printf("Format: rafft22Lg <Infile>  <Outfile> <No. FFT Points>\n");
  	exit(0);
	}
if((fptr=fopen(argv[1],"rb"))==NULL)
   {
   printf("Can't open file %s. ",argv[1]);
   exit(0);
   }
pts=atol(argv[3]);// No: of FFT points
if(pts>131072)
	{
	printf("Too many FFT points");exit(0);
	}	
fpto = fopen(argv[2],"w");
//fclose(fpto);
fptr=fopen(argv[1],"rb");

/*find length of input file*/
fseeko(fptr,0L,SEEK_END);
file_end=(long long)(ftello(fptr));

p_num=(long long int)(file_end/pts/2);

printf("No. of Bytes = %lld \n",file_end);
printf("No. of FFTs=%lld\n",p_num);
fclose(fptr);

/*read input file,decode I and q, put in data file apply fft, apply running average.
 At end of input file, output text file with averaged data*/
fptr=fopen(argv[1],"rb");
 	{
 	for(ss=0;ss<p_num;ss++)
    	{
 		for(s=0;s<2*pts;s++)
			{
    		ucha=getc(fptr);
			dats[s]=(float)ucha+0.0; //read one byte at time, conver to float
        	dats[s]=(dats[s]-127.5)/128.0; //mean zero				
			count++;
			}

/*take fourier transform*/
		four(dats-1,pts,-1);

		//convert to power and sum sequential spectra
		sum_dat();
    	}
	}

	// all spectra summed. output text file
	fpto = fopen(argv[2],"w");
	//write output file
	out_dat();
	fclose(fptr);
	fclose(fpto);
	printf("\nInfile=%s    Outfile=%s   FFT Points=%d\n",argv[1],argv[2],atoi(argv[3]));
	exit(0);
}



void out_dat(void)
{
long int tt;
float opp,logout;

for(tt=0;tt<pts;tt++)
	{
    if(tt<(pts/2))opp=(float)datr[tt+pts/2];
	else  opp=(float)datr[tt-pts/2];
    logout=10*log(opp/p_num)/log(10);
    fprintf(fpto,"%ld    %3.3f\n",(tt-pts/2),(float)(logout));
    }
}

//sum FFT powers
void sum_dat(void)
{
long int tt;
		
for(tt=0;tt<pts;tt++)
	{
	datr[tt]=datr[tt]+(float)(dats[2*tt]*dats[2*tt]+dats[2*tt+1]*dats[2*tt+1]);
	}
}


/*fast fourier transform routine*/
void four( double data[], long int nn,int isign)
 {
  long int n,mmax,m,j,istep,i,a;
  double wtemp,wr,wpr,wpi,wi,theta;
  double tempr,tempi;
  n=nn<<1;
  j=1;
  for(i=1;i<n;i+=2)
     {
     if(j>i)
	 	{
	    SWAP(data[j],data[i]);
	    SWAP(data[j+1],data[i+1]);
	    }
     m=n>>1;
     while(m>=2 && j>m)
	 	{
		j-=m;
		m>>=1;
		}
     j+=m;
     }
  	mmax=2;
  	while(n>mmax)
    	{
     	istep=2*mmax;
     	theta=6.28318530717959/(isign*mmax);
     	wtemp=sin(0.5*theta);
     	wpr=-2.0*wtemp*wtemp;
     	wpi=sin(theta);
     	wr=1.0;
     	wi=0.0;
     	for(m=1;m<mmax;m+=2)
			{
       		for(i=m;i<=n;i+=istep)
			   {
	  			j=i+mmax;
	  			tempr=wr*data[j]-wi*data[j+1];
	  			tempi=wr*data[j+1]+wi*data[j];
	  			data[j]=data[i]-tempr;
	  			data[j+1]=data[i+1]-tempi;
	  			data[i]+=tempr;
	  			data[i+1]+=tempi;
 				if(j<0)j=0;
			    }
       		wr=(wtemp=wr)*wpr-wi*wpi+wr;
       		wi=wi*wpr+wtemp*wpi+wi;
			}
     	mmax=istep;
     	}
  	if(isign==1){for(a=0;a<2*pts;a++)
		{
  		data[a]=data[a]/pts;
		}
	}
 }
