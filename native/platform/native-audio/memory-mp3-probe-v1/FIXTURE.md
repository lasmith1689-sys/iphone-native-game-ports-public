# Proposed own LayerIII silence codec smoke

No fixture/compiler/codec/native/SDK is executed during preparation. These are
self-authored bitfields, not downloaded audio or an assumed conformance file.
Actual validity/decoded zeros remain assertions for the future real decoder.

Headers: sync11bits, version MPEG1(11) or MPEG2(10), layerIII(01), protection1
(noCRC); frequency index00 (44100 or22050Hz), selected bitrate index9/128kbps for
MPEG1 or8/64kbps forMPEG2, padding0/1, private0. Channel mode11 for MPEG1 mono
or00 stereo; remaining mode/copyright/original/emphasis bits0. Thus the header
starts FFFB90[00/C0] or FFF38000, adding padding bit0x02 in byte2 as selected.
Frame bytes=floor(samples*kbps*125/rate)+padding;1152samples forMPEG1 and576for
MPEG2. All remaining frame bytes are zero, including complete17/32byte MPEG1
mono/stereo sideinfo or17byte MPEG2 stereo sideinfo. main_data_begin=0 (no
reservoir), part2_3_length=0, big_values=0, global_gain=0, scalefac_compress=0,
window-switch0 and table/region/preflag/scalefac/count1 bits0. There is no main
entropy payload; remaining zero bytes are ancillary. This is the intended
silence construction, not a claim the pinned codec already accepted it.

Three selected profiles each get1/2frames, toggled padding in second frame.
Two-frame basics include exact own ID3v2.3 flags0/body1014 and exact128byte
ID3v1 TAG tail; four additional tag/no-tag combinations exercise wrapper spans.
All input bytes are generated in process and the wrapper sees a complete known
extent. No stdio/VFS/guest/resource/retail callback supplies decoded input.
Actual returned frame metadata, view bytes, zero PCM units and EOF facts are
checked; output counters come from observed successful calls and PCM loops.
Expected success75calls/92736PCMframes/182016sample-unit zero checks includes
64simultaneous handles plus a fresh call after cleanup. These are expected
counts only until the real future Mac run, and a mismatch fails visibly.

Main calls use invalid sentinel addresses to demonstrate actual main refusal
before pointer dereference. One actual pthread owns every successful decode;
a second pthread sees the real live owner handle and must get worker refusal
without changing its own output slots. It joins before owner releases/exits.
The fixture checks occupied-slot refusal, envelope truncation/no publication,
codec-level bad-reservoir refusal/no publication, input cap refusal before
source dereference,64slots/65th-limit, views while live, releases and cleared
slots. Bad-reservoir first9sideinfo bits are511 instead of0; expectation is the
actual low-level codec returns0 and wrapper returnsCODEC. This is an actual
future assertion, not a forced/mock decoder result. Allocation-failure and
unmeasured corruption/entropy/gapless/music/full-stock behavior remain OPEN.

One Mac executable links exactly shared implementation+envelope+fixture TUs.
iPhone primary/repeat compile all three TUs, while unsigned classic support
libraries link ONLY the two production TUs. No old mixer implementation links.
Any source/tool/compile/link/audit/native failure keeps overall proof false;
raw diagnostics are retained. Native results are silence-only Mac software
codec observations, never physical audio, runtime game or lockfree/render-cost
certification. Root owns any future publication/run; no new thread required.
