"""Read five OWN compile-time layout constants from actual fixture MH_OBJECT."""
import struct
FIELDS={
 'reportBytes':'_ngp_voice_gain_report_bytes_fixture_v2',
 'innerOffset':'_ngp_voice_gain_inner_offset_fixture_v2',
 'innerBytes':'_ngp_voice_gain_inner_bytes_fixture_v2',
 'balanceOffset':'_ngp_voice_gain_balance_offset_fixture_v2',
 'validOffset':'_ngp_voice_gain_valid_offset_fixture_v2'}
def extract(raw,facts):
 assert type(raw)is bytes and len(raw)<=8*1024**2 and facts['fileType']==1
 result={}
 for key,name in FIELDS.items():
  symbols=[x for x in facts['externalDefinedSymbols']if x['name']==name];assert len(symbols)==1;s=symbols[0];assert 1<=s['section']<=len(facts['sections'])
  section=facts['sections'][s['section']-1];delta=s['value']-section['address'];assert delta>=0 and delta%4==0 and delta<=section['bytes']-4 and(section['flags']&255)==0
  off=section['fileOffset']+delta;assert 0<=off<=len(raw)-4;result[key]=struct.unpack_from('<I',raw,off)[0]
 assert 64<=result['reportBytes']<=512 and result['reportBytes']%4==0 and result['innerOffset']>=8 and result['innerBytes']>0 and result['innerOffset']+result['innerBytes']<=result['balanceOffset']<result['validOffset']<=result['reportBytes']-4
 return result
