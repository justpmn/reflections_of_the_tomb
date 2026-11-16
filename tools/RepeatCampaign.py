"""Repeatedly launch the real campaign process and record exact exit codes."""
import argparse,hashlib,json,pathlib,subprocess,time
root=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--runs',type=int,default=100)
parser.add_argument('--executable',default=str(root/'build-desktop/campaign_test.exe'))
options=parser.parse_args()
executable=pathlib.Path(options.executable).resolve()
records=[]
for iteration in range(options.runs):
    start=time.monotonic()
    result=subprocess.run([str(executable)],cwd=root,capture_output=True,text=True,timeout=20)
    records.append({'run':iteration+1,'exit_code':result.returncode,'seconds':round(time.monotonic()-start,4)})
    if result.returncode:
        raise RuntimeError(f'campaign run {iteration+1} failed with {result.returncode:#x}\n{result.stdout}\n{result.stderr}')
directory=root/'build-stability';directory.mkdir(exist_ok=True)
report={'executable':str(executable),'sha256':hashlib.sha256(executable.read_bytes()).hexdigest(),'runs':records,'clean_exits':len(records)}
(directory/'repeat-campaign.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS {len(records)}/{options.runs} campaign subprocesses exited 0 without access violations')
