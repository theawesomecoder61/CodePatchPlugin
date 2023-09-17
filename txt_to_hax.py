import binascii
import re
import struct
import sys
from keystone import *

if ".txt" not in sys.argv[1]:
  print("Expected .txt file as input")
  sys.exit(1)

instructions = {}
ks = Ks(KS_ARCH_PPC, KS_MODE_PPC32 + KS_MODE_BIG_ENDIAN)
with open(sys.argv[1], "r") as f:
  lines = f.readlines()

  for line in lines:
    parts = line.split("=")
    if len(parts) < 2 or ";" in line:
      continue
    addr = parts[0].strip()
    try:
      int(addr[2:], 16)
    except:
      print("Ignoring line, expected 32-bit address in hex, got %s" %addr)
      continue

    instr = parts[1].strip()
    instr = re.sub(r"[rf](\d{1,2})", r"\1", instr)
    instr = re.sub(r"#[\s\w]+(\[.*\])+", "", instr)
    try:
      encoding, count = ks.asm(instr)
      instructions[addr] = encoding
      print("%s = %s" %(instr, encoding))
    except KsError as e:
      print("Error: %s" %e)

with open(sys.argv[1][:-3] + "hax", "wb") as f:
  f.write(struct.pack(">h", len(instructions)))
  for k in instructions:
    v = instructions[k]
    f.write(struct.pack(">hI", len(v), int(k[2:], 16) + 0xA900000))
    f.write(bytes(v))
