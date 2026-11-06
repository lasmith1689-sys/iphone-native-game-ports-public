"""Bounded raw Mach-O facts, not complete import/ABI/provider admission."""
import hashlib,struct
def inspect(raw,final=False):
 def need(v):
  if not v:raise ValueError('bounded raw image shape')
 need(type(raw)is bytes and 32<=len(raw)<=8*1024**2)
 def span(a,n):need(type(a)is int and type(n)is int and a>=0 and n>=0 and a<=len(raw)-n);return raw[a:a+n]
 def u(fmt,at):return struct.unpack(fmt,span(at,struct.calcsize(fmt)))
 magic,cpu,sub,kind,ncmd,cmdbytes,flags,reserved=u('<8I',0)
 need(magic==0xfeedfacf and cpu==0x100000c and sub==0 and kind==(6 if final else 1)and reserved==0 and 1<=ncmd<=128 and ncmd*8<=cmdbytes<=65536);span(32,cmdbytes)
 commands=[];sections=[];libs=[];symtab=None;builds=[];pos=32;classic=0;encryption=False
 def string(data):
  end=data.find(b'\0');need(end>=0 and end<=1024);return data[:end].decode('utf8','backslashreplace')
 def fixed_name(data):
  need(len(data)==16);return data.split(b'\0',1)[0].decode('utf8','backslashreplace')
 for _ in range(ncmd):
  cmd,size=u('<II',pos);allowed={0x19,2,0xb,0x32,0x2e}if not final else{0x19,2,0xb,0xd,0xc,0x1b,0x32,0x2a,0x2c,0x22,0x80000022,0x80000033,0x26,0x29,0x80000018,0x8000001f,0x20,0x80000023};need(cmd in allowed);need(size>=8 and size%8==0 and pos+size<=32+cmdbytes);chunk=span(pos,size);commands.append(dict(command=cmd,bytes=size,sha256=hashlib.sha256(chunk).hexdigest()))
  if cmd==0x32:
   need(size>=24);plat,minimum,sdk,tools=u('<4I',pos+8);need(tools<=8 and size==24+8*tools);builds.append(dict(platform=plat,minimum=minimum,sdk=sdk))
  elif cmd==2:
   need(size==24 and symtab is None);symtab=u('<4I',pos+8)
  elif cmd in(0xc,0x80000018,0x8000001f,0x20,0x80000023,0xd):
   need(size>=24);offset=u('<I',pos+8)[0];need(24<=offset<size);name=string(chunk[offset:]);
   if cmd!=0xd:libs.append(name)
  elif cmd==0x19:
   need(size>=72);seg=fixed_name(span(pos+8,16));vm,vmbytes,off,filebytes,maxprot,initprot,count,segflags=u('<4Q4I',pos+24);need(count<=64 and size==72+80*count);span(off,filebytes)
   for i in range(count):
    at=pos+72+80*i;sect=fixed_name(span(at,16));segment=fixed_name(span(at+16,16));addr,length,offset,align,reloc,countrel,stflags,r1,r2,r3=u('<QQ8I',at+32);need(align<=31 and countrel<=8192);span(reloc,countrel*8)
    if(stflags&255)not in(1,0xc,0x12):span(offset,length)
    sections.append(dict(segment=segment,name=sect,address=addr,bytes=length,fileOffset=offset,flags=stflags,relocationCount=countrel))
  elif cmd in(0x22,0x80000022):
   need(final and size==48);classic+=1;need(classic==1);values=u('<10I',pos+8)
   for j in range(5):span(values[2*j],values[2*j+1])
  elif cmd==0x2c:
   need(final and size==24 and not encryption);off,n,cryptid,pad=u('<4I',pos+8);need(cryptid==0 and pad==0);span(off,n);encryption=True
  pos+=size
 need(pos==32+cmdbytes and len(builds)==1 and builds[0]==dict(platform=2,minimum=26<<16,sdk=(27<<16)|(1<<8))and symtab is not None)
 if final:need(classic==1 and flags&1 and all(x['command']not in(0x1d,0x80000034)for x in commands))
 symoff,count,stroff,strbytes=symtab;need(count<=32768 and strbytes<=2*1024**2);span(symoff,count*16);strings=span(stroff,strbytes);imports=[];defined=[]
 for i in range(count):
  ix,typ,section,desc,value=u('<IBBHQ',symoff+16*i);need(ix<strbytes and section<=len(sections));name=string(strings[ix:]);row=dict(index=i,name=name,type=typ,section=section,description=desc,value=value)
  if not typ&0xe0:
   if typ&0xe==0:
    ordinal=desc>>8;row.update(ordinal=ordinal,provider=libs[ordinal-1]if 1<=ordinal<=len(libs)else None);imports.append(row)
   elif typ&1:defined.append(row)
 return dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest(),fileType=kind,flags=flags,build=builds[0],commands=commands,sections=sections,dependencies=libs,undefinedSymbols=imports,externalDefinedSymbols=defined,structuralFactsOnly=True,providerOrRuntimeOrStackAdmission=False)
