"""Local-only exact public ciphertext workflow projection. No provider action."""
import argparse,hashlib,json,re
from pathlib import Path
import yaml
HERE=Path(__file__).resolve().parent
REPOSITORY='lasmith1689-sys/iphone-native-game-ports-public'
BRANCH='codex/native-cod-loading-sdk-v12-20261028'
WORKFLOW='.github/workflows/native-encrypted-cod-loading-sdk-v12.yml'
PREFIX='native/delivery/encrypted-build/'
CHECKOUT='actions/checkout@d23441a48e516b6c34aea4fa41551a30e30af803'
UPLOAD='actions/upload-artifact@cf430e030ddbb5b0abf93d22962f4752f3646cd9'
PUBLIC_TOOLS=['transport.mjs','bundle-builder.mjs','run-build.mjs']
def sha(raw):return hashlib.sha256(raw).hexdigest()
def render(bundle):
 raw=(bundle/'public-manifest.json').read_bytes();m=json.loads(raw)
 assert m['format']=='NGP-public-input-v1' and m['context']['repository']==REPOSITORY and m['context']['ref']=='refs/heads/'+BRANCH
 cipher=bundle/'input.ngp';assert cipher.stat().st_size==m['cipherBytes'] and sha(cipher.read_bytes())==m['cipherSha256']
 assert re.fullmatch(r'[a-f0-9-]{36}',m['context']['bundleId'])
 identities={PREFIX+x:sha((HERE/x).read_bytes()) for x in PUBLIC_TOOLS}
 identities[PREFIX+'public-manifest.json']=sha(raw)
 guard='''set -euo pipefail
python3 - <<'PY'
import json,os,re,pathlib
assert os.environ['GITHUB_REPOSITORY']==__REPOSITORY__
assert os.environ['GITHUB_REF']==__REF__
kind=os.environ['GITHUB_EVENT_NAME']
assert kind in ('push','workflow_dispatch')
assert re.fullmatch('[a-f0-9]{40}',os.environ['GITHUB_SHA'])
e=json.loads(pathlib.Path(os.environ['GITHUB_EVENT_PATH']).read_text())
assert e['repository']['full_name']==__REPOSITORY__ and e['repository']['private'] is False
if kind=='workflow_dispatch':
 assert e['inputs']['bundle_id']==__BUNDLE__
else:
 assert e['ref']==__REF__ and e['after']==os.environ['GITHUB_SHA'] and e.get('deleted',False) is False
print('Exact public ciphertext context accepted.')
PY
'''.replace('__REPOSITORY__',repr(REPOSITORY)).replace('__REF__',repr('refs/heads/'+BRANCH)).replace('__BUNDLE__',repr(m['context']['bundleId']))
 verify='''set -euo pipefail
node --input-type=module - <<'JS'
import fs from 'node:fs';
import crypto from 'node:crypto';
import {execFileSync} from 'node:child_process';
if(!process.versions.node.startsWith('24.'))throw new Error('Node24 required');
if(execFileSync('git',['rev-parse','HEAD'],{encoding:'utf8'}).trim()!==process.env.GITHUB_SHA)throw new Error('Checkout identity rejected');
const expected=__IDENTITIES__;
for(const [file,digest] of Object.entries(expected))if(crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex')!==digest)throw new Error('Public tooling identity rejected');
const m=JSON.parse(fs.readFileSync('__PREFIX__public-manifest.json','utf8'));
const data=fs.readFileSync('__PREFIX__input.ngp');
if(data.length!==m.cipherBytes||crypto.createHash('sha256').update(data).digest('hex')!==m.cipherSha256)throw new Error('Ciphertext identity rejected');
if(m.context.bundleId!==__BUNDLE__)throw new Error('Bundle context rejected');
if(m.cipherSha256!==__CIPHER_SHA__||m.cipherBytes!==__CIPHER_BYTES__)throw new Error('Reviewed ciphertext rejected');
console.log(JSON.stringify({status:'public-inputs-verified',node:process.versions.node,cipherSha256:m.cipherSha256,cipherBytes:data.length}));
JS
'''.replace('__IDENTITIES__',json.dumps(identities,sort_keys=True,separators=(',',':'))).replace('__PREFIX__',PREFIX).replace('__BUNDLE__',json.dumps(m['context']['bundleId'])).replace('__CIPHER_SHA__',json.dumps(m['cipherSha256'])).replace('__CIPHER_BYTES__',str(m['cipherBytes']))
 build='''set -euo pipefail
umask 077
mkdir "$RUNNER_TEMP/ngp-ciphertext"
node native/delivery/encrypted-build/run-build.mjs \\
  --input native/delivery/encrypted-build/input.ngp \\
  --manifest native/delivery/encrypted-build/public-manifest.json \\
  --output "$RUNNER_TEMP/ngp-ciphertext/result.ngp" \\
  --workspace "$RUNNER_TEMP/ngp-private-build"
'''
 steps=[{'name':'Require exact public repository, branch and explicit bundle request','id':'guard','shell':'bash','run':guard},
 {'name':'Checkout only reviewed public tooling and ciphertext','id':'checkout','uses':CHECKOUT,'with':{'ref':'${{ github.sha }}','persist-credentials':False,'fetch-depth':1,'submodules':False,'lfs':False,'sparse-checkout':PREFIX.rstrip('/')}},
 {'name':'Verify exact public tools, ciphertext and checkout','id':'verify','shell':'bash','run':verify},
 {'name':'Authenticate sources, capture private build logs and seal outputs','id':'build','shell':'bash','env':{'NGP_INPUT_KEY_20261028':'${{ secrets.NGP_INPUT_KEY_20261028 }}'},'run':build},
 {'name':'Upload ciphertext only for one day','id':'upload','if':"always() && steps.build.outputs.sealed == 'true'",'uses':UPLOAD,'with':{'name':'ngp-sealed-resource-sdk-${{ github.run_id }}-${{ github.run_attempt }}','path':'${{ runner.temp }}/ngp-ciphertext/result.ngp','if-no-files-found':'error','retention-days':1,'compression-level':0,'include-hidden-files':False,'archive':True}},
 {'name':'Require actual captured build success','id':'final','if':'always()','shell':'bash','run':"set -euo pipefail\ntest '${{ steps.build.outputs.status }}' = 'passed'\ntest '${{ steps.upload.outcome }}' = 'success'\n"}]
 job_if="github.repository == '"+REPOSITORY+"' && github.event.repository.private == false && github.ref == 'refs/heads/"+BRANCH+"' && (github.event_name == 'push' || github.event_name == 'workflow_dispatch')"
 workflow={'name':'Encrypted private native resource SDK proof','on':{'push':{'branches':[BRANCH],'paths':[WORKFLOW]},'workflow_dispatch':{'inputs':{'bundle_id':{'description':'Exact reviewed encrypted bundle ID','required':True,'type':'string'}}}},'permissions':{'contents':'read'},'concurrency':{'group':'native-encrypted-resource-sdk','cancel-in-progress':False},'jobs':{'resource-sdk':{'if':job_if,'runs-on':'xcode-27','timeout-minutes':30,'steps':steps}}}
 return workflow,yaml.safe_dump(workflow,sort_keys=False,width=1000)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--bundle',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 w,text=render(a.bundle.resolve());assert not a.output.exists() and max(len(s.get('run','')) for s in w['jobs']['resource-sdk']['steps'])<=21000
 a.output.write_text(text,encoding='utf8',newline='\n');print(json.dumps({'status':'prepared','workflowSha256':sha(text.encode()),'bytes':len(text.encode()),'steps':len(w['jobs']['resource-sdk']['steps'])}))
if __name__=='__main__':main()
