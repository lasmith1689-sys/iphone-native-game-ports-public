"""Bounded own C MH_OBJECT inspector; no SDK/target execution or name coercion."""
import struct
REQUIRED={
 'ngp_fls_v1.c':{'_'+n for n in('ngp_fls_prepare_process_v1','ngp_fls_size_v1','ngp_fls_alignment_v1','ngp_fls_init_v1','ngp_fls_allocate_v1','ngp_fls_get_v1','ngp_fls_set_v1','ngp_fls_retire_v1','ngp_fls_observe_v1')},
 'ngp_fls_protocol_v1.c':{'_ngp_fls_key_next_v1','_ngp_fls_step_v1'},
 'ngp_worker_tag_v1.c':{'_ngp_worker_tag_warm_v1','_ngp_worker_tag_read_v1'}}
ENV={'_ngp_fls_environment_host_bytes_v1','_ngp_fls_environment_active_v1','_ngp_fls_environment_depth_v1'}
SYSTEM={'_pthread_main_np','_pthread_self','_pthread_equal','_pthread_setcancelstate','_pthread_key_create','_pthread_key_delete','_pthread_getspecific','_pthread_setspecific','_memset','_memcpy','__tlv_bootstrap'}
def parse(raw,unit):
 assert unit in REQUIRED and isinstance(raw,(bytes,bytearray))and 32<=len(raw)<=4194304
 raw=bytes(raw)
 magic,cpu,sub,kind,ncmd,cmdbytes,flags,res=struct.unpack_from('<8I',raw)
 assert(magic,cpu,sub,kind)==(0xfeedfacf,0x100000c,0,1)and 0<ncmd<=64 and cmdbytes<=len(raw)-32
 at=32;sections=[];symtab=None;version=None
 for _ in range(ncmd):
  cmd,size=struct.unpack_from('<II',raw,at);assert size>=8 and size%8==0 and at+size<=32+cmdbytes
  assert cmd!=0x1d,'object must not contain a code signature'
  if cmd==0x19:
   assert size>=72;count=struct.unpack_from('<I',raw,at+64)[0];assert count<=32 and size==72+80*count
   for i in range(count):
    p=at+72+80*i;name=raw[p:p+16].split(b'\0')[0];segment=raw[p+16:p+32].split(b'\0')[0];vm,n,off,align,relo,nrel,sflags=struct.unpack_from('<QQIIIII',raw,p+32)
    assert align<=16 and nrel<=65536 and relo+8*nrel<=len(raw)
    if sflags&255 not in(1,12,18):assert off+n<=len(raw)
    sections.append(dict(nameHex=name.hex(),segmentHex=segment.hex(),vm=vm,bytes=n,offset=off,flags=sflags))
  elif cmd==2:
   assert symtab is None and size==24;symtab=struct.unpack_from('<4I',raw,at+8)
  elif cmd==0x32:
   assert version is None and size>=24;version=struct.unpack_from('<3I',raw,at+8);assert version[0]==2 and version[1]==26<<16 and version[2]>=27<<16
  at+=size
 assert at==32+cmdbytes and version and symtab and sections
 off,count,stroff,strsize=symtab;assert count<=20000 and strsize<=1048576 and off+count*16<=len(raw)and stroff+strsize<=len(raw)
 rows=[];public={};undefined=set();helper=None
 for i in range(count):
  si,typ,section,desc,value=struct.unpack_from('<IBBHQ',raw,off+16*i);assert si<strsize;end=raw.find(b'\0',stroff+si,stroff+strsize);assert end>=0;name=raw[stroff+si:end];row=dict(nameHex=name.hex(),kind=typ,section=section,desc=desc,value=value);rows.append(row)
  if typ&0xe0:continue
  if typ==1 and section==0:
   assert value==0 and not(desc&0xc0);undefined.add(name.decode('ascii'));continue
  if name in{n.encode()for n in REQUIRED[unit]}:
   assert typ in(0x0f,0x1f)and typ==0x0f and not(desc&0xc0)and 1<=section<=len(sections);s=sections[section-1];assert bytes.fromhex(s['nameHex'])==b'__text'and s['flags']&0x80000000 and s['vm']<=value<s['vm']+s['bytes']and value%4==0
   n=name.decode('ascii');assert n not in public;public[n]=dict(value=value,fileOffset=s['offset']+value-s['vm'],kind=typ)
  elif typ in(0x0f,0x1f):raise AssertionError(('unexpected external definition',name.hex()))
  if name==b'_owner_exit':
   assert typ==0xe and 1<=section<=len(sections);s=sections[section-1];assert bytes.fromhex(s['nameHex'])==b'__text';lo=s['offset']+value-s['vm'];assert lo%4==0 and lo<len(raw);helper=(lo,min(s['offset']+s['bytes'],lo+128))
 assert set(public)==REQUIRED[unit]and len({x['value']for x in public.values()})==len(public)
 allOwn=set().union(*REQUIRED.values());assert undefined<=allOwn|ENV|SYSTEM,('foreign import',sorted(undefined-(allOwn|ENV|SYSTEM)))
 return dict(unit=unit,platform=version[0],minimumPacked=version[1],SDKPacked=version[2],sections=sections,symbolRows=rows,requiredFunctions=public,undefined=sorted(undefined),ownerExitSpan=helper)
def destructor(raw,span,owner_exit_offset):
 assert span and 0<=owner_exit_offset<=4095 and owner_exit_offset%4==0
 lo,hi=span;state={0:('box',0)};out=[];store=False;branch=None
 for at in range(lo,hi,4):
  w=struct.unpack_from('<I',raw,at)[0];out.append(dict(offset=at-lo,word=w))
  if w==0xd65f03c0:
   assert store and branch==at;return dict(instructions=out,atomicOnly=True,noCallOrAllocation=True,ownerExitOffset=owner_exit_offset)
  if(w&0xff00001f)==0xb4000000:
   assert at==lo;imm=(w>>5)&0x7ffff;imm=imm-(1<<19)if imm&(1<<18)else imm;branch=at+imm*4;assert at<branch<hi
  elif(w&0xffc00000)==0xf9400000:
   rn=(w>>5)&31;rt=w&31;n=((w>>10)&4095)*8;assert state.get(rn)==('box',0)and n==0;state[rt]=('service',0)
  elif(w&0xffc00000)==0x91000000:
   rn=(w>>5)&31;rt=w&31;n=(w>>10)&4095;assert state.get(rn)==('service',0)and n==owner_exit_offset;state[rt]=('service',n)
  elif(w&0xff800000)==0x52800000:
   assert((w>>5)&65535)==1 and not((w>>21)&3);state[w&31]=('one',1)
  elif(w&0xfffffc00)==0x889ffc00:
   assert not store and state.get((w>>5)&31)==('service',owner_exit_offset)and state.get(w&31)==('one',1);store=True
  else:raise AssertionError(('unrecognized destructor word',w))
 raise AssertionError('missing bounded destructor RET')
