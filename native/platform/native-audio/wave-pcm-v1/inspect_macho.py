"""Bounded thin ARM64 dylib facts for the two own PCM WAVE C APIs; never executes."""
import struct
API={'_ngp_wave_pcm_v1_parse'}
def inspect(b):
 assert 32<=len(b)<=2*1024*1024
 magic,cpu,subtype,kind,n,nb,flags,reserved=struct.unpack_from('<8I',b)
 assert (magic,cpu,subtype,kind)==(0xfeedfacf,0x100000c,0,6)
 assert 0<n<=128 and 32+nb<=len(b)
 at=32;symbols=None;sections=[];versions=[];deps=[];names=[];encryption=[]
 for _ in range(n):
  assert at+8<=32+nb
  cmd,size=struct.unpack_from('<II',b,at);assert size>=8 and size%8==0 and at+size<=32+nb
  assert cmd not in(0x1d,0x21,0x80000034),'Unsigned classic-fixup image required; no legacy32 encryption command'
  if cmd==0x2c:
   assert size==24 and not encryption
   cryptoff,cryptsize,cryptid,pad=struct.unpack_from('<4I',b,at+8)
   assert cryptid==0 and pad==0 and cryptoff<=len(b)and cryptsize<=len(b)-cryptoff
   encryption.append(dict(cryptoff=cryptoff,cryptsize=cryptsize,cryptid=cryptid,pad=pad))
  if cmd==0x32:
   assert size>=24;v=struct.unpack_from('<III',b,at+8);assert v[0]==2 and v[1]==26<<16 and v[2]>>16>=26;versions.append(v)
  if cmd in(0xc,0xd):
   assert size>=24;off=struct.unpack_from('<I',b,at+8)[0];assert 24<=off<size
   end=b.find(b'\0',at+off,at+size);assert end!=-1
   (deps if cmd==0xc else names).append(b[at+off:end].decode('ascii'))
  if cmd==0x19:
   assert size>=72;count=struct.unpack_from('<I',b,at+64)[0];assert size==72+count*80
   segname=b[at+8:at+24].split(b'\0')[0]
   va,vmbytes,fileoff,filebytes=struct.unpack_from('<4Q',b,at+24)
   assert va+vmbytes<=1<<64 and fileoff<=len(b)and filebytes<=len(b)-fileoff
   for j in range(count):
    p=at+72+j*80;sn=b[p:p+16].split(b'\0')[0];sg=b[p+16:p+32].split(b'\0')[0]
    addr,length,offset=struct.unpack_from('<QQI',b,p+32);stype=struct.unpack_from('<I',b,p+64)[0]&255
    assert sg==segname and va<=addr and addr+length<=va+vmbytes
    if stype not in(1,0x12):
     assert offset>=32+nb and fileoff<=offset and offset+length<=fileoff+filebytes
     assert offset-fileoff==addr-va
     for _,_,_,oldlength,oldoff,oldtype in sections:
      if oldtype not in(1,0x12)and length and oldlength:assert offset+length<=oldoff or oldoff+oldlength<=offset
    sections.append((sg,sn,addr,length,offset,stype))
  if cmd==2:
   assert size==24 and symbols is None;symbols=struct.unpack_from('<4I',b,at+8)
  at+=size
 assert at==32+nb and len(versions)==1 and deps==['/usr/lib/libSystem.B.dylib']
 assert names==['@rpath/libNgpWavePcmV1.dylib']and symbols is not None
 off,count,stroff,strsize=symbols;assert count<=10000 and off<=len(b)-count*16 and stroff<=len(b)-strsize
 assert off>=32+nb and stroff>=32+nb and(off+count*16<=stroff or stroff+strsize<=off)
 for _,_,_,length,p,stype in sections:
  if stype not in(1,0x12)and length:
   assert p+length<=off or off+count*16<=p
   assert p+length<=stroff or stroff+strsize<=p
 found={};undefined=[]
 for i in range(count):
  idx,typ,section,desc,value=struct.unpack_from('<IBBHQ',b,off+i*16)
  assert idx<strsize;end=b.find(b'\0',stroff+idx,stroff+strsize);assert end!=-1
  name=b[stroff+idx:end].decode('ascii')
  if typ&1 and typ&0xe==0:undefined.append(name)
  if name in API:
   assert name not in found and typ==0xf and not desc&0x80 and 1<=section<=len(sections)
   sg,sn,a,length,p,stype=sections[section-1];assert (sg,sn,stype)==(b'__TEXT',b'__text',0)
   assert a<=value and value+4<=a+length
   start=p+value-a;assert start<=len(b)-4 and any(b[start:start+4])
   found[name]=dict(address=value,fileOffset=start,nlist=i)
 assert set(found)==API and not undefined
 return dict(ARM64=True,kind='dylib',platform=2,buildVersion=versions[0],codeSignatures=0,encryptionCommands=len(encryption),encrypted=False,encryptionInfo=encryption,
  strongFileBackedAPIs=found,undefinedImports=undefined,dependencies=deps,engineOrPhoneAdmission=False)
