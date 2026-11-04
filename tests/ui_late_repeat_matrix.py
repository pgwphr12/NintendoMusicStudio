"""Own synthesized late-format fixtures; own hidden UI children/dummy audio only."""
from pathlib import Path
import os,sys,json,subprocess
exe=Path(sys.argv[1]).resolve();root=Path(sys.argv[2]).resolve();out=Path(sys.argv[3]).resolve();out.mkdir(exist_ok=True,parents=True)
files=['channels.gsf','channels.minigsf','channels.2sf','channels.mini2sf','channels.usf','channels.miniusf','channels.bcstm','channels.bcwav','playlist-sega/01 Track.vgm','playlist-sega/02 Track.vgz','short-scc.vgm','audio/tone.mp3','audio/tone.wav','audio/tone.ogg','audio/tone.flac']
results=[]
for i,name in enumerate(files):
 target=out/str(i);r=subprocess.run([sys.executable,str(Path(__file__).with_name('ui_audio_repeat.py')),str(exe),str(root/name),str(target)],env=dict(os.environ,NGMV_REPEAT_TEST_WRAPS='2'),capture_output=True,timeout=38)
 log=r.stdout.decode(errors='replace')+r.stderr.decode(errors='replace');target.mkdir(exist_ok=True);(target/'run.log').write_text(log)
 if (target/'result.json').exists():result=json.loads((target/'result.json').read_text())
 else:result={'pass':False,'reason':log[-1000:]}
 result['format_case']=name;result['exitcode']=r.returncode;results.append(result);print(json.dumps(result),flush=True)
(out/'result.json').write_text(json.dumps({'pass':all(r['exitcode']==0 and r['pass'] for r in results),'cases':results,'audio':'dummy','user_apps_operated':False},indent=2))
assert all(r['exitcode']==0 and r['pass'] for r in results),'Some late formats failed repeated playback'
