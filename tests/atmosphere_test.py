"""Check actual GL readback for the navy gradient rather than a flat black void."""
import pathlib,subprocess,sys
executable=pathlib.Path(sys.argv[1]).resolve()
root=pathlib.Path(sys.argv[2]).resolve()
capture=root/'build-desktop/atmosphere-check.ppm'
result=subprocess.run([str(executable),'--smoke','--level','3','--capture',str(capture)],cwd=root,capture_output=True,text=True,timeout=20)
if result.returncode:raise RuntimeError(result.stdout+result.stderr)
magic,dimensions,maximum,data=capture.read_bytes().split(b'\n',3)
width,height=map(int,dimensions.split())
assert magic==b'P6' and maximum==b'255' and width==1280 and height==720
def pixel(x,y):return tuple(data[(y*width+x)*3:(y*width+x)*3+3])
upper,lower=pixel(1270,110),pixel(1270,350)
for r,g,b in (upper,lower):
    assert b>r+5 and b>g+4 and b>=15, f'background should be visible cool indigo, got {(r,g,b)}'
assert sum(abs(a-b) for a,b in zip(upper,lower))>=8, f'background should contain a gradient, got {upper} and {lower}'
print(f'PASS actual GL background is a navy gradient: {upper} -> {lower}')
