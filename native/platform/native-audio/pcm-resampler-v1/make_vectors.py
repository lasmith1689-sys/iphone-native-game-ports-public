"""Independent exact Fraction oracle; emits immutable synthetic PCM expectations."""
from pathlib import Path
from fractions import Fraction
import hashlib,json,sys
H=Path(__file__).resolve().parent
def rounded(v):
 sign=-1 if v<0 else 1;v=abs(v)
 return sign*((2*v.numerator+v.denominator)//(2*v.denominator))
def oracle(a,src,ic,dst,oc):
 n=len(a)//ic;result=[];time=Fraction(0);step=Fraction(src,dst)
 while time<n:
  i=time.numerator//time.denominator;part=time-i
  for c in range(oc):
   ch=0 if ic==1 else c
   result.append(rounded(Fraction(a[i*ic+ch])*(1-part)+Fraction(a[min(i+1,n-1)*ic+ch])*part))
  time+=step
 return result
def main():
 rows=[];lines=['/* Generated synthetic exact Fraction oracle; never retail PCM. */'];total=0
 for src in [11025,22050,44100]:
  for dst in [44100,48000]:
   for ic,oc in [(1,1),(1,2),(2,2)]:
    for n in [1,2,3,7,31,257]:
     a=[[-32768,32767,-1,0,1,-12345,23456][k%7]for k in range(n*ic)]
     expected=oracle(a,src,ic,dst,oc);k=len(rows);total+=len(expected)
     lines += [f'static const int16_t in_{k}[]={{'+','.join(map(str,a))+'};',f'static const int16_t expected_{k}[]={{'+','.join(map(str,expected))+'};']
     rows.append(f'{{in_{k},expected_{k},{n},{src},{ic},{dst},{oc},{len(expected)//oc}}}')
 lines += ['static const TestVector vectors[]={',',\n'.join(rows),'};']
 b=('\n'.join(lines)+'\n').encode()
 receipt=(json.dumps(dict(format='NGP-synthetic-Fraction-oracle-v1',vectors=len(rows),expectedSamples=total,bytes=len(b),sha256=hashlib.sha256(b).hexdigest(),retailInput=False),indent=2)+'\n').encode()
 if '--verify-existing'in sys.argv:
  assert (H/'vectors.inc').read_bytes()==b and (H/'vectors.json').read_bytes()==receipt
 else:
  (H/'vectors.inc').write_bytes(b);(H/'vectors.json').write_bytes(receipt)
 print(json.dumps(dict(vectors=len(rows),expectedSamples=total,bytes=len(b))))
if __name__=='__main__':main()
