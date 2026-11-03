# Console-first startup on the Tandy; graphics are started explicitly.
case `/bin/tty` in
    /dev/tty1)
        /bin/elkfetch
        echo 'Tandy CF ready. fm = Files; kilo = Editor; invaders / ttypong = Games'
        echo 'CGA apps: nxcga com1 or nxcga com2 (serial mouse); nxcga none (keyboard)'
        ;;
esac
