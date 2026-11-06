"""Own bounded additional code/host-image facts. No stack/provider admission."""
import hashlib,struct
from inspect_image import inspect
def need(v):
 if not v:raise ValueError('owned image/code bounds rejected')
def header(raw,kind):
 need(type(raw)is bytes and 32<=len(raw)<=16*1024**2);magic,cpu,sub,typ,n,nb,flags,reserved=struct.unpack_from('<8I',raw)
 need((magic,cpu,sub,typ,reserved)==(0xfeedfacf,0x100000c,0,kind,0)and 1<=n<=128 and n*8<=nb<=65536 and 32+nb<=len(raw));at=32;commands=[];segments=[];sections=[];symtab=None;build=None;entry=None
 for _ in range(n):
  cmd,size=struct.unpack_from('<II',raw,at);need(size>=8 and size%8==0 and at+size<=32+nb);commands.append(cmd)
  if cmd==0x19:
   need(size>=72);name=raw[at+8:at+24].split(b'\0')[0].decode();vm,vmbytes,off,count,maxp,initp,ns,sf=struct.unpack_from('<4Q4I',raw,at+24);need(ns<=64 and size==72+80*ns and off<=len(raw)-count);seg=dict(name=name,vm=vm,vmbytes=vmbytes,offset=off,bytes=count,initprot=initp);segments.append(seg)
   for j in range(ns):
    p=at+72+80*j;sname=raw[p:p+16].split(b'\0')[0].decode();sn=raw[p+16:p+32].split(b'\0')[0].decode();addr,length,fileoff,align,rel,nrel,sflags,r1,r2,r3=struct.unpack_from('<QQ8I',raw,p+32);need((sn==name or kind==1 and name=='')and align<=31 and addr>=vm and length<=vmbytes-(addr-vm));zero=(sflags&255)in(1,0xc,0x12)
    if not zero:need(fileoff>=off and fileoff<=off+count-length and fileoff<=len(raw)-length and addr-vm==fileoff-off)
    sections.append(dict(name=sname,segment=sn,address=addr,bytes=length,fileOffset=fileoff,flags=sflags,zeroFill=zero,owner=seg))
  elif cmd==2:need(size==24 and symtab is None);symtab=struct.unpack_from('<4I',raw,at+8)
  elif cmd==0x32:
   need(size>=24 and build is None);p,m,s,t=struct.unpack_from('<4I',raw,at+8);need(t<=8 and size==24+8*t);build=dict(platform=p,minimum=m,sdk=s)
  elif cmd==0x80000028:need(size==24 and entry is None);entry=struct.unpack_from('<QQ',raw,at+8)[0]
  at+=size
 need(at==32+nb and symtab is not None and build is not None);symoff,count,stroff,ns=symtab;need(count<=65536 and ns<=4*1024**2 and symoff<=len(raw)-count*16 and stroff<=len(raw)-ns);strings=raw[stroff:stroff+ns];symbols=[]
 for i in range(count):
  ix,typ,section,desc,value=struct.unpack_from('<IBBHQ',raw,symoff+i*16);need(ix<ns and section<=len(sections));end=strings.find(b'\0',ix);need(ix<=end<=ix+1024);name=strings[ix:end].decode()
  if typ&0xe0==0:symbols.append(dict(name=name,type=typ,section=section,description=desc,value=value))
 tables=[(0,32+nb),(symoff,symoff+count*16),(stroff,stroff+ns)]
 for i,(lo,hi)in enumerate(tables):
  for a,b in tables[i+1:]:need(hi<=a or b<=lo)
 backed=[(s['fileOffset'],s['fileOffset']+s['bytes'])for s in sections if not s['zeroFill']and s['bytes']]
 for i,(lo,hi)in enumerate(backed):
  for a,b in tables:need(hi<=a or b<=lo)
  for a,b in backed[i+1:]:need(hi<=a or b<=lo)
 extents=[(s['offset'],s['offset']+s['bytes'])for s in segments if s['bytes']]
 for i,(lo,hi)in enumerate(extents):
  for a,b in extents[i+1:]:need(hi<=a or b<=lo)
 return dict(commands=commands,segments=segments,sections=sections,symbols=symbols,build=build,entryOffset=entry,commandEnd=32+nb,tables=tables)
def code_definitions(raw,meta,names):
 out={}
 for name in names:
  rows=[r for r in meta['symbols']if r['name']==name and r['type']&1 and r['type']&0xe==0xe and not r['description']&0xc0];need(len(rows)==1);r=rows[0];need(r['section']>0);s=meta['sections'][r['section']-1];o=s['owner'];need(not s['zeroFill']and s['segment']=='__TEXT'and o['name']=='__TEXT'and s['name']=='__text'and s['flags']&0x80000000 and o['initprot']&4 and not o['initprot']&2 and s['fileOffset']>=meta['commandEnd']and s['address']<=r['value']<s['address']+s['bytes']and r['value']%4==0);off=s['fileOffset']+r['value']-s['address'];need(off<=len(raw)-4);out[name]=dict(fileOffset=off,instructionWord=struct.unpack_from('<I',raw,off)[0],section=s['name'])
 return out
def device(raw,final=False,required=()):
 basic=inspect(raw,final=final);m=header(raw,6 if final else 1);need(m['build']==dict(platform=2,minimum=26<<16,sdk=(27<<16)|(1<<8)))
 # MH_OBJECT segments can have broad protection bits; strong runtime code
 # admission is an additional final-image fact only, never an object floor.
 code=code_definitions(raw,m,required)if final else{}
 return dict(rawFacts=basic,requiredOwnBackedCode=code,providerOrRuntimeOrStackAdmission=False)
def mac_executable(raw):
 m=header(raw,2);need(m['build']['platform']==1 and m['build']['minimum']==14<<16 and m['entryOffset']is not None);code=code_definitions(raw,m,('_main',));need(any(s['name']=='__text'and s['fileOffset']<=m['entryOffset']<s['fileOffset']+s['bytes']for s in m['sections']));return dict(build=m['build'],main=code['_main'],entryOffset=m['entryOffset'],loadCommands=m['commands'],boundedHeaderFactsOnly=True,completeLoaderOrStackAdmission=False)
