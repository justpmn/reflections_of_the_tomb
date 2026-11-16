# Audio dependencies

Existing project music and sound assets are unchanged. Native MCI playback failed on the integration machine, so the game vendors two small source dependencies to decode the supplied WAV, MP3, FLAC and OGG files directly:

- `src/third_party/miniaudio.h`: miniaudio **0.11.23**, from https://github.com/mackron/miniaudio/blob/0.11.23/miniaudio.h. Dual public domain / MIT-0 license, retained in the header.
- `src/third_party/stb_vorbis.c`: stb_vorbis at stb commit **f0569113c93ad095470c54bf34a17b36646bbbb5**, from https://github.com/nothings/stb/blob/f0569113c93ad095470c54bf34a17b36646bbbb5/stb_vorbis.c. Dual public domain / MIT license, retained in the file.

Both are compiled into the executable; no installation or runtime audio DLL is required. Source downloads are pinned and already included, so rebuilding does not fetch these files. Only playback is used.

SHA-256 of vendored sources:

- miniaudio.h: `7e4f3f13c8fe66df2080ac3dd12a89193e3c2463cb7f067c798abd7331cd8ee6`
- stb_vorbis.c: `4c7cb2ff1f7011e9d67950446b7eb9ca044f2e464d76bfbb0b84dd2e23e65636`
