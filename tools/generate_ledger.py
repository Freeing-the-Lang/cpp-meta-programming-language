import os, glob, hashlib, datetime

repo   = os.environ.get('GITHUB_REPOSITORY','')
commit = os.environ.get('GITHUB_SHA','')
ref    = os.environ.get('GITHUB_REF','')
actor  = os.environ.get('GITHUB_ACTOR','')
run_id = os.environ.get('GITHUB_RUN_ID','')
run_no = os.environ.get('GITHUB_RUN_NUMBER','')
osname = os.environ.get('ImageOS','') or os.environ.get('RUNNER_OS','')
ts     = datetime.datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')

ledger_name = f'proofledger-{osname}.txt' if osname else 'proofledger.txt'

def sha256_of(path):
    h = hashlib.sha256()
    with open(path,'rb') as f:
        for chunk in iter(lambda: f.read(65536), b''):
            h.update(chunk)
    return h.hexdigest()

lines=[]
lines.append(f'Repository: {repo}')
lines.append(f'Commit: {commit}')
lines.append(f'Ref: {ref}')
lines.append(f'Actor: {actor}')
lines.append(f'Run: {run_id} (#{run_no})')
lines.append(f'OS: {osname}')
lines.append(f'Build Time (UTC): {ts}')
lines.append('')

# source sha256
paths = sorted(set(
    p for pat in ['src/**/*.cpp','src/**/*.hpp','src/**/*.h','src/*.cpp','src/*.hpp','src/*.h']
    for p in glob.glob(pat, recursive=True)
    if os.path.isfile(p)
))

if paths:
    lines.append('== SHA256 of source files ==')
    for p in paths:
        try:
            lines.append(f'{sha256_of(p)}  {p}')
        except Exception as e:
            lines.append(f'[ERROR] {p}: {e}')
    lines.append('')
else:
    lines.append('No C++ sources found in src/')
    lines.append('')

# build outputs sha256
bins=[]
if os.path.isdir('dist'):
    for p in glob.glob('dist/**', recursive=True):
        if os.path.isfile(p):
            bins.append(p)

if bins:
    lines.append('== SHA256 of build outputs ==')
    for p in bins:
        try:
            lines.append(f'{sha256_of(p)}  {p}')
        except Exception as e:
            lines.append(f'[ERROR] {p}: {e}')
    lines.append('')

with open(ledger_name,'w',encoding='utf-8') as f:
    f.write("\n".join(lines))

print("[OK] ProofLedger created:", ledger_name)
