# Console-first startup on the Tandy; graphics are started explicitly.
case `/bin/tty` in
    /dev/tty1)
        /bin/elkfetch
        echo 'Tandy CF ready. fm = Files; kilo = Editor; invaders / ttypong = Games'
        echo 'CGA apps: nxcga mouse (single serial port); nxcga none (keyboard)'
        ;;
esac
