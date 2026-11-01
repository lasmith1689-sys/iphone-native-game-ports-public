"""Synthetic file-metadata controls only; no native code is executed."""
import struct,unittest
from inspect_macho import inspect,API
def command(cmd,payload):
 n=(8+len(payload)+7)&~7;return struct.pack('<II',cmd,n)+payload+b'\0'*(n-8-len(payload))
def fixture(crypts=(),signature=False):
 def dylib(cmd,name):return command(cmd,struct.pack('<4I',24,0,0,0)+name.encode()+b'\0')
 names=sorted(API);strings=b'\0'+b''.join(n.encode()+b'\0'for n in names)
 before=[command(0x32,struct.pack('<4I',2,26<<16,(27<<16)|(1<<8),0)),dylib(0xc,'/usr/lib/libSystem.B.dylib'),dylib(0xd,'@rpath/libNgpWavePcmV1.dylib')]
 # Fixed-size segment/symtab/encryption commands determine the text offset.
 nb=sum(map(len,before))+152+24+24*len(crypts)+(16 if signature else 0)
 text=32+nb;symboloff=text+8;stringoff=symboloff+16*len(names)
 section=b'__text'.ljust(16,b'\0')+b'__TEXT'.ljust(16,b'\0')+struct.pack('<QQ8I',0x100000+text,8,text,2,0,0,0,0,0,0)
 segment=command(0x19,b'__TEXT'.ljust(16,b'\0')+struct.pack('<4Q4I',0x100000,stringoff+len(strings),0,stringoff+len(strings),5,5,1,0)+section)
 commands=before+[segment,command(2,struct.pack('<4I',symboloff,len(names),stringoff,len(strings)))]
 for cryptid,pad,escape in crypts:commands.append(command(0x2c,struct.pack('<4I',text if not escape else stringoff+len(strings)+1,8,cryptid,pad)))
 if signature:commands.append(command(0x1d,struct.pack('<II',0,0)))
 body=b'\xc0\x03\x5f\xd6'*2;offset=1;symbols=b''
 for k,n in enumerate(names):
  symbols+=struct.pack('<IBBHQ',offset,0xf,1,0,0x100000+text+4*k);offset+=len(n)+1
 return struct.pack('<8I',0xfeedfacf,0x100000c,0,6,len(commands),nb,0,0)+b''.join(commands)+body+symbols+strings
class MetadataControls(unittest.TestCase):
 def test_plain_unsigned(self):self.assertEqual(inspect(fixture())['codeSignatures'],0)
 def test_unsigned_ios_encryption_metadata_is_not_encryption(self):
  facts=inspect(fixture([(0,0,False)]));self.assertEqual(facts['encryptionCommands'],1);self.assertIs(facts['encrypted'],False)
 def test_actual_encryption_denied(self):
  with self.assertRaises(AssertionError):inspect(fixture([(1,0,False)]))
 def test_duplicate_metadata_denied(self):
  with self.assertRaises(AssertionError):inspect(fixture([(0,0,False)]*2))
 def test_padding_and_extent_denied(self):
  for c in[(0,1,False),(0,0,True)]:
   with self.subTest(c=c),self.assertRaises(AssertionError):inspect(fixture([c]))
 def test_signature_denied(self):
  with self.assertRaises(AssertionError):inspect(fixture(signature=True))
 def test_classic_fixups_required(self):
  b=bytearray(fixture(signature=True));at=32
  while struct.unpack_from('<I',b,at)[0]!=0x1d:at+=struct.unpack_from('<I',b,at+4)[0]
  struct.pack_into('<I',b,at,0x80000034)
  with self.assertRaises(AssertionError):inspect(bytes(b))
 def test_truncation_denied(self):
  for n in[0,31,100,len(fixture())-1]:
   with self.subTest(n=n),self.assertRaises(AssertionError):inspect(fixture()[:n])
 def test_text_must_not_alias_header(self):
  b=bytearray(fixture());at=32
  while struct.unpack_from('<I',b,at)[0]!=0x19:at+=struct.unpack_from('<I',b,at+4)[0]
  struct.pack_into('<I',b,at+72+48,0)
  with self.assertRaises(AssertionError):inspect(bytes(b))
 def test_section_must_match_segment_mapping(self):
  b=bytearray(fixture());at=32
  while struct.unpack_from('<I',b,at)[0]!=0x19:at+=struct.unpack_from('<I',b,at+4)[0]
  struct.pack_into('<Q',b,at+40,8)
  with self.assertRaises(AssertionError):inspect(bytes(b))
if __name__=='__main__':unittest.main()
