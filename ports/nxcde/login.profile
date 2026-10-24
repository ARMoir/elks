# Welcome screen and desktop only after interactive login, never in rc.sys.
/bin/elkfetch
# Only the physical console; terminal windows and serial logins stay shells.
# Create /root/nox to disable this, remove it to enable it again.
if test ! -f /root/nox; then
    case "`/bin/tty`" in
        /dev/tty1)
            if test -x /bin/nxstart; then
                /bin/nxstart
            fi
            ;;
    esac
fi
