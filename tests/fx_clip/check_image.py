"""Validate the LXR SD update container, not hardware behavior."""
from pathlib import Path
import hashlib,struct,sys
b=Path(sys.argv[1]).read_bytes()
assert b[:8]==b'LXRV2IMG'
assert len(b)>=16 and struct.unpack_from('<I',b,8)[0]==len(b)-16
assert b[12]==255-(sum(b[16:])&255)
assert b[13:16]==bytes(3)
assert b'LAB R5-CLP' in b,'modified boot identity missing'
# Initial vector table must belong to the application region behind the bootloader.
sp,reset=struct.unpack_from('<II',b,16)
assert 0x20000000<=sp<=0x20080000
assert reset&1 and 0x08008000<=reset<0x08100000
print('PASS LXRV2IMG header, payload length, checksum, modified boot identity, application vectors')
print('bytes',len(b),'sha256',hashlib.sha256(b).hexdigest())

