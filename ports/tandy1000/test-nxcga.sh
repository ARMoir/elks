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
check 'mode=tandy mouse=none protocol=ms'
check 'mode=tandy mouse=none protocol=ms' none
check 'mode=tandy mouse=/dev/ttyS0 protocol=ms' mouse
check 'mode=tandy mouse=/dev/ttyS0 protocol=ms' tandy mouse
check 'mode=tandy mouse=/dev/ttyS0 protocol=ms' mouse tandy
check 'mode=tandy mouse=none protocol=ms' tandy none
check 'mode=color mouse=none protocol=ms' color none
check 'mode=color mouse=/dev/ttyS0 protocol=ms' color mouse
check 'mode=mono mouse=none protocol=ms' mono none
check 'mode=mono mouse=/dev/ttyS0 protocol=ms' mono mouse
check 'mode=color mouse=/dev/ttyS1 protocol=ms' com2 color
check 'mode=tandy mouse=/dev/ttyS0 protocol=pc' tandy mouse
if NXCGA_PRINT_CONFIG=1 /bin/sh "$launcher" invalid-option >/dev/null 2>&1; then
    echo 'Invalid graphics or pointing-device option accepted' >&2
    exit 1
fi
echo 'nxcga mode and mouse combinations: OK'
