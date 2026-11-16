"""Render real OpenGL validation captures and encode their RGB readback as PNG."""
import pathlib,subprocess,struct,zlib
root=pathlib.Path(__file__).resolve().parents[1]
output=root/'docs/screenshots'
output.mkdir(parents=True,exist_ok=True)
exe=root/'build-desktop/reflections_of_the_tomb.exe'
def chunk(tag,data):
 return struct.pack('>I',len(data))+tag+data+struct.pack('>I',zlib.crc32(tag+data)&0xffffffff)
def convert(ppm,png):
 magic,dimensions,maximum,data=ppm.read_bytes().split(b'\n',3)
 assert magic==b'P6' and maximum==b'255'
 w,h=map(int,dimensions.split());assert len(data)==w*h*3
 rows=b''.join(b'\0'+data[y*w*3:(y+1)*w*3] for y in range(h))
 png.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows,6))+chunk(b'IEND',b''))
cases=[('menu',['--screen','start'])]
for level in range(1,6):
 cases += [(f'level{level}',['--level',str(level)]),(f'level{level}-open',['--level',str(level),'--solved'])]
for level in [4,5]:
 for state in ['safe','warning','extended']:
  cases.append((f'level{level}-spikes-{state}',['--level',str(level),'--view',f'spikes-{state}']))
cases += [('ruby-floating',['--view','ruby']),('ruby-sealed',['--view','ruby','--solved']),('door-closed',['--view','door']),('door-open',['--view','door','--solved'])]
cases += [(screen,['--screen',screen]) for screen in ['pause','controls','victory']]
for name,args in cases:
 ppm=root/'build-desktop'/f'review-{name}.ppm'
 result=subprocess.run([str(exe),'--smoke',*args,'--capture',str(ppm)],cwd=root,capture_output=True,text=True,timeout=30)
 (root/'build-desktop'/f'review-{name}.log').write_text(result.stdout+result.stderr)
 if result.returncode:raise RuntimeError(f'{name}: {result.returncode}\n{result.stdout}\n{result.stderr}')
 convert(ppm,output/f'{name}.png')
 print('Captured',name,flush=True)

# Replay real native Windows inputs and retain actual player-controlled states from Levels 3-5.
if __import__('os').name=='nt':
 result=subprocess.run([str(exe),'--control-test'],cwd=root,capture_output=True,text=True,timeout=120)
 (root/'build-desktop'/'review-native-controls.log').write_text(result.stdout+result.stderr)
 if result.returncode:raise RuntimeError(f'native controls failed: {result.returncode}\n{result.stdout}\n{result.stderr}')
 for name in ['control-level3-initial','control-level3-ruby1','control-level3-ruby2','control-level3-open','control-level4-initial','control-level4-ruby1','control-level4-open','control-level5-initial','control-level5-ruby1','control-level5-ruby2','control-level5-ruby3','control-level5-open','control-victory']:
  convert(root/'build-desktop/control-captures'/f'{name}.ppm',output/f'native-{name[8:]}.png')
  print('Captured native',name,flush=True)
