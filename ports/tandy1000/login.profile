# Console-first startup on the Tandy; graphics are started explicitly.
case `/bin/tty` in
    /dev/tty1)
        /bin/elkfetch
        echo 'Tandy CF ready. fm = Files; kilo = Editor; invaders / ttypong = Games'
        echo 'Tandy GUI: nxcga mouse or nxcga none (16-color default)'
        echo 'Alternate graphics: nxcga color none; nxcga mono mouse'
        ;;
esac
