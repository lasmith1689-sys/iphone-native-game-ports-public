"""Read-only policy/identity validation with mutation negatives; no provider."""
import argparse,copy,hashlib,json,re,sys,subprocess,tempfile,os
from pathlib import Path
import yaml
sys.dont_write_bytecode=True
from prepare_workflow import render,REPOSITORY,BRANCH,CHECKOUT,UPLOAD,PUBLIC_TOOLS,HERE
def require(v):
 if not v:raise ValueError('Encrypted workflow policy rejected')
def check(w,expected):
 require(set(w)=={'name','on','permissions','concurrency','jobs'})
 require(w['on']==expected['on'] and set(w['on'])=={'push','workflow_dispatch'})
 require(w['on']['push']=={'branches':[BRANCH],'paths':['.github/workflows/native-encrypted-cod-stack-objects-v18-mac.yml']})
 require(w['permissions']=={'contents':'read'} and w['concurrency']==expected['concurrency'])
 require(set(w['jobs'])=={'resource-sdk'});job=w['jobs']['resource-sdk'];ref=expected['jobs']['resource-sdk']
 require(set(job)=={'if','runs-on','timeout-minutes','steps'})
 require(job['if']==ref['if'] and job['runs-on']=='xcode-27' and job['timeout-minutes']==30)
 steps=job['steps'];require(len(steps)==6 and [s['id'] for s in steps]==['guard','checkout','verify','build','upload','final'])
 require(steps[0]==ref['steps'][0] and steps[2]==ref['steps'][2])
 require(steps[1]['uses']==CHECKOUT and steps[1]['with']==ref['steps'][1]['with'])
 require(steps[3]==ref['steps'][3] and steps[3]['env']=={'NGP_INPUT_KEY_20261028':'${{ secrets.NGP_INPUT_KEY_20261028 }}'})
 require(steps[4]['uses']==UPLOAD and steps[4]==ref['steps'][4])
 require(steps[5]==ref['steps'][5])
 require(all(len(s.get('run',''))<=21000 for s in steps))
 require(sum('uses' in s for s in steps)==2)
 require(sum('secrets.' in json.dumps(s) for s in steps)==1)
 # Canonical equality also rejects extra step fields, alternate artifact paths,
 # caches, upload broadening, pull_request triggers and shell trace additions.
 require(w==expected)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--bundle',type=Path,required=True);ap.add_argument('--workflow',type=Path,required=True);a=ap.parse_args()
 expected,text=render(a.bundle.resolve());raw=a.workflow.read_bytes();require(raw==text.encode());check(expected,expected)
 edits=[lambda w:w.update(permissions={'contents':'write'}),lambda w:w['on'].update(push=None),lambda w:w['on'].update(pull_request=None),lambda w:w['jobs']['resource-sdk'].update({'runs-on':'xcode-27-xlarge'}),lambda w:w['jobs']['resource-sdk'].update({'timeout-minutes':360}),lambda w:w['jobs']['resource-sdk'].update({'if':'true'}),lambda w:w['jobs']['resource-sdk']['steps'].append({'id':'leak','run':'cat plaintext.c'}),lambda w:w['jobs']['resource-sdk']['steps'][0].update(run='true'),lambda w:w['jobs']['resource-sdk']['steps'][1].update(uses='actions/checkout@main'),lambda w:w['jobs']['resource-sdk']['steps'][1]['with'].update({'persist-credentials':True}),lambda w:w['jobs']['resource-sdk']['steps'][1]['with'].update({'ref':'main'}),lambda w:w['jobs']['resource-sdk']['steps'][2].update(run='node --version'),lambda w:w['jobs']['resource-sdk']['steps'][3].update(run='set -x\nnode run-build.mjs'),lambda w:w['jobs']['resource-sdk']['steps'][3]['env'].update(KEY='other'),lambda w:w['jobs']['resource-sdk']['steps'][4].update(uses='actions/upload-artifact@main'),lambda w:w['jobs']['resource-sdk']['steps'][4]['with'].update(path='**'),lambda w:w['jobs']['resource-sdk']['steps'][4]['with'].update({'retention-days':90}),lambda w:w['jobs']['resource-sdk']['steps'][4].update({'if':'always()'}),lambda w:w['jobs']['resource-sdk']['steps'][5].update(run='true'),lambda w:w.update(jobs={'cache':{'uses':'actions/cache@main'}})]
 for edit in edits:
  bad=copy.deepcopy(expected);edit(bad)
  try:check(bad,expected)
  except (ValueError,KeyError):pass
  else:raise AssertionError('Negative policy mutation accepted')
 for edit in [lambda w:w['on']['push'].update(paths=['**']),lambda w:w['on']['push'].update(branches=['main']),lambda w:w['on'].pop('push'),lambda w:w['jobs']['resource-sdk']['steps'][4]['with'].update(archive=False)]:
  bad=copy.deepcopy(expected);edit(bad)
  try:check(bad,expected)
  except (ValueError,KeyError):pass
  else:raise AssertionError('Exact push/artifact policy mutation accepted')
 m=json.loads((a.bundle/'public-manifest.json').read_text());assert m['sourceFiles']==197 and m['sourceBytes']==142141815 and m['context']['purpose']=='cod-stack-v18-macos-object-collection'
 assert m['context']['sourceClosureSha256']=='126b99216fa302439b486b1cf27a19cdd2d8f6956b4f514c5ba119a75df2d6f1'
 guard=expected['jobs']['resource-sdk']['steps'][0]['run'].split("python3 - <<'PY'\n",1)[1].rsplit('\nPY\n',1)[0]
 checks=[('push',{},True),('workflow_dispatch',{},True),('workflow_dispatch',{'bundle':'wrong'},False),('push',{'private':True},False),('push',{'after':'b'*40},False),('push',{'ref':'refs/heads/main'},False),('pull_request',{},False)]
 with tempfile.TemporaryDirectory(prefix='ngp-context-test-') as td:
  for kind,change,ok in checks:
   event={'repository':{'full_name':REPOSITORY,'private':change.get('private',False)},'inputs':{'bundle_id':change.get('bundle',m['context']['bundleId'])},'ref':change.get('ref','refs/heads/'+BRANCH),'after':change.get('after','a'*40),'deleted':False}
   event_path=Path(td)/'event.json';event_path.write_text(json.dumps(event))
   env=dict(os.environ);env.update(GITHUB_REPOSITORY=REPOSITORY,GITHUB_REF='refs/heads/'+BRANCH,GITHUB_EVENT_NAME=kind,GITHUB_SHA='a'*40,GITHUB_EVENT_PATH=str(event_path))
   result=subprocess.run([sys.executable,'-c',guard],env=env,capture_output=True,timeout=10)
   require((result.returncode==0)==ok)
 print(json.dumps({'status':'passed','workflowSha256':hashlib.sha256(raw).hexdigest(),'workflowBytes':len(raw),'steps':6,'negativePolicyCases':len(edits)+4,'actualContextPositiveCases':2,'actualContextNegativeCases':5,'cipherSha256':m['cipherSha256'],'publicTools':{n:hashlib.sha256((HERE/n).read_bytes()).hexdigest() for n in PUBLIC_TOOLS}}))
if __name__=='__main__':main()
