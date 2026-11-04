#!/bin/sh
# No hardware or Nano-X server is needed for these launcher argument tests.
set -eu
launcher=${1:-ports/tandy1000/nxcga}

check() {
    expected=$1
    shift
    actual=$(NXCGA_PRINT_CONFIG=1 /bin/sh "$launcher" "$@")
    if [ "$actual" != "$expected" ]; then
        echo "Expected [$expected], got [$actual]" >&2
        exit 1
    fi
}
check 'mode=tandy mouse=none protocol=pc ui=deskmate'
check 'mode=tandy mouse=none protocol=pc ui=deskmate' none
check 'mode=tandy mouse=/dev/ttyS0 protocol=pc ui=deskmate' mouse
check 'mode=tandy mouse=/dev/ttyS0 protocol=pc ui=deskmate' tandy mouse
check 'mode=tandy mouse=/dev/ttyS0 protocol=pc ui=deskmate' mouse tandy
check 'mode=tandy mouse=none protocol=pc ui=deskmate' tandy none
check 'mode=color mouse=none protocol=pc ui=deskmate' color none
check 'mode=color mouse=/dev/ttyS0 protocol=pc ui=deskmate' color mouse
check 'mode=mono mouse=none protocol=pc ui=deskmate' mono none
check 'mode=mono mouse=/dev/ttyS0 protocol=pc ui=deskmate' mono mouse
check 'mode=color mouse=/dev/ttyS1 protocol=pc ui=deskmate' com2 color
MOUSE_TYPE=ms check 'mode=tandy mouse=/dev/ttyS0 protocol=ms ui=deskmate' tandy mouse
MOUSE_TYPE=ms check 'mode=tandy mouse=/dev/ttyS0 protocol=pc ui=deskmate' tandy mouse pc
MOUSE_TYPE=pc check 'mode=tandy mouse=/dev/ttyS0 protocol=ms ui=deskmate' tandy mouse ms
check 'mode=color mouse=/dev/ttyS0 protocol=pc ui=deskmate' pc color mouse
check 'mode=tandy mouse=none protocol=pc ui=classic' classic none
check 'mode=color mouse=/dev/ttyS0 protocol=ms ui=classic' color mouse ms classic
check 'mode=mono mouse=none protocol=pc ui=deskmate' mono desk
NXCGA_UI=classic check 'mode=tandy mouse=none protocol=pc ui=classic'
if NXCGA_PRINT_CONFIG=1 /bin/sh "$launcher" invalid-option >/dev/null 2>&1; then
    echo 'Invalid graphics or pointing-device option accepted' >&2
    exit 1
fi
echo 'nxcga mode and mouse combinations: OK'
