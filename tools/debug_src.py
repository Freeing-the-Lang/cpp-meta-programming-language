import os, glob

print("=== src/ ===")
if os.path.isdir("src"):
    for p in sorted(glob.glob("src/**/*", recursive=True)):
        print(p)
else:
    print("src NOT FOUND")
