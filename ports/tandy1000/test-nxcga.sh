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
check 'mode=tandy mouse=none protocol=pc'
check 'mode=tandy mouse=none protocol=pc' none
check 'mode=tandy mouse=/dev/ttyS0 protocol=pc' mouse
check 'mode=tandy mouse=/dev/ttyS0 protocol=pc' tandy mouse
check 'mode=tandy mouse=/dev/ttyS0 protocol=pc' mouse tandy
check 'mode=tandy mouse=none protocol=pc' tandy none
check 'mode=color mouse=none protocol=pc' color none
check 'mode=color mouse=/dev/ttyS0 protocol=pc' color mouse
check 'mode=mono mouse=none protocol=pc' mono none
check 'mode=mono mouse=/dev/ttyS0 protocol=pc' mono mouse
check 'mode=color mouse=/dev/ttyS1 protocol=pc' com2 color
MOUSE_TYPE=ms check 'mode=tandy mouse=/dev/ttyS0 protocol=ms' tandy mouse
MOUSE_TYPE=ms check 'mode=tandy mouse=/dev/ttyS0 protocol=pc' tandy mouse pc
MOUSE_TYPE=pc check 'mode=tandy mouse=/dev/ttyS0 protocol=ms' tandy mouse ms
check 'mode=color mouse=/dev/ttyS0 protocol=pc' pc color mouse
if NXCGA_PRINT_CONFIG=1 /bin/sh "$launcher" invalid-option >/dev/null 2>&1; then
    echo 'Invalid graphics or pointing-device option accepted' >&2
    exit 1
fi
echo 'nxcga mode and mouse combinations: OK'
