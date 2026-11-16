"""Run a campaign binary under the installed LLDB DAP debugger and record native stops.
No system settings or error-dialog policies are changed.
"""
import argparse,json,os,pathlib,queue,subprocess,threading,time
root=pathlib.Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('program');parser.add_argument('--label',default='campaign');parser.add_argument('--args',nargs='*',default=[])
a=parser.parse_args()
adapter=pathlib.Path('C:/Program Files/Android/Android Studio/plugins/android-ndk/resources/lldb/bin/lldb-dap.exe')
env=os.environ.copy();env['PATH']='C:/msys64/ucrt64/bin;'+env.get('PATH','')
p=subprocess.Popen([str(adapter),'--no-lldbinit'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env,cwd=root)
messages=queue.Queue();records=[];sequence=0;deferred=[]
threading.Thread(target=lambda:records.append({'adapter_stderr':p.stderr.read().decode(errors='replace')}),daemon=True).start()
def reader():
 try:
  while True:
   headers={}
   while True:
    line=p.stdout.readline()
    if not line:return
    if line==b'\r\n':break
    k,v=line.decode().split(':',1);headers[k.lower()]=v.strip()
   message=json.loads(p.stdout.read(int(headers['content-length'])))
   messages.put(message)
 except Exception as e:messages.put({'reader_error':str(e)})
threading.Thread(target=reader,daemon=True).start()
def send(command,arguments=None):
 global sequence
 sequence+=1;m={'seq':sequence,'type':'request','command':command,'arguments':arguments or {}}
 b=json.dumps(m).encode();p.stdin.write(f'Content-Length: {len(b)}\r\n\r\n'.encode()+b);p.stdin.flush();return sequence
def receive():
 m=messages.get(timeout=30);records.append(m);return m
def response(seq):
 while True:
  m=receive()
  if m.get('request_seq')==seq:return m
  if m.get('event') in ('stopped','exited','terminated'):deferred.append(m)
result=1
try:
 r=response(send('initialize',{'adapterID':'lldb','pathFormat':'path','linesStartAt1':True,'columnsStartAt1':True}))
 if not r.get('success'):raise RuntimeError(r)
 launch=send('launch',{'program':str(pathlib.Path(a.program).resolve()),'cwd':str(root),'args':a.args,'stopOnEntry':True,'env':{'PATH':env['PATH']}})
 while True:
  m=receive()
  if m.get('type')=='response' and m.get('request_seq')==launch and not m.get('success'):raise RuntimeError(m)
  if m.get('event')=='initialized':break
 response(send('setExceptionBreakpoints',{'filters':[]}))
 response(send('configurationDone'))
 deadline=time.monotonic()+60
 while time.monotonic()<deadline:
  m=deferred.pop(0) if deferred else receive()
  if m.get('event')=='stopped':
   thread=m['body']['threadId'];reason=m['body'].get('reason')
   stack=response(send('stackTrace',{'threadId':thread,'levels':20}))
   if reason!='entry':
    print(json.dumps({'stop':m.get('body'),'stack':stack.get('body')},indent=2),flush=True)
    info=response(send('exceptionInfo',{'threadId':thread}));print(json.dumps(info,indent=2),flush=True)
    result=2;break
   response(send('continue',{'threadId':thread}))
  if m.get('event')=='exited':result=m['body'].get('exitCode',1);print('DEBUGGEE EXIT',result,flush=True);break
 response(send('disconnect',{'terminateDebuggee':True}))
except Exception as e:
 records.append({'error':str(e)});print('DEBUGGER ERROR',e,flush=True)
finally:
 try:p.stdin.close();p.wait(timeout=5)
 except Exception:p.kill()
 directory=root/'build-stability';directory.mkdir(exist_ok=True)
 (directory/f'{a.label}-lldb.json').write_text(json.dumps(records,indent=2))
raise SystemExit(result)
