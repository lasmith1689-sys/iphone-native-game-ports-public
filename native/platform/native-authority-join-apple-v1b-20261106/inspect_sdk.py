SOURCES=('cookie_tag_policy.h', 'cookie_worker_tag.c', 'cookie_worker_tag.h', 'cpu64.h', 'fixture_authority_join.c', 'host64.c', 'host64.h', 'inc32_flags.c', 'inc32_flags.h', 'inc32_publish.c', 'inc32_publish.h', 'independent_tag_control.c', 'native_atomic_required.c', 'native_atomic_required.h', 'native_dispatch.c', 'native_dispatch.h', 'native_required.c', 'native_required.h', 'ngp_fls_environment_pc64_v1.c', 'ngp_fls_environment_v1.h', 'ngp_fls_protocol_v1.c', 'ngp_fls_protocol_v1.h', 'ngp_fls_v1.c', 'ngp_fls_v1.h', 'ngp_worker_tag_bridge_pc64_v1.c', 'ngp_worker_tag_policy_v1.h', 'ngp_worker_tag_v1.h', 'schedule_join.json', 'synthetic_profiles.c', 'synthetic_profiles.h', 'synthetic_vectors.h', 'worker_tag_frame_contract.h', 'inspect_sdk.py', 'run_apple.py', 'README.md')
PRODUCTION=('host64.c', 'native_required.c', 'native_dispatch.c', 'cookie_worker_tag.c', 'native_atomic_required.c', 'inc32_publish.c', 'ngp_fls_protocol_v1.c', 'ngp_fls_v1.c', 'ngp_worker_tag_bridge_pc64_v1.c', 'ngp_fls_environment_pc64_v1.c')
COHORT=('host64.c', 'native_required.c', 'native_dispatch.c', 'cookie_worker_tag.c', 'native_atomic_required.c', 'inc32_publish.c', 'ngp_fls_protocol_v1.c', 'ngp_fls_v1.c', 'ngp_worker_tag_bridge_pc64_v1.c', 'ngp_fls_environment_pc64_v1.c', 'synthetic_profiles.c', 'fixture_authority_join.c')
REQUIRED={'host64.c': {'_pc64_host_size': 15, '_pc64_host_init': 15, '_pc64_host_add_region': 15, '_pc64_host_set_gs': 15, '_pc64_host_set_cpuid': 15, '_pc64_host_cpuid': 15, '_pc64_host_copy_from': 15, '_pc64_host_copy_to': 15, '_pc64_host_read': 15, '_pc64_host_write': 15, '_pc64_host_gs_read': 15, '_pc64_host_gs_write': 15, '_pc64_host_atomic': 15, '_pc64_host_set_image': 15, '_pc64_host_image_address': 15, '_pc64_host_image_canonical': 15, '_pc64_host_image_delta': 15}, 'native_required.c': {'_pc64_bind_host': 15, '_pc64_active_host': 15, '_pc64_fail': 31, '_pc64_required_read': 31, '_pc64_required_write': 31, '_pc64_required_copy_from': 31, '_pc64_required_copy_to': 31, '_pc64_required_gs_read': 31, '_pc64_required_gs_write': 31, '_pc64_required_atomic': 31, '_pc64_required_cpuid': 31, '_pc64_required_image_address': 31}, 'native_dispatch.c': {'_pc64_dispatch_size': 15, '_pc64_dispatch_init': 15, '_pc64_dispatch_bind': 15, '_pc64_dispatch_unbind_after_stop': 15, '_pc64_dispatch_entries_remaining': 15, '_pc64_dispatch_depth': 15, '_pc64_canonical_indirect': 15, '_es3_cs_depth_get': 31, '_es3_cs_depth_set': 31, '_dispatch': 31, '_dispatch_jmp': 31}, 'cookie_worker_tag.c': {'_cookie_worker_tag_warm_v1': 15, '_cookie_worker_tag_read_v1': 15}, 'native_atomic_required.c': {'_pc64_inc_binding_size_v1': 15, '_pc64_inc_binding_alignment_v1': 15, '_pc64_inc_binding_init_v1': 15, '_pc64_inc_bind_v1': 15, '_pc64_inc_unbind_after_stop_v1': 15, '_pc64_required_inc32_v1': 15}, 'inc32_publish.c': {'_pc64_inc32_publish_v1': 31, '_pc64_inc32_flags_v1': 31}, 'ngp_fls_protocol_v1.c': {'_ngp_fls_key_next_v1': 15, '_ngp_fls_step_v1': 15}, 'ngp_fls_v1.c': {'_ngp_fls_prepare_process_v1': 15, '_ngp_fls_size_v1': 15, '_ngp_fls_alignment_v1': 15, '_ngp_fls_init_v1': 15, '_ngp_fls_allocate_v1': 15, '_ngp_fls_get_v1': 15, '_ngp_fls_set_v1': 15, '_ngp_fls_retire_v1': 15, '_ngp_fls_observe_v1': 15}, 'ngp_worker_tag_bridge_pc64_v1.c': {'_ngp_worker_tag_warm_v1': 15, '_ngp_worker_tag_read_v1': 15}, 'ngp_fls_environment_pc64_v1.c': {'_ngp_fls_environment_host_bytes_v1': 31, '_ngp_fls_environment_active_v1': 31, '_ngp_fls_environment_depth_v1': 31}, 'synthetic_profiles.c': {'_synthetic_inc_sites_v1': 31, '_synthetic_inc_0': 31, '_synthetic_inc_1': 31, '_synthetic_inc_2': 31, '_synthetic_inc_3': 31}, 'fixture_authority_join.c': {'_main': 31}}

import struct,hashlib
SYSTEM={'_abort','_calloc','_exit','_memcmp','_memcpy','_memset','_strcmp','_printf','_fprintf','___stderrp','_setjmp','_longjmp','__tlv_bootstrap','___memcpy_chk','___memset_chk','_pthread_main_np','_pthread_self','_pthread_equal','_pthread_create','_pthread_join','_pthread_setcancelstate','_pthread_mutex_lock','_pthread_mutex_unlock','_pthread_cond_wait','_pthread_cond_broadcast','_pthread_key_create','_pthread_key_delete','_pthread_getspecific','_pthread_setspecific'}
def identity(raw):return dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())
def backing(sections,layout):
 assert len(layout['segments'])==1;segment=layout['segments'][0];assert segment['nameHex']==''
 end=layout['commandEnd'];total=layout['objectBytes'];assert 32<=end<=total
 assert 0<=segment['fileOffset']<=total and 0<=segment['fileBytes']<=total-segment['fileOffset']
 reserved=[(0,end),(layout['symbolOffset'],layout['symbolOffset']+layout['symbolBytes']),(layout['stringOffset'],layout['stringOffset']+layout['stringBytes'])]
 reserved.extend((r['offset'],r['offset']+r['bytes'])for r in layout['extraRanges']if r['bytes'])
 for s in sections:
  if s['relocations']:reserved.append((s['relocationOffset'],s['relocationOffset']+s['relocations']*8))
 assert all(0<=a<=b<=total for a,b in reserved);ranges=[];texts=[]
 for s in sections:
  assert segment['vm']<=s['vm']<=segment['vm']+segment['vmBytes']and s['bytes']<=segment['vm']+segment['vmBytes']-s['vm']
  name=bytes.fromhex(s['nameHex']);kind=s['flags']&255
  if name==b'__text':
   assert bytes.fromhex(s['segmentHex'])==b'__TEXT'and kind==0 and s['bytes']>0 and s['flags']&0x80000000
   assert s['offset']>=end and s['offset']%4==0 and s['vm']%4==0;texts.append(s)
  if kind in(1,12,18)or not s['bytes']:continue
  a=s['offset'];b=a+s['bytes'];assert end<=a<b<=total
  assert segment['fileOffset']<=a and b<=segment['fileOffset']+segment['fileBytes']
  assert all(b<=x or y<=a for x,y in reserved),'backed section overlaps object metadata'
  assert all(b<=x or y<=a for x,y in ranges),'backed sections overlap';ranges.append((a,b))
 assert len(texts)==1

def parse(raw,unit,profile='iphone',variant='baseline'):
 assert profile in('iphone','mac')and variant in('baseline','fls-testing','fls-testing-fixture','control');assert profile=='mac'or variant=='baseline'
 required=dict(REQUIRED[unit]);testNames={'_ngp_fls_test_fault_v1':15,'_ngp_fls_test_late_exit_v1':15}
 if variant=='fls-testing':assert unit=='ngp_fls_v1.c';required.update(testNames)
 if variant=='fls-testing-fixture':assert unit=='fixture_authority_join.c'
 assert unit in REQUIRED and isinstance(raw,(bytes,bytearray))and 32<=len(raw)<=4194304;raw=bytes(raw)
 magic,cpu,sub,kind,count,size,flags,reserved=struct.unpack_from('<8I',raw);assert(magic,cpu,sub,kind,reserved)==(0xfeedfacf,0x100000c,0,1,0)and flags in(0,0x2000)and 0<count<=64 and 32+size<=len(raw)
 at=32;sections=[];segments=[];symbols=None;version=None;extra=[]
 for z in range(count):
  cmd,n=struct.unpack_from('<II',raw,at);assert n>=8 and n%8==0 and at+n<=32+size
  if cmd==0x19:
   assert n>=72;ns=struct.unpack_from('<I',raw,at+64)[0];assert ns<=32 and n==72+80*ns;svm,svmn,sfo,sfn=struct.unpack_from('<4Q',raw,at+24);segments.append(dict(nameHex=raw[at+8:at+24].rstrip(b'\0').hex(),vm=svm,vmBytes=svmn,fileOffset=sfo,fileBytes=sfn))
   for i in range(ns):
    p=at+72+80*i;name=raw[p:p+16].split(b'\0')[0];segment=raw[p+16:p+32].split(b'\0')[0];vm,extent,off,align,relo,nrel,sflags=struct.unpack_from('<QQIIIII',raw,p+32);assert align<=20 and nrel<=65536 and relo+8*nrel<=len(raw)
    if sflags&255 not in(1,12,18):assert off+extent<=len(raw)
    sections.append(dict(nameHex=name.hex(),segmentHex=segment.hex(),vm=vm,bytes=extent,offset=off,flags=sflags,relocationOffset=relo,relocations=nrel,sectionIdentity=None if sflags&255 in(1,12,18)else identity(raw[off:off+extent]),rawRelocationsHex=raw[relo:relo+8*nrel].hex()))
  elif cmd==2:assert n==24 and symbols is None;symbols=struct.unpack_from('<4I',raw,at+8)
  elif cmd==0x32:
   assert n>=24 and version is None;version=struct.unpack_from('<3I',raw,at+8);assert version==((2,26<<16,(27<<16)|(1<<8))if profile=='iphone'else(1,14<<16,27<<16)),'exact selected platform/minimum/SDK object required'
  elif cmd==0xb:assert n==80;extra.append(dict(command=cmd,rawHex=raw[at:at+n].hex()))
  elif cmd in(0x29,0x2e):
   assert n==16;off,extent=struct.unpack_from('<II',raw,at+8);assert extent<=1048576 and off+extent<=len(raw);extra.append(dict(command=cmd,payloadOffset=off,payloadBytes=extent,payload=identity(raw[off:off+extent]),payloadHex=raw[off:off+extent].hex()))
  elif cmd==0x1b:assert n==24;extra.append(dict(command=cmd,uuidHex=raw[at+8:at+24].hex()))
  else:raise AssertionError(('unsupported object command',cmd))
  at+=n
 assert at==32+size and version and symbols and sections;off,ns,sto,stn=symbols;assert ns<=20000 and stn<=1048576 and off+ns*16<=len(raw)and sto+stn<=len(raw);layout=dict(commandEnd=32+size,objectBytes=len(raw),segments=segments,symbolOffset=off,symbolBytes=ns*16,stringOffset=sto,stringBytes=stn,extraRanges=[dict(offset=e['payloadOffset'],bytes=e['payloadBytes'])for e in extra if 'payloadOffset'in e]);backing(sections,layout);rows=[];strong={};undefined=set()
 for i in range(ns):
  si,typ,sect,desc,value=struct.unpack_from('<IBBHQ',raw,off+16*i);assert si<stn;end=raw.find(b'\0',sto+si,sto+stn);assert end>=sto+si;name=raw[sto+si:end];row=dict(nameHex=name.hex(),kind=typ,section=sect,desc=desc,value=value);rows.append(row)
  if name in{n.encode()for n in required}:
   interpreted=name.decode('ascii');assert typ==required[interpreted]and not desc&0xc0 and 1<=sect<=len(sections);s=sections[sect-1];assert bytes.fromhex(s['nameHex'])==b'__text'and s['flags']&0x80000000 and s['vm']<=value<s['vm']+s['bytes']and value%4==0;assert interpreted not in strong;strong[interpreted]=dict(value=value,fileOffset=s['offset']+value-s['vm'],kind=typ,section=sect)
  elif typ in(0xf,0x1f):raise AssertionError(('foreign external definition',name.hex()))
  if typ&0xe0:continue
  if typ==1:
   assert sect==0 and value==0 and not desc&0xc0;undefined.add(name.decode('ascii'))
  elif typ&1:assert typ in(0xf,0x1f),'unsupported interpreted external kind'
 assert set(strong)==set(required)and len({r['value']for r in strong.values()})==len(strong);own=set().union(*(set(v)for v in REQUIRED.values()));own|=set(testNames)if variant in('fls-testing','fls-testing-fixture')else set();assert undefined<=own|SYSTEM,('foreign import',sorted(undefined-(own|SYSTEM)))
 return dict(unit=unit,object=identity(raw),header=dict(magic=magic,cpu=cpu,subtype=sub,filetype=kind,commands=count,commandBytes=size,flags=flags,reserved=reserved),platform=version[0],minimumPacked=version[1],SDKPacked=version[2],sections=sections,symbolRows=rows,requiredFunctions=strong,undefined=sorted(undefined),extraCommands=extra,allOpaqueNamesPreservedHex=True,actualSDKHeaderBytesIncluded=False,fileLayout=layout)
def bind_frames(metadata,frames):
 backing(metadata['sections'],metadata['fileLayout']);assert isinstance(frames,dict)and frames;out={}
 for name,n in frames.items():
  assert isinstance(name,str)and type(n)is int and 0<=n<=1048576;rawname=('_'+name).encode();rows=[r for r in metadata['symbolRows']if bytes.fromhex(r['nameHex'])==rawname and r['kind']in(0xe,0xf,0x1f)and not r['desc']&0xc0];assert len(rows)==1,(name,'SU/native symbol identity');r=rows[0];assert 1<=r['section']<=len(metadata['sections']);s=metadata['sections'][r['section']-1];assert bytes.fromhex(s['nameHex'])==b'__text'and s['vm']<=r['value']<s['vm']+s['bytes']and r['value']%4==0;out[name]=dict(ownStaticBytesExcludingCallees=n,nativeValue=r['value'],nativeFileOffset=s['offset']+r['value']-s['vm'],kind=r['kind'],machinePrologueCostProved=False)
 assert all(n[1:]in frames for n in metadata['requiredFunctions']);return out
def archive(raw,expected,objects):
 assert raw[:8]==b'!<arch>\n'and len(raw)<=16777216;at=8;members=[]
 while at<len(raw):
  assert at+60<=len(raw);h=raw[at:at+60];assert h[58:]==b'`\n';size=int(h[48:58].strip());assert 0<=size<=4194304 and at+60+size<=len(raw)and int(h[16:28].strip())==0;name=h[:16].strip();start=at+60
  if name.startswith(b'#1/'):
   n=int(name[3:]);assert 0<n<=256 and n<=size;name=raw[start:start+n].rstrip(b'\0');start+=n
  else:name=name.rstrip(b'/')
  name=name.decode('ascii');assert'/'not in name and'\\'not in name;members.append(dict(name=name,header=at,payload=start,end=at+60+size));at+=60+size
  if at&1:assert raw[at:at+1]==b'\n';at+=1
 assert at==len(raw)and len({m['name']for m in members})==len(members);indexes=[m for m in members if m['name']in('__.SYMDEF','__.SYMDEF SORTED')];assert len(indexes)==1;ordinary=[m for m in members if m not in indexes];assert {m['name']for m in ordinary}==set(expected)
 for m in ordinary:assert raw[m['payload']:m['end']]==expected[m['name']]
 index=indexes[0];b=raw[index['payload']:index['end']];assert len(b)>=8;extent=struct.unpack_from('<I',b)[0];assert extent%8==0 and 4+extent+4<=len(b);stn=struct.unpack_from('<I',b,4+extent)[0];sto=8+extent;assert sto+stn<=len(b)and len(b)-(sto+stn)<=7 and not any(b[sto+stn:]);byHeader={m['header']:m for m in ordinary};symbols={}
 for i in range(extent//8):
  si,member=struct.unpack_from('<II',b,4+8*i);assert si<stn and member in byHeader;end=b.find(b'\0',sto+si,sto+stn);assert end>=sto+si;name=b[sto+si:end].decode('ascii');assert name not in symbols;symbols[name]=byHeader[member]['name']
 wanted={n:filename for filename,metadata in objects.items()for n in metadata['requiredFunctions']};assert symbols==wanted
 return dict(archive=identity(raw),members=[dict(name=m['name'],header=m['header'],identity=identity(raw[m['payload']:m['end']]))for m in ordinary],indexName=index['name'],allIndexSymbolsBoundToExactOwnObjects=True,symbols=symbols)

# Bounded linked-own-image metadata/code-name reader. This does not execute
# code or independently replay all linked instruction relocations.
def exports(raw,base=0):
 assert isinstance(raw,bytes)and 0<len(raw)<=1048576
 out={};visited=set()
 def uleb(at):
  value=0
  for shift in range(0,70,7):
   assert at<len(raw);b=raw[at];at+=1;value|=(b&127)<<shift;assert value<=0xffffffffffffffff
   if b<128:return value,at
  raise AssertionError('unbounded ULEB')
 def walk(at,prefix,ancestors):
  assert at not in ancestors and at not in visited and at<len(raw)and len(prefix)<=512 and len(visited)<32768
  visited.add(at);size,p=uleb(at);end=p+size;assert end<=len(raw)
  if size:
   flags,q=uleb(p);assert flags==0;address,q=uleb(q);assert q==end and prefix and prefix not in out
   out[prefix]=base+address
  assert end<len(raw);count=raw[end];p=end+1
  for _ in range(count):
   q=raw.index(0,p);edge=raw[p:q];assert edge and len(edge)<=512 and all(32<=b<127 for b in edge)
   target,p=uleb(q+1);walk(target,prefix+edge,ancestors|{at})
 walk(0,b'',set());return {k.decode('ascii'):v for k,v in out.items()}

def image(raw,profile,required,kind):
 assert profile in('mac','iphone')and kind in(2,6)and isinstance(raw,bytes)and 32<=len(raw)<=67108864
 magic,cpu,sub,filetype,count,size,flags,reserved=struct.unpack_from('<8I',raw)
 assert(magic,cpu,sub,filetype,reserved)==(0xfeedfacf,0x100000c,0,kind,0)and 1<=count<=256 and size<=1048576 and 32+size<=len(raw)
 at=32;segments=[];sections=[];symbols=None;version=None;deps=[];export=None;uuid=None;codesign=None;entry=None;installName=None;linkedit=[]
 for _ in range(count):
  assert at+8<=32+size;cmd,n=struct.unpack_from('<II',raw,at);assert n>=8 and n%8==0 and at+n<=32+size
  if cmd==0x19:
   assert n>=72;name=raw[at+8:at+24].rstrip(b'\0');vm,vn,off,extent=struct.unpack_from('<4Q',raw,at+24);ns=struct.unpack_from('<I',raw,at+64)[0]
   assert ns<=128 and n==72+80*ns and off+extent<=len(raw)and vm+vn<=0xffffffffffffffff
   seg=dict(nameHex=name.hex(),vm=vm,bytes=vn,offset=off,fileBytes=extent);segments.append(seg)
   for i in range(ns):
    q=at+72+80*i;sn=raw[q:q+16].rstrip(b'\0');sg=raw[q+16:q+32].rstrip(b'\0');va,length,fo,align,relo,nrel,sflags=struct.unpack_from('<QQIIIII',raw,q+32)
    assert sg==name and align<=31 and vm<=va and va+length<=vm+vn
    zero=(sflags&255)in(1,12,18)
    if not zero and length:
     assert fo>=32+size and off<=fo and fo+length<=off+extent and va-vm==fo-off
    sections.append(dict(nameHex=sn.hex(),segmentHex=sg.hex(),vm=va,bytes=length,offset=fo,flags=sflags,zeroFill=zero))
  elif cmd==2:
   assert n==24 and symbols is None;symbols=struct.unpack_from('<4I',raw,at+8)
  elif cmd==0x32:
   assert n>=24 and version is None;version=struct.unpack_from('<3I',raw,at+8)
  elif cmd in(12,0x80000018,0x8000001f):
   assert n>=24 and cmd==12;no=struct.unpack_from('<I',raw,at+8)[0];assert 24<=no<n;q=raw.index(0,at+no,at+n);name=raw[at+no:q].decode('ascii');assert name=='/usr/lib/libSystem.B.dylib';deps.append(name)
  elif cmd==13:
   assert kind==6 and n>=24 and installName is None;no=struct.unpack_from('<I',raw,at+8)[0];assert 24<=no<n;q=raw.index(0,at+no,at+n);installName=raw[at+no:q].decode('ascii')
  elif cmd==0x8000001c:raise AssertionError('unexpected runtime search path')
  elif cmd==0x80000033:
   assert n==16 and export is None;export=struct.unpack_from('<2I',raw,at+8)
  elif cmd==0x80000022:
   assert n==48;off,length=struct.unpack_from('<2I',raw,at+40)
   if length:assert export is None;export=(off,length)
  elif cmd==0x1b:
   assert n==24 and uuid is None;uuid=raw[at+8:at+24].hex()
  elif cmd==0x1d:
   assert n==16 and codesign is None;codesign=struct.unpack_from('<2I',raw,at+8)
  elif cmd==0x80000028:
   assert n==24 and entry is None;entry=struct.unpack_from('<2Q',raw,at+8)
  if cmd in(0x1d,0x26,0x29,0x2b,0x2e,0x80000033,0x80000034):
   assert n==16;fo,fn=struct.unpack_from('<2I',raw,at+8);assert fo+fn<=len(raw)
   if fn:linkedit.append((fo,fo+fn))
  elif cmd==0x80000022:
   for q in range(8,48,8):
    fo,fn=struct.unpack_from('<2I',raw,at+q);assert fo+fn<=len(raw)
    if fn:linkedit.append((fo,fo+fn))
  at+=n
 assert at==32+size and symbols and sections and uuid and len(deps)==1
 assert version==((2,26<<16,(27<<16)|(1<<8))if profile=='iphone'else(1,14<<16,27<<16))
 if profile=='iphone':assert kind==6 and codesign is None and installName=='@rpath/libnative_authority_join_own.dylib'
 if kind==2:assert entry is not None
 so,ns,sto,stn=symbols;assert ns<=100000 and stn<=4194304 and so>=32+size and sto>=32+size and so+16*ns<=len(raw)and sto+stn<=len(raw)
 assert so+16*ns<=sto or sto+stn<=so
 protected=[(0,32+size),(so,so+16*ns),(sto,sto+stn),*linkedit];ranges=[]
 for s in sections:
  if s['zeroFill']or not s['bytes']:continue
  a,b=s['offset'],s['offset']+s['bytes'];assert all(b<=x or y<=a for x,y in protected+ranges);ranges.append((a,b))
 rows=[];strong={};undefined=set()
 for i in range(ns):
  off,typ,sec,desc,value=struct.unpack_from('<IBBHQ',raw,so+16*i);assert off<stn;end=raw.index(0,sto+off,sto+stn);name=raw[sto+off:end]
  rows.append(dict(nameHex=name.hex(),kind=typ,section=sec,desc=desc,value=value))
  if name in {n.encode()for n in required}:
   key=name.decode('ascii');assert required[key]in(15,31);assert typ in((15,)if required[key]==15 else(14,30))and not desc&0xc0 and key not in strong and 1<=sec<=len(sections)
   s=sections[sec-1];assert bytes.fromhex(s['nameHex'])==b'__text'and not s['zeroFill']and(s['flags']&255)==0 and s['flags']&0x80000000 and s['vm']<=value<s['vm']+s['bytes']and value%4==0
   strong[key]=dict(value=value,fileOffset=s['offset']+value-s['vm'],kind=typ,section=sec)
  elif typ in(1,0x11):
   assert typ==1 and sec==0 and value==0 and not desc&0xc0
   if name:undefined.add(name.decode('ascii'))
 assert set(strong)==set(required)and len({r['value']for r in strong.values()})==len(strong)
 assert undefined<=SYSTEM,('unresolved owned or foreign provider',sorted(undefined-SYSTEM))
 bases=[r['vm']for r in segments if r['offset']==0 and r['fileBytes']];assert len(bases)==1;base=bases[0]
 assert export is not None;eo,en=export;assert 0<en<=1048576 and eo+en<=len(raw);ex=exports(raw[eo:eo+en],base)
 assert all(n not in ex for n in strong if required[n]==31)
 if profile=='iphone':assert ex=={n:r['value']for n,r in strong.items()if required[n]==15}
 return dict(image=identity(raw),profile=profile,header=dict(cpu=cpu,subtype=sub,filetype=kind,flags=flags),platform=version[0],minimumPacked=version[1],SDKPacked=version[2],UUID=uuid,dependencies=deps,codeSignaturePresent=codesign is not None,installName=installName,linkeditPayloadRanges=linkedit,segments=segments,sections=sections,requiredFunctions=strong,undefined=sorted(undefined),exports=ex,symbolRows=rows,allOpaqueLocalNamesRetainedHex=True,allOwnCodeRelocationsIndependentlyReplayed=False)

def linkmap(raw,metadata,ownership):
 assert isinstance(raw,bytes)and len(raw)<=4194304;text=raw.decode('utf-8');objects={};symbols={}
 for line in text.splitlines():
  m=__import__('re').fullmatch(r'\[\s*(\d+)\]\s+(.+)',line)
  if m:
   i=int(m[1]);assert i not in objects;objects[i]=m[2]
  m=__import__('re').fullmatch(r'(0x[0-9A-Fa-f]+)\s+(0x[0-9A-Fa-f]+)\s+\[\s*(\d+)\]\s+(.+)',line)
  if m and m[4]in metadata['requiredFunctions']:
   name=m[4];assert name not in symbols;address,length,owner=int(m[1],16),int(m[2],16),int(m[3]);assert address==metadata['requiredFunctions'][name]['value']and length>0 and owner in objects
   expected=ownership[name];assert PathName(objects[owner])==expected
   s=metadata['sections'][metadata['requiredFunctions'][name]['section']-1];assert address+length<=s['vm']+s['bytes'];symbols[name]=dict(value=address,bytes=length,object=expected)
 assert set(symbols)==set(metadata['requiredFunctions']);return dict(requiredSymbolOwners=symbols,allRequiredWholeObjectsPresent=True,instructionRelocationsIndependentlyReplayed=False)

def PathName(path):return path.replace('\\','/').rsplit('/',1)[-1]
