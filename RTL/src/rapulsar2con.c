
/*rapulsar2con.c	   pwe: 02/04/2015
Takes rtlsdr .bin files, applies averaging and folding algorithm in blocks. Outputs text file of folded average.

Command format:- rapulsar2con <infile> <outfile> <clock rate(MHz)) <No. output data points> <Pulsar period(ms)>< pulsar pulse width>
Example command line:  try313.bin try313.txt 2.4 1024 714.482438 6.5              */

//obscut.bin obscutfold.txt 2.4 1024 714.473511 6.5   

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SWAP(a,b) tempr=(a); (a)=(b); (b)=tempr
#define PI (6.28318530717959/2.0)
#define MAX 1971


double datsb[32768],datbr[32768];


long long int count[32768];
long long int coun;
int PTS,v,vv;
long long int file_end,p_num;
float dats[32768],clck,pulw;
double periodf,dt,res,tim;
double smt,sumt[32768];
double targ[32768];
double mtarg[32768];
double out[32768];
double pdat[32768];
double prod[32768];
double mean=0;
unsigned char ucha;
double datout[8];
int mbin;
double outdat[4096];

FILE *fptr;
FILE *fpto;

void out_dat(void);
void bforce(double[],int,int);
float gauss(float,float,float);
void snr( int, double dat[]);
void psnr( int, double dat[]);
void four(double[],int,int);

int main(int argc,char *argv[])

{

long long int ss;
int s;



/*check command line arguments*/
if(argc !=7)
    { printf("Format: rapulsar2con <infile> <outfile> <clock rate (MHz)> <output data points> < pulsar period(ms)><pulsar p Width(ms)>\n");
    exit(0);
    }

if((fptr=fopen(argv[1],"rb"))==NULL)
    {printf("Can't open file %s. \n",argv[1]);
    exit(0);
    }

clck=1/atof(argv[3]);
PTS=atoi(argv[4]);
periodf = atof(argv[5]);
pulw = atof(argv[6]);
dt=periodf/(double)PTS;
                                        /*printf("PTS=%d   periodf =%ld\n",PTS,periodf);*/
if(pulw<=0)pulw=0.01;
pulw=pulw;

/*find length of input file*/
fseeko(fptr,0L,SEEK_END);
file_end=(long long)(ftello(fptr));


p_num=(long long)((file_end)/(PTS*2));

printf("No. Bytes = %lld   No. Blocks=%lld   PPeriod=%fms\n",(file_end),p_num,periodf);
fclose(fptr);

fptr=fopen(argv[1],"rb");
printf("Please wait....\n");

/*read input file,decode I and Q, determine power. Sum powers in folded period and on completion calculate bin average.
 At end of input file, output text file with averaged data*/

 {coun=0;

 for(ss=0;ss<p_num;ss++)
    {

    for(s=0;s<2*PTS;s+=2)
        {
        ucha=getc(fptr);
        dats[s]=(float)ucha+0.0;
        dats[s]=(dats[s]-127.5);

        ucha=getc(fptr);
        dats[s+1]=(float)ucha+0.0;
        dats[s+1]=(dats[s+1]-127.5);

        smt=dats[s]*dats[s]+dats[s+1]*dats[s+1];

        tim=(double)(s/2+(ss*PTS));
        res=(double)clck/(dt*(double)1000.0);
        res=(double)tim*(double)res;
        tim=(long long int)res%(long long int)PTS;

        sumt[(int)tim]=sumt[(int)tim]+smt;
        count[(int)tim]=count[(int)tim]+1;
coun=coun+1;
                    
        }
    }

 }

printf("No. bins=%d  Count/bin=%lld\n",PTS,count[(int)tim]);


for(v=0;v<PTS;v++){

pdat[2*v] = sumt[v]/count[v];
pdat[2*v+1] = 0;


}




bforce(pdat,PTS,1);
//pdat[PTS]=0;
for(v=0;v<PTS;v++){

targ[2*v] = gauss(v,(pulw*PTS/periodf),(int)PTS/2);
targ[2*v+1] = 0; //gauss(v,(pulw*PTS/periodf),(int)PTS/2);

//printf("%d    %f\n",v,targ[v]);
}

bforce(targ,PTS,1);

for(v=0;v<PTS;v++){

mtarg[v] = sqrt(targ[2*v]*targ[2*v]+targ[2*v+1]*targ[2*v+1]);


//printf("%d    %f\n",v,targ[v]);
}

printf("%f\n",mean);

for(v=0;v<PTS;v++){

prod[2*v] = (pdat[2*v])*mtarg[v]/mtarg[0];
prod[2*v+1] = pdat[2*v+1]*mtarg[v]/mtarg[0];

//printf("%d    %f\n",v,targ[v]);
}


bforce(prod,PTS,-1);

for(v=0;v<PTS;v++){

out[v] = (sqrt(prod[2*v]*prod[2*v]+prod[2*v+1]*prod[2*v+1]))/(float)PTS;


//printf("%d    %f\n",v,targ[v]);
}

for(v=0;v<PTS;v++){

mean=mean+out[v]/PTS;
//printf("%d    %f\n",v,targ[v]);
}

for(v=0;v<PTS;v++){

out[v] = out[v]-mean;


//printf("%d    %f\n",v,targ[v]);
}





psnr(PTS,out);

fpto = fopen(argv[2],"w");
out_dat();

fclose(fptr);
fclose(fpto);

printf("\nInfile=%s    Outfile=%s   End bin=%d   coun=%lld\n",argv[1],argv[2],(int)tim,coun);




exit(0);
}



void out_dat(void)
{
long int tt;

for(tt=0;tt<PTS;tt++)
    {

fprintf(fpto,"%ld    %3.10f\n",(tt),((float)outdat[tt])*1);

    }/*printf("%d\n",tt);*/
}


float gauss(float t, float T, float z )
{
int tt;
float m, out;

m=4.0*(float)log(2);


out= exp(-m*(t-z)*(t-z)/T/T);

return(out);

/*printf("%d\n",tt);*/
}









/*brute force fourier transform routine*/
void bforce( double data[], int nn,int isign)

 {
  int n,mmax,m,j,istep,i,a,k;
  double theta;
 
 float datao[32768];

  
  theta=6.28318530717959/nn;
 
for(n=0;n<nn;n++){
 
 datao[2*n] =0;
 datao[2*n+1]=0;

	}

for(k=0;k<nn;k++)
	{
for(n=0;n<nn;n++){
 datao[2*k] = datao[2*k] + (data[2*n]*cos(k*theta*n)+ (isign)*data[2*n+1]*sin(k*theta*n)); 
 datao[2*k+1] = datao[2*k+1] + (data[2*n+1]*cos(k*theta*n)- (isign)*data[2*n]*sin(k*theta*n)); 

	}
	}
for(n=0;n<nn;n++){
 data[2*n] = datao[2*n] ; 
 data[2*n+1] = datao[2*n+1] ;
 

	}



   
 if(isign==1){for(a=0;a<2*nn;a++)
{
  data[a]=data[a]/nn;}

 }
}



//Simple Peak to rms noise ratio 
void snr( int n, double dat[] )
{
int t,a;
double mn=0,rms=0, mx=0,nx=0,mb;
for(a=0;a<7;a++){datout[a]=0;
}
 

for(t=0;t<n;t++)
	{
	mn=mn+dat[t]/n;
	rms=rms+(dat[t]*dat[t])/n;
//printf("%f\n",dat[t]);
		if(dat[t]>mx)
		{
		mx=dat[t];
		if(t == mbin)mb=dat[t];
		nx=(float)t;
	//printf("%f	%f\n",mx,rms);
     	}	
	}

rms=sqrt(rms-mn*mn)+0.0001;
datout[0]=(mx-mn)/rms;
datout[1]=nx;
datout[3]=mn;
datout[4]=rms;
datout[5]=mx;
datout[6]=(mb-mn)/rms;
for(t=0;t<n;t++)
{outdat[t]=(dat[t]-mn)/rms;
}

}//end of noise snr


//Pulse peak to rms noise ratio	
void psnr( int n, double dat[] )
{
int t,n1,n2;
float mn=0,rms=0, mnr=0, rmsr=0, mx=0,nx=0,mb;

datout[0]=0; datout[1]=0;
datout[2]=0; datout[3]=0;
datout[4]=0; datout[5]=0;
datout[7]=0; datout[7]=0;
for(t=0;t<n;t++){

	mn=mn+dat[t];
	rms=rms+(dat[t]*dat[t]);

	if(dat[t]>mx){
	mx=dat[t];
	nx=(float)t;
     			}
	//if(t == nx)mb=dat[t];	
	}	
n1=(int)(nx-(int)((float)(0+8*pulw*PTS*1/1024)));//35 for large signals//8 small sigs
n2=(int)(nx+(int)((float)(0+8*pulw*PTS*1/1024)));
if(n1<0)n1=0;
if(n2>n-1)n2=n-1;
	for(t=n1;t<n2-1;t++){
	mnr=mnr+dat[t];
	rmsr=rmsr+(dat[t]*dat[t]);
	}
mn=(mn-mnr)/(n-n2+n1);
rms=(rms-rmsr)/(n-n2+n1);	
rms=rms-mn*mn;	

rms=sqrt(rms)+0.0000001;
datout[0]=(mx-mn)/rms;
if(sqrt(rms)<0.00001)datout[0]=0;
datout[1]=nx;
datout[3]=mn;
datout[4]=rms;
datout[5]=mx;
datout[6]=(mb-mn)/rms;
//return(datout[2]);
for(t=0;t<n;t++){
	
outdat[t]=(dat[t]-mn)/rms;
}
//printf("%f\n",datout[0]);
}//end of pulse snr







/*fast fourier transform routine*/
void four( double data[], int nn,int isign)
{
int n,mmax,m,j,istep,i,a;
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
/* printf("%d    %d    %d    %d    %d\n",i,n,m,PTS,istep);*/
    }
  	if(isign==1){for(a=0;a<2*PTS;a++){
  	data[a]=data[a]/PTS;}}
}
