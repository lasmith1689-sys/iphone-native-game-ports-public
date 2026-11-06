# Own finite MP3 padding support: Mac and iPhone SDK diagnostic

This package contains self-authored support and synthetic fixtures only. It has
no game code/assets, recovered evidence, private matrix metadata, keys or device
inputs. The four encoded tone fixtures were generated from own sine waves; only
those four files are read by this Mac diagnostic. Six padded copies are created
in fixture-owned memory. No existing asset or encoded disk file is rewritten.

The separate adapter admits all-zero padding or one initialFF followed by zeros,
at most4093bytes, then exactly the first nonzero FF candidate. Whole-source64MiB
and all unchanged strictv1 profile/frame/PCM/TLS limits apply. Padding metadata
is separate from ID3/suffix envelope. The decode wrapper calls the unchanged
decoder once, retains the complete source lifetime through return and publishes
no outputs on failure. No general search, retry, sourceFS or device operation.

This diagnostic compiles actual Apple product code without a platform lie. The
Mac fixture uses genuine main/nonmain pthreads and26C boundary controls per own
case. Ten cases include original nonzero profiles/refusal and six padded copies;
each positive matches the complete PCM bytes of an independent strict suffix
baseline, passes the original tone spectral checks and releases both handles.
The zero-buffer spectral negative runs before positives. Product behavior on
arbitrary valid headers is not forced to succeed, and there is no universal
Miles/gapless/bitstream integrity/physical audio or game claim.

Seven translation units are compiled for Mac and genuine iPhoneSDK27.1 in-place;
all seven iOS objects/SU and two four-product-unit dylib links must repeat exactly.
Readonly compiler-emitted padding ABI literals must show16B/align4/offsets0,4,8,12.
Raw ARM64/iOS26 Mach-O shapes, no signing/encryption/chained-fixup command, actual
six strong dyld exports and hidden helper boundaries are inspected. This records
local frames; it does not certify a whole app/native stack or OS-provider theorem.
The SDK is never copied/exported or executed. No provisioning/signing identity,
phone, playback, simulator installation or downloaded Apple platform is used.

Miniaudio0.11.23 is downloaded only at commit
f40cf03f80cdb7e741d43e53b7e706e8c1394bcf with exact header/license hashes and
unchanged shared profile. Its generic file APIs may exist; selected worker tests
do not call them for source resources. OS/CRT/compiler/runtime behavior remains
an environment-managed conditional provider scope.

Prior private Windows component validation had27pure preparation methods (an
earlier frozen README said24), followed by actual guarded own and corpus tests.
Those private names/hashes/content/receipts are deliberately absent here and are
not represented as Apple results. This package must earn its own real run.

Next: public own-source isolated-branch Mac/SDK execution and retained raw readback,
then root whole-candidate review before integration. Current thread; no new thread.
