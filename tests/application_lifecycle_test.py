"""Exercise normal and exceptional GL/audio/UI shutdown in actual subprocesses."""
import pathlib, subprocess, sys
executable=pathlib.Path(sys.argv[1]).resolve()
root=pathlib.Path(sys.argv[2]).resolve()
for iteration in range(5):
    result=subprocess.run([str(executable),'--smoke'],cwd=root,capture_output=True,text=True,timeout=20)
    if result.returncode!=0:
        raise RuntimeError(f'normal shutdown {iteration}: {result.returncode:#x}\n{result.stdout}\n{result.stderr}')
result=subprocess.run([str(executable),'--smoke','--test-throw'],cwd=root,capture_output=True,text=True,timeout=20)
if result.returncode!=10 or 'lifecycle test exception' not in result.stderr:
    raise RuntimeError(f'exceptional shutdown: {result.returncode:#x}\n{result.stdout}\n{result.stderr}')
print('PASS 5 clean GL/audio/UI shutdowns and exception unwinding before context destruction')
