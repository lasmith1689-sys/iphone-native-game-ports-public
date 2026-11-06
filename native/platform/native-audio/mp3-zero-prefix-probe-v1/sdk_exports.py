"""OWN bounded regular export trie, no UUID requirement or native loading."""
import struct,hashlib
from inspect_image import inspect
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

PUBLIC=('_ngp_mp3_envelope_v1_scan','_ngp_memory_mp3_v1_decode','_ngp_memory_mp3_v1_view','_ngp_memory_mp3_v1_release','_ngp_mp3_zero_prefix_v1_scan','_ngp_mp3_zero_prefix_v1_decode')
PRIVATE=('_ngp_mp3_frame_v1_header','_ngp_mp3_zero_prefix_v1_choose','_ngp_mp3_zero_prefix_v1_bounds')
def verify_exports(raw):
 """After strict iOS shape audit, bind regular code/nlist/exact strong trie.

 UUID is optional metadata, not an export-trie precondition. This observes saved
 own linked bytes; it does not load them, prove arbitrary code semantics or close
 OS providers. Generic nonregular/weak/reexport/shared-node tries are refused.
 """
 metadata=inspect(raw,True);at=32;end=32+struct.unpack_from('<I',raw,20)[0]
 locations=[];segments=[];symbol=None;dynamic=None;protected=[(0,end)]
 count=struct.unpack_from('<I',raw,16)[0]
 for _ in range(count):
  command,size=struct.unpack_from('<II',raw,at);assert size>=8 and at+size<=end
  if command==0x19:
   vm,vn,off,extent,maxprot,initprot=struct.unpack_from('<4Q2I',raw,at+24)
   assert extent<=vn and vm+vn<=1<<64 and off<=len(raw)and extent<=len(raw)-off
   name=raw[at+8:at+24].rstrip(b'\0').decode('ascii');expected={'__TEXT':5,'__DATA_CONST':3,'__DATA':3,'__LINKEDIT':1};assert name in expected and maxprot==initprot==expected[name]
   segments.append(dict(name=name,vm=vm,bytes=vn,offset=off,fileBytes=extent,initialProtection=initprot))
  elif command==2:
   assert symbol is None;symbol=struct.unpack_from('<4I',raw,at+8)
  elif command==0xb:
   assert size==80 and dynamic is None;dynamic=struct.unpack_from('<20I',raw,at)
   assert all(dynamic[i]==0 for i in(8,9,10,11,12,13,16,17,18,19)),'unselected dynamic table family'
   off,extent=dynamic[14],4*dynamic[15];assert dynamic[15]<=32768 and off<=len(raw)and extent<=len(raw)-off
   if extent:assert off>=end;protected.append((off,off+extent))
  elif command in(0x22,0x80000022):
   for i in range(5):
    off,extent=struct.unpack_from('<2I',raw,at+8+8*i)
    if extent:assert off>=end and off+extent<=len(raw);protected.append((off,off+extent))
   off,extent=struct.unpack_from('<2I',raw,at+40)
   if extent:locations.append((off,extent))
  elif command in(0x80000033,0x26,0x29):
   off,extent=struct.unpack_from('<2I',raw,at+8)
   if extent:assert off>=end and off+extent<=len(raw);protected.append((off,off+extent))
   if command==0x80000033 and extent:locations.append((off,extent))
  at+=size
 assert at==end and len(locations)==1 and symbol is not None and dynamic is not None
 so,ns,sto,stn=symbol;assert ns<=32768 and so>=end and sto>=end and so+16*ns<=len(raw)and sto+stn<=len(raw)
 assert so+16*ns<=sto or sto+stn<=so;protected.extend([(so,so+16*ns),(sto,sto+stn)])
 assert dynamic[2]==0 and dynamic[4]==dynamic[3]and dynamic[6]==dynamic[4]+dynamic[5]and dynamic[6]+dynamic[7]==ns
 assert [s['name']for s in segments]==['__TEXT','__DATA_CONST','__DATA','__LINKEDIT']
 bases=[s['vm']for s in segments if s['offset']==0 and s['fileBytes']>=end];assert len(bases)==1
 off,extent=locations[0];assert 0<extent<=1048576;declared=exports(raw[off:off+extent],bases[0])
 ranges=[]
 for s in metadata['sections']:
  if s['flags']&255 in(1,12,18)or not s['bytes']:continue
  a,b=s['fileOffset'],s['fileOffset']+s['bytes'];assert a>=end and b<=len(raw)
  owners=[g for g in segments if g['offset']<=a and b<=g['offset']+g['fileBytes']and g['vm']<=s['address']and s['address']+s['bytes']<=g['vm']+g['bytes']]
  assert len(owners)==1 and s['address']-owners[0]['vm']==a-owners[0]['offset']
  assert all(b<=x or y<=a for x,y in protected+ranges);ranges.append((a,b))
 selected={};strings=raw[sto:sto+stn]
 for i in range(ns):
  ix,typ,section,desc,value=struct.unpack_from('<IBBHQ',raw,so+16*i);assert ix<stn
  stop=strings.find(b'\0',ix);assert stop>=ix;name=strings[ix:stop]
  if name not in{n.encode()for n in PUBLIC+PRIVATE}:continue
  name=name.decode('ascii');assert name not in selected and not desc&0xc0
  assert typ in((15,)if name in PUBLIC else(14,30))and 1<=section<=len(metadata['sections'])
  s=metadata['sections'][section-1];assert s['name']=='__text'and s['segment']=='__TEXT'and s['flags']&255==0 and s['flags']&0x80000000 and value%4==0 and s['address']<=value<s['address']+s['bytes']
  fileoffset=s['fileOffset']+value-s['address'];assert fileoffset>=end and fileoffset+4<=len(raw)
  owner=[g for g in segments if g['vm']<=value<g['vm']+g['fileBytes']];assert len(owner)==1 and owner[0]['initialProtection']&4
  if name in PUBLIC:assert declared.get(name)==value
  else:assert name not in declared
  selected[name]=dict(value=value,fileOffset=fileoffset,kind=typ,section=section,strongRegularCode=True,public=name in PUBLIC)
 assert set(selected)==set(PUBLIC+PRIVATE)and len({r['value']for r in selected.values()})==len(selected)
 assert '_ngp_mp3_padding_abi_v1'not in declared and '_ngp_mp3_padding_abi_probe_v1'not in declared
 return dict(imageBytes=len(raw),imageSha256=hashlib.sha256(raw).hexdigest(),exportsOffset=off,exportsBytes=extent,exportsSha256=hashlib.sha256(raw[off:off+extent]).hexdigest(),regularExportCount=len(declared),required=selected,UUIDPresent=any(c['command']==0x1b for c in metadata['commands']),UUIDRequired=False,weakReexportNonregularOrSharedNodeAccepted=False,nativeExecuted=False,providerOrWholeStackOrGameAdmission=False)

